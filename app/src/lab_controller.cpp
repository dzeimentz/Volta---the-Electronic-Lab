#include "lab_controller.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QJsonArray>
#include <QFile>
#include <QSysInfo>

#include <QLocale>

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "model/build_guide.hpp"
#include "model/formula.hpp"
#include "services/audio_service.hpp"
#include "model/meter_display.hpp"
#include "model/resistor_code.hpp"
#include "model/schematic_layout.hpp"

namespace volta {

namespace {
constexpr int kFrameMs = 16;     // ca. 60 Bilder je Sekunde
constexpr double kNotifyS = 0.1; // Anzeigen mit ca. 10 Hz aktualisieren

QString qs(const std::string& s) { return QString::fromStdString(s); }
std::string ss(const QString& s) { return s.toStdString(); }

model::Catalog loadCatalog(services::ContentRepository& repo, QStringList& errors) {
    model::Catalog c;
    if (!repo.loadCatalog(c)) errors << repo.lastError();
    return c;
}

model::Orientation orient(bool vertical) {
    return vertical ? model::Orientation::Vertical : model::Orientation::Horizontal;
}
model::Span spanOf(bool longSpan) { return longSpan ? model::Span::Long : model::Span::Short; }
} // namespace

LabController::LabController(QString contentRoot, QObject* parent)
    : QObject(parent), m_repo(std::move(contentRoot)) {
    m_session = std::make_unique<model::LabSession>(loadCatalog(m_repo, m_errors));
    m_repo.loadTexts(QStringLiteral("volta1"), QStringLiteral("de"));
    m_experimentIds = m_repo.experimentIds(QStringLiteral("volta1"));
    m_stages = m_repo.stages(QStringLiteral("volta1"));
    rebuildExperimentList();
    m_session->onBoardChanged = [this] {
        m_errors.clear();
        for (const auto& e : m_session->netlist().errors) m_errors << qs(e);
        emit boardChanged();
    };
    connect(&m_session->undoStack(), &QUndoStack::indexChanged, this, &LabController::historyChanged);
    connect(&m_session->undoStack(), &QUndoStack::cleanChanged, this, &LabController::historyChanged);
    connect(&m_timer, &QTimer::timeout, this, &LabController::onFrame);
    // Aufbauhilfe folgt jeder Aenderung von Aufbau, Pult, Experiment und Ablauf
    for (auto sig : {&LabController::boardChanged, &LabController::deskChanged, &LabController::experimentChanged,
                     &LabController::flowChanged})
        connect(this, sig, this, &LabController::guideChanged);
    m_timer.start(kFrameMs);
    m_clock.start();
    m_session->bridge().start();
}

LabController::~LabController() {
    m_timer.stop();
    m_session.reset();
}

bool LabController::loadExperiment(const QString& id) {
    services::Experiment exp;
    if (!m_repo.loadExperiment(id, exp)) {
        m_errors = QStringList{m_repo.lastError()};
        emit boardChanged();
        return false;
    }
    m_title = m_repo.text(exp.titleKey);
    m_flow.stop();
    m_session->clearSuccess();
    m_session->loadBoard(exp.templateBoard);
    m_experiment = std::move(exp);
    emit experimentChanged();
    emit flowChanged();
    emit deskChanged();
    emit historyChanged();
    emit progressChanged(); // Bauteilfach des Experiments
    return true;
}

bool LabController::startExperiment(const QString& id) {
    m_freeLab = false;
    // neuer Aufbau: Zeitschreiber leer und ohne Kanaele
    m_recorder.clear();
    for (std::size_t c = 0; c < model::Recorder::kChannels; ++c) m_recorder.setChannel(c, {});
    emit recorderChanged();
    services::Experiment exp;
    if (!m_repo.loadExperiment(id, exp)) {
        m_errors = QStringList{m_repo.lastError()};
        emit boardChanged();
        return false;
    }
    m_title = m_repo.text(exp.titleKey);
    model::Board start;
    start.desk = exp.templateBoard.desk; // Pultstellung der Vorlage; Einschalten ist Sache des Kindes
    start.desk.mainSwitch = false;
    start.modules = exp.templateBoard.modules; // Module stecken schon in der Modulleiste (ein Modulfach folgt)
    // gespeicherte Breite, wenn der Musteraufbau hineinpasst
    start.columns = model::highestUsedColumn(exp.templateBoard) <= m_preferredColumns ? m_preferredColumns : 12;
    m_session->setSuccess(exp.successCondition, exp.templateBoard);
    m_session->loadBoard(start);
    m_flow.start(static_cast<int>(exp.hintKeys.size()), static_cast<int>(exp.observationKeys.size()));
    m_experiment = std::move(exp);
    emit experimentChanged();
    emit flowChanged();
    emit deskChanged();
    emit historyChanged();
    emit progressChanged(); // Bauteilfach des Experiments
    return true;
}

void LabController::rebuildExperimentList() {
    m_experimentList.clear();
    for (const QString& id : m_experimentIds) {
        services::Experiment exp;
        if (!m_repo.loadExperiment(id, exp)) {
            m_errors << m_repo.lastError();
            continue;
        }
        QVariantMap m;
        m.insert(QStringLiteral("id"), id);
        m.insert(QStringLiteral("title"), m_repo.text(exp.titleKey));
        m.insert(QStringLiteral("stage"), exp.stage);
        m.insert(QStringLiteral("destructive"), exp.destructive);
        m_experimentList << m;
    }
}

void LabController::useProgressFile(const QString& path) {
    m_progress = services::ProgressStore(path);
    emit progressChanged();
}

void LabController::resetProgress() {
    m_progress.reset();
    emit progressChanged();
}

void LabController::setUnlockAll(bool on) {
    if (on == m_unlockAll) return;
    m_unlockAll = on;
    emit progressChanged();
}

// Eine Stufe ist offen, wenn alle Experimente der vorigen Stufen erledigt sind (Stufe 0 und 1 immer offen).
// Freischaltung dient der Reduktion, nicht der Belohnung (Konzept 5.5).
int LabController::highestUnlockedStage() const {
    if (m_unlockAll) return 99;
    int open = 1;
    for (const auto& st : m_stages) {
        if (st.number < 1) continue;
        if (st.number > open) break;
        bool allDone = true;
        for (const QVariant& v : m_experimentList) {
            const QVariantMap e = v.toMap();
            if (e.value(QStringLiteral("stage")).toInt() == st.number && !m_progress.isDone(e.value(QStringLiteral("id")).toString()))
                allDone = false;
        }
        if (allDone) open = st.number + 1;
    }
    return open;
}

bool LabController::isUnlocked(const QString& experimentId) const {
    for (const QVariant& v : m_experimentList) {
        const QVariantMap e = v.toMap();
        if (e.value(QStringLiteral("id")).toString() == experimentId)
            return e.value(QStringLiteral("stage")).toInt() <= highestUnlockedStage();
    }
    return false;
}

QVariantList LabController::stages() const {
    QVariantList out;
    const int open = highestUnlockedStage();
    for (const auto& st : m_stages) {
        QVariantList exps;
        int done = 0;
        for (const QVariant& v : m_experimentList) {
            QVariantMap e = v.toMap();
            if (e.value(QStringLiteral("stage")).toInt() != st.number) continue;
            const bool d = m_progress.isDone(e.value(QStringLiteral("id")).toString());
            e.insert(QStringLiteral("done"), d);
            if (d) ++done;
            exps << e;
        }
        out << QVariantMap{{QStringLiteral("number"), st.number},
                           {QStringLiteral("title"), m_repo.text(st.titleKey)},
                           {QStringLiteral("question"), m_repo.text(st.questionKey)},
                           {QStringLiteral("unlocked"), st.number <= open},
                           {QStringLiteral("done"), done},
                           {QStringLiteral("total"), static_cast<int>(exps.size())},
                           {QStringLiteral("experiments"), exps}};
    }
    return out;
}

QVariantList LabController::trayParts() const {
    const QVariantList all = catalogParts();
    QVariantList out;
    const bool guided = !m_freeLab && !m_experiment.id.isEmpty() && m_flow.phase() != model::ExperimentFlow::Phase::None;
    const int open = highestUnlockedStage();
    for (const QVariant& v : all) {
        QVariantMap p = v.toMap();
        const QString type = p.value(QStringLiteral("type")).toString();
        // Gefuehrtes Experiment: nur seine Bauteile. Freies Labor: alle, gesperrte sichtbar, aber nicht benutzbar.
        const bool locked = !m_unlockAll && p.value(QStringLiteral("stage")).toInt() > open;
        p.insert(QStringLiteral("locked"), locked);
        if (m_unlockAll || !guided)
            out << p;
        else if (m_experiment.freeParts.contains(type))
            out << p;
    }
    // freigeschaltete zuerst, gesperrte danach
    std::stable_sort(out.begin(), out.end(), [](const QVariant& a, const QVariant& b) {
        return !a.toMap().value(QStringLiteral("locked")).toBool() && b.toMap().value(QStringLiteral("locked")).toBool();
    });
    return out;
}

QVariantList LabController::trayModules() const {
    QVariantList out;
    const bool guided = !m_freeLab && !m_experiment.id.isEmpty() && m_flow.phase() != model::ExperimentFlow::Phase::None;
    const int open = highestUnlockedStage();
    for (const auto& [type, e] : m_session->catalog().modules) {
        bool inExperiment = false;
        for (const auto& m : m_experiment.templateBoard.modules) inExperiment = inExperiment || m.type == type;
        if (guided && !m_unlockAll && !inExperiment) continue;
        QStringList pins;
        for (const auto& p : e.pins) pins << qs(p);
        out << QVariantMap{{QStringLiteral("type"), qs(type)},
                           {QStringLiteral("kind"), qs(e.kind)},
                           {QStringLiteral("stage"), e.fromStage},
                           {QStringLiteral("pins"), pins},
                           {QStringLiteral("locked"), !m_unlockAll && e.fromStage > open}};
    }
    return out;
}

// ------------------------------------------------------------------ Formelrechner

QVariantMap LabController::formulaParse(const QString& text) const {
    bool ok = false;
    const double v = model::formula::parse(ss(text.trimmed()), ok);
    return {{QStringLiteral("ok"), ok}, {QStringLiteral("value"), v}};
}

QString LabController::formulaFormat(double value, const QString& unit) const {
    QString s = qs(model::formula::format(value, ss(unit)));
    return s.replace(QLatin1Char('.'), QLocale().decimalPoint());
}

QVariantMap LabController::formulaEval(const QString& expression) const {
    bool ok = false;
    const double v = model::formula::evaluate(ss(expression.trimmed()), ok);
    return {{QStringLiteral("ok"), ok}, {QStringLiteral("value"), v}};
}

QVariantMap LabController::formulaSheet(const QString& sheet, const QVariantMap& in) const {
    namespace f = model::formula;
    auto num = [&in](const char* k) { return in.value(QString::fromLatin1(k)).toDouble(); };
    QVariantMap out;
    if (sheet == QLatin1String("ohm")) {
        double u = num("U"), r = num("R"), i = num("I");
        const bool ok = f::ohm(u, r, i);
        out = {{QStringLiteral("ok"), ok}, {QStringLiteral("U"), u}, {QStringLiteral("R"), r}, {QStringLiteral("I"), i}};
    } else if (sheet == QLatin1String("led")) {
        const f::LedResistor r = f::ledResistor(num("Ub"), num("Uf"), num("I"));
        out = {{QStringLiteral("ok"), r.ok},           {QStringLiteral("R"), r.ohm},
               {QStringLiteral("norm"), r.normOhm},    {QStringLiteral("normI"), r.normA},
               {QStringLiteral("P"), r.powerW},        {QStringLiteral("tooHot"), r.tooHot}};
    } else if (sheet == QLatin1String("network")) {
        std::vector<double> rs;
        for (const char* k : {"R1", "R2", "R3"})
            if (num(k) > 0.0) rs.push_back(num(k));
        out = {{QStringLiteral("ok"), rs.size() >= 2},
               {QStringLiteral("series"), f::series(rs)},
               {QStringLiteral("parallel"), f::parallel(rs)}};
    } else if (sheet == QLatin1String("power")) {
        double p = 0.0;
        const bool ok = f::power(num("U"), num("I"), num("R"), p);
        out = {{QStringLiteral("ok"), ok}, {QStringLiteral("P"), p}, {QStringLiteral("tooHot"), p > 0.25}};
    } else if (sheet == QLatin1String("divider")) {
        const double ua = f::divider(num("Ue"), num("R1"), num("R2"));
        out = {{QStringLiteral("ok"), num("R1") + num("R2") > 0.0 && num("Ue") > 0.0}, {QStringLiteral("Ua"), ua}};
    } else if (sheet == QLatin1String("tau")) {
        const double tau = f::timeConstant(num("R"), num("C"));
        QVariantList curve; // Ladekurve 0 .. 5 tau in 50 Punkten (Anteil 0..1)
        for (int k = 0; k <= 50; ++k) curve << f::chargeAt(tau, tau * 5.0 * k / 50.0);
        out = {{QStringLiteral("ok"), tau > 0.0}, {QStringLiteral("tau"), tau}, {QStringLiteral("curve"), curve}};
    } else if (sheet == QLatin1String("astable")) {
        const double hz = f::astableHz(num("R"), num("C"));
        out = {{QStringLiteral("ok"), hz > 0.0}, {QStringLiteral("f"), hz}, {QStringLiteral("T"), hz > 0.0 ? 1.0 / hz : 0.0},
               {QStringLiteral("half"), std::log(2.0) * num("R") * num("C")}};
    }
    return out;
}

QVariantMap LabController::colorCode(double ohm, int rings) const {
    const model::resistor_code::Code code = model::resistor_code::encode(ohm, rings);
    QVariantList r;
    for (int v : code.rings) r << v;
    return {{QStringLiteral("valid"), code.valid},
            {QStringLiteral("rings"), r},
            {QStringLiteral("ohm"), code.ohm},
            {QStringLiteral("tolerance"),
             code.rings.empty() ? 0.0 : model::resistor_code::tolerancePercent(code.rings.back())}};
}

double LabController::decodeColors(const QVariantList& rings) const {
    std::vector<int> r;
    for (const QVariant& v : rings) r.push_back(v.toInt());
    return model::resistor_code::decode(r);
}

double LabController::parseOhm(const QString& text) const {
    // Ohm-Zeichen und Mikro entfernen sich als Nicht-ASCII selbst; Komma der Sprache gilt
    return model::resistor_code::parseOhm(text.toStdString());
}

QVariantList LabController::eSeries(int n) const {
    QVariantList out;
    for (double v : model::resistor_code::eSeries(n)) out << v;
    return out;
}

QVariantMap LabController::nearestE(double ohm, int n) const {
    const model::resistor_code::Nearest r = model::resistor_code::nearest(ohm, n);
    return {{QStringLiteral("value"), r.value},
            {QStringLiteral("deviation"), r.deviationPercent},
            {QStringLiteral("isMember"), r.isMember}};
}

QVariantMap LabController::schematic() const {
    const model::Schematic s = model::buildSchematic(m_session->board(), m_session->catalog());
    // Rastereinheiten (model/schematic_layout) -> Bildpunkte der Zeichnung: 1 Einheit = 32 (halbe Symbolbreite)
    constexpr double k = 32.0;
    std::vector<QString> paths(s.nodes.size());
    for (const auto& l : s.lines)
        if (l.node >= 0 && static_cast<std::size_t>(l.node) < paths.size())
            paths[static_cast<std::size_t>(l.node)] += QStringLiteral("M%1 %2 L%3 %4 ")
                                                           .arg(l.x1 * k).arg(l.y1 * k).arg(l.x2 * k).arg(l.y2 * k);
    QVariantList nodes;
    for (std::size_t i = 0; i < s.nodes.size(); ++i) {
        const auto& n = s.nodes[i];
        QStringList anchors;
        for (const auto& a : n.anchors) anchors << qs(a);
        nodes << QVariantMap{{QStringLiteral("label"), qs(n.label)},
                             {QStringLiteral("anchors"), anchors},
                             {QStringLiteral("drawn"), n.drawn},
                             {QStringLiteral("x"), n.x1 * k},
                             {QStringLiteral("x2"), n.x2 * k},
                             {QStringLiteral("y"), n.y * k},
                             {QStringLiteral("path"), paths[i]}};
    }
    QVariantList elements;
    for (const auto& e : s.elements) {
        QVariantList idx;
        for (int i : e.nodes) idx << i;
        QVariantList pinLabels;
        for (const auto& p : e.pinLabels)
            pinLabels << QVariantMap{{QStringLiteral("text"), qs(p.text)},
                                     {QStringLiteral("x"), p.x * k},
                                     {QStringLiteral("y"), p.y * k}};
        elements << QVariantMap{{QStringLiteral("id"), qs(e.id)},
                                {QStringLiteral("type"), qs(e.type)},
                                {QStringLiteral("nodes"), idx},
                                {QStringLiteral("desk"), e.desk},
                                {QStringLiteral("module"), e.module},
                                {QStringLiteral("value"), e.value},
                                {QStringLiteral("x"), e.x * k},
                                {QStringLiteral("y"), e.y * k},
                                {QStringLiteral("rotation"), e.rotation},
                                {QStringLiteral("block"), e.block},
                                {QStringLiteral("w"), e.w * k},
                                {QStringLiteral("h"), e.h * k},
                                {QStringLiteral("labelX"), e.labelX * k},
                                {QStringLiteral("labelY"), e.labelY * k},
                                {QStringLiteral("pinLabels"), pinLabels}};
    }
    QVariantList dots;
    for (const auto& d : s.dots)
        dots << QVariantMap{{QStringLiteral("x"), d.x * k}, {QStringLiteral("y"), d.y * k}, {QStringLiteral("node"), d.node}};
    return {{QStringLiteral("nodes"), nodes},
            {QStringLiteral("elements"), elements},
            {QStringLiteral("dots"), dots},
            {QStringLiteral("width"), s.width * k},
            {QStringLiteral("height"), s.height * k},
            {QStringLiteral("layers"), s.layers}};
}

QVariantList LabController::lexicon() const {
    QVariantList out;
    for (const auto& [type, e] : m_session->catalog().parts)
        if (!e.lexiconKey.empty())
            out << QVariantMap{{QStringLiteral("type"), qs(type)},
                               {QStringLiteral("text"), m_repo.text(qs(e.lexiconKey))},
                               {QStringLiteral("stage"), e.fromStage}};
    return out;
}

void LabController::setContentLanguage(const QString& code) {
    m_contentLanguage = code;
    m_repo.loadTexts(QStringLiteral("volta1"), code);
    rebuildExperimentList();
    emit progressChanged();
    emit journalChanged(); // Titel in der neuen Sprache
    if (m_freeLab)
        m_title = tr("Freies Labor");
    else if (!m_experiment.titleKey.isEmpty())
        m_title = m_repo.text(m_experiment.titleKey);
    emit experimentChanged();
    emit flowChanged();
}

QVariantMap LabController::appInfo() const {
#if defined(__clang__)
    const QString compiler = QStringLiteral("Clang %1.%2").arg(__clang_major__).arg(__clang_minor__);
#elif defined(__GNUC__)
    const QString compiler = QStringLiteral("GCC %1.%2 (MinGW-w64)").arg(__GNUC__).arg(__GNUC_MINOR__);
#elif defined(_MSC_VER)
    const QString compiler = QStringLiteral("MSVC %1").arg(_MSC_VER);
#else
    const QString compiler = QStringLiteral("C++17");
#endif
    return {{QStringLiteral("name"), QCoreApplication::applicationName()},
            {QStringLiteral("version"), QCoreApplication::applicationVersion()},
            {QStringLiteral("qtVersion"), QString::fromLatin1(qVersion())},
            {QStringLiteral("compiler"), compiler},
            {QStringLiteral("platform"), QSysInfo::prettyProductName() + QStringLiteral(" (") +
                                             QSysInfo::currentCpuArchitecture() + QStringLiteral(")")},
            {QStringLiteral("hasLogo"), QFile::exists(QStringLiteral(":/brand/dc_logo.png"))}};
}

void LabController::startFreeLab() {
    m_freeLab = true;
    // neuer Aufbau: Zeitschreiber leer und ohne Kanaele
    m_recorder.clear();
    for (std::size_t c = 0; c < model::Recorder::kChannels; ++c) m_recorder.setChannel(c, {});
    emit recorderChanged();
    m_flow.stop();
    m_experiment = services::Experiment{};
    m_title = tr("Freies Labor");
    m_session->clearSuccess();
    model::Board empty;
    empty.columns = m_preferredColumns;      // gespeicherte Breite
    empty.desk.meterVisible = m_preferMeter; // Messgeraet wie zuletzt im freien Labor
    m_session->loadBoard(empty);
    emit experimentChanged();
    emit flowChanged();
    emit deskChanged();
    emit historyChanged();
    emit progressChanged(); // Bauteilfach des freien Labors
}

void LabController::chooseGuess(int index) {
    if (!m_flow.chooseGuess(index)) return;
    // Forscherheft: Vermutung festhalten, ohne sie zu bewerten
    if (index < m_experiment.guessKeys.size()) m_journal.setGuess(m_experiment.id, m_repo.text(m_experiment.guessKeys.at(index)));
    emit flowChanged();
    emit journalChanged();
}

void LabController::chooseObservation(int index) {
    const int g = m_flow.guess();
    if (!m_flow.chooseObservation(index, m_experiment.guessMatches.contains(g), m_experiment.guessOpen.contains(g)))
        return;
    static const char* const kComparison[] = {"", "passt", "anders", "offen"};
    m_journal.setObservation(m_experiment.id, m_repo.text(m_experiment.observationKeys.at(index)),
                             QString::fromLatin1(kComparison[comparison()]));
    emit flowChanged();
    emit journalChanged();
}

bool LabController::addNote(const QString& text) {
    if (m_experiment.id.isEmpty() || !m_journal.addNote(m_experiment.id, text)) return false;
    emit journalChanged();
    return true;
}

void LabController::useJournalFile(const QString& path) {
    m_journal = services::JournalStore(path);
    emit journalChanged();
}

QVariantList LabController::journal() const {
    QVariantList out;
    for (const QJsonObject& e : m_journal.entries()) {
        const QString id = e.value(QStringLiteral("id")).toString();
        QString title = id;
        for (const QVariant& v : m_experimentList)
            if (v.toMap().value(QStringLiteral("id")).toString() == id) title = v.toMap().value(QStringLiteral("title")).toString();
        const QDateTime when = QDateTime::fromString(e.value(QStringLiteral("datum")).toString(), Qt::ISODate);
        QStringList notes;
        for (const QJsonValue& n : e.value(QStringLiteral("notizen")).toArray()) notes << n.toString();
        const QString cmp = e.value(QStringLiteral("vergleich")).toString();
        out << QVariantMap{{QStringLiteral("id"), id},
                           {QStringLiteral("title"), title},
                           {QStringLiteral("date"), QLocale().toString(when, QLocale::ShortFormat)},
                           {QStringLiteral("guess"), e.value(QStringLiteral("vermutung")).toString()},
                           {QStringLiteral("observation"), e.value(QStringLiteral("beobachtung")).toString()},
                           {QStringLiteral("comparison"), cmp == QLatin1String("passt") ? 1 : cmp == QLatin1String("anders") ? 2
                                                          : cmp == QLatin1String("offen") ? 3 : 0},
                           {QStringLiteral("notes"), notes}};
    }
    return out;
}

void LabController::nextHint() {
    if (m_flow.nextHint()) emit flowChanged();
}

bool LabController::clearBoard() {
    model::Board after = m_session->board();
    if (after.parts.empty() && after.wires.empty() && after.modules.empty()) return false;
    after.parts.clear();
    after.wires.clear();
    after.modules.clear();
    return result(m_session->replaceBoard(after, tr("Alles abbauen")));
}

bool LabController::showTemplate() {
    if (!m_flow.solutionShown() || m_flow.phase() != model::ExperimentFlow::Phase::Build) return false;
    return result(m_session->replaceBoard(m_experiment.templateBoard, tr("Musteraufbau zeigen")));
}

void LabController::updateFinish() {
    const bool building = m_flow.phase() == model::ExperimentFlow::Phase::Build;
    const bool complete = building && m_session->templateComplete();
    const bool can = complete && m_session->successReached();
    if (complete == m_buildComplete && can == m_canFinish) return;
    m_buildComplete = complete;
    m_canFinish = can;
    emit finishChanged();
}

bool LabController::finishExperiment() {
    updateFinish();
    if (!m_canFinish || !m_flow.reportSuccess()) return false;
    m_progress.markDone(m_experiment.id); // Lernpfad: erledigt (bleibt erledigt)
    updateFinish();
    emit flowChanged();
    emit progressChanged();
    return true;
}

QString LabController::nextExperimentId() const {
    const qsizetype i = m_experimentIds.indexOf(m_experiment.id);
    return i >= 0 && i + 1 < m_experimentIds.size() ? m_experimentIds.at(i + 1) : QString();
}

QStringList LabController::texts(const QStringList& keys) const {
    QStringList out;
    for (const QString& k : keys) out << m_repo.text(k);
    return out;
}

void LabController::onFrame() {
    const double elapsed = m_clock.restart() / 1000.0;
    m_session->poll();
    updateFinish();
    updateAudio();
    sampleRecorder();
    m_sinceNotifyS += elapsed;
    if (m_sinceNotifyS >= kNotifyS) {
        m_sinceNotifyS = 0.0;
        emit stateChanged();
    }
}

// ------------------------------------------------------------------ Zeitschreiber

QStringList LabController::recorderChannels() const {
    QStringList out;
    for (std::size_t c = 0; c < model::Recorder::kChannels; ++c) out << qs(m_recorder.channel(c).source);
    return out;
}

void LabController::setRecorderWindow(double seconds) {
    m_recorder.setWindow(seconds);
    emit recorderChanged();
}

void LabController::setRecorderPaused(bool paused) {
    m_recorder.setPaused(paused);
    emit recorderChanged();
}

void LabController::setRecorderChannel(int channel, const QString& source) {
    if (channel < 0 || channel >= static_cast<int>(model::Recorder::kChannels)) return;
    m_recorder.setChannel(static_cast<std::size_t>(channel), ss(source));
    emit recorderChanged();
}

void LabController::clearRecorder() {
    m_recorder.clear();
    emit recorderChanged();
}

namespace {
// Einheit der Anzeige des Messgeraets und Umrechnung (Strom in mA)
QString meterUnit(core::MeterMode mode) {
    return mode == core::MeterMode::Ampere ? QStringLiteral("mA") : mode == core::MeterMode::Ohm ? QStringLiteral("Ω")
                                                                                                  : QStringLiteral("V");
}
} // namespace

void LabController::sampleRecorder() {
    if (!m_recorder.active() || !m_session->snapshotCurrent()) return;
    std::array<double, model::Recorder::kChannels> v{};
    std::array<bool, model::Recorder::kChannels> has{};
    for (std::size_t c = 0; c < model::Recorder::kChannels; ++c) {
        const std::string& src = m_recorder.channel(c).source;
        if (src.empty()) continue;
        if (src == "MESS") {
            const core::ComponentReading* r = m_session->deskReading(model::desk_id::kMeter);
            if (!r || !m_session->board().desk.meterVisible) continue;
            const bool amp = m_session->board().desk.meterMode == core::MeterMode::Ampere;
            v[c] = r->displayValue * (amp ? 1e3 : 1.0);
            has[c] = true;
        } else {
            core::NodeId n = 0;
            if (!m_session->netlist().nodeOf(src, n)) { // Feld ohne Verbindung: 0 V
                v[c] = 0.0;
                has[c] = true;
                continue;
            }
            v[c] = m_session->voltage(src);
            has[c] = true;
        }
    }
    m_recorder.push(m_session->snapshot().timeS, v, has);
}

QVariantMap LabController::recorderData(int maxPoints) const {
    QVariantList channels;
    for (std::size_t c = 0; c < model::Recorder::kChannels; ++c) {
        const auto s = m_recorder.series(c, static_cast<std::size_t>(std::max(maxPoints, 2)));
        QVariantList pts;
        pts.reserve(static_cast<int>(s.points.size() * 2));
        for (const auto& [t, v] : s.points) pts << t << v;
        const std::string& src = m_recorder.channel(c).source;
        channels << QVariantMap{{QStringLiteral("source"), qs(src)},
                                {QStringLiteral("unit"), src == "MESS" ? meterUnit(m_session->board().desk.meterMode)
                                                                       : QStringLiteral("V")},
                                {QStringLiteral("points"), pts},
                                {QStringLiteral("min"), s.min},
                                {QStringLiteral("max"), s.max},
                                {QStringLiteral("any"), s.any},
                                {QStringLiteral("last"), s.points.empty() ? 0.0 : s.points.back().second}};
    }
    return {{QStringLiteral("window"), m_recorder.window()}, {QStringLiteral("channels"), channels}};
}

// Tonausgabe: Pult-Lautsprecher (Ton oder Knacken je Impuls) und Summer auf der Grundplatte
void LabController::updateAudio() {
    if (!m_audio) return;
    std::vector<model::SynthVoice> voices;
    core::ComponentReading ls;
    if (m_session->speakerReading(ls) && !ls.destroyed) {
        if (ls.frequencyHz >= 20.0 && ls.level > 0.02)
            voices.push_back({ls.frequencyHz, ls.level});
        else if (m_lastPulses >= 0.0 && ls.displayValue > m_lastPulses)
            m_audio->click(0.7);
        m_lastPulses = ls.displayValue;
    }
    for (const auto& p : m_session->board().parts) {
        if (p.type != "summer") continue;
        const core::ComponentReading* r = m_session->reading(p.id);
        if (r && !r->destroyed && r->frequencyHz > 0.0 && r->level > 0.02)
            voices.push_back({r->frequencyHz, 0.6 * r->level});
    }
    m_audio->setVoices(voices);
}

bool LabController::result(bool ok) {
    if (!ok) {
        // Meldung in der Anzeigesprache (Quelltext und Platzhalter aus dem Modell)
        const model::Message& m = m_session->lastMessage();
        QString text = m.empty() ? QString() : QCoreApplication::translate("Meldung", m.source);
        for (const auto& a : m.args) text = text.arg(qs(a));
        m_lastEditError = text;
        emit editFailed();
    }
    return ok;
}

// ------------------------------------------------------------------ Anzeigen

double LabController::simTime() const { return m_session->snapshot().timeS; }
double LabController::realtimeRatio() const { return m_session->snapshot().realtimeRatio; }
bool LabController::mainSwitch() const { return m_session->board().desk.mainSwitch; }
int LabController::cells() const { return m_session->board().desk.cells; }

double LabController::voltage(const QString& anchor) const { return m_session->voltage(ss(anchor)); }
int LabController::voltageBars(const QString& anchor) { return m_session->voltageBars(ss(anchor)); }

QVariantMap LabController::partState(const QString& id) const {
    QVariantMap m;
    if (const model::PartPlacement* p = m_session->board().findPart(ss(id))) m.insert(QStringLiteral("closed"), p->closed);
    const core::ComponentReading* r = m_session->reading(ss(id));
    if (!r) return m;
    m.insert(QStringLiteral("currentmA"), r->currentA * 1e3);
    m.insert(QStringLiteral("voltV"), r->voltV);
    m.insert(QStringLiteral("level"), r->level);
    m.insert(QStringLiteral("frequencyHz"), r->frequencyHz);
    m.insert(QStringLiteral("overloaded"), r->overloaded);
    m.insert(QStringLiteral("destroyed"), r->destroyed);
    m.insert(QStringLiteral("tempC"), r->tempK - 273.15);
    return m;
}

void LabController::setGuideActive(bool on) {
    if (m_guideActive == on) return;
    m_guideActive = on;
    emit guideChanged();
}

namespace {
// Anschlusspunkt fuer Texte: "P:+9V" -> "+9V", "M:3" -> "M3", "MESS:VOHM" -> "V/Ohm"
QString anchorLabel(const std::string& a) {
    QString s = QString::fromStdString(a);
    if (s.startsWith(QLatin1String("P:"))) return s.mid(2);
    if (s.startsWith(QLatin1String("M:"))) return QStringLiteral("M") + s.mid(2);
    if (s.startsWith(QLatin1String("MESS:"))) return s.mid(5).replace(QLatin1String("VOHM"), QStringLiteral("V/Ω"));
    return s;
}
} // namespace

QVariantMap LabController::guideStep() const {
    QVariantMap m;
    if (!m_guideActive || m_freeLab || m_experiment.id.isEmpty()) return m;
    const auto steps = model::buildGuide(m_experiment.templateBoard, m_session->catalog());
    const std::size_t i = model::currentStep(steps, m_session->board(), m_session->catalog());
    m.insert(QStringLiteral("index"), static_cast<int>(i));
    m.insert(QStringLiteral("count"), static_cast<int>(steps.size()));
    if (i >= steps.size()) {
        m.insert(QStringLiteral("kind"), QStringLiteral("fertig"));
        return m;
    }
    const model::GuideStep& s = steps[i];
    static const char* kinds[] = {"modul", "bauteil", "leitung", "hauptschalter"};
    m.insert(QStringLiteral("kind"), QString::fromLatin1(kinds[static_cast<int>(s.kind)]));
    m.insert(QStringLiteral("id"), qs(s.id));
    m.insert(QStringLiteral("type"), qs(s.type));
    m.insert(QStringLiteral("value"), s.value);
    m.insert(QStringLiteral("from"), qs(s.from));
    m.insert(QStringLiteral("to"), qs(s.to));
    m.insert(QStringLiteral("color"), qs(s.color));
    QVariantList pins;
    for (const auto& [name, field] : s.pins)
        pins << QVariantMap{{QStringLiteral("name"), qs(name)}, {QStringLiteral("field"), qs(field)}};
    m.insert(QStringLiteral("pins"), pins);
    QStringList targets, labels;
    for (const auto& t : s.targets) {
        targets << qs(t);
        QString label = anchorLabel(t);
        // Feld der Modulleiste: Anschlussname des Moduls dazu, z. B. "M3 (ein)"
        if (t.rfind("M:", 0) == 0) {
            const int n = std::atoi(t.c_str() + 2);
            for (const auto& mod : m_session->board().modules) {
                const model::ModuleEntry* e = m_session->catalog().module(mod.type);
                if (!e || n < mod.slot || n >= mod.slot + static_cast<int>(e->pins.size())) continue;
                const std::string& pin = e->pins[static_cast<std::size_t>(n - mod.slot)];
                label += QStringLiteral(" (%1)").arg(pin == "plus" ? QStringLiteral("+")
                                                     : pin == "minus" ? QStringLiteral("−") : qs(pin));
            }
        }
        labels << label;
    }
    m.insert(QStringLiteral("targets"), targets);
    m.insert(QStringLiteral("labels"), labels);
    return m;
}

QVariantMap LabController::moduleState(const QString& id) const {
    QVariantMap m;
    const std::string mid = ss(id);
    const core::ComponentReading* r = m_session->moduleReading(mid);
    if (!r) return m;
    std::string type;
    for (const auto& mod : m_session->board().modules)
        if (mod.id == mid) type = mod.type;
    if (type == "tonmodul") {
        m.insert(QStringLiteral("active"), r->frequencyHz > 0.0 || r->displayValue >= 0.0);
        m.insert(QStringLiteral("frequencyHz"), r->frequencyHz);
        m.insert(QStringLiteral("level"), r->level);
        m.insert(QStringLiteral("note"), static_cast<int>(r->displayValue));
    } else {
        // Blinkmodul: Ausgangstransistor leitet (Kollektorstrom ueber 1 mA)
        m.insert(QStringLiteral("active"), std::fabs(r->terminalCurrentA[1]) > 1e-3);
    }
    return m;
}

QVariantMap LabController::moduleInside(const QString& id) const {
    QVariantMap m;
    const std::string mid = ss(id);
    std::string type;
    for (const auto& mod : m_session->board().modules)
        if (mod.id == mid) type = mod.type;
    m.insert(QStringLiteral("type"), qs(type));
    const model::ModuleEntry* e = m_session->catalog().module(type);
    m.insert(QStringLiteral("behavioral"), e && e->kind == "verhaltensmodell");
    const auto& net = m_session->netlist();
    const auto range = net.modules.find(mid);
    const auto& snap = m_session->snapshot();
    if (range == net.modules.end() || !m_session->snapshotCurrent()) return m;
    if (type == "tonmodul") {
        if (const core::ComponentReading* r = m_session->moduleReading(mid)) {
            m.insert(QStringLiteral("supplyV"), r->voltV);
            m.insert(QStringLiteral("outmA"), r->currentA * 1e3);
            m.insert(QStringLiteral("note"), static_cast<int>(r->displayValue));
            m.insert(QStringLiteral("frequencyHz"), r->frequencyHz);
            m.insert(QStringLiteral("playing"), r->displayValue >= 0.0);
            m.insert(QStringLiteral("inV"), m_session->voltage(mid + ".ein") - m_session->voltage(mid + ".minus"));
        }
        return m;
    }
    // Teilschaltung: Messwerte der inneren Bauteile (Kennung "<Modul>.<Name>")
    QVariantList items;
    for (std::size_t i = range->second.first; i < range->second.first + range->second.count; ++i) {
        if (i >= snap.readings.size() || i >= net.componentIds.size()) break;
        const core::ComponentReading& r = snap.readings[i];
        const std::string& cid = net.componentIds[i];
        const QString name = qs(cid.substr(cid.find('.') + 1));
        const bool transistor = name.startsWith(QLatin1Char('T'));
        // Transistor: Kollektorstrom (Anschluss 1); sonst Strom am Anschluss 0
        const double i_mA = (transistor ? r.terminalCurrentA[1] : r.currentA) * 1e3;
        items << QVariantMap{{QStringLiteral("name"), name},
                             {QStringLiteral("currentmA"), i_mA},
                             {QStringLiteral("voltV"), r.voltV},
                             {QStringLiteral("on"), transistor && std::fabs(i_mA) > 1.0}};
    }
    m.insert(QStringLiteral("items"), items);
    return m;
}

double LabController::wireCurrent(const QString& id) const {
    const auto& net = m_session->netlist();
    const auto it = net.wires.find(ss(id));
    const auto& snap = m_session->snapshot();
    if (!m_session->snapshotCurrent() || it == net.wires.end() || it->second >= snap.readings.size()) return 0.0;
    return snap.readings[it->second].currentA * 1e3;
}

bool LabController::rotatePart(const QString& id) { return result(m_session->rotatePart(ss(id))); }

bool LabController::setBoardColumns(int columns) {
    if (!result(m_session->setColumns(columns))) return false;
    emit columnsChosen(columns);
    return true;
}

QVariantList LabController::parts() const {
    QVariantList out;
    const model::SocketAssignment sockets = model::assignSockets(m_session->board());
    for (const auto& p : m_session->board().parts) {
        QVariantList pinSockets; // Loch je Anschluss (0 links ... 4 rechts)
        if (const auto it = sockets.partPins.find(p.id); it != sockets.partPins.end())
            for (int s : it->second) pinSockets << s;
        QStringList anchors;
        QStringList pinNames;
        for (const auto& [name, anchor] : p.pins) {
            pinNames << qs(name);
            anchors << qs(anchor);
        }
        // Lage aus den ersten beiden Feldern: gleiche Zeile = waagrecht; Abstand 2 = lange Spannweite
        bool vertical = false;
        bool longSpan = false;
        if (p.pins.size() == 2) {
            const std::string& a = p.pins[0].second;
            const std::string& b = p.pins[1].second;
            if (a.size() >= 2 && b.size() >= 2) {
                vertical = a[0] != b[0];
                const int d = vertical ? std::abs(b[0] - a[0]) : std::abs(std::stoi(b.substr(1)) - std::stoi(a.substr(1)));
                longSpan = d == 2;
            }
        } else if (p.pins.size() == 3) {
            vertical = p.pins[0].second[0] != p.pins[2].second[0];
        }
        // Startfeld = oberstes bzw. linkestes Feld (Bezug fuer Verschieben und Drehen)
        std::string start;
        for (const auto& pin : p.pins) {
            const std::string& f = pin.second;
            if (start.empty() || f[0] < start[0] ||
                (f[0] == start[0] && f.size() > 1 && start.size() > 1 && std::stoi(f.substr(1)) < std::stoi(start.substr(1))))
                start = f;
        }
        out << QVariantMap{{QStringLiteral("id"), qs(p.id)},
                           {QStringLiteral("type"), qs(p.type)},
                           {QStringLiteral("start"), qs(start)},
                           {QStringLiteral("value"), p.value},
                           {QStringLiteral("closed"), p.closed},
                           {QStringLiteral("vertical"), vertical},
                           {QStringLiteral("longSpan"), longSpan},
                           {QStringLiteral("pins"), pinNames},
                           {QStringLiteral("anchors"), anchors},
                           {QStringLiteral("sockets"), pinSockets}};
    }
    return out;
}

QVariantList LabController::wires() const {
    QVariantList out;
    const model::SocketAssignment sockets = model::assignSockets(m_session->board());
    for (const auto& w : m_session->board().wires) {
        const auto it = sockets.wireEnds.find(w.id);
        const std::pair<int, int> ends = it != sockets.wireEnds.end() ? it->second : std::pair<int, int>{2, 2};
        // Modulanschluss ("BL1.aus") auf dem Feld der Modulleiste darstellen
        auto shown = [this](const std::string& a) {
            const std::string slot = model::moduleSlotOf(m_session->board(), m_session->catalog(), a);
            return qs(slot.empty() ? a : slot);
        };
        out << QVariantMap{{QStringLiteral("id"), qs(w.id)},
                           {QStringLiteral("from"), shown(w.from)},
                           {QStringLiteral("to"), shown(w.to)},
                           {QStringLiteral("fromSocket"), ends.first},
                           {QStringLiteral("toSocket"), ends.second},
                           {QStringLiteral("color"), qs(w.color)}};
    }
    return out;
}

QVariantList LabController::modules() const {
    QVariantList out;
    for (const auto& m : m_session->board().modules) {
        const model::ModuleEntry* e = m_session->catalog().module(m.type);
        QStringList pins;
        if (e)
            for (const auto& p : e->pins) pins << qs(p);
        out << QVariantMap{{QStringLiteral("id"), qs(m.id)},
                           {QStringLiteral("type"), qs(m.type)},
                           {QStringLiteral("slot"), m.slot},
                           {QStringLiteral("width"), e ? static_cast<int>(e->pins.size()) : 1},
                           {QStringLiteral("pins"), pins},
                           {QStringLiteral("behavioral"), e && e->kind == "verhaltensmodell"}};
    }
    return out;
}

QStringList LabController::voltageFields() const {
    QStringList out;
    for (const auto& f : m_session->board().voltageDisplays) out << qs(f);
    return out;
}

QVariantList LabController::catalogParts() const {
    QVariantList out;
    for (const auto& [type, e] : m_session->catalog().parts)
    {
        QVariantList volts; // Gluehlampe: waehlbare Nennspannungen
        for (const auto& v : e.variants)
            if (const auto it = v.find("u_nenn_V"); it != v.end()) volts << it->second;
        out << QVariantMap{{QStringLiteral("type"), qs(type)},
                           {QStringLiteral("model"), qs(e.model)},
                           {QStringLiteral("stage"), e.fromStage},
                           {QStringLiteral("pins"), static_cast<int>(e.pins.size())},
                           {QStringLiteral("voltages"), volts},
                           {QStringLiteral("defaultVolt"), e.param("u_nenn_V", 0.0)}};
    }
    return out;
}

// ------------------------------------------------------------------ Befehle

bool LabController::addPart(const QString& type, const QString& start, bool vertical, bool longSpan) {
    for (const QVariant& v : trayParts()) {
        const QVariantMap p = v.toMap();
        if (p.value(QStringLiteral("type")).toString() == type && p.value(QStringLiteral("locked")).toBool()) {
            m_lastEditError = tr("Dieses Bauteil wird mit Stufe %1 freigeschaltet.")
                                  .arg(p.value(QStringLiteral("stage")).toInt());
            emit editFailed();
            return false;
        }
    }
    return result(m_session->addPart(ss(type), ss(start), orient(vertical), spanOf(longSpan)));
}
QVariantMap LabController::partPreview(const QString& type, const QString& start, bool vertical, bool longSpan) const {
    std::vector<std::pair<std::string, std::string>> pins;
    const bool ok = m_session->previewPart(ss(type), ss(start), orient(vertical), spanOf(longSpan), pins);
    QVariantList list;
    QStringList fields;
    for (const auto& [name, field] : pins) {
        list << QVariantMap{{QStringLiteral("name"), qs(name)}, {QStringLiteral("field"), qs(field)}};
        if (!fields.contains(qs(field))) fields << qs(field);
    }
    return {{QStringLiteral("pins"), list}, {QStringLiteral("fields"), fields}, {QStringLiteral("ok"), ok}};
}

bool LabController::movePart(const QString& id, const QString& start, bool vertical, bool longSpan) {
    return result(m_session->movePart(ss(id), ss(start), orient(vertical), spanOf(longSpan)));
}
bool LabController::removePart(const QString& id) { return result(m_session->removePart(ss(id))); }

namespace {
// "M:3" -> 3; 0 bei anderem Anschlusspunkt
int slotNumber(const QString& anchor) {
    if (!anchor.startsWith(QLatin1String("M:"))) return 0;
    bool ok = false;
    const int n = anchor.mid(2).toInt(&ok);
    return ok ? n : 0;
}
} // namespace

bool LabController::addModule(const QString& type, const QString& slotAnchor) {
    for (const QVariant& v : trayModules()) {
        const QVariantMap p = v.toMap();
        if (p.value(QStringLiteral("type")).toString() == type && p.value(QStringLiteral("locked")).toBool()) {
            m_lastEditError = tr("Dieses Modul wird mit Stufe %1 freigeschaltet.")
                                  .arg(p.value(QStringLiteral("stage")).toInt());
            emit editFailed();
            return false;
        }
    }
    return result(m_session->addModule(ss(type), slotNumber(slotAnchor)));
}
bool LabController::moveModule(const QString& id, const QString& slotAnchor) {
    return result(m_session->moveModule(ss(id), slotNumber(slotAnchor)));
}
bool LabController::removeModule(const QString& id) { return result(m_session->removeModule(ss(id))); }
bool LabController::flipPart(const QString& id) { return result(m_session->flipPart(ss(id))); }
bool LabController::setPartValue(const QString& id, double v) { return result(m_session->setPartValue(ss(id), v)); }
bool LabController::repairPart(const QString& id) { return result(m_session->repairPart(ss(id))); }
bool LabController::addWire(const QString& from, const QString& to, const QString& color) {
    std::string c = ss(color);
    if (m_autoWireColor)
        if (const std::string pole = model::poleColor(m_session->board(), ss(from), ss(to)); !pole.empty()) c = pole;
    return result(m_session->addWire(ss(from), ss(to), c));
}
bool LabController::moveWireEnd(const QString& id, bool fromEnd, const QString& anchor) {
    return result(m_session->moveWireEnd(ss(id), fromEnd, ss(anchor), m_autoWireColor));
}
bool LabController::removeWire(const QString& id) { return result(m_session->removeWire(ss(id))); }
bool LabController::setWireColor(const QString& id, const QString& color) {
    return result(m_session->setWireColor(ss(id), ss(color)));
}
bool LabController::toggleVoltageDisplay(const QString& field) {
    return result(m_session->toggleVoltageDisplay(ss(field)));
}
void LabController::undo() { m_session->undoStack().undo(); }
void LabController::redo() { m_session->undoStack().redo(); }
bool LabController::canUndo() const { return m_session->undoStack().canUndo(); }
bool LabController::canRedo() const { return m_session->undoStack().canRedo(); }
QString LabController::undoText() const { return m_session->undoStack().undoText(); }
QString LabController::redoText() const { return m_session->undoStack().redoText(); }

// ------------------------------------------------------------------ Bedienhandlungen

void LabController::setMainSwitch(bool on) {
    if (mainSwitch() == on) return;
    m_session->setMainSwitch(on);
    emit deskChanged();
}
void LabController::setCells(int cells) {
    if (this->cells() == cells) return;
    m_session->setCells(cells);
    emit deskChanged();
}
void LabController::setTimeScale(double scale) {
    if (scale == m_timeScale) return;
    m_timeScale = scale;
    m_session->bridge().setTimeScale(scale);
    emit deskChanged();
}
void LabController::setPartClosed(const QString& id, bool closed) {
    // Bedienhandlung: kein boardChanged (die Bauteile wuerden neu aufgebaut, ein gedrueckter Taster verloere den
    // Griff); die Stellung erscheint ueber partState()
    m_session->setPartClosed(ss(id), closed);
    emit stateChanged();
}
void LabController::setDeskSwitch(bool closed) {
    m_session->setDeskSwitch(closed);
    emit deskChanged();
}
void LabController::setPot(int which, double position) {
    m_session->setPot(which, position);
    emit deskChanged();
}
void LabController::setMeterVisible(bool visible) {
    if (result(m_session->setMeterVisible(visible))) {
        emit deskChanged();
        emit boardChanged(); // Buchsen erscheinen bzw. verschwinden
    }
}

void LabController::setMeterMode(int mode) {
    m_session->setMeterMode(static_cast<core::MeterMode>(std::clamp(mode, 0, 2)));
    emit deskChanged();
}

void LabController::setMeterAnalog(bool analog) {
    m_session->setMeterAnalog(analog);
    emit deskChanged();
}

bool LabController::replaceMeterFuse() { return result(m_session->replaceMeterFuse()); }

bool LabController::repairSpeaker() { return result(m_session->repairSpeaker()); }

QVariantMap LabController::speakerState() const {
    QVariantMap m;
    core::ComponentReading reading;
    if (!m_session->speakerReading(reading)) return m;
    const core::ComponentReading* r = &reading;
    m.insert(QStringLiteral("frequencyHz"), r->frequencyHz);
    m.insert(QStringLiteral("level"), r->level);
    m.insert(QStringLiteral("pulses"), r->displayValue);
    m.insert(QStringLiteral("overloaded"), r->overloaded);
    m.insert(QStringLiteral("destroyed"), r->destroyed);
    return m;
}

QVariantMap LabController::meterState() {
    QVariantMap m;
    const core::ComponentReading* r = m_session->deskReading(model::desk_id::kMeter);
    m.insert(QStringLiteral("valid"), r != nullptr);
    if (!r) return m;
    const auto mode = m_session->board().desk.meterMode;
    const model::MeterText t = model::formatMeter(mode, r->displayValue);
    QString value = QString::fromStdString(t.value);
    value.replace(QLatin1Char('.'), QLocale().decimalPoint());
    if (r->displayRange > 0.0) m_meterRangeV = r->displayRange; // letzter Bereich bleibt fuer die Skala stehen
    m.insert(QStringLiteral("text"), value);
    m.insert(QStringLiteral("unit"), QString::fromStdString(t.unit));
    m.insert(QStringLiteral("level"), r->level);
    m.insert(QStringLiteral("range"), m_meterRangeV); // V, A bzw. Skalenmitte in Ohm
    m.insert(QStringLiteral("mode"), static_cast<int>(mode));
    m.insert(QStringLiteral("overrange"), (r->displayFlags & core::kDisplayOverrange) != 0);
    m.insert(QStringLiteral("foreignVoltage"), (r->displayFlags & core::kDisplayForeignVoltage) != 0);
    m.insert(QStringLiteral("fuseBlown"), (r->displayFlags & core::kDisplayFuseBlown) != 0);
    return m;
}

void LabController::setRoomLight(int level) {
    m_session->setRoomLight(level);
    emit deskChanged();
}

QVariantList LabController::tools() const {
    QVariantList out;
    for (const auto& t : m_session->board().desk.tools)
        out << QVariantMap{{QStringLiteral("type"), qs(t.type)},
                           {QStringLiteral("col"), t.col},
                           {QStringLiteral("row"), t.row},
                           {QStringLiteral("on"), t.on}};
    return out;
}

void LabController::placeTool(const QString& type, double col, double row) {
    m_session->placeTool(ss(type), col, row);
    emit deskChanged();
}

void LabController::setToolOn(const QString& type, bool on) {
    m_session->setToolOn(ss(type), on);
    emit deskChanged();
}

void LabController::removeTool(const QString& type) {
    m_session->removeTool(ss(type));
    emit deskChanged();
}

void LabController::setMotorBlocked(const QString& id, bool blocked) {
    m_session->setMotorBlocked(ss(id), blocked);
}

} // namespace volta

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
        const QVariantMap p = v.toMap();
        const QString type = p.value(QStringLiteral("type")).toString();
        if (m_unlockAll)
            out << p;
        else if (guided ? m_experiment.freeParts.contains(type) : p.value(QStringLiteral("stage")).toInt() <= open)
            out << p;
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
    QVariantList nodes;
    for (const auto& n : s.nodes) {
        QStringList anchors;
        for (const auto& a : n.anchors) anchors << qs(a);
        nodes << QVariantMap{{QStringLiteral("label"), qs(n.label)},
                             {QStringLiteral("x"), n.x},
                             {QStringLiteral("y"), n.y},
                             {QStringLiteral("anchors"), anchors}};
    }
    QVariantList elements;
    for (const auto& e : s.elements) {
        QVariantList idx;
        for (int i : e.nodes) idx << i;
        elements << QVariantMap{{QStringLiteral("id"), qs(e.id)},
                                {QStringLiteral("type"), qs(e.type)},
                                {QStringLiteral("nodes"), idx},
                                {QStringLiteral("desk"), e.desk},
                                {QStringLiteral("par"), e.parallelIndex},
                                {QStringLiteral("parCount"), e.parallelCount}};
    }
    return {{QStringLiteral("nodes"), nodes}, {QStringLiteral("elements"), elements}, {QStringLiteral("layers"), s.layers}};
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
    m_flow.stop();
    m_experiment = services::Experiment{};
    m_title = tr("Freies Labor");
    m_session->clearSuccess();
    m_session->loadBoard(model::Board{});
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

bool LabController::showTemplate() {
    if (!m_flow.solutionShown() || m_flow.phase() != model::ExperimentFlow::Phase::Build) return false;
    return result(m_session->replaceBoard(m_experiment.templateBoard, tr("Musteraufbau zeigen")));
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
    if (m_flow.phase() == model::ExperimentFlow::Phase::Build && m_session->successReached() && m_flow.reportSuccess()) {
        m_progress.markDone(m_experiment.id); // Lernpfad: erledigt (bleibt erledigt)
        emit flowChanged();
        emit progressChanged();
    }
    m_sinceNotifyS += elapsed;
    if (m_sinceNotifyS >= kNotifyS) {
        m_sinceNotifyS = 0.0;
        emit stateChanged();
    }
}

bool LabController::result(bool ok) {
    if (!ok) {
        m_lastEditError = qs(m_session->lastError());
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

double LabController::wireCurrent(const QString& id) const {
    const auto& net = m_session->netlist();
    const auto it = net.wires.find(ss(id));
    const auto& snap = m_session->snapshot();
    if (!m_session->snapshotCurrent() || it == net.wires.end() || it->second >= snap.readings.size()) return 0.0;
    return snap.readings[it->second].currentA * 1e3;
}

bool LabController::rotatePart(const QString& id) { return result(m_session->rotatePart(ss(id))); }

bool LabController::setBoardColumns(int columns) { return result(m_session->setColumns(columns)); }

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
        out << QVariantMap{{QStringLiteral("id"), qs(w.id)},
                           {QStringLiteral("from"), qs(w.from)},
                           {QStringLiteral("to"), qs(w.to)},
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
        out << QVariantMap{{QStringLiteral("id"), qs(m.id)},
                           {QStringLiteral("type"), qs(m.type)},
                           {QStringLiteral("slot"), m.slot},
                           {QStringLiteral("width"), e ? static_cast<int>(e->pins.size()) : 1}};
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
    return result(m_session->addPart(ss(type), ss(start), orient(vertical), spanOf(longSpan)));
}
bool LabController::movePart(const QString& id, const QString& start, bool vertical, bool longSpan) {
    return result(m_session->movePart(ss(id), ss(start), orient(vertical), spanOf(longSpan)));
}
bool LabController::removePart(const QString& id) { return result(m_session->removePart(ss(id))); }
bool LabController::flipPart(const QString& id) { return result(m_session->flipPart(ss(id))); }
bool LabController::setPartValue(const QString& id, double v) { return result(m_session->setPartValue(ss(id), v)); }
bool LabController::repairPart(const QString& id) { return result(m_session->repairPart(ss(id))); }
bool LabController::addWire(const QString& from, const QString& to, const QString& color) {
    return result(m_session->addWire(ss(from), ss(to), ss(color)));
}
bool LabController::moveWireEnd(const QString& id, bool fromEnd, const QString& anchor) {
    return result(m_session->moveWireEnd(ss(id), fromEnd, ss(anchor)));
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
    const core::ComponentReading* r = m_session->deskReading(model::desk_id::kSpeaker);
    if (!r) return m;
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

void LabController::setMotorBlocked(const QString& id, bool blocked) {
    m_session->setMotorBlocked(ss(id), blocked);
}

} // namespace volta

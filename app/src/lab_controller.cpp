#include "lab_controller.hpp"

#include <QCoreApplication>
#include <QFile>
#include <QSysInfo>

#include <QLocale>

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "model/meter_display.hpp"

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
    m_flow.start(static_cast<int>(exp.hintKeys.size()));
    m_experiment = std::move(exp);
    emit experimentChanged();
    emit flowChanged();
    emit deskChanged();
    emit historyChanged();
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

void LabController::setContentLanguage(const QString& code) {
    m_contentLanguage = code;
    m_repo.loadTexts(QStringLiteral("volta1"), code);
    rebuildExperimentList();
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
}

void LabController::chooseGuess(int index) {
    if (m_flow.chooseGuess(index)) emit flowChanged();
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
    if (m_flow.phase() == model::ExperimentFlow::Phase::Build && m_session->successReached() && m_flow.reportSuccess())
        emit flowChanged();
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
        out << QVariantMap{{QStringLiteral("type"), qs(type)},
                           {QStringLiteral("model"), qs(e.model)},
                           {QStringLiteral("stage"), e.fromStage},
                           {QStringLiteral("pins"), static_cast<int>(e.pins.size())}};
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
    if (mode != 0 && m_session->board().desk.meterAnalog) m_session->setMeterAnalog(false); // analog nur fuer V
    emit deskChanged();
}

void LabController::setMeterAnalog(bool analog) {
    m_session->setMeterAnalog(analog && m_session->board().desk.meterMode == core::MeterMode::Volt);
    emit deskChanged();
}

bool LabController::replaceMeterFuse() { return result(m_session->replaceMeterFuse()); }

QVariantMap LabController::meterState() {
    QVariantMap m;
    const core::ComponentReading* r = m_session->deskReading(model::desk_id::kMeter);
    m.insert(QStringLiteral("valid"), r != nullptr);
    if (!r) return m;
    const auto mode = m_session->board().desk.meterMode;
    const model::MeterText t = model::formatMeter(mode, r->displayValue);
    QString value = QString::fromStdString(t.value);
    value.replace(QLatin1Char('.'), QLocale().decimalPoint());
    if (std::fabs(r->level) > 1e-9) {
        // Bereich aus Messwert/Ausschlag; auf die Bereiche des Geraets runden (1, 3, 10, 30 V)
        const double range = std::fabs(r->displayValue / r->level);
        for (double cand : {1.0, 3.0, 10.0, 30.0})
            if (std::fabs(range - cand) < 0.05 * cand) m_meterRangeV = cand;
    }
    m.insert(QStringLiteral("text"), value);
    m.insert(QStringLiteral("unit"), QString::fromStdString(t.unit));
    m.insert(QStringLiteral("level"), r->level);
    m.insert(QStringLiteral("rangeV"), m_meterRangeV);
    m.insert(QStringLiteral("overrange"), (r->displayFlags & core::kDisplayOverrange) != 0);
    m.insert(QStringLiteral("foreignVoltage"), (r->displayFlags & core::kDisplayForeignVoltage) != 0);
    m.insert(QStringLiteral("fuseBlown"), (r->displayFlags & core::kDisplayFuseBlown) != 0);
    return m;
}

void LabController::setMotorBlocked(const QString& id, bool blocked) {
    m_session->setMotorBlocked(ss(id), blocked);
}

} // namespace volta

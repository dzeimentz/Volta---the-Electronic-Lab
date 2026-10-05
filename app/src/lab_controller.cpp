#include "lab_controller.hpp"

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
    m_session->loadBoard(exp.templateBoard);
    emit experimentChanged();
    emit deskChanged();
    emit historyChanged();
    return true;
}

void LabController::onFrame() {
    const double elapsed = m_clock.restart() / 1000.0;
    m_session->poll();
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

QVariantList LabController::parts() const {
    QVariantList out;
    for (const auto& p : m_session->board().parts) {
        QStringList anchors;
        QStringList pinNames;
        for (const auto& [name, anchor] : p.pins) {
            pinNames << qs(name);
            anchors << qs(anchor);
        }
        out << QVariantMap{{QStringLiteral("id"), qs(p.id)},
                           {QStringLiteral("type"), qs(p.type)},
                           {QStringLiteral("value"), p.value},
                           {QStringLiteral("closed"), p.closed},
                           {QStringLiteral("pins"), pinNames},
                           {QStringLiteral("anchors"), anchors}};
    }
    return out;
}

QVariantList LabController::wires() const {
    QVariantList out;
    for (const auto& w : m_session->board().wires)
        out << QVariantMap{{QStringLiteral("id"), qs(w.id)},
                           {QStringLiteral("from"), qs(w.from)},
                           {QStringLiteral("to"), qs(w.to)},
                           {QStringLiteral("color"), qs(w.color)}};
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
    m_session->setPartClosed(ss(id), closed);
    emit boardChanged();
}
void LabController::setDeskSwitch(bool closed) {
    m_session->setDeskSwitch(closed);
    emit deskChanged();
}
void LabController::setPot(int which, double position) {
    m_session->setPot(which, position);
    emit deskChanged();
}
void LabController::setMotorBlocked(const QString& id, bool blocked) {
    m_session->setMotorBlocked(ss(id), blocked);
}

} // namespace volta

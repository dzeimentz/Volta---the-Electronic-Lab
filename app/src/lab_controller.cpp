#include "lab_controller.hpp"

#include <algorithm>
#include <cmath>

#include "volta/core/components/switch.hpp"

namespace volta {

namespace {
constexpr int kFrameMs = 16;                  // ca. 60 Bilder je Sekunde
constexpr double kNotifyS = 0.1;              // Anzeigen mit ca. 10 Hz aktualisieren
constexpr double kMaxCatchUpS = 0.05;         // hoechstens 50 Schritte je Bild aufholen
constexpr double kBarHysteresis = 0.02;       // 2 % der Batteriespannung

QString qs(const std::string& s) { return QString::fromStdString(s); }
} // namespace

LabController::LabController(QString contentRoot, QObject* parent)
    : QObject(parent), m_repo(std::move(contentRoot)) {
    if (!m_repo.loadCatalog(m_catalog)) m_errors << m_repo.lastError();
    m_repo.loadTexts(QStringLiteral("volta1"), QStringLiteral("de"));
    connect(&m_timer, &QTimer::timeout, this, &LabController::onFrame);
    m_timer.start(kFrameMs);
    m_clock.start();
    rebuild();
}

bool LabController::loadExperiment(const QString& id) {
    services::Experiment exp;
    if (!m_repo.loadExperiment(id, exp)) {
        m_errors = QStringList{m_repo.lastError()};
        emit boardChanged();
        return false;
    }
    m_board = exp.templateBoard;
    m_title = m_repo.text(exp.titleKey);
    m_voltageFields = exp.shownVoltages;
    m_bars.clear();
    rebuild();
    emit experimentChanged();
    return true;
}

void LabController::rebuild() {
    model::Netlist next = model::buildNetlist(m_board, m_catalog);
    // Zustand (Ladung, Temperatur, zerstoert ...) ueber den Topologiewechsel retten
    model::transferStates(m_sim.circuit(), next.circuit);
    m_errors.clear();
    for (const auto& e : next.errors) m_errors << qs(e);
    m_net = std::move(next);
    const core::Status st = m_sim.setCircuit(std::move(m_net.circuit));
    if (st != core::Status::Ok && st != core::Status::NotConverged)
        m_errors << QStringLiteral("Simulation: Netzliste nicht verwendbar");
    emit boardChanged();
    emit stateChanged();
}

void LabController::applyDesk() {
    // Bedienhandlungen wirken direkt auf die Bauteile des Simulators (kein Topologiewechsel)
    const auto it = m_net.desk.find(model::desk_id::kMainSwitch);
    if (it != m_net.desk.end())
        static_cast<core::Switch&>(m_sim.circuit().component(it->second)).setClosed(m_board.desk.mainSwitch);
}

void LabController::setMainSwitch(bool on) {
    if (m_board.desk.mainSwitch == on) return;
    m_board.desk.mainSwitch = on;
    applyDesk();
    emit deskChanged();
}

void LabController::advance(double seconds) {
    const int steps = static_cast<int>(std::lround(seconds / core::kDefaultStepS));
    for (int i = 0; i < steps; ++i) m_sim.step();
    emit stateChanged();
}

void LabController::onFrame() {
    const double elapsed = m_clock.restart() / 1000.0;
    m_lagS = std::min(m_lagS + elapsed, kMaxCatchUpS);
    while (m_lagS >= core::kDefaultStepS) {
        m_sim.step();
        m_lagS -= core::kDefaultStepS;
    }
    m_sinceNotifyS += elapsed;
    if (m_sinceNotifyS >= kNotifyS) {
        m_sinceNotifyS = 0.0;
        emit stateChanged();
    }
}

double LabController::voltage(const QString& anchor) const {
    core::NodeId n = 0;
    if (!m_net.nodeOf(anchor.toStdString(), n)) return 0.0;
    return m_sim.probe().voltage(n);
}

int LabController::voltageBars(const QString& anchor) {
    const double ref = batteryVolt();
    const double ratio = ref > 0.0 ? voltage(anchor) / ref : 0.0;
    // Schwellen 1/8, 3/8, 5/8, 7/8 (docs/domain-model.md); Wechsel erst bei 2 % Abstand zur Schwelle
    auto barsFor = [](double r) {
        if (r < 1.0 / 8) return 0;
        if (r < 3.0 / 8) return 1;
        if (r < 5.0 / 8) return 2;
        if (r < 7.0 / 8) return 3;
        return 4;
    };
    const int candidate = barsFor(ratio);
    const auto it = m_bars.constFind(anchor);
    if (it == m_bars.constEnd()) {
        m_bars.insert(anchor, candidate);
        return candidate;
    }
    const int current = it.value();
    if (candidate != current) {
        const int lo = std::min(candidate, current);
        const double threshold = (2.0 * lo + 1.0) / 8.0; // Schwelle zwischen lo und lo + 1
        if (std::fabs(ratio - threshold) > kBarHysteresis) {
            m_bars.insert(anchor, candidate);
            return candidate;
        }
    }
    return current;
}

QVariantMap LabController::partState(const QString& id) const {
    QVariantMap m;
    const auto it = m_net.parts.find(id.toStdString());
    if (it == m_net.parts.end() || it->second >= m_sim.probe().readings.size()) return m;
    const core::ComponentReading& r = m_sim.probe().readings[it->second];
    m.insert(QStringLiteral("currentmA"), r.currentA * 1e3);
    m.insert(QStringLiteral("level"), r.level);
    m.insert(QStringLiteral("overloaded"), r.overloaded);
    m.insert(QStringLiteral("destroyed"), r.destroyed);
    m.insert(QStringLiteral("tempC"), r.tempK - 273.15);
    return m;
}

QVariantList LabController::parts() const {
    QVariantList out;
    for (const auto& p : m_board.parts) {
        QStringList anchors;
        QStringList pinNames;
        for (const auto& [name, anchor] : p.pins) {
            pinNames << qs(name);
            anchors << qs(anchor);
        }
        out << QVariantMap{{QStringLiteral("id"), qs(p.id)},
                           {QStringLiteral("type"), qs(p.type)},
                           {QStringLiteral("value"), p.value},
                           {QStringLiteral("pins"), pinNames},
                           {QStringLiteral("anchors"), anchors}};
    }
    return out;
}

QVariantList LabController::wires() const {
    QVariantList out;
    for (const auto& w : m_board.wires)
        out << QVariantMap{{QStringLiteral("id"), qs(w.id)},
                           {QStringLiteral("from"), qs(w.from)},
                           {QStringLiteral("to"), qs(w.to)},
                           {QStringLiteral("color"), qs(w.color)}};
    return out;
}

QVariantList LabController::modules() const {
    QVariantList out;
    for (const auto& m : m_board.modules) {
        const model::ModuleEntry* e = m_catalog.module(m.type);
        out << QVariantMap{{QStringLiteral("id"), qs(m.id)},
                           {QStringLiteral("type"), qs(m.type)},
                           {QStringLiteral("slot"), m.slot},
                           {QStringLiteral("width"), e ? static_cast<int>(e->pins.size()) : 1}};
    }
    return out;
}

} // namespace volta

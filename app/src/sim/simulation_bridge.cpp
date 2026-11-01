#include "simulation_bridge.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <utility>

#include "model/netlist_builder.hpp"
#include "volta/core/components/optics.hpp"
#include "volta/core/components/sensors.hpp"
#include "volta/core/components/sources.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/meter.hpp"
#include "volta/core/components/motor.hpp"
#include "volta/core/components/potentiometer.hpp"
#include "volta/core/components/switch.hpp"

namespace volta::sim {

namespace {
using Clock = std::chrono::steady_clock;
constexpr double kPublishS = 1.0 / 120.0;    // Schnappschuesse mit ca. 120 Hz
constexpr double kMaxCatchUpS = 0.05;        // je Durchlauf hoechstens 50 ms Wanduhrzeit aufholen
constexpr std::size_t kEventBacklog = 512;   // Ereignisse, bis die Oberflaeche sie abholt
constexpr double kStatsWindowS = 0.1;        // Messfenster der Signalstatistik (5 Perioden bei 50 Hz)
} // namespace

SimulationBridge::SimulationBridge() {
    m_actions.reserve(64);
    m_actionsWork.reserve(64);
    m_eventBacklog.reserve(kEventBacklog);
    m_back.events.reserve(kEventBacklog);
    m_front.events.reserve(kEventBacklog);
}

SimulationBridge::~SimulationBridge() { stop(); }

void SimulationBridge::start() {
    if (m_thread.joinable()) return;
    m_run = true;
    m_thread = std::thread(&SimulationBridge::loop, this);
}

void SimulationBridge::stop() {
    m_run = false;
    if (m_thread.joinable()) m_thread.join();
}

std::uint64_t SimulationBridge::setCircuit(core::Circuit circuit, std::vector<StateOverride> overrides,
                                           std::shared_ptr<StepObserver> observer) {
    std::lock_guard<std::mutex> lock(m_inMutex);
    if (m_pendingCircuit && !m_pendingOverrides.empty()) {
        // Eine noch nicht uebernommene Topologie wird verdraengt: ihre Zustandsvorgaben (z. B. Undo) gelten weiter,
        // die neueren Vorgaben werden danach angewendet und haben damit Vorrang.
        m_pendingOverrides.insert(m_pendingOverrides.end(), std::make_move_iterator(overrides.begin()),
                                  std::make_move_iterator(overrides.end()));
        overrides = std::move(m_pendingOverrides);
    }
    m_pendingCircuit = std::make_unique<core::Circuit>(std::move(circuit));
    m_pendingOverrides = std::move(overrides);
    m_pendingObserver = std::move(observer);
    m_pendingTopology = ++m_nextTopology;
    return m_pendingTopology;
}

void SimulationBridge::post(Action action) {
    std::lock_guard<std::mutex> lock(m_inMutex);
    m_actions.push_back(std::move(action));
}

void SimulationBridge::setTimeScale(double scale) {
    m_timeScale = std::isfinite(scale) && scale > 0.0 ? std::min(scale, 1000.0) : 1.0;
}

void SimulationBridge::setPaused(bool paused) { m_paused = paused; }

void SimulationBridge::applyPending() {
    std::unique_ptr<core::Circuit> circuit;
    std::vector<StateOverride> overrides;
    std::uint64_t topology = 0;
    std::shared_ptr<StepObserver> observer;
    bool scopeChanged = false;
    {
        std::lock_guard<std::mutex> lock(m_inMutex);
        if (m_scopePending) {
            m_scope = m_pendingScope;
            m_scopePending = false;
            scopeChanged = true;
        }
        if (m_pendingCircuit) {
            circuit = std::move(m_pendingCircuit);
            overrides = std::move(m_pendingOverrides);
            topology = m_pendingTopology;
            observer = std::move(m_pendingObserver);
            // Aktionen fuer die alte Topologie verfallen nicht: sie beziehen sich auf Kennungen
        }
        m_actionsWork.clear();
        std::swap(m_actions, m_actionsWork);
    }
    if (circuit) {
        // Topologiewechsel (Speicheranforderungen sind hier erlaubt): Zustaende uebernehmen, Vorgaben anwenden
        model::transferStates(m_sim.circuit(), *circuit);
        std::unordered_map<std::string, std::size_t> index;
        for (std::size_t i = 0; i < circuit->componentCount(); ++i) index[circuit->component(i).id()] = i;
        for (const auto& o : overrides) {
            const auto it = index.find(o.componentId);
            if (it != index.end()) circuit->component(it->second).restoreState(o.state);
        }
        m_index = std::move(index);
        m_back.lastStatus = m_sim.setCircuit(std::move(*circuit));
        m_activeTopology = topology;
        m_observer = std::move(observer); // der alte Beobachter wird hier (ausserhalb des Schritts) freigegeben
        const std::size_t n = m_sim.circuit().componentCount();
        m_back.readings.reserve(n);
        m_back.states.reserve(n);
        m_back.nodeVolt.reserve(m_sim.circuit().nodeCount());
        m_nodeStats.reset(m_sim.circuit().nodeCount(), kStatsWindowS);
        m_readingStats.reset(2 * n, kStatsWindowS);
        m_readingValues.assign(2 * n, 0.0);
    }
    if (scopeChanged) { // Speicher nur hier (zwischen zwei Schritten)
        m_scopeRing.assign(m_scope.capacity, model::ScopeSample{});
        m_back.scope.reserve(m_scope.capacity);
        m_stepS = m_scope.stepS > 0.0 ? m_scope.stepS : core::kDefaultStepS;
    }
    if (circuit || scopeChanged) resolveScope();
    for (const auto& a : m_actionsWork) apply(a);
}

void SimulationBridge::apply(const Action& a) {
    const auto it = m_index.find(a.componentId);
    if (it == m_index.end()) return;
    core::ComponentModel& c = m_sim.circuit().component(it->second);
    const bool on = a.value != 0.0;
    switch (a.type) {
    case Action::Type::SetSwitch:
        if (auto* s = dynamic_cast<core::Switch*>(&c)) s->setClosed(on);
        else if (auto* w = dynamic_cast<core::ChangeoverSwitch*>(&c)) w->setToB(on);
        else if (auto* ac = dynamic_cast<core::AcSource*>(&c)) ac->setEnabled(on); // Hauptschalter
        break;
    case Action::Type::SetPotPosition:
        if (auto* p = dynamic_cast<core::Potentiometer*>(&c)) p->setPosition(a.value);
        break;
    case Action::Type::SetCells:
        if (auto* b = dynamic_cast<core::Battery*>(&c)) b->setCells(static_cast<int>(std::lround(a.value)));
        break;
    case Action::Type::SetMotorBlocked:
        if (auto* m = dynamic_cast<core::Motor*>(&c)) m->setBlocked(on);
        break;
    case Action::Type::SetMeterMode:
        if (auto* m = dynamic_cast<core::Meter*>(&c)) m->setMode(static_cast<core::MeterMode>(std::clamp(
            static_cast<int>(std::lround(a.value)), 0, 2)));
        break;
    case Action::Type::SetMeterAnalog:
        if (auto* m = dynamic_cast<core::Meter*>(&c)) m->setAnalog(on);
        break;
    case Action::Type::SetIlluminance:
        if (auto* l = dynamic_cast<core::Photoresistor*>(&c)) l->setIlluminance(a.value);
        else if (auto* q = dynamic_cast<core::Phototransistor*>(&c)) q->setIlluminance(a.value);
        break;
    case Action::Type::SetShade:
        if (auto* q = dynamic_cast<core::Phototransistor*>(&c)) q->setShade(a.value);
        break;
    case Action::Type::SetAmbient:
        if (auto* t = dynamic_cast<core::Thermistor*>(&c)) t->setAmbient(a.value);
        break;
    case Action::Type::SetWetness:
        if (auto* f = dynamic_cast<core::MoistureProbe*>(&c)) f->setWetness(a.value);
        break;
    case Action::Type::SetAcRms:
        if (auto* w = dynamic_cast<core::AcSource*>(&c)) w->setRms(a.value);
        break;
    case Action::Type::SetGenWaveform:
        if (auto* g = dynamic_cast<core::FunctionGenerator*>(&c))
            g->setWaveform(static_cast<core::Waveform>(std::clamp(static_cast<int>(std::lround(a.value)), 0, 2)));
        break;
    case Action::Type::SetGenFrequency:
        if (auto* g = dynamic_cast<core::FunctionGenerator*>(&c)) g->setFrequency(a.value);
        break;
    case Action::Type::SetGenAmplitude:
        if (auto* g = dynamic_cast<core::FunctionGenerator*>(&c)) g->setAmplitude(a.value);
        break;
    case Action::Type::SetGenOffset:
        if (auto* g = dynamic_cast<core::FunctionGenerator*>(&c)) g->setOffset(a.value);
        break;
    }
}

void SimulationBridge::publish(double realtimeRatio) {
    const core::Probe& p = m_sim.probe();
    m_back.topology = m_activeTopology;
    m_back.timeS = m_sim.timeS();
    m_back.valid = p.valid;
    m_back.nodeVolt.assign(p.nodeVolt.begin(), p.nodeVolt.end());
    m_back.readings.assign(p.readings.begin(), p.readings.end());
    m_back.nodeStats.assign(m_nodeStats.latest().begin(), m_nodeStats.latest().end());
    m_back.readingStats.assign(m_readingStats.latest().begin(), m_readingStats.latest().end());
    const core::Circuit& c = m_sim.circuit();
    m_back.states.resize(c.componentCount());
    for (std::size_t i = 0; i < c.componentCount(); ++i) m_back.states[i] = c.component(i).saveState();
    m_back.realtimeRatio = realtimeRatio;
    m_back.scope.resize(m_scopeCount);
    for (std::size_t k = 0; k < m_scopeCount; ++k)
        m_back.scope[k] = m_scopeRing[(m_scopeHead + m_scopeRing.size() - m_scopeCount + k) % m_scopeRing.size()];
    for (std::size_t c = 0; c < model::kScopeChannels; ++c) m_back.scopeHas[c] = m_scopeIndex[c] >= 0;
    core::SimEvent e;
    std::lock_guard<std::mutex> lock(m_outMutex);
    while (m_sim.events().pop(e))
        if (m_eventBacklog.size() < kEventBacklog) m_eventBacklog.push_back(e);
    std::swap(m_front.topology, m_back.topology);
    std::swap(m_front.timeS, m_back.timeS);
    std::swap(m_front.valid, m_back.valid);
    std::swap(m_front.nodeVolt, m_back.nodeVolt);
    std::swap(m_front.readings, m_back.readings);
    std::swap(m_front.nodeStats, m_back.nodeStats);
    std::swap(m_front.readingStats, m_back.readingStats);
    std::swap(m_front.states, m_back.states);
    std::swap(m_front.lastStatus, m_back.lastStatus);
    std::swap(m_front.realtimeRatio, m_back.realtimeRatio);
    std::swap(m_front.scope, m_back.scope);
    std::swap(m_front.scopeHas, m_back.scopeHas);
    m_back.lastStatus = m_front.lastStatus;
    m_fresh = true;
}

bool SimulationBridge::latest(Snapshot& out) {
    std::lock_guard<std::mutex> lock(m_outMutex);
    out.events.clear();
    out.events.insert(out.events.end(), m_eventBacklog.begin(), m_eventBacklog.end());
    m_eventBacklog.clear();
    if (!m_fresh) return !out.events.empty();
    out.topology = m_front.topology;
    out.timeS = m_front.timeS;
    out.valid = m_front.valid;
    out.nodeVolt.assign(m_front.nodeVolt.begin(), m_front.nodeVolt.end());
    out.readings.assign(m_front.readings.begin(), m_front.readings.end());
    out.nodeStats.assign(m_front.nodeStats.begin(), m_front.nodeStats.end());
    out.readingStats.assign(m_front.readingStats.begin(), m_front.readingStats.end());
    out.states.assign(m_front.states.begin(), m_front.states.end());
    out.lastStatus = m_front.lastStatus;
    out.realtimeRatio = m_front.realtimeRatio;
    out.scope.assign(m_front.scope.begin(), m_front.scope.end());
    out.scopeHas = m_front.scopeHas;
    m_fresh = false;
    return true;
}

void SimulationBridge::step() {
    const double t0 = m_sim.timeS();
    const core::Status s = m_sim.step();
    if (s != core::Status::Ok) m_back.lastStatus = s;
    const double dt = m_sim.timeS() - t0;
    const core::Probe& p = m_sim.probe();
    if (dt > 0.0 && p.valid) {
        m_nodeStats.add(p.nodeVolt.data(), p.nodeVolt.size(), dt);
        const std::size_t n = std::min(p.readings.size(), m_readingValues.size() / 2);
        for (std::size_t i = 0; i < n; ++i) {
            m_readingValues[2 * i] = p.readings[i].voltV;
            m_readingValues[2 * i + 1] = p.readings[i].currentA;
        }
        m_readingStats.add(m_readingValues.data(), 2 * n, dt);
    }
    if (dt > 0.0 && p.valid && !m_scopeRing.empty() && m_sim.timeS() - m_scopeLastT >= m_scope.intervalS * 0.999) {
        m_scopeLastT = m_sim.timeS();
        model::ScopeSample& s = m_scopeRing[m_scopeHead];
        s.t = m_scopeLastT;
        for (std::size_t c = 0; c < model::kScopeChannels; ++c) {
            const long i = m_scopeIndex[c];
            s.v[c] = i >= 0 && static_cast<std::size_t>(i) < p.readings.size() ? p.readings[i].voltV : 0.0;
        }
        m_scopeHead = (m_scopeHead + 1) % m_scopeRing.size();
        m_scopeCount = std::min(m_scopeCount + 1, m_scopeRing.size());
    }
    if (m_observer) m_observer->afterStep(m_sim);
}

void SimulationBridge::setScope(const ScopeConfig& config) {
    std::lock_guard<std::mutex> lock(m_inMutex);
    m_pendingScope = config;
    m_scopePending = true;
}

void SimulationBridge::resolveScope() {
    for (std::size_t c = 0; c < model::kScopeChannels; ++c) {
        const auto it = m_scope.probeIds[c].empty() ? m_index.end() : m_index.find(m_scope.probeIds[c]);
        m_scopeIndex[c] = it == m_index.end() ? -1 : static_cast<long>(it->second);
    }
    m_scopeHead = m_scopeCount = 0;
    m_scopeLastT = -1.0;
}

void SimulationBridge::runFor(double seconds) {
    if (m_thread.joinable()) return;
    applyPending();
    const int steps = static_cast<int>(std::lround(seconds / m_stepS));
    for (int i = 0; i < steps; ++i) {
        step();
    }
    publish(1.0);
}

void SimulationBridge::loop() {
    auto last = Clock::now();
    double owedS = 0.0;      // noch zu simulierende Zeit (Simulationssekunden)
    double sincePublish = 0.0;
    double wallWindow = 0.0; // fuer realtimeRatio
    double simWindow = 0.0;
    double ratio = 1.0;
    while (m_run) {
        applyPending();
        const auto now = Clock::now();
        const double wall = std::chrono::duration<double>(now - last).count();
        last = now;
        const double scale = m_timeScale;
        if (!m_paused && m_sim.hasCircuit()) {
            owedS = std::min(owedS + std::min(wall, kMaxCatchUpS) * scale, kMaxCatchUpS * scale);
            const double t0 = m_sim.timeS();
            const auto budgetEnd = now + std::chrono::duration<double>(kMaxCatchUpS);
            while (owedS >= m_stepS && Clock::now() < budgetEnd) {
                step();
                owedS -= m_stepS;
            }
            simWindow += m_sim.timeS() - t0;
        } else {
            owedS = 0.0;
        }
        wallWindow += wall;
        if (wallWindow >= 0.5) {
            ratio = m_paused ? 0.0 : simWindow / (wallWindow * scale);
            wallWindow = simWindow = 0.0;
        }
        sincePublish += wall;
        if (sincePublish >= kPublishS) {
            sincePublish = 0.0;
            publish(ratio);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

} // namespace volta::sim

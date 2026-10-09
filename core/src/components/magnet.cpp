#include "volta/core/components/magnet.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace volta::core {

// ------------------------------------------------------------------ Elektromagnet

Electromagnet::Electromagnet(std::string id, NodeId a, NodeId b, ElectromagnetParams params)
    : ComponentModel(std::move(id), {a, b}), m_params(params) {
    if (!(m_params.ohm > kMinOhm) || !std::isfinite(m_params.ohm)) m_params.ohm = 100.0;
    if (!(m_params.nominalA > 0.0) || !std::isfinite(m_params.nominalA)) m_params.nominalA = 0.09;
}

void Electromagnet::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    sys.stampConductance(terminal(0), terminal(1), g());
}

double Electromagnet::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    const double i = (x.voltage(terminal(0)) - x.voltage(terminal(1))) * g();
    return t == 0 ? i : -i;
}

void Electromagnet::thermal(const SolutionView& x, double dtS) noexcept {
    const double v = x.voltage(terminal(0)) - x.voltage(terminal(1));
    m_lastPowerW = v * v * g();
    integrateThermal(m_lastPowerW, dtS, m_params.thermal);
}

void Electromagnet::checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept {
    m_overloaded = !isDestroyed() && m_lastPowerW > m_params.pmaxW;
    checkThermalLimit(m_params.thermal, events, index, timeS);
}

ComponentReading Electromagnet::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    r.level = std::clamp(r.currentA / m_params.nominalA, -1.5, 1.5); // Feldstaerke, Vorzeichen = Polung
    return r;
}

// ------------------------------------------------------------------ Relais

Relay::Relay(std::string id, NodeId a, NodeId b, NodeId common, NodeId normallyOpen, RelayParams params)
    : ComponentModel(std::move(id), {a, b, common, normallyOpen}), m_params(params) {
    if (!(m_params.coilOhm > kMinOhm) || !std::isfinite(m_params.coilOhm)) m_params.coilOhm = 180.0;
    if (!(m_params.contactOhm >= kMinOhm) || !std::isfinite(m_params.contactOhm)) m_params.contactOhm = 0.05;
    if (!(m_params.dropA < m_params.pullA)) m_params.dropA = 0.4 * m_params.pullA;
}

void Relay::beginSolve(const StampContext& /*ctx*/) noexcept { m_closed = m_pending; }

void Relay::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    sys.stampConductance(terminal(0), terminal(1), coilG());
    sys.stampConductance(terminal(2), terminal(3), contactG());
}

double Relay::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    const double coil = (x.voltage(terminal(0)) - x.voltage(terminal(1))) * coilG();
    const double contact = (x.voltage(terminal(2)) - x.voltage(terminal(3))) * contactG();
    switch (t) {
    case 0: return coil;
    case 1: return -coil;
    case 2: return contact;
    default: return -contact;
    }
}

void Relay::updateState(const SolutionView& x, double dtS) noexcept {
    const double i = std::fabs((x.voltage(terminal(0)) - x.voltage(terminal(1))) * coilG());
    const double v = x.voltage(terminal(0)) - x.voltage(terminal(1));
    m_coilPowerW = v * v * coilG();
    m_contactA = std::fabs((x.voltage(terminal(2)) - x.voltage(terminal(3))) * contactG());
    // Anziehen bzw. Abfallen nach der Schaltverzoegerung; im Arbeitspunkt sofort
    const bool wants = m_pending ? i >= m_params.dropA : i >= m_params.pullA;
    if (wants == m_pending) {
        m_timerS = 0.0;
        return;
    }
    if (dtS <= 0.0) {
        m_pending = wants;
        m_timerS = 0.0;
        return;
    }
    m_timerS += dtS;
    if (m_timerS >= (wants ? m_params.pullDelayS : m_params.dropDelayS) - 1e-12) {
        m_pending = wants;
        m_timerS = 0.0;
    }
}

void Relay::checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept {
    if (isDestroyed()) return;
    m_overloaded = m_coilPowerW > m_params.coilPmaxW;
    // Kontakt deutlich ueberlastet (Kurzschluss ueber den Kontakt): verschweisst nicht, sondern brennt ab
    if (m_contactA > 3.0 * m_params.contactMaxA) destroy(events, index, timeS);
    else if (m_contactA > m_params.contactMaxA) m_overloaded = true;
}

ComponentReading Relay::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    r.currentA = r.terminalCurrentA[0]; // Spulenstrom
    r.level = m_pending ? 1.0 : 0.0;
    return r;
}

ComponentState Relay::saveState() const noexcept {
    ComponentState s = ComponentModel::saveState();
    s.internal[0] = m_pending ? 1.0 : 0.0;
    return s;
}

void Relay::restoreState(const ComponentState& state) noexcept {
    ComponentModel::restoreState(state);
    m_pending = state.internal[0] > 0.5;
    m_timerS = 0.0;
}

} // namespace volta::core

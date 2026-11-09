#include "volta/core/modules/gate_module.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace volta::core {

namespace {
constexpr double kEventMarginS = 1e-7;

double crossing(double fa, double fn) noexcept {
    if (!(fa <= 0.0 && fn > 0.0)) return 2.0;
    return fa / (fa - fn);
}
} // namespace

NandGate::NandGate(std::string id, NodeId plus, NodeId minus, NodeId a, NodeId b, NodeId y, NandGateParams params)
    : ComponentModel(std::move(id), {plus, minus, a, b, y}), m_params(params) {
    if (!(m_params.outOhm > 0.0)) m_params.outOhm = 200.0;
    if (m_params.startLow) {
        m_in = {true, true};
        m_out = false;
    }
    if (!(m_params.upperFraction > m_params.lowerFraction)) {
        m_params.upperFraction = 0.6;
        m_params.lowerFraction = 0.4;
    }
}

double NandGate::rel(const SolutionView& x, std::size_t t) const noexcept {
    return x.voltage(terminal(t)) - x.voltage(terminal(1));
}

void NandGate::beginSolve(const StampContext& ctx) noexcept {
    m_powered = !m_destroyed && rel(ctx.accepted, 0) >= m_params.minSupplyV;
}

void NandGate::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    if (!m_powered) return;
    sys.stampConductance(terminal(4), m_out ? terminal(0) : terminal(1), 1.0 / m_params.outOhm);
}

double NandGate::outputCurrent(const SolutionView& x) const noexcept {
    if (!m_powered) return 0.0;
    const double vy = x.voltage(terminal(4));
    return (x.voltage(terminal(m_out ? 0 : 1)) - vy) / m_params.outOhm;
}

double NandGate::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    const double iout = outputCurrent(x);
    switch (t) {
    case 0: return m_out ? iout : 0.0;
    case 1: return m_out ? 0.0 : iout;
    case 4: return -iout;
    default: return 0.0; // Eingaenge stromlos
    }
}

double NandGate::eventFraction(const SolutionView& a, const SolutionView& n, double dtS) const noexcept {
    if (!m_powered || !(dtS > 0.0)) return 1.0;
    double theta = 2.0;
    for (std::size_t k = 0; k < 2; ++k) {
        const std::size_t t = 2 + k;
        if (m_in[k]) { // H: faellt unter die untere Schwelle
            const double fa = m_params.lowerFraction * rel(a, 0) - rel(a, t);
            const double fn = m_params.lowerFraction * rel(n, 0) - rel(n, t);
            theta = std::min(theta, crossing(fa, fn));
        } else { // L: steigt ueber die obere Schwelle
            const double fa = rel(a, t) - m_params.upperFraction * rel(a, 0);
            const double fn = rel(n, t) - m_params.upperFraction * rel(n, 0);
            theta = std::min(theta, crossing(fa, fn));
        }
    }
    if (theta > 1.0) return 1.0;
    return std::min(1.0, theta + kEventMarginS / dtS);
}

void NandGate::updateState(const SolutionView& x, double /*dtS*/) noexcept {
    m_lastOutA = outputCurrent(x);
    const double u = rel(x, 0);
    if (m_destroyed || !m_powered || u < m_params.minSupplyV) { // unversorgt geloest: Ausgang war offen
        m_in = {m_params.startLow, m_params.startLow}; // ohne Versorgung zurueck in den Einschaltzustand
        m_out = !m_params.startLow;
        return;
    }
    for (std::size_t k = 0; k < 2; ++k) {
        const double v = rel(x, 2 + k);
        if (m_in[k] && v < m_params.lowerFraction * u) m_in[k] = false;
        else if (!m_in[k] && v > m_params.upperFraction * u) m_in[k] = true;
    }
    m_out = !(m_in[0] && m_in[1]);
}

void NandGate::checkLimits(EventQueue& /*events*/, std::size_t /*index*/, double /*timeS*/) noexcept {
    m_overloaded = !isDestroyed() && std::fabs(m_lastOutA) > m_params.ratedA;
}

ComponentReading NandGate::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    r.voltV = rel(x, 0);
    r.level = m_out ? 1.0 : 0.0;
    r.displayValue = rel(x, 4);
    return r;
}

ComponentState NandGate::saveState() const noexcept {
    ComponentState s = ComponentModel::saveState();
    s.internal[0] = m_in[0] ? 1.0 : 0.0;
    s.internal[1] = m_in[1] ? 1.0 : 0.0;
    s.internal[2] = m_out ? 1.0 : 0.0;
    return s;
}

void NandGate::restoreState(const ComponentState& state) noexcept {
    ComponentModel::restoreState(state);
    m_in = {state.internal[0] > 0.5, state.internal[1] > 0.5};
    m_out = state.internal[2] > 0.5 || !(m_in[0] && m_in[1]);
}

SubcircuitRange addGateModule(Circuit& circuit, const std::string& prefix, const GateModulePins& pins) {
    SubcircuitRange range;
    range.first = circuit.componentCount();
    for (std::size_t g = 0; g < 4; ++g) {
        NandGateParams params;
        params.startLow = (g % 2) == 1;
        circuit.add<NandGate>(prefix + ".G" + std::to_string(g + 1), pins.plus, pins.minus, pins.io[3 * g],
                              pins.io[3 * g + 1], pins.io[3 * g + 2], params);
    }
    range.count = circuit.componentCount() - range.first;
    return range;
}

} // namespace volta::core

#include "volta/core/modules/timer555_module.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace volta::core {

namespace {
constexpr double kEventMarginS = 1e-7; // Teilschritt endet knapp hinter dem Kipppunkt, damit der Komparator sicher kippt
// Vereinfachung: offener Reset-Eingang gilt als hoch (schwacher Zug nach +U), sonst bliebe ein Aufbau ohne Leitung
// an R stumm. Beschaltet (an +U oder ueber Taster nach GND) wirkt er wie im Datenblatt.
constexpr double kResetPullOhm = 100e3;

// Bruchteil des Schritts, an dem f linear interpoliert von <= 0 nach > 0 wechselt; 2 = kein Wechsel
double crossing(double fa, double fn) noexcept {
    if (!(fa <= 0.0 && fn > 0.0)) return 2.0;
    return fa / (fa - fn);
}
} // namespace

Timer555::Timer555(std::string id, const Timer555Pins& pins, Timer555Params params)
    : ComponentModel(std::move(id), {pins.gnd, pins.trigger, pins.out, pins.reset, pins.control, pins.threshold,
                                     pins.discharge, pins.vcc}),
      m_params(params) {
    if (!(m_params.dividerOhm > 0.0)) m_params.dividerOhm = 5000.0;
    if (!(m_params.quiescentOhm > 0.0)) m_params.quiescentOhm = 1500.0;
    if (!(m_params.highOhm > 0.0)) m_params.highOhm = 15.0;
    if (!(m_params.lowOhm > 0.0)) m_params.lowOhm = 10.0;
    if (!(m_params.dischargeOhm > 0.0)) m_params.dischargeOhm = 10.0;
}

void Timer555::beginSolve(const StampContext& ctx) noexcept {
    m_powered = !m_destroyed &&
                ctx.accepted.voltage(terminal(7)) - ctx.accepted.voltage(terminal(0)) >= m_params.minSupplyV;
}

void Timer555::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    const NodeId gnd = terminal(0), out = terminal(2), reset = terminal(3), cv = terminal(4), dis = terminal(6),
                 vcc = terminal(7), x = internalNode(0);
    const double gd = 1.0 / m_params.dividerOhm;
    sys.stampConductance(vcc, cv, gd);
    sys.stampConductance(cv, x, gd);
    sys.stampConductance(x, gnd, gd);
    sys.stampConductance(vcc, gnd, 1.0 / m_params.quiescentOhm);
    sys.stampConductance(reset, vcc, 1.0 / kResetPullOhm);
    if (!m_powered) return;
    if (m_q) { // Ausgang hoch: Quelle +U - highDropV hinter highOhm
        const double gh = 1.0 / m_params.highOhm;
        sys.stampConductance(out, vcc, gh);
        sys.stampCurrentSource(out, vcc, gh * m_params.highDropV);
    } else { // Ausgang niedrig: lowV hinter lowOhm, Entladetransistor leitet
        const double gl = 1.0 / m_params.lowOhm;
        sys.stampConductance(out, gnd, gl);
        sys.stampCurrentSource(gnd, out, gl * m_params.lowV);
        sys.stampConductance(dis, gnd, 1.0 / m_params.dischargeOhm);
    }
}

double Timer555::outputCurrent(const SolutionView& x) const noexcept {
    if (!m_powered) return 0.0;
    const double vout = x.voltage(terminal(2));
    if (m_q) return -(vout - x.voltage(terminal(7)) + m_params.highDropV) / m_params.highOhm;
    return -(vout - x.voltage(terminal(0)) - m_params.lowV) / m_params.lowOhm;
}

double Timer555::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    const double vg = x.voltage(terminal(0)), vcv = x.voltage(terminal(4)), vcc = x.voltage(terminal(7));
    const double vx = x.voltage(internalNode(0)), vr = x.voltage(terminal(3));
    const double gd = 1.0 / m_params.dividerOhm, gq = 1.0 / m_params.quiescentOhm, gr = 1.0 / kResetPullOhm;
    const bool low = m_powered && !m_q, high = m_powered && m_q;
    const double idis = low ? (x.voltage(terminal(6)) - vg) / m_params.dischargeOhm : 0.0;
    const double iout = outputCurrent(x); // aus dem Ausgang heraus
    switch (t) {
    case 0: return gd * (vg - vx) + gq * (vg - vcc) - (low ? -iout : 0.0) - idis;
    case 2: return -iout;
    case 3: return gr * (vr - vcc);
    case 4: return gd * (vcv - vcc) + gd * (vcv - vx);
    case 6: return idis;
    case 7: return gd * (vcc - vcv) + gq * (vcc - vg) + gr * (vcc - vr) - (high ? -iout : 0.0);
    default: return 0.0; // TR, THR: Komparatoreingaenge ohne Strom
    }
}

bool Timer555::logic(const SolutionView& x, bool q) const noexcept {
    const double vg = x.voltage(terminal(0));
    if (x.voltage(terminal(7)) - vg < m_params.minSupplyV || m_destroyed) return false;
    const double cv = x.voltage(terminal(4)) - vg;
    if (x.voltage(terminal(3)) - vg < m_params.resetV) return false;
    if (x.voltage(terminal(1)) - vg < 0.5 * cv) return true;
    if (x.voltage(terminal(5)) - vg > cv) return false;
    return q;
}

double Timer555::eventFraction(const SolutionView& a, const SolutionView& n, double dtS) const noexcept {
    if (!m_powered || !(dtS > 0.0)) return 1.0;
    auto rel = [this](const SolutionView& x, std::size_t t) { return x.voltage(terminal(t)) - x.voltage(terminal(0)); };
    double theta = 2.0;
    if (m_q) {
        // Reset faellt unter die Schwelle
        theta = std::min(theta, crossing(m_params.resetV - rel(a, 3), m_params.resetV - rel(n, 3)));
        // THR steigt ueber CV (sofern TR nicht gleichzeitig setzt)
        if (rel(n, 1) >= 0.5 * rel(n, 4)) theta = std::min(theta, crossing(rel(a, 5) - rel(a, 4), rel(n, 5) - rel(n, 4)));
    } else if (rel(n, 3) >= m_params.resetV) {
        // TR faellt unter CV/2
        theta = std::min(theta, crossing(0.5 * rel(a, 4) - rel(a, 1), 0.5 * rel(n, 4) - rel(n, 1)));
    }
    if (theta > 1.0) return 1.0;
    return std::min(1.0, theta + kEventMarginS / dtS);
}

void Timer555::updateState(const SolutionView& x, double /*dtS*/) noexcept {
    m_lastOutA = outputCurrent(x);
    m_q = logic(x, m_q);
}

void Timer555::checkLimits(EventQueue& /*events*/, std::size_t /*index*/, double /*timeS*/) noexcept {
    m_overloaded = !isDestroyed() && std::fabs(m_lastOutA) > m_params.ratedA;
}

ComponentReading Timer555::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    r.voltV = x.voltage(terminal(7)) - x.voltage(terminal(0));
    r.level = m_q ? 1.0 : 0.0;
    r.displayValue = x.voltage(terminal(2)) - x.voltage(terminal(0));
    return r;
}

ComponentState Timer555::saveState() const noexcept {
    ComponentState s = ComponentModel::saveState();
    s.internal[0] = m_q ? 1.0 : 0.0;
    return s;
}

void Timer555::restoreState(const ComponentState& state) noexcept {
    ComponentModel::restoreState(state);
    m_q = state.internal[0] > 0.5;
}

SubcircuitRange addTimer555(Circuit& circuit, const std::string& prefix, const Timer555Pins& pins) {
    SubcircuitRange range;
    range.first = circuit.componentCount();
    circuit.add<Timer555>(prefix, pins);
    range.count = circuit.componentCount() - range.first;
    return range;
}

} // namespace volta::core

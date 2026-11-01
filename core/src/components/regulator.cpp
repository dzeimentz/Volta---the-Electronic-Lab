#include "volta/core/components/regulator.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace volta::core {

namespace {
constexpr double kQuiescentKneeV = 1.0; // Ruhestrom setzt mit der Eingangsspannung weich ein
constexpr double kMaxStepV = 0.5;      // Begrenzung der Ausgangsspannung je Newton-Iteration
} // namespace

VoltageRegulator::VoltageRegulator(std::string id, NodeId in, NodeId ground, NodeId out, RegulatorParams params)
    : ComponentModel(std::move(id), {in, ground, out}), m_params(params) {
    if (!(m_params.errorV > 0.0)) m_params.errorV = 0.02;
    if (!(m_params.headroomV > 0.0)) m_params.headroomV = 0.1;
    if (!(m_params.limitA > 0.0)) m_params.limitA = 0.14;
    if (!(m_params.dividerOhm > 0.0)) m_params.dividerOhm = 5000.0;
    if (!(m_params.shutdownSpanK > 0.0)) m_params.shutdownSpanK = 25.0;
}

void VoltageRegulator::beginSolve(const StampContext& /*ctx*/) noexcept {
    m_scale = std::clamp((m_params.shutdownK + m_params.shutdownSpanK - m_tempK) / m_params.shutdownSpanK, 0.0, 1.0);
}

void VoltageRegulator::evaluate(double vin, double vgnd, double vout) noexcept {
    m_v0 = {vin, vgnd, vout};
    const double tc = std::tanh((m_params.refV - (vout - vgnd)) / m_params.errorV);
    const double tf = std::tanh((vin - vout - m_params.dropoutV) / m_params.headroomV);
    const double c = 0.5 * (1.0 + tc), f = 0.5 * (1.0 + tf);
    const double dc = 0.5 * (1.0 - tc * tc) / m_params.errorV;   // dc / d(Uref - Uaus + Umasse)
    const double df = 0.5 * (1.0 - tf * tf) / m_params.headroomV; // df / d(Uein - Uaus)
    const double k = m_destroyed ? 0.0 : m_params.limitA * m_scale;
    m_i0 = k * c * f;
    m_g[0] = k * c * df;             // d I / d Uein
    m_g[1] = k * dc * f;             // d I / d Umasse
    m_g[2] = -k * (dc * f + c * df); // d I / d Uaus
    const double vq = vin - vgnd;
    const double tq = std::tanh(vq / kQuiescentKneeV);
    m_qv0 = vq;
    m_q0 = m_destroyed ? 0.0 : m_params.quiescentA * tq;
    m_qg = m_destroyed ? 0.0 : m_params.quiescentA * (1.0 - tq * tq) / kQuiescentKneeV;
}

bool VoltageRegulator::linearize(const SolutionView& iterate) noexcept {
    // Ausserhalb des Regelbereichs sind die tanh-Stufen flach: Newton sprang dort zwischen voller Begrenzung und null
    // hin und her. Darum aendert sich die Ausgangsspannung (gegen Masse) je Iteration hoechstens um kMaxStepV.
    const double vin = iterate.voltage(terminal(0)), vgnd = iterate.voltage(terminal(1));
    double vout = iterate.voltage(terminal(2));
    const double old = m_v0[2] - m_v0[1], now = vout - vgnd;
    bool limited = false;
    // im Regelbereich (Sigmoide steil) kleiner, sonst pendelt Newton um den Sollwert
    const double maxStep = std::fabs(old - m_params.refV) < 5.0 * m_params.errorV ? 2.0 * m_params.errorV : kMaxStepV;
    if (std::fabs(now - old) > maxStep) {
        vout = vgnd + old + std::copysign(maxStep, now - old);
        limited = true;
    }
    evaluate(vin, vgnd, vout);
    return limited;
}

void VoltageRegulator::resetLinearization(const SolutionView& accepted) noexcept {
    evaluate(accepted.voltage(terminal(0)), accepted.voltage(terminal(1)), accepted.voltage(terminal(2)));
}

void VoltageRegulator::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    const NodeId in = terminal(0), gnd = terminal(1), out = terminal(2);
    // Laengsstrom Ein -> Aus: I = sum g_k v_k + ieq
    double ieq = m_i0;
    for (std::size_t k = 0; k < 3; ++k) {
        ieq -= m_g[k] * m_v0[k];
        sys.stampNodeMatrix(in, terminal(k), m_g[k]);
        sys.stampNodeMatrix(out, terminal(k), -m_g[k]);
    }
    sys.stampNodeRhs(in, -ieq);
    sys.stampNodeRhs(out, ieq);
    // Ruhestrom Ein -> Masse (Leitwert + Stromquelle) und innerer Teiler Aus -> Masse
    sys.stampConductance(in, gnd, m_qg);
    sys.stampCurrentSource(in, gnd, m_q0 - m_qg * m_qv0);
    sys.stampConductance(out, gnd, m_destroyed ? kGmin : 1.0 / m_params.dividerOhm);
}

double VoltageRegulator::passCurrent(const SolutionView& x) const noexcept {
    double i = m_i0;
    for (std::size_t k = 0; k < 3; ++k) i += m_g[k] * (x.voltage(terminal(k)) - m_v0[k]);
    return i;
}

double VoltageRegulator::quiescent(const SolutionView& x) const noexcept {
    return m_q0 + m_qg * (x.voltage(terminal(0)) - x.voltage(terminal(1)) - m_qv0);
}

double VoltageRegulator::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    const double pass = passCurrent(x), q = quiescent(x);
    const double div = (x.voltage(terminal(2)) - x.voltage(terminal(1))) * (m_destroyed ? kGmin : 1.0 / m_params.dividerOhm);
    switch (t) {
    case 0: return pass + q;        // hinein am Eingang
    case 1: return -q - div;        // Masse gibt Ruhestrom und Teilerstrom ab
    default: return div - pass;     // Ausgang: Laengsstrom heraus, Teilerstrom hinein
    }
}

void VoltageRegulator::thermal(const SolutionView& x, double dtS) noexcept {
    const double vin = x.voltage(terminal(0)), vgnd = x.voltage(terminal(1)), vout = x.voltage(terminal(2));
    const double pass = passCurrent(x);
    m_lastOutA = pass;
    const double p = std::max(0.0, (vin - vout) * pass) + std::max(0.0, (vin - vgnd) * quiescent(x));
    integrateThermal(p, dtS, m_params.thermal);
}

void VoltageRegulator::checkLimits(EventQueue& /*events*/, std::size_t /*index*/, double /*timeS*/) noexcept {
    // Schutzschaltung: ueberlastet (Strom ueber Dauerwert oder Uebertemperaturschutz aktiv), nie zerstoert
    m_overloaded = !isDestroyed() && (m_lastOutA > m_params.ratedA || m_tempK > m_params.shutdownK);
}

ComponentReading VoltageRegulator::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    r.level = std::max(0.0, -r.terminalCurrentA[2]) / m_params.ratedA;
    r.displayValue = x.voltage(terminal(2)) - x.voltage(terminal(1));
    return r;
}

} // namespace volta::core

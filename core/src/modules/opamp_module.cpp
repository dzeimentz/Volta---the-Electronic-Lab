#include "volta/core/modules/opamp_module.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace volta::core {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kSplitEpsA = 1e-6;    // Glaettung der Aufteilung des Ausgangsstroms auf +U und -U
constexpr double kMaxOutStepV = 1.0;   // Begrenzung der Ausgangsspannung je Newton-Iteration
constexpr double kSupplyKneeV = 0.5;   // OP schaltet oberhalb minSupplyV weich ein
constexpr double kWindupV = 1.0;       // innerer Zustand hoechstens so weit ueber die Aussteuergrenze
constexpr double kMaxArgStep = 2.0;    // Schritt des tanh-Arguments der Aussteuerung je Iteration im Uebergang
constexpr double kSaturatedArg = 3.0;  // ab hier gilt der OP als ausgesteuert
constexpr int kVerifiedSteps = 64;     // so viele Schritte im linearen Bereich ohne erneuten Anstoss
} // namespace

OpAmpModule::OpAmpModule(std::string id, const OpAmpModulePins& pins, OpAmpParams params)
    : ComponentModel(std::move(id), {pins.plus, pins.minus, pins.inP, pins.inN, pins.out}), m_params(params) {
    if (!(m_params.openLoopGain > 1.0)) m_params.openLoopGain = 2.0e5;
    if (!(m_params.gbwHz > 0.0)) m_params.gbwHz = 1.0e6;
    if (!(m_params.slewVPerS > 0.0)) m_params.slewVPerS = 0.5e6;
    if (!(m_params.outputOhm > 0.0)) m_params.outputOhm = 75.0;
    if (!(m_params.currentLimitA > 0.0)) m_params.currentLimitA = 0.025;
    if (!(m_params.inputOhm > 0.0)) m_params.inputOhm = 10.0e6;
}

void OpAmpModule::beginSolve(const StampContext& ctx) noexcept { m_dtS = ctx.dtS; }

double OpAmpModule::targetFor(double vd, double* dTarget) const noexcept {
    const double a0 = m_params.openLoopGain;
    double ys = 0.0, dys = 0.0;
    if (m_dtS > 0.0) {
        const double tau = a0 / (2.0 * kPi * m_params.gbwHz);
        const double a = m_dtS / tau;
        const double ylin = (m_y + a * a0 * vd) / (1.0 + a);
        const double s = m_params.slewVPerS * m_dtS;
        const double t = std::tanh((ylin - m_y) / s);
        ys = m_y + s * t;
        dys = a * a0 / (1.0 + a) * (1.0 - t * t);
    } else { // Arbeitspunkt: eingeschwungen
        ys = a0 * vd;
        dys = a0;
    }
    const double w = std::tanh((ys - m_mid) / m_half);
    if (dTarget) *dTarget = dys * (1.0 - w * w);
    return m_mid + m_half * w;
}

void OpAmpModule::evaluate(double vd, double vout) noexcept {
    double dT = 0.0;
    const double target = targetFor(vd, &dT);
    const double ilim = m_params.currentLimitA;
    const double t = std::tanh((target - vout) / (ilim * m_params.outputOhm));
    const double k = m_destroyed ? 0.0 : m_on;
    m_vd0 = vd;
    m_vo0 = vout;
    m_iout = k * ilim * t;
    const double dIdT = k * (1.0 - t * t) / m_params.outputOhm;
    m_dIdVd = dIdT * dT;
    m_dIdVo = -dIdT;
    const double root = std::sqrt(m_iout * m_iout + kSplitEpsA * kSplitEpsA);
    m_ip = 0.5 * (m_iout + root);
    m_dIpdI = 0.5 * (1.0 + m_iout / root);
}

bool OpAmpModule::linearize(const SolutionView& iterate) noexcept {
    // Versorgung aus der aktuellen Naeherung (als Konstante, Batterien halten sie fest)
    const double vp = iterate.voltage(terminal(0)), vm = iterate.voltage(terminal(1));
    const double supply = vp - vm;
    m_mid = 0.5 * (vp + vm);
    m_half = std::max(0.1, 0.5 * supply - m_params.headroomV);
    m_on = std::clamp((supply - m_params.minSupplyV) / kSupplyKneeV, 0.0, 1.0);
    m_iq = m_destroyed ? 0.0 : m_params.quiescentA * std::tanh(std::max(0.0, supply));
    double vd = iterate.voltage(terminal(2)) - iterate.voltage(terminal(3));
    double vout = iterate.voltage(terminal(4));
    bool limited = false;
    // Die Aussteuerung ist ausserhalb eines schmalen Bereichs (wenige Mikrovolt) flach: springt vd von einer Seite
    // in die andere, haette Newton keine Steigung und pendelte zwischen den Grenzen. Darum wird das Argument
    // u = (y_lin(vd) - Mitte) / h begrenzt: aus der Aussteuerung heraus hoechstens bis u = 0, im Uebergang um
    // kMaxArgStep; tiefer in dieselbe Aussteuerung hinein frei.
    double c0 = 0.0, c1 = m_params.openLoopGain; // y_lin = c0 + c1 * vd
    if (m_dtS > 0.0) {
        const double a = m_dtS / (m_params.openLoopGain / (2.0 * kPi * m_params.gbwHz));
        c0 = m_y / (1.0 + a);
        c1 = a * m_params.openLoopGain / (1.0 + a);
    }
    const double uOld = (c0 + c1 * m_vd0 - m_mid) / m_half, uNew = (c0 + c1 * vd - m_mid) / m_half;
    const double hi = uOld >= kSaturatedArg ? uNew : std::max(uOld + kMaxArgStep, 0.0);
    const double lo = uOld <= -kSaturatedArg ? uNew : std::min(uOld - kMaxArgStep, 0.0);
    if (m_kickPending) {
        // Anstoss: Newton beginnt an der Aussteuergrenze auf der Seite, auf der der Ausgang liegt. Bei Gegenkopplung
        // ist das keine Loesung, die Begrenzung oben fuehrt in den linearen Bereich zurueck. Bei Mitkopplung bleibt
        // der OP an der Grenze - wie in echt, wo das Gleichgewicht in der Mitte in Mikrosekunden zerfaellt. Ohne
        // Anstoss hielte das implizite Verfahren (Schritt viel groesser als diese Zeit) den Ausgang in der Mitte fest.
        m_kickPending = false;
        m_kicked = true;
        const double side = vout >= m_mid ? 1.0 : -1.0;
        evaluate(((kSaturatedArg + 1.0) * side * m_half + m_mid - c0) / c1, m_mid + side * m_half);
        return true;
    }
    if (uNew > hi || uNew < lo) {
        vd = (std::clamp(uNew, lo, hi) * m_half + m_mid - c0) / c1;
        limited = true;
    }
    if (std::fabs(vout - m_vo0) > kMaxOutStepV) { // flache Begrenzungen: sonst springt Newton hin und her
        vout = m_vo0 + std::copysign(kMaxOutStepV, vout - m_vo0);
        limited = true;
    }
    evaluate(vd, vout);
    return limited;
}

void OpAmpModule::resetLinearization(const SolutionView& accepted) noexcept {
    m_vd0 = accepted.voltage(terminal(2)) - accepted.voltage(terminal(3));
    m_vo0 = accepted.voltage(terminal(4));
    // im linearen Bereich ohne Nachweis der Gegenkopplung: anstossen (siehe linearize)
    const double vp = accepted.voltage(terminal(0)), vm = accepted.voltage(terminal(1));
    const double half = std::max(0.1, 0.5 * (vp - vm) - m_params.headroomV);
    m_kickPending = m_verified <= 0 && std::fabs(m_y - 0.5 * (vp + vm)) < kSaturatedArg * half;
    m_kicked = false;
    linearize(accepted);
}

namespace {
// Strom in den Anschluss node: k + c * dI mit dI = dIdVd * (vd - vd0) + dIdVo * (vo - vo0)
void stampLinear(MnaSystem& sys, NodeId node, NodeId inP, NodeId inN, NodeId out, double k, double c, double dIdVd,
                 double dIdVo, double vd0, double vo0) noexcept {
    sys.stampNodeMatrix(node, inP, c * dIdVd);
    sys.stampNodeMatrix(node, inN, -c * dIdVd);
    sys.stampNodeMatrix(node, out, c * dIdVo);
    sys.stampNodeRhs(node, -(k - c * (dIdVd * vd0 + dIdVo * vo0)));
}
} // namespace

void OpAmpModule::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    const NodeId p = terminal(0), m = terminal(1), inP = terminal(2), inN = terminal(3), out = terminal(4);
    sys.stampConductance(inP, inN, 1.0 / m_params.inputOhm);
    stampLinear(sys, p, inP, inN, out, m_iq + m_ip, m_dIpdI, m_dIdVd, m_dIdVo, m_vd0, m_vo0);
    stampLinear(sys, m, inP, inN, out, -m_iq - m_ip + m_iout, 1.0 - m_dIpdI, m_dIdVd, m_dIdVo, m_vd0, m_vo0);
    stampLinear(sys, out, inP, inN, out, -m_iout, -1.0, m_dIdVd, m_dIdVo, m_vd0, m_vo0);
}

double OpAmpModule::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    const double vd = x.voltage(terminal(2)) - x.voltage(terminal(3));
    const double dI = m_dIdVd * (vd - m_vd0) + m_dIdVo * (x.voltage(terminal(4)) - m_vo0);
    switch (t) {
    case 0: return m_iq + m_ip + m_dIpdI * dI;
    case 1: return -m_iq - m_ip + m_iout + (1.0 - m_dIpdI) * dI;
    case 2: return vd / m_params.inputOhm;
    case 3: return -vd / m_params.inputOhm;
    default: return -(m_iout + dI);
    }
}

void OpAmpModule::updateState(const SolutionView& x, double dtS) noexcept {
    const double vp = x.voltage(terminal(0)), vm = x.voltage(terminal(1));
    const double mid = 0.5 * (vp + vm), half = std::max(0.1, 0.5 * (vp - vm) - m_params.headroomV);
    const double vd = x.voltage(terminal(2)) - x.voltage(terminal(3));
    m_lastVd = vd;
    const double saved = m_dtS;
    m_dtS = dtS;
    double y = 0.0;
    if (dtS > 0.0) {
        const double a0 = m_params.openLoopGain;
        const double a = dtS / (a0 / (2.0 * kPi * m_params.gbwHz));
        const double ylin = (m_y + a * a0 * vd) / (1.0 + a);
        const double s = m_params.slewVPerS * dtS;
        y = m_y + s * std::tanh((ylin - m_y) / s);
    } else {
        y = m_params.openLoopGain * vd;
    }
    m_dtS = saved;
    // gegen Aufwickeln: der innere Zustand bleibt nahe der Aussteuergrenze
    const double y2 = std::clamp(y, mid - half - kWindupV, mid + half + kWindupV);
    m_y = std::isfinite(y2) ? y2 : mid;
    // Gegenkopplung nachgewiesen, wenn der OP nach einem Anstoss im linearen Bereich ankommt; ausgesteuert verfaellt
    // der Nachweis (die Schaltung kann sich durch Bedienung geaendert haben), im linearen Bereich nach kVerifiedSteps
    const bool linear = std::fabs(m_y - mid) < kSaturatedArg * half;
    if (!linear) m_verified = 0;
    else if (m_kicked) m_verified = kVerifiedSteps;
    else --m_verified;
    m_kicked = false;
}

ComponentReading OpAmpModule::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    const double vp = x.voltage(terminal(0)), vm = x.voltage(terminal(1)), vo = x.voltage(terminal(4));
    r.level = vo > 0.5 * (vp + vm) ? 1.0 : 0.0;
    r.displayValue = vo - vm;
    return r;
}

ComponentState OpAmpModule::saveState() const noexcept {
    ComponentState s = ComponentModel::saveState();
    s.internal[0] = m_y;
    return s;
}

void OpAmpModule::restoreState(const ComponentState& state) noexcept {
    ComponentModel::restoreState(state);
    m_y = std::isfinite(state.internal[0]) ? state.internal[0] : 0.0;
    m_verified = 0;
}

SubcircuitRange addOpAmpModule(Circuit& circuit, const std::string& prefix, const OpAmpModulePins& pins) {
    SubcircuitRange range;
    range.first = circuit.componentCount();
    circuit.add<OpAmpModule>(prefix, pins);
    range.count = circuit.componentCount() - range.first;
    return range;
}

} // namespace volta::core

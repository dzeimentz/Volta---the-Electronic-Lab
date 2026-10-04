#include "volta/core/components/diode.hpp"

#include <cmath>
#include <utility>

#include "pn_junction.hpp"

namespace volta::core {

namespace {
constexpr double kBoltzmannJPerK = 1.380649e-23;
constexpr double kElementaryChargeC = 1.602176634e-19;
} // namespace

Diode::Diode(std::string id, NodeId anode, NodeId cathode, DiodeParams params)
    : ComponentModel(std::move(id), {anode, cathode}), m_params(params) {
    const DiodeParams def = DiodeParams::silicon();
    if (!(m_params.n > 0.0) || !std::isfinite(m_params.n)) m_params.n = def.n;
    if (!(m_params.ifRefA > 0.0) || !std::isfinite(m_params.ifRefA)) m_params.ifRefA = def.ifRefA;
    if (!(m_params.rsOhm >= 0.0) || !std::isfinite(m_params.rsOhm)) m_params.rsOhm = 0.0;
    if (m_params.rsOhm > 0.0 && m_params.rsOhm < kMinOhm) m_params.rsOhm = kMinOhm;
    if (!std::isfinite(m_params.ufV)) m_params.ufV = def.ufV;
    if (!(m_params.imaxA > 0.0) || !std::isfinite(m_params.imaxA)) m_params.imaxA = def.imaxA;
    if (!(m_params.overloadFactor >= 1.0) || !std::isfinite(m_params.overloadFactor)) m_params.overloadFactor = 2.5;
    if (!(m_params.overloadFailS >= 0.0) || !std::isfinite(m_params.overloadFailS)) m_params.overloadFailS = 3.0;

    const double vt = kBoltzmannJPerK * kAmbientK / kElementaryChargeC; // ca. 25,3 mV bei 20 Grad C
    m_nVt = m_params.n * vt;
    // Is aus Uf beim Bezugsstrom: Uf = n*Vt*ln(If/Is + 1) + If*Rs; Uebergangsspannung mindestens 0,05 V
    double vj = m_params.ufV - m_params.ifRefA * m_params.rsOhm;
    if (vj < 0.05) vj = 0.05;
    m_isA = m_params.ifRefA / std::expm1(vj / m_nVt);
    if (!(m_isA > 0.0) || !std::isfinite(m_isA)) m_isA = 1e-14;
    m_vcrit = m_nVt * std::log(m_nVt / (std::sqrt(2.0) * m_isA));
    // Untergrenze: bei unsinnigen Kennwerten (sehr grosses Is) waere vcrit negativ und log(vnew/nVt) in pnjlim
    // nicht definiert.
    if (!std::isfinite(m_vcrit) || m_vcrit < m_nVt) m_vcrit = m_nVt;
    m_xMax = pn::linearFromArg(m_isA, m_nVt, kMaxConductance - kJunctionGmin);
    evaluate(0.0);
}

double Diode::junctionCurrentA(double vdV) const noexcept {
    double e = 0.0;
    double de = 0.0;
    pn::safeExp(vdV / m_nVt, m_xMax, e, de);
    return m_isA * (e - 1.0);
}

void Diode::evaluate(double vdV) noexcept {
    double e = 0.0;
    double de = 0.0;
    pn::safeExp(vdV / m_nVt, m_xMax, e, de);
    const double id = m_isA * (e - 1.0);
    // Oberhalb m_xMax ist die Kennlinie linear mit der Steigung kMaxConductance: Leitwert und Kennlinie passen
    // zusammen (Vertrag ComponentModel), Newton konvergiert dort sofort.
    const double gd = m_isA * de / m_nVt;
    m_vdLin = vdV;
    m_g = gd + kJunctionGmin; // parallel zum Uebergang (SPICE GMIN), haelt den inneren Knoten in Sperrrichtung
    m_ieq = id - gd * vdV;
}

bool Diode::linearize(const SolutionView& iterate) noexcept {
    if (m_destroyed) { // offener Uebergang: linear, keine Begrenzung (sonst konvergiert der Schritt nie)
        m_vdLin = junctionVoltage(iterate);
        return false;
    }
    bool limited = false;
    const double vd = pn::pnjlim(junctionVoltage(iterate), m_vdLin, m_nVt, m_vcrit, limited);
    evaluate(vd);
    return limited;
}

void Diode::resetLinearization(const SolutionView& accepted) noexcept {
    const double vd = junctionVoltage(accepted);
    if (m_destroyed) {
        m_vdLin = vd;
        return;
    }
    // Nach Undo einer Zerstoerung kann vd weit ueber vcrit liegen; pnjlim fuehrt die folgenden Iterationen
    // dann auf vcrit zurueck (Zweig arg <= 0).
    evaluate(vd);
}

void Diode::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    const NodeId ja = junctionAnode();
    if (m_params.rsOhm > 0.0) sys.stampConductance(terminal(0), ja, 1.0 / m_params.rsOhm);
    if (m_destroyed) { // zerstoert: Uebergang offen, nur kJunctionGmin haelt den inneren Knoten
        sys.stampConductance(ja, terminal(1), kJunctionGmin);
        return;
    }
    sys.stampConductance(ja, terminal(1), m_g);
    sys.stampCurrentSource(ja, terminal(1), m_ieq);
}

double Diode::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    double i = 0.0;
    if (m_params.rsOhm > 0.0) {
        // Strom durch den Bahnwiderstand; am Kathodenanschluss der Strom des Uebergangs (gleich bis auf Rundung)
        if (t == 0) return (x.voltage(terminal(0)) - x.voltage(internalNode(0))) / m_params.rsOhm;
    }
    const double vd = junctionVoltage(x);
    i = m_destroyed ? kJunctionGmin * vd : m_g * vd + m_ieq;
    return t == 0 ? i : -i;
}

void Diode::thermal(const SolutionView& x, double dtS) noexcept {
    m_lastDtS = dtS;
    m_lastCurrentA = terminalCurrent(x, 0);
    const double p = (x.voltage(terminal(0)) - x.voltage(terminal(1))) * m_lastCurrentA;
    integrateThermal(m_destroyed ? 0.0 : p, dtS, m_params.thermal);
}

void Diode::checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept {
    if (m_destroyed) return;
    m_overloaded = m_lastCurrentA > m_params.imaxA;
    const bool severe = m_lastCurrentA > m_params.overloadFactor * m_params.imaxA;
    m_overloadS = severe ? m_overloadS + m_lastDtS : 0.0;
    if (severe && m_overloadS >= m_params.overloadFailS - 1e-9) destroy(events, index, timeS);
    checkThermalLimit(m_params.thermal, events, index, timeS);
}

ComponentReading Diode::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    r.level = (m_destroyed || r.currentA <= 0.0) ? 0.0 : r.currentA / m_params.ifRefA;
    return r;
}

ComponentState Diode::saveState() const noexcept {
    ComponentState s = ComponentModel::saveState();
    s.internal[0] = m_overloadS;
    return s;
}

void Diode::restoreState(const ComponentState& state) noexcept {
    ComponentModel::restoreState(state);
    m_overloadS = std::isfinite(state.internal[0]) ? state.internal[0] : 0.0;
}

} // namespace volta::core

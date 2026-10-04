#include "volta/core/components/diode.hpp"

#include <cmath>
#include <utility>

namespace volta::core {

namespace {
constexpr double kBoltzmannJPerK = 1.380649e-23;
constexpr double kElementaryChargeC = 1.602176634e-19;
constexpr double kMaxExpArg = 80.0; // darueber wird exp linear fortgesetzt (Schutz vor Ueberlauf)

// exp mit linearer Fortsetzung oberhalb kMaxExpArg; liefert auch die Ableitung.
void safeExp(double x, double& value, double& deriv) noexcept {
    if (x > kMaxExpArg) {
        const double e = std::exp(kMaxExpArg);
        value = e * (1.0 + x - kMaxExpArg);
        deriv = e;
    } else {
        value = std::exp(x);
        deriv = value;
    }
}

// Spannungsbegrenzung fuer pn-Uebergaenge nach SPICE3 (DEVpnjlim).
double pnjlim(double vnew, double vold, double nVt, double vcrit, bool& limited) noexcept {
    if (vnew > vcrit && std::fabs(vnew - vold) > 2.0 * nVt) {
        limited = true;
        if (vold > 0.0) {
            const double arg = 1.0 + (vnew - vold) / nVt;
            return arg > 0.0 ? vold + nVt * std::log(arg) : vcrit;
        }
        return nVt * std::log(vnew / nVt);
    }
    return vnew;
}
} // namespace

Diode::Diode(std::string id, NodeId anode, NodeId cathode, DiodeParams params)
    : ComponentModel(std::move(id), {anode, cathode}), m_params(params) {
    const DiodeParams def = DiodeParams::silicon();
    if (!(m_params.n > 0.0) || !std::isfinite(m_params.n)) m_params.n = def.n;
    if (!(m_params.ifRefA > 0.0) || !std::isfinite(m_params.ifRefA)) m_params.ifRefA = def.ifRefA;
    if (!(m_params.rsOhm >= 0.0) || !std::isfinite(m_params.rsOhm)) m_params.rsOhm = 0.0;
    if (m_params.rsOhm > 0.0 && m_params.rsOhm < kMinOhm) m_params.rsOhm = kMinOhm;
    if (!std::isfinite(m_params.ufV)) m_params.ufV = def.ufV;

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
    evaluate(0.0);
}

double Diode::junctionCurrentA(double vdV) const noexcept {
    double e = 0.0;
    double de = 0.0;
    safeExp(vdV / m_nVt, e, de);
    return m_isA * (e - 1.0);
}

void Diode::evaluate(double vdV) noexcept {
    double e = 0.0;
    double de = 0.0;
    safeExp(vdV / m_nVt, e, de);
    const double id = m_isA * (e - 1.0);
    // Leitwert begrenzen (Vertrag ComponentModel): flachere Gerade durch den exakten Punkt (vdV, id). Newton
    // konvergiert dann nur linear; die Grenze greift erst bei ca. 50 A und ist ein reiner Schutz.
    double gd = m_isA * de / m_nVt;
    if (gd > kMaxConductance - kGmin) gd = kMaxConductance - kGmin;
    m_vdLin = vdV;
    m_g = gd + kGmin; // GMIN parallel zum Uebergang (SPICE), haelt den inneren Knoten auch in Sperrrichtung
    m_ieq = id - gd * vdV;
}

bool Diode::linearize(const SolutionView& iterate) noexcept {
    if (m_destroyed) { // offener Uebergang: linear, keine Begrenzung (sonst konvergiert der Schritt nie)
        m_vdLin = junctionVoltage(iterate);
        return false;
    }
    bool limited = false;
    const double vd = pnjlim(junctionVoltage(iterate), m_vdLin, m_nVt, m_vcrit, limited);
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
    if (m_destroyed) { // zerstoert: Uebergang offen, nur GMIN haelt den inneren Knoten
        sys.stampConductance(ja, terminal(1), kGmin);
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
    i = m_destroyed ? kGmin * vd : m_g * vd + m_ieq;
    return t == 0 ? i : -i;
}

} // namespace volta::core

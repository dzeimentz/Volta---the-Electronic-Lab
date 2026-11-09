#include "volta/core/components/mosfet.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include "pn_junction.hpp"

namespace volta::core {

namespace {
constexpr double kBoltzmannJPerK = 1.380649e-23;
constexpr double kElementaryChargeC = 1.602176634e-19;
constexpr double kGateStepV = 10.0; // groesste Aenderung der Gate-Spannung je Newton-Schritt (Kennlinie ohne e-Funktion)

MosfetParams sanitized(MosfetParams p) noexcept {
    const MosfetParams def;
    auto fix = [](double& v, double fallback, double minimum) {
        if (!std::isfinite(v) || v < minimum) v = fallback;
    };
    fix(p.vthV, def.vthV, 0.0);
    fix(p.kAPerV2, def.kAPerV2, 1e-6);
    fix(p.lambdaPerV, def.lambdaPerV, 0.0);
    fix(p.smoothV, def.smoothV, 1e-3);
    fix(p.cgsF, def.cgsF, 0.0);
    fix(p.bodyIsA, def.bodyIsA, 1e-30);
    fix(p.iMaxA, def.iMaxA, 1e-6);
    fix(p.overloadFactor, def.overloadFactor, 1.0);
    fix(p.overloadFailS, def.overloadFailS, 0.0);
    return p;
}
} // namespace

Mosfet::Mosfet(std::string id, NodeId gate, NodeId drain, NodeId source, MosfetParams params)
    : ComponentModel(std::move(id), {gate, drain, source}), m_params(sanitized(params)) {
    m_sign = m_params.pChannel ? -1.0 : 1.0;
    m_vt = kBoltzmannJPerK * kAmbientK / kElementaryChargeC;
    m_vcrit = m_vt * std::log(m_vt / (std::sqrt(2.0) * m_params.bodyIsA));
    if (!std::isfinite(m_vcrit) || m_vcrit < m_vt) m_vcrit = m_vt;
    m_xMax = pn::linearFromArg(m_params.bodyIsA, m_vt, kMaxConductance / 4.0);
    evaluate(0.0, 0.0);
}

void Mosfet::channel(double ugs, double uds, double& i, double& gm, double& gds) const noexcept {
    const double sm = m_params.smoothV;
    const double x = (ugs - m_params.vthV) / sm;
    double vov = 0.0, dvov = 0.0;
    if (x > 40.0) {
        vov = ugs - m_params.vthV;
        dvov = 1.0;
    } else {
        const double e = std::exp(x);
        vov = sm * std::log1p(e);
        dvov = e / (1.0 + e);
    }
    double base = 0.0, dBaseVov = 0.0, dBaseVds = 0.0;
    if (uds < vov) {
        base = vov * uds - 0.5 * uds * uds;
        dBaseVov = uds;
        dBaseVds = vov - uds;
    } else {
        base = 0.5 * vov * vov;
        dBaseVov = vov;
    }
    const double k = m_params.kAPerV2;
    const double m = 1.0 + m_params.lambdaPerV * uds;
    i = k * base * m;
    gm = k * dBaseVov * dvov * m;
    gds = k * (dBaseVds * m + base * m_params.lambdaPerV);
}

double Mosfet::channelCurrent(double ugs, double uds) const noexcept {
    double i = 0.0, gm = 0.0, gds = 0.0;
    if (uds >= 0.0) {
        channel(ugs, uds, i, gm, gds);
        return i;
    }
    channel(ugs, -uds, i, gm, gds);
    return -i;
}

void Mosfet::evaluate(double ugs, double uds) noexcept {
    double i = 0.0, gm = 0.0, gds = 0.0;
    if (uds >= 0.0) {
        channel(ugs, uds, i, gm, gds);
    } else {
        // Rueckwaerts: Der Kanal leitet in Gegenrichtung, gesteuert weiter von Ugs (Substrat an der Source)
        double i2 = 0.0, gm2 = 0.0, gds2 = 0.0;
        channel(ugs, -uds, i2, gm2, gds2);
        i = -i2;
        gm = -gm2;
        gds = gds2;
    }
    // Body-Diode Source -> Drain (Spannung Usd)
    double e = 0.0, de = 0.0;
    pn::safeExp(-uds / m_vt, m_xMax, e, de);
    i -= m_params.bodyIsA * (e - 1.0) - kJunctionGmin * uds;
    gds += m_params.bodyIsA * de / m_vt + kJunctionGmin;
    m_vgs0 = ugs;
    m_vds0 = uds;
    m_id0 = i;
    m_gm = gm;
    m_gds = gds;
}

bool Mosfet::linearize(const SolutionView& iterate) noexcept {
    if (m_destroyed) return false;
    bool limited = false;
    double ugs = vgs(iterate);
    if (std::fabs(ugs - m_vgs0) > kGateStepV) {
        ugs = m_vgs0 + std::copysign(kGateStepV, ugs - m_vgs0);
        limited = true;
    }
    // Body-Diode wie ein pn-Uebergang begrenzen (Spannung Usd)
    const double usd = pn::pnjlim(-vds(iterate), -m_vds0, m_vt, m_vcrit, limited);
    evaluate(ugs, -usd);
    return limited;
}

void Mosfet::resetLinearization(const SolutionView& accepted) noexcept {
    if (m_destroyed) return;
    evaluate(vgs(accepted), vds(accepted));
}

void Mosfet::beginSolve(const StampContext& ctx) noexcept {
    m_invDt = ctx.isDc() ? 0.0 : 1.0 / ctx.dtS;
    m_qOld = m_qNext;
}

double Mosfet::drainLinear(double ugs, double uds) const noexcept {
    return m_id0 + m_gm * (ugs - m_vgs0) + m_gds * (uds - m_vds0);
}

double Mosfet::gateLinear(double ugs) const noexcept {
    return (m_params.cgsF * ugs - m_qOld) * m_invDt + kJunctionGmin * ugs;
}

void Mosfet::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    if (m_destroyed) return; // offen; aeussere Knoten haben GMIN
    // Drain: I = gm*Ugs + gds*Uds + Konstante
    sys.stampNodeMatrix(d(), g(), m_gm);
    sys.stampNodeMatrix(d(), d(), m_gds);
    sys.stampNodeMatrix(d(), s(), -(m_gm + m_gds));
    sys.stampNodeRhs(d(), -m_sign * (m_id0 - m_gm * m_vgs0 - m_gds * m_vds0));
    // Gate: Kapazitaet und GMIN gegen Source
    const double gg = std::min(m_params.cgsF * m_invDt, kMaxConductance / 4.0) + kJunctionGmin;
    const double ig0 = -m_qOld * m_invDt;
    sys.stampConductance(g(), s(), gg);
    sys.stampCurrentSource(g(), s(), m_sign * ig0);
    // Source: Gegenstrom des Drains
    sys.stampNodeMatrix(s(), g(), -m_gm);
    sys.stampNodeMatrix(s(), d(), -m_gds);
    sys.stampNodeMatrix(s(), s(), m_gm + m_gds);
    sys.stampNodeRhs(s(), m_sign * (m_id0 - m_gm * m_vgs0 - m_gds * m_vds0));
}

double Mosfet::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    if (m_destroyed || t > 2) return 0.0;
    const double ugs = vgs(x);
    const double iD = drainLinear(ugs, vds(x));
    const double iG = gateLinear(ugs);
    if (t == 0) return m_sign * iG;
    if (t == 1) return m_sign * iD;
    return -m_sign * (iD + iG);
}

void Mosfet::updateState(const SolutionView& x, double /*dtS*/) noexcept {
    if (m_destroyed) return;
    m_qNext = m_params.cgsF * vgs(x);
}

void Mosfet::thermal(const SolutionView& x, double dtS) noexcept {
    m_lastDtS = dtS;
    m_lastIA = std::fabs(terminalCurrent(x, 1));
    double p = 0.0;
    for (std::size_t t = 0; t < 3; ++t) p += x.voltage(terminal(t)) * terminalCurrent(x, t);
    integrateThermal(m_destroyed ? 0.0 : p, dtS, m_params.thermal);
}

void Mosfet::checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept {
    if (isDestroyed()) return;
    m_overloaded = m_lastIA > m_params.iMaxA;
    const bool severe = m_lastIA > m_params.overloadFactor * m_params.iMaxA;
    m_overloadS = severe ? m_overloadS + m_lastDtS : 0.0;
    if (severe && m_overloadS >= m_params.overloadFailS - 1e-9) destroy(events, index, timeS);
    checkThermalLimit(m_params.thermal, events, index, timeS);
}

ComponentReading Mosfet::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    r.voltV = x.voltage(d()) - x.voltage(s()); // fuer die Anzeige: Uds
    r.currentA = terminalCurrent(x, 1);        // und Id
    r.level = m_sign * r.currentA / m_params.iMaxA;
    return r;
}

ComponentState Mosfet::saveState() const noexcept {
    ComponentState st = ComponentModel::saveState();
    st.internal[0] = m_overloadS;
    st.internal[1] = m_qNext;
    return st;
}

void Mosfet::restoreState(const ComponentState& state) noexcept {
    ComponentModel::restoreState(state);
    auto finite = [](double v) { return std::isfinite(v) ? v : 0.0; };
    m_overloadS = finite(state.internal[0]);
    m_qNext = finite(state.internal[1]);
}

} // namespace volta::core

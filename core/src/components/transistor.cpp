#include "volta/core/components/transistor.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include "pn_junction.hpp"

namespace volta::core {

namespace {
constexpr double kBoltzmannJPerK = 1.380649e-23;
constexpr double kElementaryChargeC = 1.602176634e-19;
} // namespace

Npn::Npn(std::string id, NodeId base, NodeId collector, NodeId emitter, BjtParams params)
    : ComponentModel(std::move(id), {base, collector, emitter}), m_params(params) {
    const BjtParams def;
    auto fix = [](double& v, double fallback, double minimum) {
        if (!std::isfinite(v) || v < minimum) v = fallback;
    };
    fix(m_params.isA, def.isA, 1e-30);
    fix(m_params.betaF, def.betaF, 1.0);
    fix(m_params.betaR, def.betaR, 0.01);
    fix(m_params.icMaxA, def.icMaxA, 1e-6);
    fix(m_params.cbeF, def.cbeF, 0.0);
    fix(m_params.cbcF, def.cbcF, 0.0);
    fix(m_params.tauFS, def.tauFS, 0.0);
    fix(m_params.tauRS, def.tauRS, 0.0);
    fix(m_params.overloadFactor, def.overloadFactor, 1.0);
    fix(m_params.overloadFailS, def.overloadFailS, 0.0);
    m_vt = kBoltzmannJPerK * kAmbientK / kElementaryChargeC;
    m_vcrit = m_vt * std::log(m_vt / (std::sqrt(2.0) * m_params.isA));
    if (!std::isfinite(m_vcrit) || m_vcrit < m_vt) m_vcrit = m_vt;
    m_xMax = pn::linearFromArg(m_params.isA, m_vt, kMaxConductance / 4.0);
    evaluate(0.0, 0.0);
}

void Npn::evaluate(double vbe, double vbc) noexcept {
    double ebe = 0.0, dbe = 0.0, ebc = 0.0, dbc = 0.0;
    pn::safeExp(vbe / m_vt, m_xMax, ebe, dbe);
    pn::safeExp(vbc / m_vt, m_xMax, ebc, dbc);
    const double is = m_params.isA;
    const double iF = is * (ebe - 1.0);
    const double iR = is * (ebc - 1.0);
    // Oberhalb m_xMax linear mit Steigung kMaxConductance/4: Ableitung und Kennlinie passen zusammen.
    const double gF = is * dbe / m_vt;
    const double gR = is * dbc / m_vt;
    const double bf = m_params.betaF;
    const double br = m_params.betaR;

    // Strom in den Kollektor bzw. die Basis (inkl. kJunctionGmin parallel zu B-E und B-C)
    const double ic = iF - iR * (1.0 + 1.0 / br) - kJunctionGmin * vbc;
    const double ib = iF / bf + iR / br + kJunctionGmin * (vbe + vbc);
    const double dIcBe = gF;
    const double dIcBc = -gR * (1.0 + 1.0 / br) - kJunctionGmin;
    const double dIbBe = gF / bf + kJunctionGmin;
    const double dIbBc = gR / br + kJunctionGmin;

    // Ladungsspeicher (Rueckwaerts-Euler): i = (Q(v) - Q_alt) / dt, B->E bzw. B->C
    const double iQbe = (m_params.cbeF * vbe + m_params.tauFS * iF - m_qbeOld) * m_invDt;
    const double iQbc = (m_params.cbcF * vbc + m_params.tauRS * iR - m_qbcOld) * m_invDt;
    const double gQbe = (m_params.cbeF + m_params.tauFS * gF) * m_invDt;
    const double gQbc = (m_params.cbcF + m_params.tauRS * gR) * m_invDt;

    m_vbe0 = vbe;
    m_vbc0 = vbc;
    m_i0[0] = ib + iQbe + iQbc;
    m_i0[1] = ic - iQbc;
    m_i0[2] = -(ib + ic) - iQbe;
    m_dbe[0] = dIbBe + gQbe;
    m_dbe[1] = dIcBe;
    m_dbe[2] = -(dIbBe + dIcBe) - gQbe;
    m_dbc[0] = dIbBc + gQbc;
    m_dbc[1] = dIcBc - gQbc;
    m_dbc[2] = -(dIbBc + dIcBc);
}

double Npn::chargeBe(double vbeV) const noexcept {
    double e = 0.0, d = 0.0;
    pn::safeExp(vbeV / m_vt, m_xMax, e, d);
    return m_params.cbeF * vbeV + m_params.tauFS * m_params.isA * (e - 1.0);
}

double Npn::chargeBc(double vbcV) const noexcept {
    double e = 0.0, d = 0.0;
    pn::safeExp(vbcV / m_vt, m_xMax, e, d);
    return m_params.cbcF * vbcV + m_params.tauRS * m_params.isA * (e - 1.0);
}

bool Npn::linearize(const SolutionView& iterate) noexcept {
    if (m_destroyed) return false;
    bool limited = false;
    const double vbeNew = pn::pnjlim(vbe(iterate), m_vbe0, m_vt, m_vcrit, limited);
    const double vbcNew = pn::pnjlim(vbc(iterate), m_vbc0, m_vt, m_vcrit, limited);
    evaluate(vbeNew, vbcNew);
    return limited;
}

void Npn::resetLinearization(const SolutionView& accepted) noexcept {
    if (m_destroyed) return;
    evaluate(vbe(accepted), vbc(accepted));
}

void Npn::beginSolve(const StampContext& ctx) noexcept {
    m_invDt = ctx.isDc() ? 0.0 : 1.0 / ctx.dtS;
    m_qbeOld = m_qbeNext;
    m_qbcOld = m_qbcNext;
}

double Npn::linearCurrent(std::size_t t, double vbeV, double vbcV) const noexcept {
    return m_i0[t] + m_dbe[t] * (vbeV - m_vbe0) + m_dbc[t] * (vbcV - m_vbc0);
}

void Npn::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    if (m_destroyed) return; // offen; aeussere Knoten haben GMIN
    // Strom in den Anschluss t verlaesst dessen Knoten: Zeile(Knoten t) += dI/dv * v, rechte Seite -= Konstante
    for (std::size_t t = 0; t < 3; ++t) {
        const NodeId n = terminal(t);
        sys.stampNodeMatrix(n, b(), m_dbe[t] + m_dbc[t]);
        sys.stampNodeMatrix(n, e(), -m_dbe[t]);
        sys.stampNodeMatrix(n, c(), -m_dbc[t]);
        sys.stampNodeRhs(n, -(m_i0[t] - m_dbe[t] * m_vbe0 - m_dbc[t] * m_vbc0));
    }
}

void Npn::updateState(const SolutionView& x, double /*dtS*/) noexcept {
    if (m_destroyed) return;
    m_qbeNext = chargeBe(vbe(x));
    m_qbcNext = chargeBc(vbc(x));
}

double Npn::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    if (m_destroyed || t > 2) return 0.0;
    return linearCurrent(t, vbe(x), vbc(x));
}

void Npn::thermal(const SolutionView& x, double dtS) noexcept {
    m_lastDtS = dtS;
    // Ueberlast nach dem groessten Anschlussstrom (auch Rueckwaertsbetrieb und Basis direkt an der Batterie)
    m_lastIcA = std::max({std::fabs(terminalCurrent(x, 0)), std::fabs(terminalCurrent(x, 1)),
                          std::fabs(terminalCurrent(x, 2))});
    double p = 0.0;
    for (std::size_t t = 0; t < 3; ++t) p += x.voltage(terminal(t)) * terminalCurrent(x, t);
    integrateThermal(m_destroyed ? 0.0 : p, dtS, m_params.thermal);
}

void Npn::checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept {
    if (m_destroyed) return;
    m_overloaded = m_lastIcA > m_params.icMaxA;
    const bool severe = m_lastIcA > m_params.overloadFactor * m_params.icMaxA;
    m_overloadS = severe ? m_overloadS + m_lastDtS : 0.0;
    if (severe && m_overloadS >= m_params.overloadFailS - 1e-9) destroy(events, index, timeS);
    checkThermalLimit(m_params.thermal, events, index, timeS);
}

ComponentReading Npn::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x); // voltV = Vbc, currentA = Ib (Anschluss 0/1)
    r.voltV = x.voltage(c()) - x.voltage(e());     // fuer die Anzeige: Uce
    r.currentA = terminalCurrent(x, 1);            // und Ic
    r.level = r.currentA / m_params.icMaxA;
    return r;
}

ComponentState Npn::saveState() const noexcept {
    ComponentState s = ComponentModel::saveState();
    s.internal[0] = m_overloadS;
    s.internal[1] = m_qbeNext;
    s.internal[2] = m_qbcNext;
    return s;
}

void Npn::restoreState(const ComponentState& state) noexcept {
    ComponentModel::restoreState(state);
    auto finite = [](double v) { return std::isfinite(v) ? v : 0.0; };
    m_overloadS = finite(state.internal[0]);
    m_qbeNext = finite(state.internal[1]);
    m_qbcNext = finite(state.internal[2]);
}

} // namespace volta::core

#include "volta/core/components/inductor.hpp"

#include <cmath>
#include <utility>

namespace volta::core {

Inductor::Inductor(std::string id, NodeId a, NodeId b, InductorParams params)
    : ComponentModel(std::move(id), {a, b}), m_params(params) {
    const InductorParams def;
    if (!(m_params.inductanceH >= 1e-9) || !std::isfinite(m_params.inductanceH)) m_params.inductanceH = def.inductanceH;
    if (!(m_params.windingOhm >= kMinOhm) || !std::isfinite(m_params.windingOhm)) m_params.windingOhm = kMinOhm;
    if (!(m_params.parallelOhm >= kMinOhm) || !std::isfinite(m_params.parallelOhm)) m_params.parallelOhm = def.parallelOhm;
    if (!(m_params.imaxA > 0.0) || !std::isfinite(m_params.imaxA)) m_params.imaxA = def.imaxA;
}

void Inductor::stamp(MnaSystem& sys, const StampContext& ctx) const noexcept {
    if (m_destroyed) { // Wicklung unterbrochen; Rp bleibt (Kern, Kapazitaet)
        sys.stampBranchOpen(firstBranch());
        sys.stampConductance(terminal(0), terminal(1), 1.0 / m_params.parallelOhm);
        return;
    }
    sys.stampConductance(terminal(0), terminal(1), 1.0 / m_params.parallelOhm);
    if (ctx.isDc()) {
        sys.stampVoltageBranch(terminal(0), terminal(1), firstBranch(), 0.0, m_params.windingOhm);
        return;
    }
    const double lOverDt = m_params.inductanceH / ctx.dtS;
    const IntegrationCoeffs k = integrationCoeffs(ctx, m_havePrev, m_lastDtS);
    if (k.secondOrder) { // BDF2: U_L = L/dt * (a0 i - a1 i(n) + a2 i(n-1))
        sys.stampVoltageBranch(terminal(0), terminal(1), firstBranch(),
                               -lOverDt * (k.a1 * m_currentA - k.a2 * m_prevCurrentA),
                               m_params.windingOhm + k.a0 * lOverDt);
        return;
    }
    sys.stampVoltageBranch(terminal(0), terminal(1), firstBranch(), -lOverDt * m_currentA,
                           m_params.windingOhm + lOverDt);
}

bool Inductor::stampAc(ComplexMna& sys, const AcContext& ctx) const noexcept {
    sys.stampAdmittance(terminal(0), terminal(1), 1.0 / m_params.parallelOhm);
    if (m_destroyed) {
        sys.stampBranchOpen(firstBranch());
        return true;
    }
    sys.stampImpedanceBranch(terminal(0), terminal(1), firstBranch(), 0.0,
                             Complex(m_params.windingOhm, ctx.omegaRadS * m_params.inductanceH));
    return true;
}

double Inductor::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    double i = (x.voltage(terminal(0)) - x.voltage(terminal(1))) / m_params.parallelOhm;
    if (!m_destroyed) i += x.branchCurrent(firstBranch());
    return t == 0 ? i : -i;
}

void Inductor::updateState(const SolutionView& x, double dtS) noexcept {
    m_lastDtS = dtS;
    m_havePrev = false;
    if (m_destroyed) {
        m_currentA = 0.0;
        return;
    }
    const double i = x.branchCurrent(firstBranch());
    if (!std::isfinite(i)) return;
    if (dtS > 0.0) { // Arbeitspunkt (dtS 0) beginnt die Vorgeschichte neu
        m_prevCurrentA = m_currentA;
        m_havePrev = true;
    }
    m_currentA = i;
}

void Inductor::thermal(const SolutionView& /*x*/, double dtS) noexcept {
    const double p = m_destroyed ? 0.0 : m_currentA * m_currentA * m_params.windingOhm;
    integrateThermal(p, dtS, m_params.thermal);
}

void Inductor::checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept {
    if (isDestroyed()) return;
    m_overloaded = std::fabs(m_currentA) > m_params.imaxA;
    checkThermalLimit(m_params.thermal, events, index, timeS);
}

ComponentState Inductor::saveState() const noexcept {
    ComponentState s = ComponentModel::saveState();
    s.internal[0] = m_currentA;
    return s;
}

void Inductor::restoreState(const ComponentState& state) noexcept {
    ComponentModel::restoreState(state);
    m_currentA = std::isfinite(state.internal[0]) ? state.internal[0] : 0.0;
    m_havePrev = false; // der naechste Schritt rechnet mit Rueckwaerts-Euler
}

} // namespace volta::core

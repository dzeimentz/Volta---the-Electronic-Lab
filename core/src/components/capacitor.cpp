#include "volta/core/components/capacitor.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace volta::core {

namespace {
constexpr double kMinCapacitanceF = 1e-12;
// Arbeitspunkt: Serienwiderstand tau_ref / C (mindestens kMinOhm). Proportional zu 1/C, damit parallel liegende,
// verschieden geladene Kondensatoren den ladungsgewichteten Mittelwert sum(C*U)/sum(C) annehmen.
constexpr double kDcTauRefS = 1e-5; // bis 10 mF oberhalb kMinOhm
} // namespace

Capacitor::Capacitor(std::string id, NodeId a, NodeId b, CapacitorParams params)
    : ComponentModel(std::move(id), {a, b}), m_params(params) {
    if (!(m_params.capacitanceF >= kMinCapacitanceF) || !std::isfinite(m_params.capacitanceF))
        m_params.capacitanceF = kMinCapacitanceF;
    if (!(m_params.leakOhm >= kMinOhm) || !std::isfinite(m_params.leakOhm)) m_params.leakOhm = 1e9;
    if (!(m_params.umaxV > 0.0) || !std::isfinite(m_params.umaxV)) m_params.umaxV = 1e9;
    if (!(m_params.reverseMaxV >= 0.0) || !std::isfinite(m_params.reverseMaxV)) m_params.reverseMaxV = 0.0;
    if (!(m_params.failAfterS >= 0.0) || !std::isfinite(m_params.failAfterS)) m_params.failAfterS = 0.2;
}

void Capacitor::setVoltage(double voltV) noexcept {
    m_voltV = std::isfinite(voltV) ? voltV : 0.0;
    m_havePrev = false;
}

void Capacitor::beginSolve(const StampContext& ctx) noexcept {
    // Volta 1 und 2 (Rueckwaerts-Euler) bleiben bitgleich: dort uebernimmt der Kondensator weiter die Klemmenspannung.
    m_extraOhm = 0.0;
    if (!ctx.secondOrder || ctx.isDc()) return;
    const IntegrationCoeffs k = integrationCoeffs(ctx, m_havePrev, m_lastDtS);
    const double idealOhm = ctx.dtS / ((k.secondOrder ? k.a0 : 1.0) * m_params.capacitanceF);
    if (idealOhm < kMinOhm) m_extraOhm = kMinOhm - idealOhm;
}

void Capacitor::stamp(MnaSystem& sys, const StampContext& ctx) const noexcept {
    if (m_destroyed) { // offen; Zweigstrom null
        sys.stampBranchOpen(firstBranch());
        return;
    }
    sys.stampConductance(terminal(0), terminal(1), 1.0 / m_params.leakOhm);
    const IntegrationCoeffs k = integrationCoeffs(ctx, m_havePrev, m_lastDtS);
    if (k.secondOrder) { // BDF2: U = (a1 u(n) - a2 u(n-1)) / a0 + dt / (a0 C) * I
        const double seriesOhm = std::max(ctx.dtS / (k.a0 * m_params.capacitanceF), kMinOhm);
        sys.stampVoltageBranch(terminal(0), terminal(1), firstBranch(), (k.a1 * m_voltV - k.a2 * m_prevVoltV) / k.a0,
                               seriesOhm);
        return;
    }
    const double tau = ctx.isDc() ? kDcTauRefS : ctx.dtS;
    const double seriesOhm = std::max(tau / m_params.capacitanceF, kMinOhm);
    sys.stampVoltageBranch(terminal(0), terminal(1), firstBranch(), m_voltV, seriesOhm);
}

bool Capacitor::stampAc(ComplexMna& sys, const AcContext& ctx) const noexcept {
    if (m_destroyed) {
        sys.stampBranchOpen(firstBranch());
        return true;
    }
    sys.stampAdmittance(terminal(0), terminal(1), 1.0 / m_params.leakOhm);
    // Zweig mit Impedanz 1 / (j w C); bei w = 0 offen (I = 0)
    if (ctx.omegaRadS > 0.0)
        sys.stampImpedanceBranch(terminal(0), terminal(1), firstBranch(), 0.0,
                                 Complex(0.0, -1.0 / (ctx.omegaRadS * m_params.capacitanceF)));
    else
        sys.stampBranchOpen(firstBranch());
    return true;
}

double Capacitor::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    double i = 0.0;
    if (!m_destroyed) {
        i = x.branchCurrent(firstBranch()) + (x.voltage(terminal(0)) - x.voltage(terminal(1))) / m_params.leakOhm;
    }
    return t == 0 ? i : -i;
}

void Capacitor::updateState(const SolutionView& x, double dtS) noexcept {
    m_lastDtS = dtS;
    m_havePrev = false;
    if (m_destroyed) return;
    double v = x.voltage(terminal(0)) - x.voltage(terminal(1));
    // Begrenzter Serienwiderstand: Die Ladung folgt nur dem Strom, nicht dem Spannungsabfall am Verlustanteil
    if (m_extraOhm > 0.0 && dtS > 0.0) v -= m_extraOhm * x.branchCurrent(firstBranch());
    if (!std::isfinite(v)) return;
    if (dtS > 0.0) { // Arbeitspunkt (dtS 0) beginnt die Vorgeschichte neu
        m_prevVoltV = m_voltV;
        m_havePrev = true;
    }
    m_voltV = v;
}

void Capacitor::checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept {
    if (isDestroyed()) return;
    const bool over = std::fabs(m_voltV) > m_params.umaxV;
    const bool reverse = m_params.polarized && m_voltV < -m_params.reverseMaxV;
    m_overloaded = over || reverse;
    m_violationS = m_overloaded ? m_violationS + m_lastDtS : 0.0;
    if (m_overloaded && m_violationS >= m_params.failAfterS - 1e-9) destroy(events, index, timeS);
}

ComponentState Capacitor::saveState() const noexcept {
    ComponentState s = ComponentModel::saveState();
    s.internal[0] = m_voltV;
    s.internal[1] = m_violationS;
    return s;
}

void Capacitor::restoreState(const ComponentState& state) noexcept {
    ComponentModel::restoreState(state);
    setVoltage(state.internal[0]); // ohne Vorgeschichte: der naechste Schritt rechnet mit Rueckwaerts-Euler
    m_violationS = std::isfinite(state.internal[1]) ? state.internal[1] : 0.0;
}

void Capacitor::setCapacitance(double capacitanceF) noexcept {
    if (capacitanceF > 0.0 && std::isfinite(capacitanceF)) m_params.capacitanceF = capacitanceF;
}

} // namespace volta::core

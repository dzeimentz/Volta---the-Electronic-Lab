#include "volta/core/components/lamp.hpp"

#include <cmath>
#include <utility>

namespace volta::core {

Lamp::Lamp(std::string id, NodeId a, NodeId b, LampParams params)
    : ComponentModel(std::move(id), {a, b}), m_params(params) {
    const LampParams def;
    auto fix = [](double& v, double fallback, double minimum) {
        if (!std::isfinite(v) || v < minimum) v = fallback;
    };
    fix(m_params.uNomV, def.uNomV, 0.1);
    fix(m_params.iNomA, def.iNomA, 1e-4);
    if (!std::isfinite(m_params.coldRatio) || m_params.coldRatio <= 0.0 || m_params.coldRatio >= 1.0)
        m_params.coldRatio = def.coldRatio;
    fix(m_params.alpha, def.alpha, 0.5);
    fix(m_params.filamentTauS, def.filamentTauS, 1e-3);
    fix(m_params.wearVoltageFactor, def.wearVoltageFactor, 1.0);
    fix(m_params.wearFailS, def.wearFailS, 0.0);
    fix(m_params.burstVoltageFactor, def.burstVoltageFactor, m_params.wearVoltageFactor);

    m_coldOhm = hotResistanceOhm() * m_params.coldRatio;
    // Nenntemperatur aus R_heiss / R_kalt = (T/T0)^alpha; Rth aus Nennleistung
    m_nominalK = kAmbientK * std::pow(1.0 / m_params.coldRatio, 1.0 / m_params.alpha);
    const double pNom = m_params.uNomV * m_params.iNomA;
    const double t0 = kAmbientK;
    const double tn = m_nominalK;
    m_radiationWPerK4 = pNom / (tn * tn * tn * tn - t0 * t0 * t0 * t0);
    // Zeitkonstante der linearisierten Waermeabgabe bei Nenntemperatur: tau = Cth / (4 k Tn^3)
    m_cthJPerK = m_params.filamentTauS * 4.0 * m_radiationWPerK4 * tn * tn * tn;
    m_burstK = steadyTempK(m_params.burstVoltageFactor * m_params.uNomV);
    m_wearK = steadyTempK(m_params.wearVoltageFactor * m_params.uNomV);
    m_g = 1.0 / resistanceOhm();
}

void Lamp::beginSolve(const StampContext& /*ctx*/) noexcept { m_g = 1.0 / resistanceOhm(); }

double Lamp::resistanceAt(double tempK) const noexcept {
    if (!(tempK > kAmbientK)) return m_coldOhm;
    return m_coldOhm * std::pow(tempK / kAmbientK, m_params.alpha);
}

double Lamp::resistanceOhm() const noexcept { return resistanceAt(m_tempK); }

double Lamp::steadyTempK(double uV) const noexcept {
    // k * (T^4 - T0^4) = u^2 / R(T); die Differenz steigt monoton in T -> Bisektion
    const double t0 = kAmbientK;
    const double t04 = t0 * t0 * t0 * t0;
    double lo = t0;
    double hi = std::pow(uV * uV / (m_coldOhm * m_radiationWPerK4) + t04, 0.25);
    for (int k = 0; k < 100; ++k) {
        const double mid = 0.5 * (lo + hi);
        const double f = m_radiationWPerK4 * (mid * mid * mid * mid - t04) - uV * uV / resistanceAt(mid);
        (f < 0.0 ? lo : hi) = mid;
    }
    return 0.5 * (lo + hi);
}

void Lamp::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    sys.stampConductance(terminal(0), terminal(1), conductance());
}

double Lamp::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    const double i = (x.voltage(terminal(0)) - x.voltage(terminal(1))) * conductance();
    return t == 0 ? i : -i;
}

void Lamp::thermal(const SolutionView& x, double dtS) noexcept {
    m_lastDtS = dtS;
    const double v = x.voltage(terminal(0)) - x.voltage(terminal(1));
    const double p = m_destroyed ? 0.0 : v * v * conductance();
    if (!(dtS > 0.0) || !std::isfinite(p)) return;
    // linearisiert-implizites Euler-Verfahren fuer Cth*dT/dt = P - k*(T^4 - T0^4): stabil fuer jede Schrittweite
    const double t = m_tempK;
    const double t0 = kAmbientK;
    const double k = m_radiationWPerK4;
    const double f = p - k * (t * t * t * t - t0 * t0 * t0 * t0);
    const double next = t + dtS * f / (m_cthJPerK + dtS * 4.0 * k * t * t * t);
    m_tempK = std::isfinite(next) && next > t0 ? next : t0;
}

void Lamp::checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept {
    if (isDestroyed()) return;
    m_overloaded = m_tempK > m_wearK;
    m_wearS = m_overloaded ? m_wearS + m_lastDtS : 0.0;
    if (m_overloaded && m_wearS >= m_params.wearFailS - 1e-9) destroy(events, index, timeS);
    if (m_tempK > m_burstK) destroy(events, index, timeS);
}

ComponentReading Lamp::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    r.level = m_destroyed ? 0.0 : r.powerW / (m_params.uNomV * m_params.iNomA);
    return r;
}

ComponentState Lamp::saveState() const noexcept {
    ComponentState s = ComponentModel::saveState();
    s.internal[0] = m_wearS;
    return s;
}

void Lamp::restoreState(const ComponentState& state) noexcept {
    ComponentModel::restoreState(state);
    m_wearS = std::isfinite(state.internal[0]) ? state.internal[0] : 0.0;
}

} // namespace volta::core

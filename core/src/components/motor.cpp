#include "volta/core/components/motor.hpp"

#include <cmath>
#include <utility>

namespace volta::core {

namespace {
constexpr double kPi = 3.14159265358979323846;
} // namespace

Motor::Motor(std::string id, NodeId a, NodeId b, MotorParams params)
    : ComponentModel(std::move(id), {a, b}), m_params(params) {
    const MotorParams def;
    auto fix = [](double& v, double fallback, double minimum) {
        if (!std::isfinite(v) || v < minimum) v = fallback;
    };
    fix(m_params.raOhm, def.raOhm, kMinOhm);
    fix(m_params.kVs, def.kVs, 1e-6);
    fix(m_params.inertiaKgm2, def.inertiaKgm2, 1e-12);
    fix(m_params.viscousNms, def.viscousNms, 0.0);
    fix(m_params.frictionNm, def.frictionNm, 0.0);
    fix(m_params.uNomV, def.uNomV, 0.1);
    fix(m_params.overloadA, def.overloadA, 0.0);
}

double Motor::noLoadOmegaRadS() const noexcept {
    const MotorParams& p = m_params;
    const double w = (p.uNomV - p.raOhm * p.frictionNm / p.kVs) / (p.kVs + p.raOhm * p.viscousNms / p.kVs);
    return w > 0.0 ? w : p.uNomV / p.kVs;
}

double Motor::rpm() const noexcept { return m_omega * 60.0 / (2.0 * kPi); }

void Motor::setBlocked(bool blocked) noexcept {
    m_blocked = blocked;
    if (blocked) m_omega = 0.0;
}

int Motor::motionSign() const noexcept {
    if (m_blocked) return 0;
    if (m_omega > 0.0) return 1;
    if (m_omega < 0.0) return -1;
    // Stillstand: anlaufen nur, wenn das Moment des letzten Stroms die Haftreibung uebersteigt
    const double torque = m_params.kVs * m_lastCurrentA;
    if (std::fabs(torque) <= m_params.frictionNm) return 0;
    return torque > 0.0 ? 1 : -1;
}

void Motor::stamp(MnaSystem& sys, const StampContext& ctx) const noexcept {
    if (m_destroyed) { // Wicklung durchgebrannt: offen
        sys.stampBranchOpen(firstBranch());
        return;
    }
    const int s = motionSign();
    double seriesOhm = m_params.raOhm;
    double emfV = 0.0;
    if (s != 0) {
        const double k = m_params.kVs;
        if (ctx.isDc()) {
            emfV = k * m_omega;
        } else {
            const double denom = m_params.inertiaKgm2 + m_params.viscousNms * ctx.dtS;
            seriesOhm += k * k * ctx.dtS / denom;
            emfV = k * (m_params.inertiaKgm2 * m_omega - ctx.dtS * m_params.frictionNm * s) / denom;
        }
    }
    sys.stampVoltageBranch(terminal(0), terminal(1), firstBranch(), emfV, seriesOhm);
}

double Motor::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    const double i = m_destroyed ? 0.0 : x.branchCurrent(firstBranch());
    return t == 0 ? i : -i;
}

void Motor::updateState(const SolutionView& x, double dtS) noexcept {
    const double i = m_destroyed ? 0.0 : x.branchCurrent(firstBranch());
    const int s = motionSign(); // Modus, mit dem gestempelt wurde
    m_lastCurrentA = std::isfinite(i) ? i : 0.0;
    if (s == 0 || m_destroyed) {
        m_omega = 0.0;
        return;
    }
    // gleiche Rechnung wie im Stempel: implizites Euler mit Reibung in Bewegungsrichtung
    const double denom = m_params.inertiaKgm2 + m_params.viscousNms * dtS;
    double omega =
        (m_params.inertiaKgm2 * m_omega + dtS * (m_params.kVs * m_lastCurrentA - m_params.frictionNm * s)) / denom;
    // Vorzeichenwechsel durch Reibung: Motor bleibt stehen (Haftreibung)
    if (omega * s < 0.0) omega = 0.0;
    m_omega = std::isfinite(omega) ? omega : 0.0;
}

void Motor::thermal(const SolutionView& /*x*/, double dtS) noexcept {
    integrateThermal(m_lastCurrentA * m_lastCurrentA * m_params.raOhm, dtS, m_params.thermal);
}

void Motor::checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept {
    if (m_destroyed) return;
    m_overloaded = std::fabs(m_lastCurrentA) > m_params.overloadA;
    checkThermalLimit(m_params.thermal, events, index, timeS);
    if (m_destroyed) m_omega = 0.0;
}

ComponentReading Motor::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    r.level = m_omega / noLoadOmegaRadS();
    return r;
}

ComponentState Motor::saveState() const noexcept {
    ComponentState s = ComponentModel::saveState();
    s.internal[0] = m_omega;
    s.internal[1] = m_lastCurrentA;
    return s;
}

void Motor::restoreState(const ComponentState& state) noexcept {
    ComponentModel::restoreState(state);
    m_omega = std::isfinite(state.internal[0]) && !m_blocked ? state.internal[0] : 0.0;
    m_lastCurrentA = std::isfinite(state.internal[1]) ? state.internal[1] : 0.0;
}

} // namespace volta::core

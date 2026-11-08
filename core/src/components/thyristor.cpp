#include "volta/core/components/thyristor.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include "pn_junction.hpp"

namespace volta::core {

namespace {
constexpr double kBoltzmannJPerK = 1.380649e-23;
constexpr double kElementaryChargeC = 1.602176634e-19;
constexpr double kEventMarginS = 1e-7; // Teilschritt endet knapp hinter dem Kipppunkt (wie Timer555)

// Bruchteil des Schritts, an dem f linear interpoliert von <= 0 nach > 0 wechselt; 2 = kein Wechsel
double crossing(double fa, double fn) noexcept {
    if (!(fa <= 0.0 && fn > 0.0)) return 2.0;
    return fa / (fa - fn);
}

ThyristorParams sanitized(ThyristorParams p) noexcept {
    const ThyristorParams def;
    auto fix = [](double& v, double fallback, double minimum) {
        if (!std::isfinite(v) || v < minimum) v = fallback;
    };
    fix(p.onV, def.onV, 0.0);
    fix(p.onOhm, def.onOhm, 1e-3);
    fix(p.offSiemens, def.offSiemens, 0.0);
    fix(p.holdA, def.holdA, 1e-6);
    fix(p.igtA, def.igtA, 1e-9);
    fix(p.gateIsA, def.gateIsA, 1e-30);
    fix(p.gateShuntOhm, def.gateShuntOhm, 1.0);
    fix(p.breakoverV, def.breakoverV, 0.0);
    fix(p.iMaxA, def.iMaxA, 1e-6);
    fix(p.overloadFactor, def.overloadFactor, 1.0);
    fix(p.overloadFailS, def.overloadFailS, 0.0);
    if (p.kind == ThyristorKind::Diac && !(p.breakoverV > p.onV)) p.breakoverV = p.onV + 1.0;
    return p;
}
} // namespace

Thyristor::Thyristor(std::string id, NodeId a, NodeId b, NodeId gate, ThyristorParams params)
    : ComponentModel(std::move(id), {a, b, gate}), m_params(sanitized(params)) {
    m_vt = kBoltzmannJPerK * kAmbientK / kElementaryChargeC;
    m_vcrit = m_vt * std::log(m_vt / (std::sqrt(2.0) * m_params.gateIsA));
    if (!std::isfinite(m_vcrit) || m_vcrit < m_vt) m_vcrit = m_vt;
    m_xMax = pn::linearFromArg(m_params.gateIsA, m_vt, kMaxConductance / 4.0);
    m_vTrig = m_vt * std::log(m_params.igtA / m_params.gateIsA + 1.0);
    m_vLatch = m_params.onV + m_params.holdA * m_params.onOhm;
    evaluateGate(0.0);
}

Thyristor::Thyristor(std::string id, NodeId a, NodeId b, ThyristorParams params)
    : ComponentModel(std::move(id), {a, b}), m_params(sanitized(params)) {
    m_params.kind = ThyristorKind::Diac;
    m_params = sanitized(m_params);
    m_vt = kBoltzmannJPerK * kAmbientK / kElementaryChargeC;
    m_vLatch = m_params.onV + m_params.holdA * m_params.onOhm;
}

void Thyristor::gateJunction(double vg, double& i, double& g) const noexcept {
    const double is = m_params.gateIsA;
    double ep = 0.0, dp = 0.0;
    pn::safeExp(vg / m_vt, m_xMax, ep, dp);
    i = is * (ep - 1.0) + kJunctionGmin * vg;
    g = is * dp / m_vt + kJunctionGmin;
    if (m_params.kind == ThyristorKind::Triac) { // zweiter Uebergang MT1 -> Gate
        double en = 0.0, dn = 0.0;
        pn::safeExp(-vg / m_vt, m_xMax, en, dn);
        i -= is * (en - 1.0);
        g += is * dn / m_vt;
    }
}

void Thyristor::evaluateGate(double vg) noexcept {
    m_vg0 = vg;
    gateJunction(vg, m_ig0, m_gg);
}

bool Thyristor::linearize(const SolutionView& iterate) noexcept {
    if (m_destroyed || !hasGate()) return false;
    bool limited = false;
    const double v = vGate(iterate);
    double vNew = v;
    if (v >= 0.0 || m_params.kind != ThyristorKind::Triac) {
        vNew = pn::pnjlim(v, m_vg0, m_vt, m_vcrit, limited);
    } else {
        vNew = -pn::pnjlim(-v, -m_vg0, m_vt, m_vcrit, limited);
    }
    evaluateGate(vNew);
    return limited;
}

void Thyristor::resetLinearization(const SolutionView& accepted) noexcept {
    if (m_destroyed || !hasGate()) return;
    evaluateGate(vGate(accepted));
}

double Thyristor::mainCurrent(double v, bool on, double s) const noexcept {
    double i = m_params.offSiemens * v;
    if (on) i += (v - s * m_params.onV) / m_params.onOhm;
    return i;
}

void Thyristor::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    if (m_destroyed) return; // offen; aeussere Knoten haben GMIN
    const NodeId a = terminal(0), b = terminal(1);
    double g = m_params.offSiemens;
    if (m_on) {
        g += 1.0 / m_params.onOhm;
        sys.stampCurrentSource(a, b, -m_sign * m_params.onV / m_params.onOhm);
    }
    sys.stampConductance(a, b, g);
    if (!hasGate()) return;
    const NodeId gate = terminal(2);
    sys.stampConductance(gate, b, m_gg + 1.0 / m_params.gateShuntOhm);
    sys.stampCurrentSource(gate, b, m_ig0 - m_gg * m_vg0);
}

double Thyristor::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    if (m_destroyed || t >= terminalCount()) return 0.0;
    const double iMain = mainCurrent(vMain(x), m_on, m_sign);
    double iGate = 0.0;
    if (hasGate()) {
        const double vg = vGate(x);
        iGate = m_ig0 + m_gg * (vg - m_vg0) + vg / m_params.gateShuntOhm;
    }
    if (t == 0) return iMain;
    if (t == 2) return iGate;
    return -(iMain + iGate);
}

double Thyristor::fireMargin(const SolutionView& x, double s) const noexcept {
    const double v = s * vMain(x);
    if (!hasGate()) return v - m_params.breakoverV;
    const double vg = m_params.kind == ThyristorKind::Triac ? std::fabs(vGate(x)) : vGate(x);
    return std::min(vg - m_vTrig, v - m_vLatch);
}

double Thyristor::holdMargin(const SolutionView& x) const noexcept {
    return m_vLatch - m_sign * vMain(x);
}

double Thyristor::eventFraction(const SolutionView& a, const SolutionView& n, double dtS) const noexcept {
    if (m_destroyed || !(dtS > 0.0)) return 1.0;
    double theta = 2.0;
    if (m_on) {
        theta = crossing(holdMargin(a), holdMargin(n));
    } else {
        theta = crossing(fireMargin(a, 1.0), fireMargin(n, 1.0));
        if (m_params.kind != ThyristorKind::Scr)
            theta = std::min(theta, crossing(fireMargin(a, -1.0), fireMargin(n, -1.0)));
    }
    if (theta > 1.0) return 1.0;
    return std::min(1.0, theta + kEventMarginS / dtS);
}

void Thyristor::updateState(const SolutionView& x, double /*dtS*/) noexcept {
    if (m_destroyed) {
        m_on = false;
        return;
    }
    if (m_on && holdMargin(x) > 0.0) m_on = false;
    if (!m_on) {
        if (fireMargin(x, 1.0) > 0.0) {
            m_on = true;
            m_sign = 1.0;
        } else if (m_params.kind != ThyristorKind::Scr && fireMargin(x, -1.0) > 0.0) {
            m_on = true;
            m_sign = -1.0;
        }
    }
}

void Thyristor::thermal(const SolutionView& x, double dtS) noexcept {
    m_lastDtS = dtS;
    m_lastIA = std::fabs(terminalCurrent(x, 0));
    double p = 0.0;
    for (std::size_t t = 0; t < terminalCount(); ++t) p += x.voltage(terminal(t)) * terminalCurrent(x, t);
    integrateThermal(m_destroyed ? 0.0 : p, dtS, m_params.thermal);
}

void Thyristor::checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept {
    if (isDestroyed()) return;
    m_overloaded = m_lastIA > m_params.iMaxA;
    const bool severe = m_lastIA > m_params.overloadFactor * m_params.iMaxA;
    m_overloadS = severe ? m_overloadS + m_lastDtS : 0.0;
    if (severe && m_overloadS >= m_params.overloadFailS - 1e-9) destroy(events, index, timeS);
    checkThermalLimit(m_params.thermal, events, index, timeS);
}

ComponentReading Thyristor::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    r.level = (m_on && !isDestroyed()) ? 1.0 : 0.0; // gezuendet
    return r;
}

ComponentState Thyristor::saveState() const noexcept {
    ComponentState s = ComponentModel::saveState();
    s.internal[0] = m_on ? 1.0 : 0.0;
    s.internal[1] = m_sign;
    s.internal[2] = m_overloadS;
    return s;
}

void Thyristor::restoreState(const ComponentState& state) noexcept {
    ComponentModel::restoreState(state);
    m_on = state.internal[0] > 0.5 && !state.destroyed;
    m_sign = state.internal[1] < 0.0 ? -1.0 : 1.0;
    m_overloadS = std::isfinite(state.internal[2]) ? state.internal[2] : 0.0;
}

} // namespace volta::core

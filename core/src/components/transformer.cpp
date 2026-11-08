#include "volta/core/components/transformer.hpp"

#include <cmath>
#include <utility>

#include "volta/core/complex_mna.hpp"
#include "volta/core/mna_system.hpp"

namespace volta::core {

Transformer::Transformer(std::string id, NodeId a1, NodeId e1, NodeId a2, NodeId e2, TransformerParams params)
    : ComponentModel(std::move(id), {a1, e1, a2, e2}), m_params(params) {
    const TransformerParams def;
    auto fix = [](double& v, double lo, double d) {
        if (!(v >= lo) || !std::isfinite(v)) v = d;
    };
    fix(m_params.inductance1H, 1e-9, def.inductance1H);
    fix(m_params.inductance2H, 1e-9, def.inductance2H);
    fix(m_params.winding1Ohm, kMinOhm, kMinOhm);
    fix(m_params.winding2Ohm, kMinOhm, kMinOhm);
    fix(m_params.parallelOhm, kMinOhm, def.parallelOhm);
    if (!(m_params.coupling >= 0.0) || !(m_params.coupling < 1.0)) m_params.coupling = def.coupling;
    if (!(m_params.imaxA > 0.0) || !std::isfinite(m_params.imaxA)) m_params.imaxA = def.imaxA;
}

double Transformer::mutualH() const noexcept {
    return m_params.coupling * std::sqrt(m_params.inductance1H * m_params.inductance2H);
}

void Transformer::beginSolve(const StampContext& ctx) noexcept {
    const double r1 = m_params.winding1Ohm, r2 = m_params.winding2Ohm;
    if (ctx.isDc()) { // nur Wicklungswiderstaende
        m_y[0][0] = 1.0 / r1;
        m_y[1][1] = 1.0 / r2;
        m_y[0][1] = m_y[1][0] = 0.0;
        m_j[0] = m_j[1] = 0.0;
        return;
    }
    const IntegrationCoeffs k = integrationCoeffs(ctx, m_havePrev, m_lastDtS);
    const double l1 = m_params.inductance1H, l2 = m_params.inductance2H, m = mutualH();
    const double c = k.a0 / ctx.dtS;
    const double z11 = r1 + c * l1, z22 = r2 + c * l2, z12 = c * m;
    const double det = z11 * z22 - z12 * z12;
    m_y[0][0] = z22 / det;
    m_y[1][1] = z11 / det;
    m_y[0][1] = m_y[1][0] = -z12 / det;
    // h = -(1/dt) L (a1 i(n) - a2 i(n-1))
    const double q1 = k.a1 * m_i[0] - k.a2 * m_prev[0], q2 = k.a1 * m_i[1] - k.a2 * m_prev[1];
    const double h1 = -(l1 * q1 + m * q2) / ctx.dtS, h2 = -(m * q1 + l2 * q2) / ctx.dtS;
    m_j[0] = -(m_y[0][0] * h1 + m_y[0][1] * h2);
    m_j[1] = -(m_y[1][0] * h1 + m_y[1][1] * h2);
}

void Transformer::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    const double gp = 1.0 / m_params.parallelOhm;
    sys.stampConductance(terminal(0), terminal(1), gp);
    sys.stampConductance(terminal(2), terminal(3), gp);
    if (m_destroyed) return;
    // Zweitor: Strom i_w fliesst am Anfang der Wicklung w hinein und am Ende heraus
    for (int w = 0; w < 2; ++w) {
        const NodeId a = terminal(2 * w), e = terminal(2 * w + 1);
        for (int v = 0; v < 2; ++v) {
            const NodeId av = terminal(2 * v), ev = terminal(2 * v + 1);
            const double y = m_y[w][v];
            sys.stampNodeMatrix(a, av, y);
            sys.stampNodeMatrix(a, ev, -y);
            sys.stampNodeMatrix(e, av, -y);
            sys.stampNodeMatrix(e, ev, y);
        }
        sys.stampCurrentSource(a, e, m_j[w]);
    }
}

bool Transformer::stampAc(ComplexMna& sys, const AcContext& ctx) const noexcept {
    const double gp = 1.0 / m_params.parallelOhm;
    sys.stampAdmittance(terminal(0), terminal(1), gp);
    sys.stampAdmittance(terminal(2), terminal(3), gp);
    if (m_destroyed) return true;
    const Complex jw(0.0, ctx.omegaRadS);
    const Complex z11 = m_params.winding1Ohm + jw * m_params.inductance1H;
    const Complex z22 = m_params.winding2Ohm + jw * m_params.inductance2H;
    const Complex z12 = jw * mutualH();
    const Complex det = z11 * z22 - z12 * z12;
    const Complex y[2][2] = {{z22 / det, -z12 / det}, {-z12 / det, z11 / det}};
    auto& a = sys.matrix();
    const std::size_t n = sys.size();
    auto add = [&](NodeId r, NodeId c, Complex v) {
        if (r != kGround && c != kGround) a[(r - 1) * n + (c - 1)] += v;
    };
    for (int w = 0; w < 2; ++w)
        for (int v = 0; v < 2; ++v) {
            const NodeId aw = terminal(2 * w), ew = terminal(2 * w + 1), av = terminal(2 * v), ev = terminal(2 * v + 1);
            add(aw, av, y[w][v]);
            add(aw, ev, -y[w][v]);
            add(ew, av, -y[w][v]);
            add(ew, ev, y[w][v]);
        }
    return true;
}

double Transformer::windingVolt(const SolutionView& x, int w) const noexcept {
    return x.voltage(terminal(2 * w)) - x.voltage(terminal(2 * w + 1));
}

double Transformer::windingCurrent(const SolutionView& x, int w) const noexcept {
    if (m_destroyed) return 0.0;
    return m_y[w][0] * windingVolt(x, 0) + m_y[w][1] * windingVolt(x, 1) + m_j[w];
}

double Transformer::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    const int w = static_cast<int>(t / 2);
    const double i = windingVolt(x, w) / m_params.parallelOhm + windingCurrent(x, w);
    return t % 2 == 0 ? i : -i;
}

void Transformer::updateState(const SolutionView& x, double dtS) noexcept {
    m_lastDtS = dtS;
    m_havePrev = false;
    if (m_destroyed) {
        m_i[0] = m_i[1] = 0.0;
        return;
    }
    const double i0 = windingCurrent(x, 0), i1 = windingCurrent(x, 1);
    if (!std::isfinite(i0) || !std::isfinite(i1)) return;
    if (dtS > 0.0) {
        m_prev[0] = m_i[0];
        m_prev[1] = m_i[1];
        m_havePrev = true;
    }
    m_i[0] = i0;
    m_i[1] = i1;
}

void Transformer::thermal(const SolutionView& /*x*/, double dtS) noexcept {
    const double p = m_destroyed ? 0.0
                                 : m_i[0] * m_i[0] * m_params.winding1Ohm + m_i[1] * m_i[1] * m_params.winding2Ohm;
    integrateThermal(p, dtS, m_params.thermal);
}

void Transformer::checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept {
    if (isDestroyed()) return;
    m_overloaded = std::fabs(m_i[0]) > m_params.imaxA || std::fabs(m_i[1]) > m_params.imaxA;
    checkThermalLimit(m_params.thermal, events, index, timeS);
}

ComponentReading Transformer::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x); // Wicklung 1: Spannung und Strom
    return r;
}

ComponentState Transformer::saveState() const noexcept {
    ComponentState s = ComponentModel::saveState();
    s.internal[0] = m_i[0];
    s.internal[1] = m_i[1];
    return s;
}

void Transformer::restoreState(const ComponentState& state) noexcept {
    ComponentModel::restoreState(state);
    m_i[0] = std::isfinite(state.internal[0]) ? state.internal[0] : 0.0;
    m_i[1] = std::isfinite(state.internal[1]) ? state.internal[1] : 0.0;
    m_havePrev = false;
}

} // namespace volta::core

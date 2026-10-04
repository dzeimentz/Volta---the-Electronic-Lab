#include "volta/core/circuit.hpp"

#include <cmath>
#include <utility>

namespace volta::core {

namespace {
constexpr double kGmin = 1e-9;      // Ableitleitwert je Knoten gegen Masse
constexpr double kPivotEps = 1e-18; // Schwelle fuer singulaere Matrix
} // namespace

NodeId Circuit::addNode() { return m_nodeCount++; }

void Circuit::addResistor(ResistorSpec spec) { m_resistors.push_back(std::move(spec)); }

void Circuit::addVoltageSource(VoltageSourceSpec spec) { m_sources.push_back(std::move(spec)); }

DcSolution solveDc(const Circuit& circuit) noexcept {
    DcSolution result;
    const std::size_t nNodes = circuit.nodeCount() - 1; // ohne Masse
    const std::size_t nSrc = circuit.sources().size();
    const std::size_t n = nNodes + nSrc;
    if (n == 0) {
        result.ok = true;
        result.nodeVolt.assign(1, 0.0);
        return result;
    }

    std::vector<double> g(n * n, 0.0);
    std::vector<double> b(n, 0.0);
    auto at = [&](std::size_t r, std::size_t c) -> double& { return g[r * n + c]; };
    auto idx = [](NodeId node) { return node - 1; };

    for (std::size_t i = 0; i < nNodes; ++i) at(i, i) += kGmin;

    auto stampConductance = [&](NodeId a, NodeId c, double gval) {
        if (a != kGround) at(idx(a), idx(a)) += gval;
        if (c != kGround) at(idx(c), idx(c)) += gval;
        if (a != kGround && c != kGround) {
            at(idx(a), idx(c)) -= gval;
            at(idx(c), idx(a)) -= gval;
        }
    };

    for (const auto& r : circuit.resistors()) {
        if (r.ohm <= 0.0) continue;
        stampConductance(r.a, r.b, 1.0 / r.ohm);
    }

    for (std::size_t k = 0; k < nSrc; ++k) {
        const auto& s = circuit.sources()[k];
        const std::size_t row = nNodes + k;
        if (s.plus != kGround) { at(idx(s.plus), row) += 1.0; at(row, idx(s.plus)) += 1.0; }
        if (s.minus != kGround) { at(idx(s.minus), row) -= 1.0; at(row, idx(s.minus)) -= 1.0; }
        at(row, row) -= s.innerOhm; // Innenwiderstand: V+ - V- - Ri*I = U
        b[row] = s.volt;
    }

    // LU mit Teilpivotsuche (Gauss-Elimination), dicht.
    for (std::size_t col = 0; col < n; ++col) {
        std::size_t piv = col;
        double best = std::fabs(at(col, col));
        for (std::size_t r = col + 1; r < n; ++r) {
            const double v = std::fabs(at(r, col));
            if (v > best) { best = v; piv = r; }
        }
        if (best < kPivotEps) return result; // ok = false
        if (piv != col) {
            for (std::size_t c = 0; c < n; ++c) std::swap(at(col, c), at(piv, c));
            std::swap(b[col], b[piv]);
        }
        for (std::size_t r = col + 1; r < n; ++r) {
            const double f = at(r, col) / at(col, col);
            if (f == 0.0) continue;
            for (std::size_t c = col; c < n; ++c) at(r, c) -= f * at(col, c);
            b[r] -= f * b[col];
        }
    }
    std::vector<double> x(n, 0.0);
    for (std::size_t i = n; i-- > 0;) {
        double sum = b[i];
        for (std::size_t c = i + 1; c < n; ++c) sum -= at(i, c) * x[c];
        x[i] = sum / at(i, i);
        if (!std::isfinite(x[i])) return result;
    }

    result.nodeVolt.assign(circuit.nodeCount(), 0.0);
    for (std::size_t i = 0; i < nNodes; ++i) result.nodeVolt[i + 1] = x[i];
    result.sourceCurrent.assign(x.begin() + static_cast<std::ptrdiff_t>(nNodes), x.end());
    result.ok = true;
    return result;
}

} // namespace volta::core

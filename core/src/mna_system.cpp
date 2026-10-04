#include "volta/core/mna_system.hpp"

#include <algorithm>
#include <cmath>

namespace volta::core {

void MnaSystem::resize(std::size_t nodeCount, std::size_t branchCount) {
    m_nodeUnknowns = nodeCount > 0 ? nodeCount - 1 : 0;
    m_size = m_nodeUnknowns + branchCount;
    m_g.assign(m_size * m_size, 0.0);
    m_b.assign(m_size, 0.0);
}

void MnaSystem::clear() noexcept {
    std::fill(m_g.begin(), m_g.end(), 0.0);
    std::fill(m_b.begin(), m_b.end(), 0.0);
}

void MnaSystem::stampConductance(NodeId a, NodeId c, double g) noexcept {
    if (!std::isfinite(g)) return; // nicht endlich: Zweig wirkt offen
    g = std::clamp(g, -kMaxConductance, kMaxConductance);
    if (a != kGround) at(row(a), row(a)) += g;
    if (c != kGround) at(row(c), row(c)) += g;
    if (a != kGround && c != kGround) {
        at(row(a), row(c)) -= g;
        at(row(c), row(a)) -= g;
    }
}

void MnaSystem::stampCurrentSource(NodeId from, NodeId to, double i) noexcept {
    if (from != kGround) m_b[row(from)] -= i;
    if (to != kGround) m_b[row(to)] += i;
}

void MnaSystem::stampVoltageBranch(NodeId plus, NodeId minus, std::size_t branch, double volt,
                                   double seriesOhm) noexcept {
    const std::size_t k = branchRow(branch);
    if (plus != kGround) {
        at(row(plus), k) += 1.0; // Zweigstrom verlaesst den Knoten plus in den Zweig
        at(k, row(plus)) += 1.0;
    }
    if (minus != kGround) {
        at(row(minus), k) -= 1.0;
        at(k, row(minus)) -= 1.0;
    }
    at(k, k) -= seriesOhm;
    m_b[k] += volt;
}

void MnaSystem::stampBranchOpen(std::size_t branch) noexcept {
    const std::size_t k = branchRow(branch);
    at(k, k) += 1.0;
}

void MnaSystem::stampGmin(std::size_t nodeCount, double g) noexcept {
    const std::size_t n = nodeCount > 0 ? std::min(nodeCount - 1, m_nodeUnknowns) : 0;
    for (std::size_t i = 0; i < n; ++i) at(i, i) += g;
}

} // namespace volta::core

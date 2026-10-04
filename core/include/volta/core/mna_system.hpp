#pragma once

#include <cstddef>
#include <vector>

#include "volta/core/types.hpp"

namespace volta::core {

// Gleichungssystem der modifizierten Knotenpotentialanalyse: G * x = b.
// Unbekannte x = [Potentiale der Knoten 1..N-1, Zweigstroeme 0..M-1]. Masse (Knoten 0) ist keine Unbekannte.
// Speicher wird nur in resize() angefordert (Topologiewechsel); clear() und alle Stempel sind allokationsfrei.
class MnaSystem {
public:
    // Legt das System fuer nodeCount Knoten (inkl. Masse) und branchCount Zweigstroeme an.
    void resize(std::size_t nodeCount, std::size_t branchCount);

    std::size_t size() const noexcept { return m_size; }
    std::size_t nodeUnknowns() const noexcept { return m_nodeUnknowns; }
    std::size_t branchCount() const noexcept { return m_size - m_nodeUnknowns; }

    // Setzt Matrix und rechte Seite auf null, ohne Speicher freizugeben.
    void clear() noexcept;

    // Leitwert g (S) zwischen den Knoten a und c. Begrenzt auf +-kMaxConductance; nicht endlich -> offen.
    void stampConductance(NodeId a, NodeId c, double g) noexcept;
    // Stromquelle: Strom i (A) fliesst durch die Quelle von Knoten from nach Knoten to,
    // d. h. er verlaesst den Knoten from und wird in den Knoten to eingespeist.
    void stampCurrentSource(NodeId from, NodeId to, double i) noexcept;
    // Spannungszweig mit Serienwiderstand: V(plus) - V(minus) - seriesOhm * I = volt.
    // I ist der Zweigstrom, der am Anschluss plus in den Zweig hineinfliesst.
    void stampVoltageBranch(NodeId plus, NodeId minus, std::size_t branch, double volt,
                            double seriesOhm) noexcept;
    // Zweig ohne Strom (z. B. zerstoerter Kondensator): I = 0.
    void stampBranchOpen(std::size_t branch) noexcept;
    // Leitwert g von den Knoten 1..nodeCount-1 gegen Masse (GMIN fuer die aeusseren Knoten).
    void stampGmin(std::size_t nodeCount, double g) noexcept;

    const std::vector<double>& matrix() const noexcept { return m_g; }
    const std::vector<double>& rhs() const noexcept { return m_b; }

private:
    double& at(std::size_t r, std::size_t c) noexcept { return m_g[r * m_size + c]; }
    static std::size_t row(NodeId node) noexcept { return node - 1; }
    std::size_t branchRow(std::size_t branch) const noexcept { return m_nodeUnknowns + branch; }

    std::size_t m_size = 0;
    std::size_t m_nodeUnknowns = 0;
    std::vector<double> m_g; // zeilenweise, m_size * m_size
    std::vector<double> m_b;
};

} // namespace volta::core

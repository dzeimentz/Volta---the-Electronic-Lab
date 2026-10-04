#pragma once

#include <cstddef>
#include <vector>

namespace volta::core {

// Dichte LU-Zerlegung mit Teilpivotsuche (Volta 1: < 100 Knoten).
// Zerlegung und Loesung sind getrennt, damit eine Zerlegung fuer mehrere rechte Seiten genutzt werden kann
// (Vorleistung fuer Volta 2). Speicher nur in resize(); factor() und solve() sind allokationsfrei.
class DenseLu {
public:
    void resize(std::size_t n);
    std::size_t size() const noexcept { return m_n; }

    // Zerlegt die zeilenweise gespeicherte n*n-Matrix a (wird kopiert). false bei (numerisch) singulaerer Matrix.
    bool factor(const std::vector<double>& a) noexcept;
    // Loest L*U*x = P*b mit der letzten Zerlegung. false, wenn keine gueltige Zerlegung vorliegt
    // oder das Ergebnis nicht endlich ist.
    bool solve(const std::vector<double>& b, std::vector<double>& x) const noexcept;

    bool valid() const noexcept { return m_valid; }

private:
    std::size_t m_n = 0;
    std::vector<double> m_lu;
    std::vector<std::size_t> m_perm;
    bool m_valid = false;
};

} // namespace volta::core

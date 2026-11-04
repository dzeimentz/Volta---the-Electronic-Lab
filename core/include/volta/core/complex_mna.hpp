#pragma once

#include <complex>
#include <cstddef>
#include <vector>

#include "volta/core/types.hpp"

namespace volta::core {

using Complex = std::complex<double>;

// Komplexes Gleichungssystem der Wechselstromanalyse (Volta 3, Konzept Kap. 4): dieselbe Anordnung der Unbekannten
// wie MnaSystem (Knoten 1..N-1, dann Zweigstroeme), Eintraege komplex. Speicher nur in resize().
class ComplexMna {
public:
    void resize(std::size_t nodeCount, std::size_t branchCount);
    std::size_t size() const noexcept { return m_size; }
    std::size_t nodeUnknowns() const noexcept { return m_nodeUnknowns; }

    void clear() noexcept;
    // Uebernimmt die reelle Matrix eines MnaSystem gleicher Groesse (Kleinsignal-Leitwerte am Arbeitspunkt).
    void addReal(const std::vector<double>& matrix) noexcept;

    // Admittanz y zwischen den Knoten a und c.
    void stampAdmittance(NodeId a, NodeId c, Complex y) noexcept;
    // Zweig mit Impedanz: V(plus) - V(minus) - z * I = v; I fliesst am Anschluss plus in den Zweig.
    void stampImpedanceBranch(NodeId plus, NodeId minus, std::size_t branch, Complex v, Complex z) noexcept;
    // Zweig ohne Strom: I = 0.
    void stampBranchOpen(std::size_t branch) noexcept;

    std::vector<Complex>& matrix() noexcept { return m_a; }
    std::vector<Complex>& rhs() noexcept { return m_b; }

private:
    Complex& at(std::size_t r, std::size_t c) noexcept { return m_a[r * m_size + c]; }
    std::size_t branchRow(std::size_t branch) const noexcept { return m_nodeUnknowns + branch; }

    std::size_t m_size = 0;
    std::size_t m_nodeUnknowns = 0;
    std::vector<Complex> m_a;
    std::vector<Complex> m_b;
};

// Kontext eines Wechselstrom-Stempels.
struct AcContext {
    double omegaRadS = 0.0; // Kreisfrequenz 2 pi f
    bool excite = false;    // dieses Bauteil ist die Anregung der Analyse (Quelle mit 1 V)
};

// Dichte komplexe LU-Zerlegung mit Teilpivotsuche (Wechselstromanalyse, nicht im Echtzeitpfad).
// Loest a * x = b; a wird ueberschrieben. false bei singulaerer Matrix.
bool solveComplexDense(std::vector<Complex>& a, std::vector<Complex>& b, std::size_t n) noexcept;

} // namespace volta::core

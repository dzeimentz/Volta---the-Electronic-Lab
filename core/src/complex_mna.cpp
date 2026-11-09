#include "volta/core/complex_mna.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace volta::core {

void ComplexMna::resize(std::size_t nodeCount, std::size_t branchCount) {
    m_nodeUnknowns = nodeCount > 0 ? nodeCount - 1 : 0;
    m_size = m_nodeUnknowns + branchCount;
    m_a.assign(m_size * m_size, Complex{});
    m_b.assign(m_size, Complex{});
}

void ComplexMna::clear() noexcept {
    std::fill(m_a.begin(), m_a.end(), Complex{});
    std::fill(m_b.begin(), m_b.end(), Complex{});
}

void ComplexMna::addReal(const std::vector<double>& matrix) noexcept {
    const std::size_t n = std::min(matrix.size(), m_a.size());
    for (std::size_t i = 0; i < n; ++i) m_a[i] += matrix[i];
}

void ComplexMna::stampAdmittance(NodeId a, NodeId c, Complex y) noexcept {
    if (!std::isfinite(y.real()) || !std::isfinite(y.imag())) return;
    if (a != kGround) at(a - 1, a - 1) += y;
    if (c != kGround) at(c - 1, c - 1) += y;
    if (a != kGround && c != kGround) {
        at(a - 1, c - 1) -= y;
        at(c - 1, a - 1) -= y;
    }
}

void ComplexMna::stampImpedanceBranch(NodeId plus, NodeId minus, std::size_t branch, Complex v, Complex z) noexcept {
    const std::size_t k = branchRow(branch);
    if (plus != kGround) {
        at(plus - 1, k) += 1.0;
        at(k, plus - 1) += 1.0;
    }
    if (minus != kGround) {
        at(minus - 1, k) -= 1.0;
        at(k, minus - 1) -= 1.0;
    }
    at(k, k) -= z;
    m_b[k] += v;
}

void ComplexMna::stampBranchOpen(std::size_t branch) noexcept {
    const std::size_t k = branchRow(branch);
    at(k, k) += 1.0;
}

bool solveComplexDense(std::vector<Complex>& a, std::vector<Complex>& b, std::size_t n) noexcept {
    // Schwelle je Spalte: ein offenes Feld haengt nur ueber kGmin (1 nS) an 0V, waehrend ein Kondensator bei 1 Hz
    // Impedanzen von vielen MOhm in die Matrix bringt. Eine Schwelle relativ zum groessten Eintrag der ganzen Matrix
    // hielte das offene Feld faelschlich fuer singulaer; gemessen wird daher an der eigenen Spalte.
    std::vector<double> colMax(n, 0.0);
    for (std::size_t r = 0; r < n; ++r)
        for (std::size_t c = 0; c < n; ++c) colMax[c] = std::max(colMax[c], std::abs(a[r * n + c]));
    for (std::size_t k = 0; k < n; ++k) {
        const double tiny = colMax[k] * 1e-13;
        std::size_t piv = k;
        double best = std::abs(a[k * n + k]);
        for (std::size_t r = k + 1; r < n; ++r) {
            const double v = std::abs(a[r * n + k]);
            if (v > best) {
                best = v;
                piv = r;
            }
        }
        if (!(best > tiny) || best == 0.0) return false;
        if (piv != k) {
            for (std::size_t c = 0; c < n; ++c) std::swap(a[k * n + c], a[piv * n + c]);
            std::swap(b[k], b[piv]);
        }
        const Complex inv = 1.0 / a[k * n + k];
        for (std::size_t r = k + 1; r < n; ++r) {
            const Complex f = a[r * n + k] * inv;
            if (f == Complex{}) continue;
            a[r * n + k] = Complex{};
            for (std::size_t c = k + 1; c < n; ++c) a[r * n + c] -= f * a[k * n + c];
            b[r] -= f * b[k];
        }
    }
    for (std::size_t k = n; k-- > 0;) {
        Complex s = b[k];
        for (std::size_t c = k + 1; c < n; ++c) s -= a[k * n + c] * b[c];
        b[k] = s / a[k * n + k];
        if (!std::isfinite(b[k].real()) || !std::isfinite(b[k].imag())) return false;
    }
    return true;
}

} // namespace volta::core

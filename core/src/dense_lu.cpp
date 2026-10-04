#include "volta/core/dense_lu.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace volta::core {

namespace {
constexpr double kPivotRel = 1e-14; // Pivot relativ zum betragsgroessten Matrixelement
} // namespace

void DenseLu::resize(std::size_t n) {
    m_n = n;
    m_lu.assign(n * n, 0.0);
    m_perm.assign(n, 0);
    m_valid = false;
}

bool DenseLu::factor(const std::vector<double>& a) noexcept {
    m_valid = false;
    const std::size_t n = m_n;
    if (a.size() != n * n) return false;
    std::copy(a.begin(), a.end(), m_lu.begin());
    for (std::size_t i = 0; i < n; ++i) m_perm[i] = i;

    double scale = 0.0;
    for (double v : m_lu) {
        if (!std::isfinite(v)) return false;
        scale = std::max(scale, std::fabs(v));
    }
    const double pivotMin = std::max(scale * kPivotRel, 1e-300);

    auto at = [&](std::size_t r, std::size_t c) -> double& { return m_lu[r * n + c]; };
    for (std::size_t col = 0; col < n; ++col) {
        std::size_t piv = col;
        double best = std::fabs(at(col, col));
        for (std::size_t r = col + 1; r < n; ++r) {
            const double v = std::fabs(at(r, col));
            if (v > best) {
                best = v;
                piv = r;
            }
        }
        if (best < pivotMin) return false;
        if (piv != col) {
            for (std::size_t c = 0; c < n; ++c) std::swap(at(col, c), at(piv, c));
            std::swap(m_perm[col], m_perm[piv]);
        }
        const double d = at(col, col);
        for (std::size_t r = col + 1; r < n; ++r) {
            const double f = at(r, col) / d;
            at(r, col) = f; // L unterhalb der Diagonale, Einsen implizit
            if (f == 0.0) continue;
            for (std::size_t c = col + 1; c < n; ++c) at(r, c) -= f * at(col, c);
        }
    }
    m_valid = true;
    return true;
}

bool DenseLu::solve(const std::vector<double>& b, std::vector<double>& x) const noexcept {
    const std::size_t n = m_n;
    if (!m_valid || b.size() != n || x.size() != n) return false;
    auto at = [&](std::size_t r, std::size_t c) { return m_lu[r * n + c]; };
    // Vorwaertseinsetzen L*y = P*b (y in x)
    for (std::size_t i = 0; i < n; ++i) {
        double sum = b[m_perm[i]];
        for (std::size_t c = 0; c < i; ++c) sum -= at(i, c) * x[c];
        x[i] = sum;
    }
    // Rueckwaertseinsetzen U*x = y
    for (std::size_t i = n; i-- > 0;) {
        double sum = x[i];
        for (std::size_t c = i + 1; c < n; ++c) sum -= at(i, c) * x[c];
        x[i] = sum / at(i, i);
        if (!std::isfinite(x[i])) return false;
    }
    return true;
}

} // namespace volta::core

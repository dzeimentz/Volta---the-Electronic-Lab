#include "volta/core/signal_stats.hpp"

#include <algorithm>
#include <cmath>

namespace volta::core {

namespace {
constexpr double kHysteresis = 0.05; // Anteil der Spannweite des vorigen Fensters
} // namespace

void SignalWindow::reset(std::size_t channels, double windowS) {
    m_windowS = windowS > 0.0 ? windowS : 0.1;
    m_elapsedS = 0.0;
    m_acc.assign(channels, Acc{});
    m_latest.assign(channels, SignalSummary{});
}

void SignalWindow::add(const double* values, std::size_t count, double dtS) noexcept {
    if (!(dtS > 0.0)) return;
    const std::size_t n = std::min(count, m_acc.size());
    const double t = m_elapsedS + dtS; // Zeitpunkt dieses Werts im Fenster
    for (std::size_t i = 0; i < n; ++i) {
        const double v = std::isfinite(values[i]) ? values[i] : 0.0;
        Acc& a = m_acc[i];
        const bool first = m_elapsedS == 0.0;
        a.sum += v * dtS;
        a.sumSq += v * v * dtS;
        a.min = first ? v : std::min(a.min, v);
        a.max = first ? v : std::max(a.max, v);
        // Durchgaenge durch den Mittelwert des vorigen Fensters
        const SignalSummary& last = m_latest[i];
        if (last.valid) {
            const double h = kHysteresis * (last.max - last.min);
            if (h > 1e-6) {
                if (v < last.mean - h) a.armed = true;
                if (a.armed && v > last.mean + h && a.havePrev) {
                    a.armed = false;
                    const double span = v - a.prev;
                    const double frac = span > 0.0 ? std::clamp((last.mean - a.prev) / span, 0.0, 1.0) : 1.0;
                    const double at = m_elapsedS + frac * dtS;
                    if (a.rises == 0) a.firstRiseS = at;
                    a.lastRiseS = at;
                    ++a.rises;
                }
            }
        }
        a.prev = v;
        a.havePrev = true;
    }
    m_elapsedS = t;
    if (m_elapsedS + 1e-12 < m_windowS) return;
    // Fenster abschliessen
    for (std::size_t i = 0; i < m_acc.size(); ++i) {
        Acc& a = m_acc[i];
        SignalSummary& s = m_latest[i];
        s.valid = true;
        s.mean = a.sum / m_elapsedS;
        s.rms = std::sqrt(std::max(0.0, a.sumSq / m_elapsedS));
        s.acRms = std::sqrt(std::max(0.0, s.rms * s.rms - s.mean * s.mean));
        s.min = a.min;
        s.max = a.max;
        s.frequencyHz = a.rises >= 2 && a.lastRiseS > a.firstRiseS ? (a.rises - 1) / (a.lastRiseS - a.firstRiseS) : 0.0;
        // Durchgangszustand (armed, prev) bleibt ueber die Fenstergrenze erhalten
        a.sum = a.sumSq = 0.0;
        a.rises = 0;
    }
    m_elapsedS = 0.0;
}

} // namespace volta::core

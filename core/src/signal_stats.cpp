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
                if (v < last.mean - h) {
                    a.armed = true;
                    a.haveCross = false;
                }
                // Zeitpunkt des Durchgangs durch den Mittelwert selbst; gezaehlt wird er erst, wenn die Spannung danach
                // ueber Mittelwert + Hysterese steigt. (Der Zeitpunkt der Bestaetigung wandert bei einer abklingenden
                // Schwingung und verfaelschte deren Frequenz um rund 5 %.)
                if (a.armed && a.havePrev && a.prev < last.mean && v >= last.mean) {
                    a.crossS = m_elapsedS + (last.mean - a.prev) / (v - a.prev) * dtS;
                    a.haveCross = true;
                }
                if (a.armed && a.haveCross && v > last.mean + h) {
                    a.armed = false;
                    if (a.rises > 0 && a.intervalCount < static_cast<int>(a.intervals.size()))
                        a.intervals[static_cast<std::size_t>(a.intervalCount++)] = a.crossS - a.lastRiseS;
                    if (a.rises == 0) a.firstRiseS = a.crossS;
                    a.lastRiseS = a.crossS;
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
        // Median der Abstaende: Pausen zwischen Schwingungszuegen (Rechteck stoesst einen Schwingkreis an) zaehlen
        // nicht mit; bei gleichmaessigen Signalen gleich dem Mittel
        if (a.rises >= 2 && a.intervalCount == a.rises - 1) {
            std::array<double, 64> sorted = a.intervals;
            const auto n = static_cast<std::size_t>(a.intervalCount);
            std::sort(sorted.begin(), sorted.begin() + static_cast<std::ptrdiff_t>(n));
            const double med = n % 2 == 1 ? sorted[n / 2] : 0.5 * (sorted[n / 2 - 1] + sorted[n / 2]);
            // Mittel der Abstaende nahe am Median (gleicht die Rasterung der Schritte aus)
            double sum = 0.0;
            int k = 0;
            for (std::size_t j = 0; j < n; ++j)
                if (sorted[j] > 0.75 * med && sorted[j] < 1.25 * med) {
                    sum += sorted[j];
                    ++k;
                }
            if (k > 0 && sum > 0.0) s.frequencyHz = k / sum;
        }
        // Durchgangszustand (armed, prev) bleibt ueber die Fenstergrenze erhalten
        a.sum = a.sumSq = 0.0;
        a.rises = 0;
        a.intervalCount = 0;
        a.crossS -= m_elapsedS; // Durchgang kurz vor der Fenstergrenze, Bestaetigung im naechsten Fenster
    }
    m_elapsedS = 0.0;
}

} // namespace volta::core

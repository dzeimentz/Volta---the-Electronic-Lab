#include "oscilloscope.hpp"

#include <algorithm>
#include <cmath>

namespace volta::model {

namespace {
constexpr double kMinStepS = 20e-6; // feinste Schrittweite der Simulation (wie die hoerbare Simulation)
constexpr double kMaxStepS = 1e-3;  // Standard-Schrittweite
constexpr int kStepsPerDiv = 25;
constexpr double kPreTriggerDiv = 1.0; // Triggerpunkt eine Teilung vom linken Rand
constexpr double kHysteresis = 0.1;    // Frequenzmessung: Anteil der Spannweite
} // namespace

const std::vector<double>& scopeTimebases() {
    static const std::vector<double> t = {100e-6, 200e-6, 500e-6, 1e-3, 2e-3, 5e-3, 10e-3, 20e-3, 50e-3,
                                          0.1,    0.2,    0.5,    1.0,  2.0,  5.0,  10.0, 20.0, 60.0};
    return t;
}

const std::vector<double>& scopeVoltsPerDiv() {
    static const std::vector<double> v = {0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1.0, 2.0, 5.0};
    return v;
}

double ScopeSettings::timebaseS() const {
    const auto& t = scopeTimebases();
    const double base = t[static_cast<std::size_t>(std::clamp(timebaseIndex, 0, static_cast<int>(t.size()) - 1))];
    return stretch > 0 ? base / stretchFactor() : base;
}

int ScopeSettings::stretchFactor() const {
    static constexpr int f[] = {1, 10, 100, 1000};
    return f[std::clamp(stretch, 0, 3)];
}

double ScopeSettings::simStepS() const {
    // mit Lupe darf die Simulation um den Dehnungsfaktor feiner rechnen (sie laeuft dann ebenso viel langsamer)
    return std::clamp(timebaseS() / kStepsPerDiv, kMinStepS / stretchFactor(), kMaxStepS);
}

ScopeMeasure scopeMeasure(const std::vector<ScopeSample>& samples, std::size_t channel, double fromS, double toS) {
    ScopeMeasure m;
    double sum = 0.0;
    int n = 0;
    for (const auto& s : samples) {
        if (s.t < fromS || s.t > toS) continue;
        const double v = s.v[channel];
        m.min = n == 0 ? v : std::min(m.min, v);
        m.max = n == 0 ? v : std::max(m.max, v);
        sum += v;
        ++n;
    }
    if (n == 0) return m;
    m.any = true;
    m.mean = sum / n;
    const double span = m.max - m.min;
    if (span < 1e-3) return m; // Gleichspannung
    // Schwelle: Mittelwert, solange er mit Hysterese innerhalb der Spannweite liegt (sonst die Mitte zwischen Minimum
    // und Maximum, z. B. bei schmalen Pulsen). Bei einer abklingenden Schwingung liegt der Mittelwert auf ihrer
    // Nulllinie, die Mitte dagegen um den halben Unterschied der ersten Ausschlaege daneben.
    const double h = kHysteresis * span;
    const double mid = m.mean - h > m.min && m.mean + h < m.max ? m.mean : 0.5 * (m.max + m.min);
    // Gezaehlt wird der Zeitpunkt, an dem die Spannung die Schwelle selbst ueberschreitet; bestaetigt erst, wenn sie
    // danach ueber Schwelle + Hysterese steigt (nicht der Zeitpunkt der Bestaetigung: der wandert mit kleiner
    // werdender Schwingung und verfaelschte die Frequenz einer gedaempften Schwingung um rund 5 %).
    bool armed = false, havePrev = false, haveCross = false;
    double prevT = 0.0, prevV = 0.0, first = 0.0, last = 0.0, cross = 0.0;
    int rises = 0;
    std::vector<double> intervals;
    for (const auto& s : samples) {
        if (s.t < fromS || s.t > toS) continue;
        const double v = s.v[channel];
        if (v < mid - h) {
            armed = true;
            haveCross = false;
        }
        if (armed && havePrev && prevV < mid && v >= mid) {
            cross = prevT + (mid - prevV) / (v - prevV) * (s.t - prevT);
            haveCross = true;
        }
        if (armed && haveCross && v > mid + h) {
            armed = false;
            if (rises > 0) intervals.push_back(cross - last);
            if (rises == 0) first = cross;
            last = cross;
            ++rises;
        }
        prevT = s.t;
        prevV = v;
        havePrev = true;
    }
    if (rises >= 2 && last > first) {
        // Median der Abstaende: Pausen zwischen Schwingungszuegen zaehlen nicht mit (wie core::SignalWindow)
        std::sort(intervals.begin(), intervals.end());
        const std::size_t n = intervals.size();
        const double med = n % 2 == 1 ? intervals[n / 2] : 0.5 * (intervals[n / 2 - 1] + intervals[n / 2]);
        // Mittel der Abstaende nahe am Median (gleicht die Rasterung der Abtastpunkte aus)
        double sum = 0.0;
        int k = 0;
        for (double d : intervals)
            if (d > 0.75 * med && d < 1.25 * med) {
                sum += d;
                ++k;
            }
        m.frequencyHz = k > 0 && sum > 0.0 ? k / sum : (rises - 1) / (last - first);
    }
    return m;
}

void scopeAutoChannel(ScopeSettings& s, std::size_t channel, const ScopeMeasure& m) {
    if (!m.any || channel >= kScopeChannels) return;
    auto& ch = s.channels[channel];
    ch.position = m.min >= 0.0 ? -3 : m.max <= 0.0 ? 3 : 0;
    const double up = (kScopeDivY / 2 - ch.position) * 0.9, down = (kScopeDivY / 2 + ch.position) * 0.9;
    const auto& vpd = scopeVoltsPerDiv();
    ch.voltsIndex = static_cast<int>(vpd.size()) - 1;
    for (std::size_t i = 0; i < vpd.size(); ++i)
        if (m.max <= up * vpd[i] && -m.min <= down * vpd[i]) {
            ch.voltsIndex = static_cast<int>(i);
            break;
        }
}

int scopeAutoTimebase(double frequencyHz) {
    if (frequencyHz <= 0.0) return -1;
    const auto& tb = scopeTimebases();
    for (int i = 0; i <= kScopeLastNormalTimebase; ++i)
        if (tb[static_cast<std::size_t>(i)] * kScopeDivX * frequencyHz >= 2.0) return i;
    return kScopeLastNormalTimebase;
}

void scopeAutoTrigger(ScopeSettings& s, std::size_t channel, const ScopeMeasure& m) {
    if (!m.any || channel >= kScopeChannels) return;
    s.triggerChannel = static_cast<int>(channel);
    s.triggerRising = true;
    const double mid = 0.5 * (m.min + m.max);
    // auf 0,1 V wie die Anzeige; bei kleinen Signalen genauer, damit der Pegel zwischen Minimum und Maximum bleibt
    s.triggerLevelV = std::clamp(m.max - m.min >= 0.5 ? std::round(mid * 10.0) / 10.0 : std::round(mid * 1000.0) / 1000.0,
                                 -20.0, 20.0);
}

ScopeFrame scopeFrame(const std::vector<ScopeSample>& samples, const ScopeSettings& settings,
                      const std::array<bool, kScopeChannels>& has) {
    ScopeFrame f;
    if (samples.empty()) return f;
    const double tb = settings.timebaseS();
    const double width = settings.screenS();
    const double newest = samples.back().t, oldest = samples.front().t;
    f.startS = newest - width; // frei laufend bzw. Rollbetrieb: das Neueste am rechten Rand
    const std::size_t tc = static_cast<std::size_t>(std::clamp(settings.triggerChannel, 0, 1));
    if (!settings.roll && !settings.xy && has[tc]) {
        // juengster Durchgang durch den Triggerpegel, nach dem noch ein ganzes Bild aufgezeichnet ist
        const double pre = kPreTriggerDiv * tb, level = settings.triggerLevelV;
        for (std::size_t i = samples.size() - 1; i > 0; --i) {
            const double a = samples[i - 1].v[tc], b = samples[i].v[tc];
            const bool cross = settings.triggerRising ? (a < level && b >= level) : (a > level && b <= level);
            if (!cross) continue;
            const double at = samples[i - 1].t + (level - a) / (b - a) * (samples[i].t - samples[i - 1].t);
            if (at + width - pre > newest) continue; // Bild noch nicht vollstaendig
            if (at - pre < oldest) break;           // zu alt fuer den Puffer
            f.triggered = true;
            f.startS = at - pre;
            break;
        }
    }
    const double end = f.startS + width;
    for (std::size_t c = 0; c < kScopeChannels; ++c) {
        if (!has[c]) continue;
        f.measure[c] = scopeMeasure(samples, c, f.startS, end);
        if (settings.xy) continue;
        auto& tr = f.traces[c];
        tr.reserve(kScopePointsPerScreen + 2);
        for (const auto& s : samples)
            if (s.t >= f.startS && s.t <= end) tr.emplace_back((s.t - f.startS) / tb, s.v[c]);
    }
    if (settings.xy && has[0] && has[1])
        for (const auto& s : samples)
            if (s.t >= f.startS && s.t <= end) f.xy.emplace_back(s.v[0], s.v[1]);
    return f;
}

} // namespace volta::model

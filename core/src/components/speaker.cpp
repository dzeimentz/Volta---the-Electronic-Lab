#include "volta/core/components/speaker.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace volta::core {

namespace {
constexpr double kMeanTauS = 0.3;     // Gleichanteil
constexpr double kPeakTauS = 0.15;    // Abklingen der Huellkurve
constexpr double kRmsTauS = 0.1;      // Effektivwert
constexpr double kMinSwingA = 1e-3;   // kleinster Wechselstrom, der als Ton/Impuls zaehlt
constexpr double kHysteresis = 0.25;  // Schwelle relativ zur Huellkurve
constexpr double kSilenceS = 1.5;     // so lange ohne Nulldurchgang (mindestens) -> Stille
constexpr double kPeriodSmooth = 0.3; // Glaettung der Periodendauer

double decay(double dtS, double tauS) noexcept { return std::exp(-dtS / tauS); }
} // namespace

Speaker::Speaker(std::string id, NodeId a, NodeId b, SpeakerParams params)
    : ComponentModel(std::move(id), {a, b}), m_params(params) {
    if (!(m_params.ohm > kMinOhm)) m_params.ohm = 8.0;
    if (!(m_params.fullScaleA > 0.0)) m_params.fullScaleA = 0.1;
}

void Speaker::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    sys.stampConductance(terminal(0), terminal(1), conductance());
}

double Speaker::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    const double i = current(x);
    return t == 0 ? i : -i;
}

void Speaker::resetTracking() noexcept {
    m_meanA = m_peakA = m_msA2 = 0.0;
    m_high = false;
    m_lastEdgeS = -1.0;
    m_periodS = m_freqHz = 0.0;
    m_started = false;
}

void Speaker::updateState(const SolutionView& x, double dtS) noexcept {
    const double i = current(x);
    if (dtS <= 0.0) { // Arbeitspunkt: Gleichanteil uebernehmen (kein Knacken beim Laden einer Schaltung)
        m_meanA = i;
        m_started = true;
        return;
    }
    if (!m_started) {
        m_meanA = i;
        m_started = true;
    }
    m_timeS += dtS;
    const double ac = i - m_meanA;
    m_meanA += (i - m_meanA) * (1.0 - decay(dtS, kMeanTauS));
    m_peakA = std::max(std::fabs(ac), m_peakA * decay(dtS, kPeakTauS));
    m_msA2 += (ac * ac - m_msA2) * (1.0 - decay(dtS, kRmsTauS));

    const double threshold = std::max(kHysteresis * m_peakA, kMinSwingA);
    if (!m_high && ac > threshold) {
        m_high = true;
        m_pulses += 1.0;
        if (m_lastEdgeS >= 0.0) {
            const double period = m_timeS - m_lastEdgeS;
            m_periodS = m_periodS > 0.0 ? m_periodS + (period - m_periodS) * kPeriodSmooth : period;
            m_freqHz = m_periodS > 0.0 ? 1.0 / m_periodS : 0.0;
        }
        m_lastEdgeS = m_timeS;
    } else if (m_high && ac < 0.0) {
        m_high = false;
    }
    // Stille: drei Perioden (mindestens kSilenceS) ohne steigenden Nulldurchgang
    if (m_lastEdgeS >= 0.0 && m_timeS - m_lastEdgeS > std::max(kSilenceS, 3.0 * m_periodS)) {
        m_freqHz = 0.0;
        m_periodS = 0.0;
        m_lastEdgeS = -1.0;
    }
    if (m_pulses > 1e9) m_pulses = 0.0; // Zaehler bleibt endlich (die Anzeige wertet nur Aenderungen aus)
}

void Speaker::thermal(const SolutionView& x, double dtS) noexcept {
    const double i = current(x);
    m_lastPowerW = m_destroyed ? 0.0 : i * i * m_params.ohm;
    integrateThermal(m_lastPowerW, dtS, m_params.thermal);
}

void Speaker::checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept {
    m_overloaded = !isDestroyed() && m_lastPowerW > m_params.pmaxW;
    checkThermalLimit(m_params.thermal, events, index, timeS);
}

ComponentReading Speaker::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    const double rms = std::sqrt(std::max(0.0, m_msA2));
    r.level = m_destroyed ? 0.0 : std::min(1.0, rms / m_params.fullScaleA);
    r.frequencyHz = m_destroyed ? 0.0 : m_freqHz;
    r.displayValue = m_pulses;
    return r;
}

void Speaker::restoreState(const ComponentState& state) noexcept {
    ComponentModel::restoreState(state);
    resetTracking();
}

} // namespace volta::core

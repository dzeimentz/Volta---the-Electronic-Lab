#include "volta/core/components/sensors.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace volta::core {

namespace {
double sane(double v, double fallback) noexcept { return std::isfinite(v) && v > 0.0 ? v : fallback; }
} // namespace

// ------------------------------------------------------------------ Fotowiderstand

Photoresistor::Photoresistor(std::string id, NodeId a, NodeId b, PhotoresistorParams params, double lux)
    : ComponentModel(std::move(id), {a, b}), m_params(params) {
    m_params.r10Ohm = sane(m_params.r10Ohm, 10000.0);
    m_params.gamma = sane(m_params.gamma, 0.7);
    m_params.darkOhm = sane(m_params.darkOhm, 1.0e6);
    m_params.riseTauS = sane(m_params.riseTauS, 0.03);
    m_params.fallTauS = sane(m_params.fallTauS, 0.1);
    setIlluminance(lux);
    m_lux = m_targetLux;
    m_g = conductanceFor(m_lux);
}

void Photoresistor::setIlluminance(double lux) noexcept {
    m_targetLux = std::isfinite(lux) ? std::clamp(lux, 0.0, 200000.0) : 0.0;
}

double Photoresistor::conductanceFor(double lux) const noexcept {
    // Dunkelleitwert plus Fotoleitwert; ohne Licht bleibt nur der Dunkelwiderstand
    double g = 1.0 / m_params.darkOhm;
    if (lux > 0.0) g += std::pow(lux / 10.0, m_params.gamma) / m_params.r10Ohm;
    return std::min(g, 1.0 / kMinOhm);
}

void Photoresistor::beginSolve(const StampContext& /*ctx*/) noexcept { m_g = m_destroyed ? kGmin : conductanceFor(m_lux); }

void Photoresistor::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    sys.stampConductance(terminal(0), terminal(1), m_g);
}

double Photoresistor::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    const double i = (x.voltage(terminal(0)) - x.voltage(terminal(1))) * m_g;
    return t == 0 ? i : -i;
}

void Photoresistor::updateState(const SolutionView& /*x*/, double dtS) noexcept {
    if (dtS <= 0.0) { // Arbeitspunkt: sofort eingeschwungen
        m_lux = m_targetLux;
        return;
    }
    const double tau = m_targetLux > m_lux ? m_params.riseTauS : m_params.fallTauS;
    m_lux = m_targetLux + (m_lux - m_targetLux) * std::exp(-dtS / tau);
}

void Photoresistor::thermal(const SolutionView& x, double dtS) noexcept {
    const double v = x.voltage(terminal(0)) - x.voltage(terminal(1));
    m_lastPowerW = v * v * m_g;
    integrateThermal(m_lastPowerW, dtS, m_params.thermal);
}

void Photoresistor::checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept {
    m_overloaded = !isDestroyed() && m_lastPowerW > m_params.pmaxW;
    checkThermalLimit(m_params.thermal, events, index, timeS);
}

ComponentReading Photoresistor::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    // level: Helligkeit am Bauteil 0..1 (logarithmisch, 1 lx .. 10000 lx) fuer die Darstellung
    r.level = std::clamp(std::log10(std::max(m_lux, 1.0)) / 4.0, 0.0, 1.0);
    r.displayValue = 1.0 / m_g; // aktueller Widerstand in Ohm
    return r;
}

ComponentState Photoresistor::saveState() const noexcept {
    ComponentState s = ComponentModel::saveState();
    s.internal[0] = m_lux;
    return s;
}

void Photoresistor::restoreState(const ComponentState& state) noexcept {
    ComponentModel::restoreState(state);
    if (std::isfinite(state.internal[0]) && state.internal[0] >= 0.0) m_lux = state.internal[0];
}

// ------------------------------------------------------------------ Heissleiter

Thermistor::Thermistor(std::string id, NodeId a, NodeId b, ThermistorParams params, double ambientK)
    : ComponentModel(std::move(id), {a, b}), m_params(params) {
    m_params.r25Ohm = sane(m_params.r25Ohm, 10000.0);
    m_params.betaK = sane(m_params.betaK, 3950.0);
    setAmbient(ambientK);
    m_tempK = m_ambientK;
    m_g = 1.0 / resistanceAt(m_tempK);
}

void Thermistor::setAmbient(double kelvin) noexcept {
    m_ambientK = std::isfinite(kelvin) ? std::clamp(kelvin, 233.15, 473.15) : kAmbientK;
}

double Thermistor::resistanceAt(double kelvin) const noexcept {
    const double t = std::clamp(kelvin, 200.0, 600.0);
    return std::max(m_params.r25Ohm * std::exp(m_params.betaK * (1.0 / t - 1.0 / 298.15)), kMinOhm);
}

void Thermistor::beginSolve(const StampContext& /*ctx*/) noexcept {
    m_g = m_destroyed ? kGmin : 1.0 / resistanceAt(m_tempK);
}

void Thermistor::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    sys.stampConductance(terminal(0), terminal(1), m_g);
}

double Thermistor::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    const double i = (x.voltage(terminal(0)) - x.voltage(terminal(1))) * m_g;
    return t == 0 ? i : -i;
}

void Thermistor::updateState(const SolutionView& /*x*/, double dtS) noexcept {
    if (dtS <= 0.0 && m_tempK < m_ambientK - 50.0) m_tempK = m_ambientK; // unsinniger Startwert
}

void Thermistor::thermal(const SolutionView& x, double dtS) noexcept {
    const double v = x.voltage(terminal(0)) - x.voltage(terminal(1));
    m_lastPowerW = v * v * m_g;
    integrateThermal(m_lastPowerW, dtS, m_params.thermal, m_ambientK);
}

void Thermistor::checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept {
    m_overloaded = !isDestroyed() && m_lastPowerW > m_params.pmaxW;
    checkThermalLimit(m_params.thermal, events, index, timeS);
}

ComponentReading Thermistor::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    // level: Erwaermung ueber Raumtemperatur 0..1 (bis +60 K) fuer die Darstellung
    r.level = std::clamp((m_tempK - kAmbientK) / 60.0, 0.0, 1.0);
    r.displayValue = 1.0 / m_g; // aktueller Widerstand in Ohm
    return r;
}

void Thermistor::restoreState(const ComponentState& state) noexcept {
    ComponentModel::restoreState(state);
    m_g = 1.0 / resistanceAt(m_tempK);
}

} // namespace volta::core

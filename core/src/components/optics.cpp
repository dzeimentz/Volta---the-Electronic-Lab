#include "volta/core/components/optics.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace volta::core {

// ------------------------------------------------------------------ Fototransistor

Phototransistor::Phototransistor(std::string id, NodeId collector, NodeId emitter, PhototransistorParams params,
                                 double lux)
    : ComponentModel(std::move(id), {collector, emitter}), m_params(params) {
    if (!(m_params.kneeV > 0.0)) m_params.kneeV = 0.15;
    if (!(m_params.earlyV > 0.0)) m_params.earlyV = 50.0;
    if (!(m_params.imaxA > 0.0)) m_params.imaxA = 0.02;
    m_params.inverseGain = std::clamp(m_params.inverseGain, 0.0, 1.0);
    setIlluminance(lux);
}

void Phototransistor::setIlluminance(double lux) noexcept {
    m_lux = std::isfinite(lux) ? std::clamp(lux, 0.0, 200000.0) : 0.0;
}

void Phototransistor::setShade(double share) noexcept {
    m_shade = std::isfinite(share) ? std::clamp(share, 0.0, 1.0) : 1.0;
}

bool Phototransistor::addEmitter(const Diode* emitter, double coupling) noexcept {
    if (!emitter || m_emitterCount >= kMaxEmitters || !std::isfinite(coupling) || coupling <= 0.0) return false;
    m_emitters[m_emitterCount] = emitter;
    m_coupling[m_emitterCount] = coupling;
    ++m_emitterCount;
    return true;
}

void Phototransistor::beginSolve(const StampContext& /*ctx*/) noexcept {
    // Lichtstrom aus Beleuchtung und dem Strom der IR-LEDs im letzten angenommenen Schritt (zerstoerte LED strahlt
    // nicht; der Strom einer zerstoerten LED ist ohnehin null)
    double ir = 0.0;
    for (std::size_t k = 0; k < m_emitterCount; ++k)
        if (!m_emitters[k]->isDestroyed()) ir += m_coupling[k] * m_emitters[k]->currentA();
    const double light = m_params.darkA + m_params.luxSensitivityA * m_lux + m_shade * ir;
    m_lightA = m_destroyed ? 0.0 : std::min(light, 1.0); // nie mehr als 1 A (Robustheit)
}

void Phototransistor::evaluate(double vceV) noexcept {
    const double il = m_lightA;
    const double vk = m_params.kneeV;
    double i = 0.0, g = 0.0;
    // tanh-Kennlinie: vorwaerts Knick bei kneeV, verkehrt herum bei inverseGain * kneeV; beide Aeste haben bei
    // Uce = 0 dieselbe Steigung IL / kneeV (ohne Knick pendelt Newton nicht um null)
    if (vceV >= 0.0) {
        const double t = std::tanh(vceV / vk);
        const double early = 1.0 + vceV / m_params.earlyV;
        i = il * t * early;
        g = il * ((1.0 - t * t) / vk * early + t / m_params.earlyV);
    } else if (m_params.inverseGain > 0.0) { // verkehrt herum: kleiner Strom in Gegenrichtung, ebenfalls begrenzt
        const double vr = vk * m_params.inverseGain;
        const double t = std::tanh(vceV / vr);
        i = m_params.inverseGain * il * t;
        g = il * (1.0 - t * t) / vk;
    }
    m_vLin = vceV;
    m_g = std::min(g, kMaxConductance) + kJunctionGmin;
    m_ieq = i - (m_g - kJunctionGmin) * vceV;
}

bool Phototransistor::linearize(const SolutionView& iterate) noexcept {
    // Begrenzung: In der Saettigung ist die Kennlinie flach, Newton springt sonst ueber den Knick hinweg hin und
    // her (Reihenwiderstand + Stromquelle). Auf den Knick zu hoechstens halbieren, dann konvergiert Newton im Knick.
    double v = iterate.voltage(terminal(0)) - iterate.voltage(terminal(1));
    const double old = m_vLin, knee = m_params.kneeV;
    bool limited = false;
    if (old > knee && v < 0.5 * old) {
        v = 0.5 * old;
        limited = true;
    } else if (old < -knee * m_params.inverseGain && v > 0.5 * old) {
        v = 0.5 * old;
        limited = true;
    }
    evaluate(v);
    return limited;
}

void Phototransistor::resetLinearization(const SolutionView& accepted) noexcept {
    evaluate(accepted.voltage(terminal(0)) - accepted.voltage(terminal(1)));
}

void Phototransistor::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    sys.stampConductance(terminal(0), terminal(1), m_g);
    sys.stampCurrentSource(terminal(0), terminal(1), m_ieq);
}

double Phototransistor::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    const double i = m_g * (x.voltage(terminal(0)) - x.voltage(terminal(1))) + m_ieq;
    return t == 0 ? i : -i;
}

void Phototransistor::thermal(const SolutionView& x, double dtS) noexcept {
    m_lastCurrentA = terminalCurrent(x, 0);
    const double p = (x.voltage(terminal(0)) - x.voltage(terminal(1))) * m_lastCurrentA;
    integrateThermal(std::max(0.0, p), dtS, m_params.thermal);
}

void Phototransistor::checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept {
    m_overloaded = !isDestroyed() && std::fabs(m_lastCurrentA) > m_params.imaxA;
    checkThermalLimit(m_params.thermal, events, index, timeS);
}

ComponentReading Phototransistor::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    r.level = std::clamp(r.currentA / 1e-3, 0.0, 10.0);
    r.displayValue = m_lightA;
    return r;
}

// ------------------------------------------------------------------ Optokoppler

Optocoupler::Optocoupler(std::string id, NodeId anode, NodeId cathode, NodeId collector, NodeId emitter,
                         OptocouplerParams params)
    : ComponentModel(id, {anode, cathode, collector, emitter}), m_led(id + ".LED", anode, cathode, params.led),
      m_out(id + ".T", collector, emitter, params.output, 0.0) {
    m_out.addEmitter(&m_led, params.ctr > 0.0 && std::isfinite(params.ctr) ? params.ctr : 1.0);
}

void Optocoupler::beginSolve(const StampContext& ctx) noexcept {
    if (internalNodeCount() > 0) m_led.assignFirstInternalNode(internalNode(0));
    m_led.applyPendingState();
    m_out.applyPendingState();
    m_led.beginSolve(ctx);
    m_out.beginSolve(ctx);
}

bool Optocoupler::linearize(const SolutionView& iterate) noexcept {
    const bool a = m_led.linearize(iterate);
    const bool b = m_out.linearize(iterate);
    return a || b;
}

void Optocoupler::resetLinearization(const SolutionView& accepted) noexcept {
    m_led.resetLinearization(accepted);
    m_out.resetLinearization(accepted);
}

void Optocoupler::stamp(MnaSystem& sys, const StampContext& ctx) const noexcept {
    m_led.stamp(sys, ctx);
    m_out.stamp(sys, ctx);
}

double Optocoupler::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    return t < 2 ? m_led.terminalCurrent(x, t) : m_out.terminalCurrent(x, t - 2);
}

void Optocoupler::updateState(const SolutionView& x, double dtS) noexcept {
    m_led.updateState(x, dtS);
    m_out.updateState(x, dtS);
}

void Optocoupler::thermal(const SolutionView& x, double dtS) noexcept {
    m_led.thermal(x, dtS);
    m_out.thermal(x, dtS);
    m_tempK = std::max(m_led.tempK(), m_out.tempK());
}

void Optocoupler::checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept {
    // die Teile melden ihre Zerstoerung selbst (mit dem Index des Optokopplers); hier nur vormerken
    m_led.checkLimits(events, index, timeS);
    m_out.checkLimits(events, index, timeS);
    m_overloaded = m_led.isOverloaded() || m_out.isOverloaded();
    if (!isDestroyed() && (m_led.isDestroyed() || m_out.isDestroyed())) m_destroyPending = true;
}

ComponentReading Optocoupler::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    r.level = std::max(0.0, r.currentA) / 0.02;
    r.displayValue = m_out.lightCurrentA();
    return r;
}

ComponentState Optocoupler::saveState() const noexcept {
    ComponentState s = ComponentModel::saveState();
    const ComponentState l = m_led.saveState(), o = m_out.saveState();
    s.internal[0] = l.destroyed ? 1.0 : 0.0;
    s.internal[1] = l.tempK;
    s.internal[2] = o.destroyed ? 1.0 : 0.0;
    s.internal[3] = o.tempK;
    return s;
}

void Optocoupler::restoreState(const ComponentState& state) noexcept {
    ComponentModel::restoreState(state);
    auto part = [](double destroyed, double tempK) {
        ComponentState p;
        p.destroyed = destroyed > 0.5;
        p.tempK = std::isfinite(tempK) && tempK > 0.0 ? tempK : kAmbientK;
        return p;
    };
    m_led.restoreState(part(state.internal[0], state.internal[1]));
    m_out.restoreState(part(state.internal[2], state.internal[3]));
}

} // namespace volta::core

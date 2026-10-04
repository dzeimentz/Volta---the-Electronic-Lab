#include "volta/core/components/resistor.hpp"

#include <cmath>
#include <utility>

namespace volta::core {

namespace {
// Unsinnige Werte abfangen: nicht endlich -> praktisch offen, zu klein -> kMinOhm.
double sanitizeOhm(double ohm) noexcept {
    if (!std::isfinite(ohm)) return 1e12;
    return ohm < kMinOhm ? kMinOhm : ohm;
}
} // namespace

Resistor::Resistor(std::string id, NodeId a, NodeId b, ResistorParams params)
    : ComponentModel(std::move(id), {a, b}), m_params(params) {
    m_params.ohm = sanitizeOhm(m_params.ohm);
}

void Resistor::setOhm(double ohm) noexcept { m_params.ohm = sanitizeOhm(ohm); }

void Resistor::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    // Durchgebrannt: offen (nur GMIN, damit isolierte Knoten loesbar bleiben)
    sys.stampConductance(terminal(0), terminal(1), m_destroyed ? kGmin : 1.0 / m_params.ohm);
}

double Resistor::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    const double g = m_destroyed ? kGmin : 1.0 / m_params.ohm;
    const double i = (x.voltage(terminal(0)) - x.voltage(terminal(1))) * g;
    return t == 0 ? i : -i;
}

double Resistor::powerW(const SolutionView& x) const noexcept {
    if (m_destroyed) return 0.0;
    const double v = x.voltage(terminal(0)) - x.voltage(terminal(1));
    return v * v / m_params.ohm;
}

void Resistor::thermal(const SolutionView& x, double dtS) noexcept {
    m_lastPowerW = powerW(x);
    integrateThermal(m_lastPowerW, dtS, m_params.thermal);
}

void Resistor::checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept {
    m_overloaded = !isDestroyed() && m_lastPowerW > m_params.pmaxW;
    checkThermalLimit(m_params.thermal, events, index, timeS);
}

} // namespace volta::core

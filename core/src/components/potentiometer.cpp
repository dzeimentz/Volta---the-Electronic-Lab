#include "volta/core/components/potentiometer.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace volta::core {

Potentiometer::Potentiometer(std::string id, NodeId a, NodeId wiper, NodeId b, PotentiometerParams params,
                             double position)
    : ComponentModel(std::move(id), {a, wiper, b}), m_params(params) {
    const PotentiometerParams def;
    if (!std::isfinite(m_params.ohm) || m_params.ohm < 1.0) m_params.ohm = def.ohm;
    if (!std::isfinite(m_params.pmaxW) || m_params.pmaxW <= 0.0) m_params.pmaxW = def.pmaxW;
    if (!std::isfinite(m_params.minOhm) || m_params.minOhm < kMinOhm) m_params.minOhm = def.minOhm;
    setPosition(position);
    applyPosition();
}

void Potentiometer::setPosition(double position) noexcept {
    m_position = std::isfinite(position) ? std::clamp(position, 0.0, 1.0) : 0.5;
}

void Potentiometer::applyPosition() noexcept {
    m_gAS = 1.0 / std::max(m_position * m_params.ohm, m_params.minOhm);
    m_gSB = 1.0 / std::max((1.0 - m_position) * m_params.ohm, m_params.minOhm);
}

void Potentiometer::beginSolve(const StampContext& /*ctx*/) noexcept { applyPosition(); }

void Potentiometer::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    const double off = kGmin; // durchgebrannte Bahn: offen
    sys.stampConductance(terminal(0), terminal(1), m_destroyed ? off : m_gAS);
    sys.stampConductance(terminal(1), terminal(2), m_destroyed ? off : m_gSB);
}

double Potentiometer::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    const double gAS = m_destroyed ? kGmin : m_gAS;
    const double gSB = m_destroyed ? kGmin : m_gSB;
    const double va = x.voltage(terminal(0));
    const double vs = x.voltage(terminal(1));
    const double vb = x.voltage(terminal(2));
    const double iAS = (va - vs) * gAS; // von a zum Schleifer
    const double iSB = (vs - vb) * gSB; // vom Schleifer nach b
    if (t == 0) return iAS;
    if (t == 2) return -iSB;
    return iSB - iAS;
}

void Potentiometer::thermal(const SolutionView& x, double /*dtS*/) noexcept {
    const double vas = x.voltage(terminal(0)) - x.voltage(terminal(1));
    const double vsb = x.voltage(terminal(1)) - x.voltage(terminal(2));
    m_lastPowerAS = m_destroyed ? 0.0 : vas * vas * m_gAS;
    m_lastPowerSB = m_destroyed ? 0.0 : vsb * vsb * m_gSB;
}

void Potentiometer::checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept {
    if (isDestroyed()) return;
    // Belastbarkeit anteilig zur Bahnlaenge; ein kurzes Bahnstueck verkraftet nur einen Bruchteil von pmaxW.
    const double limAS = m_params.pmaxW * std::max(m_position, 0.02);
    const double limSB = m_params.pmaxW * std::max(1.0 - m_position, 0.02);
    m_overloaded = m_lastPowerAS > limAS || m_lastPowerSB > limSB;
    // Grob ueberlastete Bahn brennt sofort durch (vereinfachte Thermik; Kalibrierung Phase 0)
    if (m_lastPowerAS > 4.0 * limAS || m_lastPowerSB > 4.0 * limSB) destroy(events, index, timeS);
}

ComponentReading Potentiometer::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    r.level = m_position; // Stellung fuer den Lichtring
    return r;
}

} // namespace volta::core

#include "volta/core/components/potentiometer.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace volta::core {

namespace {

// Exakte Loesung von Cth*dT/dt = P - (T - Tamb)/Rth ueber dtS fuer eine Teilbahn (stabil fuer jede Schrittweite)
double stepTemp(double tempK, double powerW, double rthKPerW, double tauS, double dtS) noexcept {
    if (!std::isfinite(powerW)) powerW = 0.0;
    const double steady = kAmbientK + std::max(powerW, 0.0) * rthKPerW;
    return steady + (tempK - steady) * std::exp(-dtS / tauS);
}

double validTemp(double tempK) noexcept { return std::isfinite(tempK) && tempK > 0.0 ? tempK : kAmbientK; }

} // namespace

Potentiometer::Potentiometer(std::string id, NodeId a, NodeId wiper, NodeId b, PotentiometerParams params,
                             double position)
    : ComponentModel(std::move(id), {a, wiper, b}), m_params(params) {
    const PotentiometerParams def;
    if (!std::isfinite(m_params.ohm) || m_params.ohm < 1.0) m_params.ohm = def.ohm;
    if (!std::isfinite(m_params.pmaxW) || m_params.pmaxW <= 0.0) m_params.pmaxW = def.pmaxW;
    if (!std::isfinite(m_params.minOhm) || m_params.minOhm < kMinOhm) m_params.minOhm = def.minOhm;
    if (!std::isfinite(m_params.minShare) || m_params.minShare <= 0.0 || m_params.minShare > 1.0)
        m_params.minShare = def.minShare;
    const ThermalParams& t = m_params.thermal;
    if (!(t.rthKPerW > 0.0) || !(t.cthJPerK > 0.0) || !std::isfinite(t.rthKPerW) || !std::isfinite(t.cthJPerK) ||
        !std::isfinite(t.tmaxK))
        m_params.thermal = def.thermal;
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

// Waermewirksamer Anteil einer Teilbahn: ihre Laenge, fuer kurze Stuecke mindestens minShare
double Potentiometer::share(double fraction) const noexcept { return std::max(fraction, m_params.minShare); }

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

void Potentiometer::thermal(const SolutionView& x, double dtS) noexcept {
    const double vas = x.voltage(terminal(0)) - x.voltage(terminal(1));
    const double vsb = x.voltage(terminal(1)) - x.voltage(terminal(2));
    m_lastPowerAS = m_destroyed ? 0.0 : vas * vas * m_gAS;
    m_lastPowerSB = m_destroyed ? 0.0 : vsb * vsb * m_gSB;
    if (dtS > 0.0) {
        const ThermalParams& p = m_params.thermal;
        const double tauS = p.rthKPerW * p.cthJPerK;
        m_tempASK = stepTemp(m_tempASK, m_lastPowerAS, p.rthKPerW / share(m_position), tauS, dtS);
        m_tempSBK = stepTemp(m_tempSBK, m_lastPowerSB, p.rthKPerW / share(1.0 - m_position), tauS, dtS);
    }
    m_tempK = std::max(m_tempASK, m_tempSBK);
}

void Potentiometer::checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept {
    if (isDestroyed()) return;
    m_overloaded = m_lastPowerAS > m_params.pmaxW * share(m_position) ||
                   m_lastPowerSB > m_params.pmaxW * share(1.0 - m_position);
    checkThermalLimit(m_params.thermal, events, index, timeS); // heissere Teilbahn (tempK)
}

ComponentReading Potentiometer::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    r.level = m_position; // Stellung fuer den Lichtring
    return r;
}

ComponentState Potentiometer::saveState() const noexcept {
    ComponentState s = ComponentModel::saveState();
    s.internal[0] = m_tempASK;
    s.internal[1] = m_tempSBK;
    return s;
}

void Potentiometer::restoreState(const ComponentState& state) noexcept {
    ComponentModel::restoreState(state);
    // neues Bauteil (Reparieren, ComponentState{}): beide Teilbahnen auf Umgebungstemperatur
    m_tempASK = validTemp(state.internal[0]);
    m_tempSBK = validTemp(state.internal[1]);
    m_tempK = std::max(m_tempASK, m_tempSBK);
}

} // namespace volta::core

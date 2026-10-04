#include "volta/core/component_model.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace volta::core {

ComponentModel::ComponentModel(std::string id, std::initializer_list<NodeId> terminals)
    : m_id(std::move(id)) {
    for (NodeId n : terminals) {
        if (m_terminalCount == kMaxTerminals) break;
        m_terminals[m_terminalCount++] = n;
    }
}

ComponentReading ComponentModel::read(const SolutionView& x) const noexcept {
    ComponentReading r;
    if (m_terminalCount >= 2) {
        r.voltV = x.voltage(m_terminals[0]) - x.voltage(m_terminals[1]);
        r.currentA = terminalCurrent(x, 0);
    }
    double p = 0.0;
    for (std::size_t t = 0; t < m_terminalCount; ++t) {
        const double i = terminalCurrent(x, t);
        r.terminalCurrentA[t] = i;
        p += x.voltage(m_terminals[t]) * i;
    }
    r.powerW = p;
    r.tempK = m_tempK;
    r.overloaded = m_overloaded && !isDestroyed();
    r.destroyed = isDestroyed();
    return r;
}

ComponentState ComponentModel::saveState() const noexcept {
    ComponentState s;
    s.destroyed = isDestroyed();
    s.tempK = m_tempK;
    return s;
}

void ComponentModel::restoreState(const ComponentState& state) noexcept {
    m_destroyed = state.destroyed;
    m_destroyPending = false;
    m_tempK = std::isfinite(state.tempK) ? state.tempK : kAmbientK;
    m_overloaded = false;
}

void ComponentModel::integrateThermal(double powerW, double dtS, const ThermalParams& p) noexcept {
    if (!(p.rthKPerW > 0.0) || !(p.cthJPerK > 0.0) || !(dtS > 0.0)) return;
    if (!std::isfinite(powerW)) powerW = 0.0;
    const double tau = p.rthKPerW * p.cthJPerK;
    const double steady = kAmbientK + std::max(powerW, 0.0) * p.rthKPerW;
    m_tempK = steady + (m_tempK - steady) * std::exp(-dtS / tau);
}

void ComponentModel::destroy(EventQueue& events, std::size_t index, double timeS) noexcept {
    if (isDestroyed()) return;
    m_destroyPending = true;
    m_overloaded = false;
    events.push({SimEventType::Destroyed, index, timeS});
}

void ComponentModel::checkThermalLimit(const ThermalParams& p, EventQueue& events, std::size_t index,
                                       double timeS) noexcept {
    if (p.tmaxK > 0.0 && m_tempK > p.tmaxK) destroy(events, index, timeS);
}

} // namespace volta::core

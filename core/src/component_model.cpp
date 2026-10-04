#include "volta/core/component_model.hpp"

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
    r.destroyed = m_destroyed;
    return r;
}

ComponentState ComponentModel::saveState() const noexcept {
    ComponentState s;
    s.destroyed = m_destroyed;
    s.tempK = m_tempK;
    return s;
}

void ComponentModel::restoreState(const ComponentState& state) noexcept {
    m_destroyed = state.destroyed;
    m_tempK = state.tempK;
}

} // namespace volta::core

#include "volta/core/circuit.hpp"

namespace volta::core {

std::size_t Circuit::indexOf(const std::string& id) const noexcept {
    for (std::size_t i = 0; i < m_components.size(); ++i)
        if (m_components[i]->id() == id) return i;
    return kNoComponent;
}

bool Circuit::isValid() const noexcept {
    for (const auto& c : m_components) {
        if (!c) return false;
        for (std::size_t t = 0; t < c->terminalCount(); ++t)
            if (c->terminal(t) >= m_nodeCount) return false;
    }
    return true;
}

} // namespace volta::core

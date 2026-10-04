#include "volta/core/sim_event.hpp"

namespace volta::core {

void EventQueue::reserve(std::size_t capacity) {
    m_buf.assign(capacity, SimEvent{});
    clear();
}

bool EventQueue::push(const SimEvent& e) noexcept {
    if (m_count >= m_buf.size()) {
        ++m_dropped;
        return false;
    }
    m_buf[(m_head + m_count) % m_buf.size()] = e;
    ++m_count;
    return true;
}

bool EventQueue::pop(SimEvent& out) noexcept {
    if (m_count == 0) return false;
    out = m_buf[m_head];
    m_head = (m_head + 1) % m_buf.size();
    --m_count;
    return true;
}

void EventQueue::clear() noexcept {
    m_head = 0;
    m_count = 0;
    m_dropped = 0;
}

} // namespace volta::core

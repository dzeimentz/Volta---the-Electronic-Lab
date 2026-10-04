#pragma once

#include <cstddef>
#include <vector>

namespace volta::core {

enum class SimEventType {
    Destroyed,         // Bauteil zerstoert (Ueberlast, Verpolung, Ueberspannung)
    BatteryHot,        // Batterie erwaermt sich (Kurzschluss)
    FuseBlown,         // Sicherung des Messgeraets ausgeloest
    RelayClick,        // Relaiskontakt hat umgeschaltet
    ConvergenceTrouble // Loeser konvergiert nicht; letzter gueltiger Zustand bleibt erhalten
};

inline constexpr std::size_t kNoComponent = static_cast<std::size_t>(-1);

struct SimEvent {
    SimEventType type = SimEventType::ConvergenceTrouble;
    std::size_t component = kNoComponent; // Index in Circuit, kNoComponent fuer schaltungsweite Ereignisse
    double timeS = 0.0;
};

// Ringpuffer fester Groesse fuer Ereignisse. Speicher nur in reserve(); push/pop sind allokationsfrei.
// Bei vollem Puffer wird das neue Ereignis verworfen und gezaehlt.
class EventQueue {
public:
    void reserve(std::size_t capacity);

    bool push(const SimEvent& e) noexcept;
    bool pop(SimEvent& out) noexcept;

    std::size_t size() const noexcept { return m_count; }
    bool empty() const noexcept { return m_count == 0; }
    std::size_t dropped() const noexcept { return m_dropped; }
    void clear() noexcept;

private:
    std::vector<SimEvent> m_buf;
    std::size_t m_head = 0;
    std::size_t m_count = 0;
    std::size_t m_dropped = 0;
};

} // namespace volta::core

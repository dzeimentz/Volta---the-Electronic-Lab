#pragma once

// Ereignisgesteuerte Logiksimulation mit Einheitsverzoegerung je Gatter.
// Alle Gatter, deren Eingaenge sich in einer Runde geaendert haben, rechnen in der naechsten Runde gleichzeitig;
// kurze Stoerimpulse durch Laufzeitunterschiede entstehen dadurch echt (Konzept Zuse 8.2). Speicherfrei nach load().
#include <cstddef>
#include <vector>

#include "volta/logic/netlist.hpp"

namespace volta::logic {

enum class SettleStatus { Stable, Oscillating };

class LogicSim {
public:
    void load(const Netlist& netlist);
    const Netlist& netlist() const noexcept { return m_net; }

    void setInput(std::size_t index, Level level) noexcept;
    Level input(std::size_t index) const noexcept;
    Level output(std::size_t index) const noexcept;
    Level net(NetId id) const noexcept { return id < m_value.size() ? m_value[id] : Level::X; }

    // Eine Runde (eine Gatterlaufzeit). Liefert true, wenn sich etwas geaendert hat.
    bool round() noexcept;
    // Rechnet, bis nichts mehr wechselt, hoechstens maxRounds Runden (sonst schwingt die Schaltung).
    SettleStatus settle(int maxRounds = 10000) noexcept;
    int lastRounds() const noexcept { return m_lastRounds; }

private:
    Netlist m_net;
    std::vector<Level> m_value;
    std::vector<std::vector<std::size_t>> m_fanout; // Leitung -> Gatter, die sie lesen
    std::vector<std::size_t> m_pending;              // Gatter, die in der naechsten Runde rechnen
    std::vector<char> m_isPending;
    std::vector<std::size_t> m_nextPending;
    std::vector<std::pair<NetId, Level>> m_changes;
    std::vector<char> m_inputNet;                    // von aussen gesetzte Leitung (Eingang)
    int m_lastRounds = 0;
};

// Pruefung mit vollstaendiger Wahrheitstabelle: Zeile r belegt die Eingaenge binaer mit r (erster Eingang hoechstes
// Bit). expected[r] sind die erwarteten Ausgaenge. Jede Zeile laeuft durch die Simulation.
struct TruthRow {
    std::vector<Level> inputs;
    std::vector<Level> expected;
    std::vector<Level> actual;
    bool ok = false;
};

struct CheckResult {
    bool passed = false;
    bool oscillating = false; // mindestens eine Zeile kam nicht zur Ruhe
    std::vector<TruthRow> rows;
};

CheckResult checkTruthTable(const Netlist& netlist, const std::vector<std::vector<Level>>& expected);

// Wahrheitstabelle eines Chips (Ausgaenge je Zeile), wie sie die Simulation liefert
std::vector<std::vector<Level>> truthTable(const Netlist& netlist);

} // namespace volta::logic

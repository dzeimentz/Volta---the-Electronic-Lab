#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace volta::core {

// Knotenindex; 0 ist Masse (Pultfeld 0V).
using NodeId = std::size_t;
inline constexpr NodeId kGround = 0;

// Minimale Netzliste fuer Phase 0. Bauteilmodelle folgen gemaess docs/simulation-core.md.
struct ResistorSpec {
    std::string id;
    NodeId a = kGround;
    NodeId b = kGround;
    double ohm = 1.0;
};

struct VoltageSourceSpec {
    std::string id;
    NodeId plus = kGround;
    NodeId minus = kGround;
    double volt = 0.0;
    double innerOhm = 0.0;
};

class Circuit {
public:
    NodeId addNode();
    std::size_t nodeCount() const noexcept { return m_nodeCount; }

    void addResistor(ResistorSpec spec);
    void addVoltageSource(VoltageSourceSpec spec);

    const std::vector<ResistorSpec>& resistors() const noexcept { return m_resistors; }
    const std::vector<VoltageSourceSpec>& sources() const noexcept { return m_sources; }

private:
    std::size_t m_nodeCount = 1; // Masse existiert immer
    std::vector<ResistorSpec> m_resistors;
    std::vector<VoltageSourceSpec> m_sources;
};

// Ergebnis einer DC-Loesung.
struct DcSolution {
    bool ok = false;
    std::vector<double> nodeVolt;      // Index = NodeId
    std::vector<double> sourceCurrent; // Index = Reihenfolge der Quellen
};

// Dichte MNA-Loesung (Phase 0, nur lineare Bauteile). Wirft keine Ausnahmen.
DcSolution solveDc(const Circuit& circuit) noexcept;

} // namespace volta::core

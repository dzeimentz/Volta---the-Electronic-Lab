#pragma once

#include <cstddef>
#include <vector>

#include "volta/core/component_model.hpp"
#include "volta/core/types.hpp"

namespace volta::core {

// Messschnittstelle: Schnappschuss aller Knotenpotentiale und Bauteilmesswerte nach einem angenommenen Schritt.
// Spannungsanzeige und Messgeraet lesen ausschliesslich hierueber. Enthaelt nie NaN/Inf.
struct Probe {
    double timeS = 0.0;
    bool valid = false;                     // false, solange noch kein Zustand berechnet wurde
    std::vector<double> nodeVolt;           // Index = NodeId, nodeVolt[0] = 0 (Masse)
    std::vector<ComponentReading> readings; // Index = Bauteilindex im Circuit

    double voltage(NodeId node) const noexcept { return node < nodeVolt.size() ? nodeVolt[node] : 0.0; }
};

} // namespace volta::core

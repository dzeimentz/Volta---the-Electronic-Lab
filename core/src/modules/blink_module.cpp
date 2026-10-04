#include "volta/core/modules/blink_module.hpp"

#include "volta/core/components/capacitor.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/transistor.hpp"

namespace volta::core {

SubcircuitRange addBlinkModule(Circuit& circuit, const std::string& prefix, const BlinkModulePins& pins) {
    SubcircuitRange range;
    range.first = circuit.componentCount();
    auto name = [&](const char* part) { return prefix + "." + part; };

    const NodeId c1 = circuit.addNode(); // Kollektor T1
    const NodeId c2 = circuit.addNode(); // Kollektor T2
    const NodeId b1 = circuit.addNode(); // Basis T1
    const NodeId b2 = circuit.addNode(); // Basis T2

    // Kippstufe: Halbperiode ca. 0,69 * R_B * C (R_B einschliesslich Anteil von R_takt).
    // Leicht unsymmetrisch (R_B1 != R_B2), damit die Schwingung sicher anlaeuft.
    circuit.add<Resistor>(name("R_takt"), pins.plus, pins.rate, ResistorParams{10000.0});
    circuit.add<Resistor>(name("RC1"), pins.plus, c1, ResistorParams{1000.0});
    circuit.add<Resistor>(name("RC2"), pins.plus, c2, ResistorParams{1000.0});
    circuit.add<Resistor>(name("RB1"), pins.rate, b1, ResistorParams{22000.0});
    circuit.add<Resistor>(name("RB2"), pins.rate, b2, ResistorParams{27000.0});
    circuit.add<Capacitor>(name("C1"), c1, b2, CapacitorParams::electrolytic(22e-6));
    circuit.add<Capacitor>(name("C2"), c2, b1, CapacitorParams::electrolytic(22e-6));
    circuit.add<Npn>(name("T1"), b1, c1, pins.minus);
    circuit.add<Npn>(name("T2"), b2, c2, pins.minus);

    // Ausgangsstufe: T3 leitet, wenn T2 sperrt (Kollektor T2 hoch); offener Kollektor an aus.
    const NodeId b3 = circuit.addNode();
    circuit.add<Resistor>(name("RB3"), c2, b3, ResistorParams{4700.0});
    circuit.add<Npn>(name("T3"), b3, pins.out, pins.minus);

    range.count = circuit.componentCount() - range.first;
    return range;
}

} // namespace volta::core

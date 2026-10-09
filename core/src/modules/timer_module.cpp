#include "volta/core/modules/timer_module.hpp"

#include "volta/core/components/capacitor.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/transistor.hpp"

namespace volta::core {

SubcircuitRange addTimerModule(Circuit& circuit, const std::string& prefix, const TimerModulePins& pins) {
    SubcircuitRange range;
    range.first = circuit.componentCount();
    auto name = [&](const char* part) { return prefix + "." + part; };

    const NodeId c1 = circuit.addNode(); // Kollektor T1
    const NodeId c2 = circuit.addNode(); // Kollektor T2
    const NodeId b1 = circuit.addNode(); // Basis T1
    const NodeId b2 = circuit.addNode(); // Basis T2
    const NodeId b3 = circuit.addNode(); // Basis T3

    circuit.add<Resistor>(name("R_zeit"), pins.plus, b1, ResistorParams{47000.0});
    circuit.add<Resistor>(name("RC1"), pins.plus, c1, ResistorParams{2200.0});
    circuit.add<Resistor>(name("RC2"), pins.plus, c2, ResistorParams{2200.0});
    circuit.add<Resistor>(name("RB2"), c1, b2, ResistorParams{22000.0});    // Selbsthaltung waehrend der Zeit
    circuit.add<Resistor>(name("R_start"), pins.start, b2, ResistorParams{10000.0});
    circuit.add<Resistor>(name("R_ab"), b2, pins.minus, ResistorParams{47000.0}); // T2 sicher aus in Ruhe
    circuit.add<Capacitor>(name("C"), c2, b1, CapacitorParams::electrolytic(100e-6));
    circuit.add<Npn>(name("T1"), b1, c1, pins.minus);
    circuit.add<Npn>(name("T2"), b2, c2, pins.minus);
    // Ausgangsstufe: T3 leitet, wenn T1 sperrt (Kollektor T1 hoch); offener Kollektor an aus
    circuit.add<Resistor>(name("RB3"), c1, b3, ResistorParams{4700.0});
    circuit.add<Npn>(name("T3"), b3, pins.out, pins.minus);

    range.count = circuit.componentCount() - range.first;
    return range;
}

SubcircuitRange addAmplifierModule(Circuit& circuit, const std::string& prefix, const AmplifierModulePins& pins) {
    SubcircuitRange range;
    range.first = circuit.componentCount();
    auto name = [&](const char* part) { return prefix + "." + part; };

    const NodeId b1 = circuit.addNode(); // Basis T1
    const NodeId b2 = circuit.addNode(); // Basis T2 = Emitter T1

    circuit.add<Resistor>(name("R_ein"), pins.in, b1, ResistorParams{10000.0});
    circuit.add<Resistor>(name("R_ab1"), b1, pins.minus, ResistorParams{1.0e6}); // offener Eingang: aus
    circuit.add<Npn>(name("T1"), b1, pins.plus, b2);
    circuit.add<Resistor>(name("R_ab2"), b2, pins.minus, ResistorParams{47000.0});
    circuit.add<Npn>(name("T2"), b2, pins.out, pins.minus);

    range.count = circuit.componentCount() - range.first;
    return range;
}

} // namespace volta::core

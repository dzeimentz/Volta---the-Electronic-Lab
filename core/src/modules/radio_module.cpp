#include "volta/core/modules/radio_module.hpp"

#include "volta/core/components/capacitor.hpp"
#include "volta/core/components/inductor.hpp"
#include "volta/core/components/radio.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/transistor.hpp"

namespace volta::core {

SubcircuitRange addMwModule(Circuit& circuit, const std::string& prefix, const MwModulePins& pins) {
    SubcircuitRange range;
    range.first = circuit.componentCount();
    auto name = [&](const char* part) { return prefix + "." + part; };

    const NodeId det = circuit.addNode();  // Kathode der Detektordiode (Ton)
    const NodeId base = circuit.addNode(); // Basis T1

    circuit.add<Capacitor>(name("C_ant"), pins.ant, pins.tune, CapacitorParams::film(10e-12));
    circuit.add<Inductor>(name("L"), pins.tune, pins.minus, InductorParams::uH220());
    circuit.add<DetectorDiode>(name("D"), pins.tune, det, DiodeParams::germanium());
    circuit.add<Resistor>(name("R_last"), det, pins.minus, ResistorParams{22000.0});
    circuit.add<Capacitor>(name("C_ton"), base, det, CapacitorParams::electrolytic(10e-6)); // Plus zur Basis
    circuit.add<Resistor>(name("R_b"), pins.plus, base, ResistorParams{150000.0});
    circuit.add<Npn>(name("T1"), base, pins.out, pins.minus);

    range.count = circuit.componentCount() - range.first;
    return range;
}

} // namespace volta::core

#pragma once

#include <cstddef>
#include <string>

#include "volta/core/circuit.hpp"

namespace volta::core {

// Anschluesse des Blinkmoduls in der Netzliste (Katalog: plus, minus, aus, takt).
struct BlinkModulePins {
    NodeId plus = kGround;
    NodeId minus = kGround;
    NodeId out = kGround;  // offener Kollektor nach minus: Last (LED mit Vorwiderstand) von plus nach aus
    NodeId rate = kGround; // takt: ein Widerstand oder Regler von takt nach plus macht das Blinken schneller;
                           // unbeschaltet blinkt das Modul mit ca. 1 Hz
};

// Bereich der Bauteile, die eine Teilschaltung im Circuit belegt (fuer »Blick hinein«).
struct SubcircuitRange {
    std::size_t first = 0;
    std::size_t count = 0;
};

// Blinkmodul als echte Teilschaltung (docs/simulation-core.md, Module): astabile Kippstufe aus zwei
// NPN-Transistoren mit Ausgangstransistor. Die Bauteile erhalten die Kennungen "<prefix>.<Name>"
// (z. B. "BL1.T1"); innere Knoten werden im Circuit angelegt.
SubcircuitRange addBlinkModule(Circuit& circuit, const std::string& prefix, const BlinkModulePins& pins);

} // namespace volta::core

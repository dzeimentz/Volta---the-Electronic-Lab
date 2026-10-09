#pragma once

#include <string>

#include "volta/core/circuit.hpp"
#include "volta/core/modules/blink_module.hpp"

namespace volta::core {

// Anschluesse des Zeitmoduls (Katalog: plus, minus, start, aus).
struct TimerModulePins {
    NodeId plus = kGround;
    NodeId minus = kGround;
    NodeId start = kGround; // Ausloeser: kurz mit plus verbinden (Taster)
    NodeId out = kGround;   // offener Kollektor nach minus, waehrend der Zeit leitend
};

// Zeitmodul als echte Teilschaltung (Konzept 5.4: monostabile Kippstufe, in Stufe 4 aus Einzelteilen nachgebaut):
// T1 leitet in Ruhe (Basis ueber R_zeit an plus), T2 sperrt. Ein Impuls an start schaltet T2 ein; dessen
// Kollektorsprung koppelt ueber den Elko negativ auf die Basis von T1, T1 sperrt und haelt T2 ueber RB2 eingeschaltet.
// Erst wenn der Elko ueber R_zeit umgeladen ist (ca. ln 2 * R_zeit * C = 0,69 * 47 kOhm * 100 uF = 3,2 s), leitet T1
// wieder und T2 faellt ab. Der Ausgangstransistor T3 leitet, solange T1 sperrt.
SubcircuitRange addTimerModule(Circuit& circuit, const std::string& prefix, const TimerModulePins& pins);

// Anschluesse des Verstaerkermoduls (Katalog: plus, minus, ein, aus).
struct AmplifierModulePins {
    NodeId plus = kGround;
    NodeId minus = kGround;
    NodeId in = kGround;  // Eingang: schon wenige Mikroampere schalten den Ausgang
    NodeId out = kGround; // offener Kollektor nach minus
};

// Verstaerkermodul als Teilschaltung: Darlington-Stufe aus zwei NPN-Transistoren (Treiber T1 aus plus versorgt,
// Endstufe T2 an aus), Eingangswiderstand 10 kOhm, Ableitwiderstaende gegen ungewolltes Einschalten. Die
// Stromverstaerkung ist das Produkt beider Transistoren (rund 200 * 200): Ein Strom von 10 uA am Eingang reicht fuer
// 20 mA am Ausgang - etwa durch Wasser oder einen Fotowiderstand im Dunkeln.
SubcircuitRange addAmplifierModule(Circuit& circuit, const std::string& prefix, const AmplifierModulePins& pins);

} // namespace volta::core

#pragma once

#include <string>

#include "volta/core/circuit.hpp"
#include "volta/core/modules/blink_module.hpp"

namespace volta::core {

// Anschluesse des MW-Moduls (Katalog: plus, minus, ant, abst, aus; ab Volta 3).
struct MwModulePins {
    NodeId plus = kGround;
    NodeId minus = kGround;
    NodeId ant = kGround;  // Antenne (Pultfeld ANT der Senderanlage)
    NodeId tune = kGround; // oberes Ende des Schwingkreises: Drehkondensator von abst nach minus stecken
    NodeId out = kGround;  // Kollektor der Endstufe: Lautsprecher zwischen plus und aus
};

// MW-Modul als echte Teilschaltung (Konzept Volta 3, 5.4; Nachtrag 08.11.2026 (4)): derselbe Empfaenger, den das
// Kind in Stufe 2 aus Einzelteilen baut. Antenne ueber C_ant (10 pF, lose Kopplung) an den Kreis aus L (220 uH) und
// dem aussen gesteckten Drehkondensator; Detektordiode D (Germanium, Huellkurvendetektor) mit R_last (22 kOhm);
// C_ton (Elko 10 uF) koppelt den Ton auf die Basis von T1, R_b (150 kOhm) stellt den Arbeitspunkt ein (etwa 12 mA),
// der Lautsprecher liegt im Kollektorkreis. Abgestimmt wird wie beim Einzelaufbau ueber tuneRadio.
SubcircuitRange addMwModule(Circuit& circuit, const std::string& prefix, const MwModulePins& pins);

} // namespace volta::core

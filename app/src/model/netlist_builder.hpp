#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "board.hpp"
#include "catalog.hpp"
#include "volta/core/circuit.hpp"
#include "volta/core/modules/blink_module.hpp"
#include "volta/core/modules/timer_module.hpp"
#include "volta/core/modules/tone_module.hpp"

namespace volta::model {

// Kennungen der fest eingebauten Pultbauteile in der Netzliste.
namespace desk_id {
inline constexpr const char* kBattery = "PULT.B";
inline constexpr const char* kMainSwitch = "PULT.HS";
inline constexpr const char* kPot1 = "PULT.P1";
inline constexpr const char* kPot2 = "PULT.P2";
inline constexpr const char* kSwitch = "PULT.S";
inline constexpr const char* kSpeaker = "PULT.LS";
inline constexpr const char* kMeter = "PULT.MESS";
} // namespace desk_id

// Ergebnis der Netzlistenbildung: Kern-Netzliste plus Zuordnung von Anschlusspunkten und Bauteilen.
struct Netlist {
    core::Circuit circuit;
    std::map<std::string, core::NodeId> nodes;       // Anschlusspunkt (kanonisch) -> Knoten
    std::map<std::string, std::size_t> parts;        // Bauteil-ID -> Index im Circuit
    std::map<std::string, std::size_t> wires;        // Leitungs-ID -> Index im Circuit
    std::map<std::string, core::SubcircuitRange> modules;
    // Modul-ID -> Bauteil mit den Messwerten des Moduls (Erfolgsbedingungen, Anzeige): Tonmodul das Verhaltensmodell,
    // Blinkmodul der Ausgangstransistor
    std::map<std::string, std::size_t> moduleMain;
    std::map<std::string, std::size_t> desk;         // desk_id -> Index im Circuit
    std::vector<std::string> componentIds;           // Kennung je Index im Circuit (Zustandsschnappschuesse)
    std::vector<std::string> errors;                 // unbekannte Typen, ungueltige Anschlusspunkte ...

    bool ok() const { return errors.empty(); }
    // Knoten eines Anschlusspunkts (auch Modulanschluss), kGround fuer P:0V; false, wenn unbenutzt/unbekannt.
    bool nodeOf(const std::string& anchor, core::NodeId& out) const;
};

// Bildet aus dem Aufbau die Netzliste fuer den Simulationskern (docs/architecture.md):
//  - Jeder benutzte Anschlusspunkt ist ein Knoten; P:0V ist Masse.
//  - Leitungen sind Bauteile (core::Wire, 0,01 Ohm), damit ihr Strom fuer Stromfluss und Gluehen bekannt ist
//    (ADR-0007).
//  - Das Bedienpult ist immer bestueckt: Batterie mit Hauptschalter an P:+9V, Drehregler P1/P2, Schalter S1-S2,
//    Lautsprecher LS1-LS2 (vorerst 8 Ohm als ohmsche Last), Dreifach-Messgeraet an MESS:COM/VOHM/A.
//  - Module werden als Teilschaltung expandiert; ihre Anschluesse sind die Felder der Modulleiste.
Netlist buildNetlist(const Board& board, const Catalog& catalog);

// Uebernimmt die Bauteilzustaende (Ladung, Temperatur, zerstoert, Sicherung ...) aus einer alten in eine neue
// Netzliste, zugeordnet ueber die Bauteilkennung. Damit bleibt der Zustand bei jedem Topologiewechsel erhalten.
void transferStates(const core::Circuit& from, core::Circuit& to);

} // namespace volta::model

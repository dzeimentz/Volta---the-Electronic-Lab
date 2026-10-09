#pragma once

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "volta/core/component_model.hpp"
#include "volta/core/components/meter.hpp"

namespace volta::model {

// Bauteil auf der Grundplatte (docs/domain-model.md). pins: Anschlussname des Katalogs -> Anschlusspunkt.
struct PartPlacement {
    std::string id;   // z. B. "R1"
    std::string type; // Katalogtyp, z. B. "widerstand"
    std::vector<std::pair<std::string, std::string>> pins;
    double value = 0.0;    // Ohm bzw. Farad, je nach Typ (Katalog)
    bool closed = false;   // Schalter/Taster/Reedkontakt geschlossen bzw. Wechselschalter in Stellung B
    double position = 0.5; // Potentiometer-Stellung
};

// Gezogene Leitung zwischen zwei Anschlusspunkten.
struct WirePlacement {
    std::string id;
    std::string from;
    std::string to;
    std::string color = "rot";
    bool colorFixed = false; // Farbe von Hand gewaehlt: die Farbautomatik aendert sie nicht mehr
};

// Modul in der Modulleiste; belegt die Felder slot .. slot + Anzahl Anschluesse - 1.
struct ModulePlacement {
    std::string id;   // z. B. "BL1"
    std::string type; // Katalogtyp, z. B. "blinkmodul"
    int slot = 1;
};

// Zustand des Bedienpults (Bedienhandlungen, nicht Teil des Undo-Verlaufs).
struct DeskState {
    int cells = 6;              // 2 / 3 / 6 Zellen = 3 / 4,5 / 9 V
    bool mainSwitch = false;    // Hauptschalter der Batterie
    bool switchClosed = false;  // Pultschalter S1-S2
    double p1 = 0.5;            // Drehregler P1
    double p2 = 0.5;            // Drehregler P2
    core::MeterMode meterMode = core::MeterMode::Volt;
    bool meterAnalog = false;
    bool meterVisible = false;  // Messgeraet eingeblendet (Bedienhandlung; Buchsen nur dann steckbar)
};

// Aufbau: alles, was das Kind gesteckt, gezogen und eingestellt hat. Die Netzliste fuer den Kern wird daraus bei
// jedem Topologiewechsel neu gebildet (NetlistBuilder).
struct Board {
    std::vector<PartPlacement> parts;
    std::vector<WirePlacement> wires;
    std::vector<ModulePlacement> modules;
    std::vector<std::string> voltageDisplays; // Felder mit eingeschalteter Spannungsanzeige (Aufbau, undo-faehig)
    int columns = 12; // genutzte Spalten der Grundplatte: 8, 10 oder 12 (Aufbau, undo-faehig)
    DeskState desk;

    const PartPlacement* findPart(const std::string& id) const;
};

} // namespace volta::model

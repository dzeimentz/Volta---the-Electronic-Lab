#pragma once

#include <functional>
#include <string>
#include <vector>

#include "board.hpp"
#include "catalog.hpp"
#include "edit_message.hpp"
#include "volta/core/component_model.hpp"

namespace volta::model {

// Fehlersuche (Werkzeugkasten, Konzept 5.x): prueft auf Wunsch den Aufbau und nennt Auffaelligkeiten an der
// betreffenden Stelle - als Frage, nie als Loesung. Zwei Arten von Pruefungen:
//  - am Aufbau (immer): Kurzschluss ueber Leitungen, ueberbrueckte Bauteile, Beine und Leitungen ohne Verbindung,
//    offener Kreis (offener Schalter oder Sackgasse), fehlende Verbindung zu +9V/0V, Hauptschalter aus;
//  - an der laufenden Simulation (Hauptschalter ein): zerstoerte und ueberlastete Bauteile, verpolte Leuchtdiode
//    oder Elko, sehr grosser Strom durch eine Leuchtdiode (fehlender Vorwiderstand), Kurzschlussstrom.
// Die Pruefung am Aufbau ist bewusst grosszuegig (Mehranschluss-Bauteile und Module verbinden alle Anschluesse,
// Kondensatoren gelten als leitend), damit sie nie einen funktionierenden Aufbau bemaengelt.
struct CircuitFinding {
    enum class Kind {
        Short,         // +9V und 0V direkt verbunden (oder Kurzschlussstrom)
        Bridged,       // beide Beine eines Bauteils direkt verbunden
        Broken,        // Bauteil zerstoert
        Overload,      // Bauteil ueberlastet
        Reversed,      // Leuchtdiode oder Elko verpolt
        NoResistor,    // sehr grosser Strom durch eine Leuchtdiode
        Dangling,      // Bein ohne Verbindung
        LooseWire,     // Leitung, die nichts verbindet
        OpenSwitch,    // Kreis nur durch einen offenen Schalter unterbrochen
        DeadEnd,       // Kreis endet an einem Feld
        NoSupply,      // Aufbau nicht mit +9V bzw. 0V verbunden
        MainSwitchOff  // Kreis geschlossen, Hauptschalter aus
    };
    Kind kind;
    std::string partId;               // betroffenes Bauteil (leer: keins)
    std::vector<std::string> anchors; // Stellen zum Hervorheben (Felder, Pultfelder)
    Message message;                  // Frage an das Kind
};

struct CircuitCheckInput {
    const Board& board;
    const Catalog& catalog;
    // Messwerte der laufenden Simulation je Bauteil-ID (nullptr: keine); Batterie unter der Kennung "PULT.B"
    std::function<const core::ComponentReading*(const std::string&)> reading;
};

// Auffaelligkeiten, wichtigste zuerst (Kurzschluss, Zerstoerung, Verpolung ... Hauptschalter)
std::vector<CircuitFinding> checkCircuit(const CircuitCheckInput& in);

} // namespace volta::model

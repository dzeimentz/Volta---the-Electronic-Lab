#pragma once

#include <string>
#include <utility>
#include <vector>

#include "board.hpp"
#include "catalog.hpp"

namespace volta::model {

// Schritt-fuer-Schritt-Aufbauhilfe (Konzept 5.6): Die Vorlage eines Experiments wird in einzelne Schritte zerlegt
// (Module, Bauteile, Leitungen, zuletzt der Hauptschalter). Die Oberflaeche blendet jeweils das naechste Teil
// schemenhaft ein und hebt seine Zielfelder hervor. Ein Schritt gilt als erledigt, sobald der Aufbau des Kindes das
// Teil an derselben Stelle enthaelt - ohne Reihenfolgezwang: Der aktuelle Schritt ist immer der erste noch offene.
// Bewertet wird damit nur die Naehe zur Vorlage als Hilfe; ob das Experiment gelingt, entscheidet weiterhin die
// Erfolgsbedingung (Verhalten der Schaltung).
struct GuideStep {
    enum class Kind { Module, Part, Wire, MainSwitch };
    Kind kind = Kind::Part;
    std::string id;   // Kennung in der Vorlage
    std::string type; // Bauteil- bzw. Modultyp
    std::vector<std::pair<std::string, std::string>> pins; // Bauteil: Anschluss -> Feld (Vorlage)
    double value = 0.0;                                    // Wert (Widerstand, Nennspannung), 0 = beliebig
    std::string from, to;                                  // Leitung (Modulanschluesse als Feld der Leiste)
    std::string color;
    int slot = 0;                                          // Modul: erstes Feld der Leiste
    // Felder, die hervorgehoben werden (Bauteil: seine Felder; Leitung: beide Enden; Modul: seine Felder)
    std::vector<std::string> targets;
    bool anyLedColour = false; // LED-Farbe egal (Experiment ohne »farbe_fest«)
};

// Schritte aus der Vorlage: Module, Bauteile und Leitungen in Vorlagenreihenfolge, dann der Hauptschalter.
std::vector<GuideStep> buildGuide(const Board& templateBoard, const Catalog& catalog, bool colourFixed = true);

// Ist der Schritt im Aufbau erfuellt?
//  - Bauteil: gleicher Typ auf denselben Feldern. Ungepolte Bauteile (Widerstand, Lampe, Taster, Reedkontakt,
//    Motor, Kondensator) in beliebiger Richtung, gepolte mit derselben Anschlussbelegung (Transistor: beide
//    Emitterfuesse gleichwertig). Ist ein Wert vorgegeben (Widerstand, Gluehlampe), muss er stimmen.
//  - Leitung: eine Leitung zwischen denselben beiden Anschlusspunkten, Richtung egal (Modulanschluss und Feld der
//    Leiste darunter gelten als gleich).
//  - Modul: gleicher Typ ab demselben Feld der Leiste.
//  - Hauptschalter: eingeschaltet.
bool stepDone(const GuideStep& step, const Board& board, const Catalog& catalog);

// Index des ersten offenen Schritts (steps.size(), wenn alle erledigt sind).
std::size_t currentStep(const std::vector<GuideStep>& steps, const Board& board, const Catalog& catalog);

} // namespace volta::model

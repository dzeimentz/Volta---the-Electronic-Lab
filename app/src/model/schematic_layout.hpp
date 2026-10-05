#pragma once

#include <string>
#include <vector>

#include "board.hpp"
#include "catalog.hpp"

namespace volta::model {

// Automatischer Schaltplan aus dem Aufbau (Konzept 5.7, Schaltplanansicht ab Stufe 2): Knoten sind die elektrisch
// verbundenen Anschlusspunkte (Felder, die ueber Leitungen zusammenhaengen), beschriftet mit ihren Feldnamen.
// Bauteile, Module und die angeschlossenen Pultelemente sind Elemente zwischen ihren Knoten. Anordnung in Ebenen:
// Pluspol oben (Ebene 0), 0 V unten, die uebrigen Knoten nach ihrem Abstand vom Pluspol (Breitensuche ueber die
// Elemente). So liest man den Stromweg von oben nach unten. Koordinaten normiert auf 0..1.
struct SchematicNode {
    std::vector<std::string> anchors; // Anschlusspunkte des Knotens (Felder zuerst, in Lesereihenfolge)
    std::string label;                // z. B. "+9V · B3"
    int layer = 0;
    double x = 0.5;
    double y = 0.5;
};

struct SchematicElement {
    std::string id;           // Bauteil-ID, Modul-ID oder Pultelement (z. B. "PULT.B")
    std::string type;         // Katalogtyp bzw. "batterie", "pultschalter", "potentiometer", "lautsprecher", Modultyp
    std::vector<int> nodes;   // Knotenindex je Anschluss (Katalogreihenfolge)
    bool desk = false;        // Element des Bedienpults
    bool module = false;
    int parallelIndex = 0; // Zweipole zwischen denselben Knoten: Nummer und Anzahl (zum Versetzen)
    int parallelCount = 1;
};

struct Schematic {
    std::vector<SchematicNode> nodes;
    std::vector<SchematicElement> elements;
    int layers = 1;
};

// Schaltplan des Aufbaus. Nur Knoten mit mindestens zwei Anschluessen an Elementen oder Leitungen erscheinen;
// Pultelemente nur, wenn an ihnen etwas angeschlossen ist (die Batterie, sobald +9V oder 0V benutzt wird).
Schematic buildSchematic(const Board& board, const Catalog& catalog);

} // namespace volta::model

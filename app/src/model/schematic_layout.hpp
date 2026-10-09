#pragma once

#include <string>
#include <vector>

#include "board.hpp"
#include "catalog.hpp"

namespace volta::model {

// Automatischer Schaltplan aus dem Aufbau (Konzept 5.7, Schaltplanansicht ab Stufe 2), angeordnet nach
// DIN EN 61082-1 (Regeln fuer Schaltplaene) mit Schaltzeichen nach DIN EN 60617:
//  - Jeder Knoten (elektrisch verbundene Anschlusspunkte) ist eine waagrechte Leitung in einer eigenen Zeile:
//    Pluspol ganz oben, 0 V ganz unten, dazwischen die uebrigen Knoten nach ihrem Abstand vom Pluspol.
//  - Jedes Bauteil steht senkrecht in einer eigenen Spalte zwischen seinen Knoten (Strompfade nebeneinander,
//    Reihenschaltung uebereinander); die Batterie ganz links, danach in Signalrichtung Schalter, Regler, Bauteile,
//    Module und Verbraucher, der Lautsprecher rechts.
//  - Mehrpolige Bauteile: Transistor mit C oben, E unten, B von links; Potentiometer mit Schleifer von links;
//    Wechselschalter mit Mitte links; Module und Messgeraet als Block (Plus oben, Minus unten, Eingaenge links,
//    Ausgaenge rechts).
//  - Nur waagrechte und senkrechte Leitungen; Verbindungspunkt an jeder Abzweigung; Kreuzungen ohne Punkt sind
//    keine Verbindung. Beschriftung neben dem Symbol.
// Koordinaten in Rastereinheiten (1 Einheit = halbe Symbolbreite; ein Schaltzeichen ist 2 x 1 Einheiten).
struct SchematicNode {
    std::vector<std::string> anchors; // Anschlusspunkte des Knotens (Pultfelder, dann Felder in Lesereihenfolge)
    std::string label;                // z. B. "+9V · B3"
    int layer = 0;                    // Abstand vom Pluspol (Breitensuche), 0 V zuletzt
    bool drawn = false;               // false: offener Anschluss (nur ein Bein, keine Leitung) - keine Zeile
    double y = 0.0;                   // Zeile
    double x1 = 0.0, x2 = 0.0;        // waagrechte Leitung des Knotens
};

struct SchematicPinLabel {
    std::string text;
    double x = 0.0, y = 0.0; // Mitte der Beschriftung
};

struct SchematicElement {
    std::string id;           // Bauteil-ID, Modul-ID oder Pultelement (z. B. "PULT.B")
    std::string type;         // Katalogtyp bzw. "batterie", "pultschalter", "potentiometer", "lautsprecher", Modultyp
    std::vector<int> nodes;   // Knotenindex je Anschluss (Katalogreihenfolge)
    double value = 0.0;       // Wert des Bauteils (Ohm, Nennspannung), 0 = keiner
    bool desk = false;        // Element des Bedienpults
    bool module = false;
    int parallelIndex = 0; // Zweipole zwischen denselben Knoten: Nummer und Anzahl
    int parallelCount = 1;
    // Zeichnung: Mitte des Symbols, Drehung im Uhrzeigersinn (0, 90, 180, 270), Block (Modul, Messgeraet) mit Groesse
    double x = 0.0, y = 0.0;
    int rotation = 0;
    bool block = false;
    double w = 2.0, h = 1.0;
    double labelX = 0.0, labelY = 0.0; // linke Kante, Mitte der Kennzeichnung
    std::vector<SchematicPinLabel> pinLabels; // Block: Anschlussnamen innen
};

// Leitungsstueck (immer waagrecht oder senkrecht) und Verbindungspunkt, je einem Knoten zugeordnet
struct SchematicLine {
    double x1 = 0.0, y1 = 0.0, x2 = 0.0, y2 = 0.0;
    int node = -1;
};
struct SchematicDot {
    double x = 0.0, y = 0.0;
    int node = -1;
};

struct Schematic {
    std::vector<SchematicNode> nodes;
    std::vector<SchematicElement> elements;
    std::vector<SchematicLine> lines;
    std::vector<SchematicDot> dots;
    int layers = 1;
    double width = 0.0, height = 0.0; // Ausdehnung in Rastereinheiten (ab 0, mit Rand)
};

// Schaltplan des Aufbaus. Pultelemente nur, wenn an ihnen etwas angeschlossen ist (die Batterie, sobald +9V oder
// 0V benutzt wird).
Schematic buildSchematic(const Board& board, const Catalog& catalog);

} // namespace volta::model

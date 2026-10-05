#pragma once

#include <string>
#include <vector>

#include "board.hpp"
#include "catalog.hpp"

namespace volta::model {

// Lage eines Bauteils auf der Grundplatte: Startfeld, Richtung und Spannweite (docs/domain-model.md).
enum class Orientation { Horizontal, Vertical };
enum class Span { Short = 1, Long = 2 }; // ein bzw. zwei Feldabstaende

inline constexpr int kSocketsPerField = 4;

// Felder, die ein Bauteil vom Startfeld aus belegt; leer, wenn es nicht auf die Grundplatte passt.
// Zweipolig: Startfeld und Feld im Abstand span. Traegerplatine (drei Anschluesse): drei benachbarte Felder.
std::vector<std::string> footprint(const CatalogEntry& entry, const std::string& startField, Orientation o, Span span);

// Anschlussbelegung aus der Lage (Katalogreihenfolge bzw. "lage" der Traegerplatine).
bool placePins(const CatalogEntry& entry, const std::string& startField, Orientation o, Span span,
               std::vector<std::pair<std::string, std::string>>& pins, std::string& error);

// Freie Kennung fuer einen neuen Bauteiltyp, z. B. "R3" (Praefix nach Typ, kleinste freie Nummer).
std::string nextPartId(const Board& board, const std::string& type);
std::string nextWireId(const Board& board);

// Prueft, ob ein Bauteil gesetzt werden kann: Typ bekannt, Anschluesse auf gueltigen Feldern, Spannweite erlaubt,
// Steckplaetze frei (hoechstens vier Beine je Feld). ignoreId: bei Verschieben das Bauteil selbst nicht mitzaehlen.
bool canPlace(const Board& board, const Catalog& catalog, const PartPlacement& part, std::string& error,
              const std::string& ignoreId = {});

// Belegte Steckplaetze eines Felds (Bauteilbeine; Leitungen belegen keinen Steckplatz)
int usedSockets(const Board& board, const std::string& field, const std::string& ignoreId = {});

// Prueft eine Leitung: beide Enden gueltige Anschlusspunkte, nicht dasselbe Feld.
bool canWire(const Board& board, const std::string& from, const std::string& to, std::string& error);

// Umgedreht: Felder in umgekehrter Reihenfolge der Lage (z. B. LED mit Anode unten bzw. rechts).
bool isFlipped(const PartPlacement& part, const CatalogEntry& entry);
// Kehrt die Feldzuordnung um (Anschluesse bleiben, Felder tauschen die Reihenfolge).
void flipPins(PartPlacement& part, const CatalogEntry& entry);

PartPlacement* findPart(Board& board, const std::string& id);
WirePlacement* findWire(Board& board, const std::string& id);

} // namespace volta::model

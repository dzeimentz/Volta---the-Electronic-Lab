#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "board.hpp"
#include "catalog.hpp"
#include "edit_message.hpp"

namespace volta::model {

// Lage eines Bauteils auf der Grundplatte: Startfeld, Richtung und Spannweite (docs/domain-model.md).
enum class Orientation { Horizontal, Vertical };
enum class Span { Short = 1, Long = 2 }; // ein bzw. zwei Feldabstaende

inline constexpr int kSocketsPerField = 5;
// Einstellbare Breite der Grundplatte (Spalten je Reihe), Schritt 2
inline constexpr int kMinColumns = 8;
inline constexpr int kMaxColumns = 12;
inline constexpr int kColumnStep = 2;

// Liegt der Anschlusspunkt auf der genutzten Grundplatte? (Felder rechts von board.columns: nein; andere: ja)
bool withinBoard(const Board& board, const std::string& anchor);
// Groesste von Bauteilen oder Leitungen belegte Spalte (0: keine)
int highestUsedColumn(const Board& board);

// Felder, die ein Bauteil vom Startfeld aus belegt; leer, wenn es nicht auf die Grundplatte passt.
// Zweipolig: Startfeld und Feld im Abstand span. Traegerplatine (drei Anschluesse): drei benachbarte Felder.
// Quadratische Platine ("platine_2x2", Transistor): Startfeld oben links und die drei Nachbarn im 2-x-2-Block.
std::vector<std::string> footprint(const CatalogEntry& entry, const std::string& startField, Orientation o, Span span);

// Anschlussbelegung aus der Lage (Katalogreihenfolge bzw. "lage" der Traegerplatine).
bool placePins(const CatalogEntry& entry, const std::string& startField, Orientation o, Span span,
               std::vector<std::pair<std::string, std::string>>& pins, Message& error);

// Freie Kennung fuer einen neuen Bauteiltyp, z. B. "R3" (Praefix nach Typ, kleinste freie Nummer).
std::string nextPartId(const Board& board, const std::string& type);
std::string nextWireId(const Board& board);

// Prueft, ob ein Bauteil gesetzt werden kann: Typ bekannt, Anschluesse auf gueltigen Feldern, Spannweite erlaubt,
// Steckplaetze frei (hoechstens fuenf Beine je Feld). ignoreId: bei Verschieben das Bauteil selbst nicht mitzaehlen.
bool canPlace(const Board& board, const Catalog& catalog, const PartPlacement& part, Message& error,
              const std::string& ignoreId = {});

// Belegte Steckplaetze eines Felds durch Bauteilbeine (Grenze kSocketsPerField). Leitungen stecken zusaetzlich in
// freien Steckplaetzen, begrenzen aber nicht (siehe assignSockets).
int usedSockets(const Board& board, const std::string& field, const std::string& ignoreId = {});

// Steckplatz (Loch) je Bauteilbein und Leitungsende: 0 = ganz links ... kSocketsPerField - 1 = ganz rechts;
// Felder mit nur einem Steckplatz (Modulleiste, Modulanschluss) immer 0. Wird bei jeder Darstellung aus dem Aufbau
// berechnet (kein gespeicherter Zustand, Undo-sicher) und ist deterministisch:
// 1. Bauteilbeine in Aufbaureihenfolge. Bevorzugt die Seite zum Partnerfeld: Partner rechts -> Loch 3, 4, 2, 1, 0;
//    links -> 1, 0, 2, 3, 4; gleiche Spalte (senkrecht) -> Mitte 2, 1, 3, 0, 4.
// 2. Leitungsenden in Aufbaureihenfolge in freie Loecher: anderes Ende auf der Grundplatte weiter rechts -> von
//    aussen rechts (4, 3, ...), weiter links -> von aussen links (0, 1, ...), sonst Mitte zuerst. Sind alle Loecher
//    belegt, teilt sich die Leitung das am wenigsten belegte bevorzugte Loch.
struct SocketAssignment {
    std::map<std::string, std::vector<int>> partPins;    // Bauteil-ID -> Loch je Anschluss (Reihenfolge pins)
    std::map<std::string, std::pair<int, int>> wireEnds; // Leitungs-ID -> Loch am Anfang (von), am Ende (nach)
};
SocketAssignment assignSockets(const Board& board);
// Anzahl Steckplaetze eines Anschlusspunkts (Feld, Pultfeld, Buchse: kSocketsPerField; sonst 1)
int socketCount(const std::string& anchor);

// Farbautomatik (Konvention wie in der Elektrotechnik): Liegt ein Ende der Leitung am Pluspol der Batterie - direkt
// (P:+9V) oder ueber andere Leitungen damit verbunden -, ist sie "rot"; am Minuspol (P:0V) "schwarz"; sonst "" (die
// gewaehlte Farbe gilt). Beruehrt sie beide Pole (Kurzschluss), ebenfalls "". ignoreWire: diese Leitung nicht
// mitzaehlen (beim Umsetzen eines Endes).
std::string poleColor(const Board& board, const std::string& from, const std::string& to,
                      const std::string& ignoreWire = {});

// Prueft eine Leitung: beide Enden gueltige Anschlusspunkte, nicht dasselbe Feld.
bool canWire(const Board& board, const std::string& from, const std::string& to, Message& error);

// Umgedreht: Felder in umgekehrter Reihenfolge der Lage (z. B. LED mit Anode unten bzw. rechts).
bool isFlipped(const PartPlacement& part, const CatalogEntry& entry);
// Kehrt die Feldzuordnung um (Anschluesse bleiben, Felder tauschen die Reihenfolge); 2-x-2-Platine: halbe Drehung.
void flipPins(PartPlacement& part, const CatalogEntry& entry);

// Quadratische 2-x-2-Platine (Katalog "platine_2x2")?
bool isSquarePlate(const CatalogEntry& entry);
// Dreht eine 2-x-2-Platine um 90 Grad im Uhrzeigersinn; die vier Felder bleiben dieselben
// (oben links -> oben rechts -> unten rechts -> unten links -> oben links).
void rotatePlate(PartPlacement& part);
// Verschiebt eine 2-x-2-Platine so, dass ihr Feld oben links startField ist; Drehung bleibt erhalten.
// Liefert false, wenn ein Feld nicht mehr auf der Grundplatte liegt.
bool translatePlate(PartPlacement& part, const std::string& startField);

// ---- Module in der Modulleiste (docs/domain-model.md, Modulleiste)
// Anzahl der Felder der Modulleiste (M:1 .. M:kModuleSlots)
inline constexpr int kModuleSlots = 16;
// Prueft, ob ein Modul ab seinem Startfeld in die Modulleiste passt: Typ bekannt, alle Anschluesse innerhalb der
// Leiste, kein anderes Modul im Bereich. ignoreId: beim Verschieben das Modul selbst nicht mitzaehlen.
bool canPlaceModule(const Board& board, const Catalog& catalog, const ModulePlacement& module, Message& error,
                    const std::string& ignoreId = {});
// Freie Kennung fuer ein neues Modul, z. B. "BL1" (Blinkmodul), "TM2" (Tonmodul)
std::string nextModuleId(const Board& board, const std::string& type);
// Feld der Modulleiste unter einem Modulanschluss ("BL1.aus" -> "M:3"); "" wenn es Modul oder Anschluss nicht gibt
std::string moduleSlotOf(const Board& board, const Catalog& catalog, const std::string& modulePin);

PartPlacement* findPart(Board& board, const std::string& id);
ModulePlacement* findModule(Board& board, const std::string& id);
WirePlacement* findWire(Board& board, const std::string& id);

} // namespace volta::model

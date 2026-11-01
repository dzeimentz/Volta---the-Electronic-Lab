#pragma once

#include <string>
#include <vector>

namespace volta::model {

// Art eines Anschlusspunkts (docs/content-format.md, Anschlussbezeichner).
enum class AnchorKind {
    Invalid,
    Field,      // Kontaktfeld der Grundplatte, z. B. "B3"
    Desk,       // Pultfeld, z. B. "P:+9V"
    MeterJack,  // Buchse des Messgeraets, z. B. "MESS:COM"
    ModuleSlot, // Feld der Modulleiste, z. B. "M:3"
    ModulePin   // Modulanschluss ueber den Namen, z. B. "BL1.takt"
};

// Aufbau der Grundplatte von Volta 1: Zeilen A-E, Spalten 1-12; Pultleiste; Messgeraet; Modulleiste M1-M16.
struct BoardLayout {
    char firstRow = 'A';
    char lastRow = 'E';
    int columns = 12;
    int moduleSlots = 16;

    static const BoardLayout& volta1();
};

AnchorKind classifyAnchor(const std::string& anchor, const BoardLayout& layout = BoardLayout::volta1());

// Alle Felder der Grundplatte in Lesereihenfolge (A1, A2, ..., E12).
std::vector<std::string> boardFields(const BoardLayout& layout = BoardLayout::volta1());
// Pultfelder in Anzeigereihenfolge (P:+9V, P:0V, P:P1a, ...).
const std::vector<std::string>& deskFields();
// Pultfelder, die es erst ab Volta 2 gibt (Batterie 2: P:+B2, P:-B2; Wechselquelle: P:W~a, P:W~b; Generator: P:G,
// P:G0; Kennlinienschreiber: P:KS+, P:KS-); nur steckbar, wenn das Pultteil im Pult
// steht (Board::deskParts, Pult nach Bedarf).
const std::vector<std::string>& extraDeskFields();
// Pultteile (Gruppen der Pultleiste) in Anzeigereihenfolge: batterie, batterie2, wechselquelle, generator,
// kennlinienschreiber, p1, p2, schalter, lautsprecher
const std::vector<std::string>& deskGroupNames();
// Pultfelder eines Pultteils (leer bei unbekanntem Namen)
std::vector<std::string> deskGroupFields(const std::string& group);
// Pultteil eines Pultfelds ("P:P1s" -> "p1"); leer fuer andere Anschlusspunkte
std::string deskGroupOf(const std::string& anchor);
// Buchsen des Messgeraets (MESS:COM, MESS:VOHM, MESS:A).
const std::vector<std::string>& meterJacks();
// Feld der Modulleiste, 1-basiert: "M:<n>".
std::string moduleSlot(int n);

inline constexpr const char* kGroundAnchor = "P:0V";
inline constexpr const char* kPlusAnchor = "P:+9V";

} // namespace volta::model

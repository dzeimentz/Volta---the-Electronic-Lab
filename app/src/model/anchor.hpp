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

// Aufbau der Grundplatte von Volta 1: Zeilen A-E, Spalten 1-8; Pultleiste; Messgeraet; Modulleiste M1-M16.
struct BoardLayout {
    char firstRow = 'A';
    char lastRow = 'E';
    int columns = 8;
    int moduleSlots = 16;

    static const BoardLayout& volta1();
};

AnchorKind classifyAnchor(const std::string& anchor, const BoardLayout& layout = BoardLayout::volta1());

// Alle Felder der Grundplatte in Lesereihenfolge (A1, A2, ..., E8).
std::vector<std::string> boardFields(const BoardLayout& layout = BoardLayout::volta1());
// Pultfelder in Anzeigereihenfolge (P:+9V, P:0V, P:P1a, ...).
const std::vector<std::string>& deskFields();
// Buchsen des Messgeraets (MESS:COM, MESS:VOHM, MESS:A).
const std::vector<std::string>& meterJacks();
// Feld der Modulleiste, 1-basiert: "M:<n>".
std::string moduleSlot(int n);

inline constexpr const char* kGroundAnchor = "P:0V";
inline constexpr const char* kPlusAnchor = "P:+9V";

} // namespace volta::model

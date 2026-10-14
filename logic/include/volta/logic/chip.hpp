#pragma once

// Bausteine der Zuse-Reihe: Pegel, Grundgatter, Chip-Definitionen und die Bibliothek der bekannten Chips.
// Qt-frei (nur C++17-Standardbibliothek), siehe docs/Konzept_Zuse.md Abschnitt 8 und ADR-0010.
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace volta::logic {

// Pegel einer Leitung: 0, 1 oder unbestimmt (X: nicht angeschlossen, Konflikt, noch nicht eingeschwungen).
// Hochohmig (Z) fuer Busse folgt mit den Bustreibern.
enum class Level : std::uint8_t { Zero, One, X };

char toChar(Level level) noexcept; // '0', '1', '?'

// Grundgatter. Buf dient nur intern (Verbindung zweier Anschluesse innerhalb eines Chips).
enum class Primitive : std::uint8_t { Nand, And, Or, Not, Xor, Buf };

// Name im Bausteinformat ("NAND", "AND", "OR", "NOT", "XOR"); Buf hat keinen.
std::optional<Primitive> primitiveFromName(const std::string& name) noexcept;
const char* primitiveName(Primitive p) noexcept;
Level evaluate(Primitive p, Level a, Level b) noexcept;

enum class Direction : std::uint8_t { In, Out };

// Anschluss am Rahmen eines Chips (Z0: Breite 1 Bit; Busse mit 4 Bit folgen)
struct Port {
    std::string name;
    Direction direction = Direction::In;
};

// Ein Teil auf der Werkbank: Grundgatter ("NAND") oder ein Chip aus der Bibliothek (Kuerzel)
struct Part {
    std::string id;   // eindeutig im Chip, z. B. "n1"
    std::string type; // "NAND" oder Kuerzel eines Chips
    double x = 0.0;   // Lage auf der Werkbank (dp), nur fuer die Darstellung
    double y = 0.0;
};

// Verbindung zweier Endpunkte: Anschluss des Chips ("A") oder Anschluss eines Teils ("n1.a")
struct Connection {
    std::string from;
    std::string to;
};

struct ChipDef {
    std::string kuerzel; // z. B. "XOR"
    int version = 1;
    std::vector<Port> ports;
    std::vector<Part> parts;
    std::vector<Connection> connections;

    std::vector<std::string> inputNames() const;
    std::vector<std::string> outputNames() const;
    const Part* findPart(const std::string& id) const noexcept;
};

// Bekannte Chips: selbst gebaute und vorgegebene. Grundgatter sind immer bekannt.
class Library {
public:
    // Fuegt einen Chip hinzu oder ersetzt ihn (neue Version). Kuerzel von Grundgattern sind nicht erlaubt.
    bool add(ChipDef def);
    bool remove(const std::string& kuerzel);
    const ChipDef* find(const std::string& kuerzel) const noexcept;
    std::vector<std::string> kuerzel() const;

    // Anschlussnamen eines Typs (Grundgatter: Eingaenge "a", "b" bzw. "a", Ausgang "y")
    bool pinsOf(const std::string& type, std::vector<std::string>& inputs, std::vector<std::string>& outputs) const;

private:
    std::map<std::string, std::shared_ptr<const ChipDef>> m_chips;
};

} // namespace volta::logic

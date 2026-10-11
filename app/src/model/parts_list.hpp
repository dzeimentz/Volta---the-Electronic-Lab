#pragma once

#include <string>
#include <vector>

#include "board.hpp"

namespace volta::model {

// Teileliste eines Aufbaus (Werkzeugkasten »Teileliste«, Aufbaukarte): gleiche Bauteile mit gleichem Wert werden
// zusammengefasst, Leitungen nach Farbe gezaehlt. Reihenfolge: Module, Bauteile (nach Typ, dann Wert), Leitungen,
// Werkzeuge. Die Bezeichnungen (R1, D1 ...) stehen dabei, damit man sie auf der Aufbaukarte wiederfindet.
struct PartsLine {
    enum class Kind { Module, Part, Wire, Tool };
    Kind kind = Kind::Part;
    std::string type;  // Bauteil-/Modul-/Werkzeugtyp; Leitung: Farbe
    double value = 0.0; // Wert des Bauteils (0 = ohne)
    int count = 0;
    std::vector<std::string> ids;
};

std::vector<PartsLine> partsList(const Board& board);

} // namespace volta::model

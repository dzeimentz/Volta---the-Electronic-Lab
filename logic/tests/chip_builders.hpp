#pragma once

// Chips der ersten Zuse-Stufen aus NAND (Musterloesungen fuer die Tests)
#include <initializer_list>
#include <string>
#include <utility>

#include "volta/logic/chip.hpp"

namespace volta::logic::test {

inline ChipDef chip(std::string kuerzel, std::initializer_list<const char*> ins, std::initializer_list<const char*> outs,
                    std::initializer_list<std::pair<const char*, const char*>> parts,
                    std::initializer_list<std::pair<const char*, const char*>> wires) {
    ChipDef d;
    d.kuerzel = std::move(kuerzel);
    for (const char* n : ins) d.ports.push_back({n, Direction::In});
    for (const char* n : outs) d.ports.push_back({n, Direction::Out});
    for (const auto& p : parts) d.parts.push_back({p.first, p.second, 0.0, 0.0});
    for (const auto& w : wires) d.connections.push_back({w.first, w.second});
    return d;
}

inline ChipDef notFromNand() {
    return chip("NICHT", {"A"}, {"Y"}, {{"n1", "NAND"}}, {{"A", "n1.a"}, {"A", "n1.b"}, {"n1.y", "Y"}});
}

inline ChipDef andFromNand() { // NAND und dahinter NICHT
    return chip("UND", {"A", "B"}, {"Y"}, {{"n1", "NAND"}, {"i1", "NICHT"}},
                {{"A", "n1.a"}, {"B", "n1.b"}, {"n1.y", "i1.A"}, {"i1.Y", "Y"}});
}

inline ChipDef orFromNand() { // De Morgan: A oder B = NAND(nicht A, nicht B)
    return chip("ODER", {"A", "B"}, {"Y"}, {{"i1", "NICHT"}, {"i2", "NICHT"}, {"n1", "NAND"}},
                {{"A", "i1.A"}, {"B", "i2.A"}, {"i1.Y", "n1.a"}, {"i2.Y", "n1.b"}, {"n1.y", "Y"}});
}

inline ChipDef xorFromNand() { // klassisch aus vier NAND
    return chip("XODER", {"A", "B"}, {"Y"}, {{"n1", "NAND"}, {"n2", "NAND"}, {"n3", "NAND"}, {"n4", "NAND"}},
                {{"A", "n1.a"}, {"B", "n1.b"}, {"A", "n2.a"}, {"n1.y", "n2.b"}, {"B", "n3.a"}, {"n1.y", "n3.b"},
                 {"n2.y", "n4.a"}, {"n3.y", "n4.b"}, {"n4.y", "Y"}});
}

inline ChipDef halfAdder() {
    return chip("HA", {"A", "B"}, {"S", "C"}, {{"x1", "XODER"}, {"u1", "UND"}},
                {{"A", "x1.A"}, {"B", "x1.B"}, {"A", "u1.A"}, {"B", "u1.B"}, {"x1.Y", "S"}, {"u1.Y", "C"}});
}

inline Library stageLibrary() {
    Library lib;
    lib.add(notFromNand());
    lib.add(andFromNand());
    lib.add(orFromNand());
    lib.add(xorFromNand());
    lib.add(halfAdder());
    return lib;
}

} // namespace volta::logic::test

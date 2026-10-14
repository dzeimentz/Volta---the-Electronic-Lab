#pragma once

// Aufloesung eines Chips bis zum Gatter (flache Netzliste) mit erhaltener Hierarchie fuer »Blick hinein«.
#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "volta/logic/chip.hpp"

namespace volta::logic {

using NetId = std::size_t;

struct Gate {
    Primitive kind = Primitive::Nand;
    NetId a = 0;
    NetId b = 0; // bei NOT/Buf ohne Bedeutung
    NetId y = 0;
};

// Eine Ebene der Hierarchie: welcher Endpunkt ("A", "n1.a") an welcher Leitung haengt; Kinder je Teil-ID
struct Scope {
    std::string kuerzel;
    std::map<std::string, NetId> endpoints;
    std::map<std::string, std::size_t> children; // Teil-ID -> Index des Kind-Scopes (nur Chips, keine Gatter)
};

struct Netlist {
    std::size_t netCount = 0;
    std::vector<Gate> gates;
    std::vector<NetId> inputs;  // je Eingang des obersten Chips (Reihenfolge der Anschluesse)
    std::vector<NetId> outputs; // je Ausgang
    std::vector<Scope> scopes;  // scopes[0] = oberster Chip
};

enum class ElaborateError {
    None,
    UnknownType,     // Teil mit unbekanntem Typ
    UnknownEndpoint, // Verbindung zu einem Anschluss, den es nicht gibt
    MultipleDrivers, // zwei Ausgaenge (oder zwei Eingaenge des Chips) auf einer Leitung
    Recursion,       // Chip enthaelt sich selbst (auch ueber Umwege)
};

struct ElaborateResult {
    ElaborateError error = ElaborateError::None;
    std::string where; // betroffener Endpunkt oder Typ
    bool ok() const noexcept { return error == ElaborateError::None; }
};

// Loest def mit den Chips aus lib auf. Unverbundene Eingaenge bleiben unbestimmt (X).
ElaborateResult elaborate(const ChipDef& def, const Library& lib, Netlist& out);

// Prueft die Verbindungen eines Chips einer Ebene (ohne Aufloesung): gleiche Fehler wie elaborate.
ElaborateResult validate(const ChipDef& def, const Library& lib);

} // namespace volta::logic

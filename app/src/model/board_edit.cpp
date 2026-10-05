#include "board_edit.hpp"

#include <algorithm>
#include <map>
#include <set>

#include "anchor.hpp"

namespace volta::model {

namespace {

bool parseField(const std::string& f, char& row, int& col) {
    if (classifyAnchor(f) != AnchorKind::Field) return false;
    row = f[0];
    col = std::stoi(f.substr(1));
    return true;
}

std::string fieldName(char row, int col) { return std::string(1, row) + std::to_string(col); }

bool allowsSpan(const CatalogEntry& e, const std::string& name) {
    return e.spans.empty() || std::find(e.spans.begin(), e.spans.end(), name) != e.spans.end();
}

bool isCarrier(const CatalogEntry& e) {
    return std::find(e.spans.begin(), e.spans.end(), "traeger") != e.spans.end() || e.pins.size() == 3;
}

// Kennungs-Praefixe nach Bauteiltyp (Schaltzeichen-Konvention, kurz und eindeutig)
std::string prefixFor(const std::string& type) {
    static const std::map<std::string, std::string> prefixes = {
        {"widerstand", "R"},  {"potentiometer", "P"}, {"gluehlampe", "L"},     {"taster", "S"},
        {"schalter", "S"},    {"reedkontakt", "RK"},  {"wechselschalter", "W"}, {"motor", "M"},
        {"summer", "SU"},     {"kondensator", "C"},   {"elko", "C"},           {"transistor_npn", "T"},
        {"diode_si", "D"},    {"diode_ge", "D"},      {"fotowiderstand", "LDR"}};
    const auto it = prefixes.find(type);
    if (it != prefixes.end()) return it->second;
    if (type.rfind("led_", 0) == 0) return "D";
    return "X";
}

} // namespace

std::vector<std::string> footprint(const CatalogEntry& entry, const std::string& startField, Orientation o, Span span) {
    char row = 0;
    int col = 0;
    if (!parseField(startField, row, col)) return {};
    const BoardLayout& layout = BoardLayout::volta1();
    const int steps = isCarrier(entry) ? 2 : static_cast<int>(span);
    const int count = isCarrier(entry) ? 3 : 2;
    std::vector<std::string> fields;
    for (int k = 0; k < count; ++k) {
        const int dist = isCarrier(entry) ? k : k * steps;
        const char r = o == Orientation::Vertical ? static_cast<char>(row + dist) : row;
        const int c = o == Orientation::Horizontal ? col + dist : col;
        if (r > layout.lastRow || c > layout.columns) return {};
        fields.push_back(fieldName(r, c));
    }
    return fields;
}

bool placePins(const CatalogEntry& entry, const std::string& startField, Orientation o, Span span,
               std::vector<std::pair<std::string, std::string>>& pins, std::string& error) {
    pins.clear();
    if (!isCarrier(entry)) {
        if (!allowsSpan(entry, span == Span::Short ? "kurz" : "lang")) {
            error = entry.type + ": Spannweite nicht moeglich";
            return false;
        }
    }
    const auto fields = footprint(entry, startField, o, span);
    if (fields.empty()) {
        error = entry.type + ": passt nicht auf die Grundplatte";
        return false;
    }
    const auto& order = entry.layout.empty() ? entry.pins : entry.layout;
    if (order.size() != fields.size()) {
        error = entry.type + ": Anschlusszahl passt nicht zur Lage";
        return false;
    }
    // Belegung in Katalogreihenfolge der Anschluesse
    for (const auto& pinName : entry.pins) {
        const auto pos = std::find(order.begin(), order.end(), pinName) - order.begin();
        if (pos >= static_cast<long>(fields.size())) {
            error = entry.type + ": Anschluss '" + pinName + "' fehlt in der Lage";
            return false;
        }
        pins.emplace_back(pinName, fields[static_cast<std::size_t>(pos)]);
    }
    return true;
}

std::string nextPartId(const Board& board, const std::string& type) {
    const std::string prefix = prefixFor(type);
    std::set<int> used;
    for (const auto& p : board.parts) {
        if (p.id.size() > prefix.size() && p.id.compare(0, prefix.size(), prefix) == 0) {
            const std::string rest = p.id.substr(prefix.size());
            if (std::all_of(rest.begin(), rest.end(), [](char ch) { return ch >= '0' && ch <= '9'; }))
                used.insert(std::stoi(rest));
        }
    }
    int n = 1;
    while (used.count(n)) ++n;
    return prefix + std::to_string(n);
}

std::string nextWireId(const Board& board) {
    std::set<int> used;
    for (const auto& w : board.wires)
        if (w.id.size() > 1 && w.id[0] == 'L' &&
            std::all_of(w.id.begin() + 1, w.id.end(), [](char ch) { return ch >= '0' && ch <= '9'; }))
            used.insert(std::stoi(w.id.substr(1)));
    int n = 1;
    while (used.count(n)) ++n;
    return "L" + std::to_string(n);
}

int usedSockets(const Board& board, const std::string& field, const std::string& ignoreId) {
    int n = 0;
    for (const auto& p : board.parts) {
        if (p.id == ignoreId) continue;
        for (const auto& pin : p.pins)
            if (pin.second == field) ++n;
    }
    return n;
}

bool canPlace(const Board& board, const Catalog& catalog, const PartPlacement& part, std::string& error,
              const std::string& ignoreId) {
    const CatalogEntry* e = catalog.part(part.type);
    if (!e) {
        error = "unbekannter Bauteiltyp '" + part.type + "'";
        return false;
    }
    if (part.pins.size() != e->pins.size()) {
        error = part.id + ": Anschluesse unvollstaendig";
        return false;
    }
    std::map<std::string, int> needed;
    for (const auto& pin : part.pins) {
        if (classifyAnchor(pin.second) != AnchorKind::Field) {
            error = part.id + ": Bauteile stecken nur auf Feldern der Grundplatte";
            return false;
        }
        ++needed[pin.second];
    }
    if (needed.size() != part.pins.size()) {
        error = part.id + ": zwei Beine im selben Feld";
        return false;
    }
    for (const auto& [field, count] : needed) {
        if (usedSockets(board, field, ignoreId.empty() ? part.id : ignoreId) + count > kSocketsPerField) {
            error = "Feld " + field + " ist voll";
            return false;
        }
    }
    for (const auto& p : board.parts)
        if (p.id == part.id && p.id != ignoreId) {
            error = "Kennung " + part.id + " schon vergeben";
            return false;
        }
    return true;
}

bool canWire(const Board& board, const std::string& from, const std::string& to, std::string& error) {
    auto valid = [&board](const std::string& a) {
        const AnchorKind k = classifyAnchor(a);
        if (k == AnchorKind::Invalid) return false;
        if (k != AnchorKind::ModulePin) return true;
        const std::string id = a.substr(0, a.find('.'));
        return std::any_of(board.modules.begin(), board.modules.end(), [&id](const auto& m) { return m.id == id; });
    };
    if (!valid(from) || !valid(to)) {
        error = "Leitung: ungueltiger Anschlusspunkt";
        return false;
    }
    if (from == to) {
        error = "Leitung: Anfang und Ende im selben Feld";
        return false;
    }
    return true;
}

namespace {
// Felder in Lage-Reihenfolge (Traegerplatine: "lage", sonst Katalogreihenfolge der Anschluesse)
std::vector<std::string> fieldsInLayoutOrder(const PartPlacement& part, const CatalogEntry& entry) {
    const auto& order = entry.layout.empty() ? entry.pins : entry.layout;
    std::vector<std::string> fields;
    for (const auto& name : order)
        for (const auto& pin : part.pins)
            if (pin.first == name) fields.push_back(pin.second);
    return fields;
}
bool fieldBefore(const std::string& a, const std::string& b) {
    if (a[0] != b[0]) return a[0] < b[0];
    return std::stoi(a.substr(1)) < std::stoi(b.substr(1));
}
} // namespace

bool isFlipped(const PartPlacement& part, const CatalogEntry& entry) {
    const auto fields = fieldsInLayoutOrder(part, entry);
    return fields.size() >= 2 && classifyAnchor(fields.front()) == AnchorKind::Field &&
           classifyAnchor(fields.back()) == AnchorKind::Field && fieldBefore(fields.back(), fields.front());
}

void flipPins(PartPlacement& part, const CatalogEntry& entry) {
    auto fields = fieldsInLayoutOrder(part, entry);
    std::reverse(fields.begin(), fields.end());
    const auto& order = entry.layout.empty() ? entry.pins : entry.layout;
    for (std::size_t k = 0; k < order.size() && k < fields.size(); ++k)
        for (auto& pin : part.pins)
            if (pin.first == order[k]) pin.second = fields[k];
}

PartPlacement* findPart(Board& board, const std::string& id) {
    for (auto& p : board.parts)
        if (p.id == id) return &p;
    return nullptr;
}

WirePlacement* findWire(Board& board, const std::string& id) {
    for (auto& w : board.wires)
        if (w.id == id) return &w;
    return nullptr;
}

} // namespace volta::model

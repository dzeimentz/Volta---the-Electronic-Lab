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

// Quadratische Platine ueber 2 x 2 Feldern (Transistor: B | C oben, E | E unten)
bool isSquareCarrier(const CatalogEntry& e) {
    return std::find(e.spans.begin(), e.spans.end(), "platine_2x2") != e.spans.end();
}

// Traegerplatine ueber drei Felder in einer Reihe (Wechselschalter)
bool isCarrier(const CatalogEntry& e) {
    return !isSquareCarrier(e) &&
           (std::find(e.spans.begin(), e.spans.end(), "traeger") != e.spans.end() || e.pins.size() == 3);
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
    if (isSquareCarrier(entry)) { // Lage fest: Startfeld oben links; Richtung und Spannweite ohne Bedeutung
        if (row + 1 > layout.lastRow || col + 1 > layout.columns) return {};
        return {fieldName(row, col), fieldName(row, col + 1), fieldName(static_cast<char>(row + 1), col),
                fieldName(static_cast<char>(row + 1), col + 1)};
    }
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
               std::vector<std::pair<std::string, std::string>>& pins, Message& error) {
    pins.clear();
    if (!isCarrier(entry) && !isSquareCarrier(entry)) {
        if (!allowsSpan(entry, span == Span::Short ? "kurz" : "lang")) {
            error = msg(QT_TRANSLATE_NOOP("Meldung", "So weit lassen sich die Beine dieses Bauteils nicht spreizen."));
            return false;
        }
    }
    const auto fields = footprint(entry, startField, o, span);
    if (fields.empty()) {
        error = msg(QT_TRANSLATE_NOOP("Meldung", "Das Bauteil passt dort nicht auf die Grundplatte."));
        return false;
    }
    const auto& order = entry.layout.empty() ? entry.pins : entry.layout;
    if (order.size() != fields.size()) {
        error = msg(QT_TRANSLATE_NOOP("Meldung", "Die Anschlüsse passen nicht zur Lage des Bauteils."));
        return false;
    }
    // Belegung in Katalogreihenfolge der Anschluesse
    for (const auto& pinName : entry.pins) {
        const auto pos = std::find(order.begin(), order.end(), pinName) - order.begin();
        if (pos >= static_cast<long>(fields.size())) {
            error = msg(QT_TRANSLATE_NOOP("Meldung", "Anschluss %1 fehlt in der Lage des Bauteils."), {pinName});
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

bool withinBoard(const Board& board, const std::string& anchor) {
    char row = 0;
    int col = 0;
    return !parseField(anchor, row, col) || col <= board.columns;
}

int highestUsedColumn(const Board& board) {
    int used = 0;
    auto note = [&used](const std::string& anchor) {
        char row = 0;
        int col = 0;
        if (parseField(anchor, row, col)) used = std::max(used, col);
    };
    for (const auto& p : board.parts)
        for (const auto& pin : p.pins) note(pin.second);
    for (const auto& w : board.wires) {
        note(w.from);
        note(w.to);
    }
    return used;
}

int socketCount(const std::string& anchor) {
    switch (classifyAnchor(anchor)) {
    case AnchorKind::Field:
    case AnchorKind::Desk:
    case AnchorKind::MeterJack:
        return kSocketsPerField;
    default:
        return 1;
    }
}

namespace {

// Spalte eines Felds der Grundplatte, 0 fuer andere Anschlusspunkte
int columnOf(const std::string& anchor) {
    char row = 0;
    int col = 0;
    return parseField(anchor, row, col) ? col : 0;
}

// Bevorzugte Loecher nach Richtung (-1 links, 0 Mitte, +1 rechts); fuer Bauteile nah am Partner, fuer Leitungen
// von aussen
std::vector<int> preference(int direction, bool wire) {
    static_assert(kSocketsPerField == 5, "Vorzugsreihenfolgen sind fuer fuenf Steckplaetze festgelegt");
    if (direction > 0) return wire ? std::vector<int>{4, 3, 2, 1, 0} : std::vector<int>{3, 4, 2, 1, 0};
    if (direction < 0) return wire ? std::vector<int>{0, 1, 2, 3, 4} : std::vector<int>{1, 0, 2, 3, 4};
    return {2, 1, 3, 0, 4};
}

int direction(const std::string& from, const std::string& to) {
    const int a = columnOf(from);
    const int b = columnOf(to);
    if (a == 0 || b == 0 || a == b) return 0;
    return b > a ? 1 : -1;
}

} // namespace

SocketAssignment assignSockets(const Board& board) {
    SocketAssignment out;
    std::map<std::string, std::vector<int>> use; // Anschlusspunkt -> Belegung je Loch
    auto take = [&use](const std::string& anchor, const std::vector<int>& order) {
        const int n = socketCount(anchor);
        if (n == 1) return 0;
        std::vector<int>& u = use[anchor];
        u.resize(static_cast<std::size_t>(n), 0);
        int best = order.front();
        for (int s : order) {
            if (u[static_cast<std::size_t>(s)] == 0) {
                best = s;
                break;
            }
            if (u[static_cast<std::size_t>(s)] < u[static_cast<std::size_t>(best)]) best = s;
        }
        ++u[static_cast<std::size_t>(best)];
        return best;
    };

    for (const auto& p : board.parts) {
        std::vector<int>& sockets = out.partPins[p.id];
        for (std::size_t i = 0; i < p.pins.size(); ++i) {
            // Richtung zum Schwerpunkt der uebrigen Anschluesse (Traegerplatine: Mitte bleibt Mitte)
            int sum = 0;
            int cnt = 0;
            for (std::size_t k = 0; k < p.pins.size(); ++k) {
                if (k == i) continue;
                sum += direction(p.pins[i].second, p.pins[k].second);
                ++cnt;
            }
            const int dir = cnt == 0 || sum == 0 ? 0 : (sum > 0 ? 1 : -1);
            sockets.push_back(take(p.pins[i].second, preference(dir, false)));
        }
    }
    for (const auto& w : board.wires) {
        const int a = take(w.from, preference(direction(w.from, w.to), true));
        const int b = take(w.to, preference(direction(w.to, w.from), true));
        out.wireEnds[w.id] = {a, b};
    }
    return out;
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

bool canPlace(const Board& board, const Catalog& catalog, const PartPlacement& part, Message& error,
              const std::string& ignoreId) {
    const CatalogEntry* e = catalog.part(part.type);
    if (!e) {
        error = msg(QT_TRANSLATE_NOOP("Meldung", "Unbekanntes Bauteil „%1“."), {part.type});
        return false;
    }
    if (part.pins.size() != e->pins.size()) {
        error = msg(QT_TRANSLATE_NOOP("Meldung", "%1: Die Anschlüsse sind unvollständig."), {part.id});
        return false;
    }
    std::map<std::string, int> needed;
    for (const auto& pin : part.pins) {
        if (classifyAnchor(pin.second) != AnchorKind::Field) {
            error = msg(QT_TRANSLATE_NOOP("Meldung", "Bauteile stecken nur auf Feldern der Grundplatte."));
            return false;
        }
        if (!withinBoard(board, pin.second)) {
            error = msg(QT_TRANSLATE_NOOP("Meldung", "Feld %1 liegt außerhalb der Grundplatte (Felder 1–%2)."), {pin.second, std::to_string(board.columns)});
            return false;
        }
        ++needed[pin.second];
    }
    if (needed.size() != part.pins.size()) {
        error = msg(QT_TRANSLATE_NOOP("Meldung", "Zwei Beine dürfen nicht im selben Feld stecken."));
        return false;
    }
    for (const auto& [field, count] : needed) {
        if (usedSockets(board, field, ignoreId.empty() ? part.id : ignoreId) + count > kSocketsPerField) {
            error = msg(QT_TRANSLATE_NOOP("Meldung", "Feld %1 ist voll."), {field});
            return false;
        }
    }
    for (const auto& p : board.parts)
        if (p.id == part.id && p.id != ignoreId) {
            error = msg(QT_TRANSLATE_NOOP("Meldung", "Die Kennung %1 ist schon vergeben."), {part.id});
            return false;
        }
    return true;
}

bool canWire(const Board& board, const std::string& from, const std::string& to, Message& error) {
    auto valid = [&board](const std::string& a) {
        const AnchorKind k = classifyAnchor(a);
        if (k == AnchorKind::Invalid || !withinBoard(board, a)) return false;
        if (k == AnchorKind::MeterJack && !board.desk.meterVisible) return false; // Messgeraet ausgeblendet
        if (k != AnchorKind::ModulePin) return true;
        const std::string id = a.substr(0, a.find('.'));
        return std::any_of(board.modules.begin(), board.modules.end(), [&id](const auto& m) { return m.id == id; });
    };
    if (!valid(from) || !valid(to)) {
        error = msg(QT_TRANSLATE_NOOP("Meldung", "Dort kann keine Leitung stecken."));
        return false;
    }
    if (from == to) {
        error = msg(QT_TRANSLATE_NOOP("Meldung", "Anfang und Ende der Leitung liegen im selben Feld."));
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

bool isSquarePlate(const CatalogEntry& entry) { return isSquareCarrier(entry); }

namespace {
// Feld oben links eines Blocks (kleinste Zeile, kleinste Spalte)
bool plateOrigin(const PartPlacement& part, char& row, int& col) {
    row = 'Z';
    col = 1000;
    for (const auto& pin : part.pins) {
        char r = 0;
        int c = 0;
        if (!parseField(pin.second, r, c)) return false;
        row = std::min(row, r);
        col = std::min(col, c);
    }
    return !part.pins.empty();
}
} // namespace

void rotatePlate(PartPlacement& part) {
    char r0 = 0;
    int c0 = 0;
    if (!plateOrigin(part, r0, c0)) return;
    for (auto& pin : part.pins) {
        char r = 0;
        int c = 0;
        parseField(pin.second, r, c);
        const int dr = r - r0;
        const int dc = c - c0;
        // im Uhrzeigersinn: (Zeile, Spalte) -> (Spalte, 1 - Zeile) innerhalb des 2-x-2-Blocks
        pin.second = fieldName(static_cast<char>(r0 + dc), c0 + (1 - dr));
    }
}

bool translatePlate(PartPlacement& part, const std::string& startField) {
    char r0 = 0;
    int c0 = 0;
    char rs = 0;
    int cs = 0;
    if (!plateOrigin(part, r0, c0) || !parseField(startField, rs, cs)) return false;
    for (auto& pin : part.pins) {
        char r = 0;
        int c = 0;
        parseField(pin.second, r, c);
        const char nr = static_cast<char>(r - r0 + rs);
        const int nc = c - c0 + cs;
        pin.second = fieldName(nr, nc);
        if (classifyAnchor(pin.second) != AnchorKind::Field) return false;
    }
    return true;
}

std::string poleColor(const Board& board, const std::string& from, const std::string& to,
                      const std::string& ignoreWire) {
    // Felder, die ueber Leitungen mit dem Pol verbunden sind (Bauteile leiten hier nicht: nur "direkt" verbunden)
    auto reach = [&](const std::string& pole) {
        std::set<std::string> seen{pole};
        bool grew = true;
        while (grew) {
            grew = false;
            for (const auto& w : board.wires) {
                if (w.id == ignoreWire) continue;
                const bool a = seen.count(w.from) > 0, b = seen.count(w.to) > 0;
                if (a != b) {
                    seen.insert(a ? w.to : w.from);
                    grew = true;
                }
            }
        }
        return seen;
    };
    const auto plus = reach(kPlusAnchor);
    const auto minus = reach(kGroundAnchor);
    const bool toPlus = plus.count(from) || plus.count(to);
    const bool toMinus = minus.count(from) || minus.count(to);
    if (toPlus && !toMinus) return "rot";
    if (toMinus && !toPlus) return "schwarz";
    return {};
}

bool canPlaceModule(const Board& board, const Catalog& catalog, const ModulePlacement& module, Message& error,
                    const std::string& ignoreId) {
    const ModuleEntry* e = catalog.module(module.type);
    if (!e || e->pins.empty()) {
        error = msg(QT_TRANSLATE_NOOP("Meldung", "Unbekanntes Modul „%1“."), {module.type});
        return false;
    }
    const int width = static_cast<int>(e->pins.size());
    if (module.slot < 1 || module.slot + width - 1 > kModuleSlots) {
        error = msg(QT_TRANSLATE_NOOP("Meldung", "Das Modul passt dort nicht in die Modulleiste."));
        return false;
    }
    for (const auto& m : board.modules) {
        if (m.id == ignoreId) continue;
        const ModuleEntry* o = catalog.module(m.type);
        const int ow = o ? static_cast<int>(o->pins.size()) : 1;
        if (module.slot <= m.slot + ow - 1 && m.slot <= module.slot + width - 1) {
            error = msg(QT_TRANSLATE_NOOP("Meldung", "Dort steckt schon ein Modul."));
            return false;
        }
    }
    return true;
}

std::string nextModuleId(const Board& board, const std::string& type) {
    const std::string prefix = type == "blinkmodul" ? "BL" : (type == "tonmodul" ? "TM" : "MOD");
    std::set<int> used;
    for (const auto& m : board.modules) {
        if (m.id.size() > prefix.size() && m.id.compare(0, prefix.size(), prefix) == 0) {
            const std::string rest = m.id.substr(prefix.size());
            if (std::all_of(rest.begin(), rest.end(), [](char ch) { return ch >= '0' && ch <= '9'; }))
                used.insert(std::stoi(rest));
        }
    }
    int n = 1;
    while (used.count(n)) ++n;
    return prefix + std::to_string(n);
}

std::string moduleSlotOf(const Board& board, const Catalog& catalog, const std::string& modulePin) {
    const auto dot = modulePin.find('.');
    if (dot == std::string::npos) return {};
    const std::string id = modulePin.substr(0, dot);
    const std::string pin = modulePin.substr(dot + 1);
    for (const auto& m : board.modules) {
        if (m.id != id) continue;
        const ModuleEntry* e = catalog.module(m.type);
        if (!e) return {};
        const auto it = std::find(e->pins.begin(), e->pins.end(), pin);
        if (it == e->pins.end()) return {};
        return moduleSlot(m.slot + static_cast<int>(it - e->pins.begin()));
    }
    return {};
}

PartPlacement* findPart(Board& board, const std::string& id) {
    for (auto& p : board.parts)
        if (p.id == id) return &p;
    return nullptr;
}

ModulePlacement* findModule(Board& board, const std::string& id) {
    for (auto& m : board.modules)
        if (m.id == id) return &m;
    return nullptr;
}

WirePlacement* findWire(Board& board, const std::string& id) {
    for (auto& w : board.wires)
        if (w.id == id) return &w;
    return nullptr;
}

} // namespace volta::model

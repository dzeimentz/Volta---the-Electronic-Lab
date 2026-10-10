#include "circuit_check.hpp"

#include <algorithm>
#include <cmath>
#include <deque>
#include <map>
#include <set>

#include "anchor.hpp"
#include "board_edit.hpp"
#include "netlist_builder.hpp"

namespace volta::model {

namespace {

constexpr const char* kPlus = "P:+9V";
constexpr const char* kMinus = "P:0V";

// Vereinigungsmenge ueber Anschlusspunkte (Leitungen verbinden zwei Punkte zu einem Knoten)
class Nodes {
public:
    const std::string& find(const std::string& a) {
        auto it = m_parent.find(a);
        if (it == m_parent.end()) it = m_parent.emplace(a, a).first;
        if (it->second == a) return it->first;
        const std::string root = find(it->second);
        m_parent[a] = root;
        return m_parent.find(root)->first;
    }
    void join(const std::string& a, const std::string& b) {
        const std::string ra = find(a), rb = find(b);
        if (ra != rb) m_parent[ra] = rb;
    }
    // alle bekannten Punkte eines Knotens
    std::vector<std::string> members(const std::string& root) {
        std::vector<std::string> out;
        std::vector<std::string> keys;
        for (const auto& [k, v] : m_parent) keys.push_back(k);
        for (const auto& k : keys)
            if (find(k) == root) out.push_back(k);
        return out;
    }

private:
    std::map<std::string, std::string> m_parent;
};

// Anschluss eines Bauteils, Moduls oder des Pults an einem Knoten
struct Terminal {
    std::string owner; // Bauteil-/Modul-ID oder Pultgruppe
    std::string pin;
};

struct Edge {
    std::string to;
    std::string owner;
};

bool isSwitch(const CatalogEntry* e) { return e && e->model == "switch"; }
bool isChangeover(const CatalogEntry* e) { return e && e->model == "changeover_switch"; }

} // namespace

std::vector<CircuitFinding> checkCircuit(const CircuitCheckInput& in) {
    const Board& board = in.board;
    const Catalog& catalog = in.catalog;
    Nodes nodes;
    auto canon = [&](const std::string& a) -> std::string {
        if (classifyAnchor(a) == AnchorKind::ModulePin) {
            const std::string slot = moduleSlotOf(board, catalog, a);
            return slot.empty() ? a : slot;
        }
        return a;
    };

    // ---- Knoten: Leitungen verbinden ihre Enden
    for (const auto& d : deskFields()) nodes.find(d);
    for (const auto& w : board.wires) nodes.join(canon(w.from), canon(w.to));

    // ---- Anschluesse je Knoten
    std::map<std::string, std::vector<Terminal>> terminals;
    auto addTerminal = [&](const std::string& anchor, const std::string& owner, const std::string& pin) {
        terminals[nodes.find(anchor)].push_back({owner, pin});
    };
    for (const auto& p : board.parts)
        for (const auto& [pin, field] : p.pins) addTerminal(field, p.id, pin);
    for (const auto& m : board.modules)
        if (const ModuleEntry* e = catalog.module(m.type))
            for (std::size_t i = 0; i < e->pins.size(); ++i)
                addTerminal(moduleSlot(m.slot + static_cast<int>(i)), m.id, e->pins[i]);
    for (const auto& d : deskFields()) addTerminal(d, "PULT", d);
    if (board.desk.meterVisible)
        for (const auto& j : meterJacks()) addTerminal(j, "MESS", j);

    // ---- Kanten: leitende Verbindungen durch Bauteile, Module und Pult (Batterie zaehlt nicht)
    std::map<std::string, std::vector<Edge>> edges;
    auto connect = [&](const std::string& a, const std::string& b, const std::string& owner) {
        const std::string ra = nodes.find(a), rb = nodes.find(b);
        edges[ra].push_back({rb, owner});
        edges[rb].push_back({ra, owner});
    };
    auto pinField = [](const PartPlacement& p, const std::string& name) -> std::string {
        for (const auto& [pin, field] : p.pins)
            if (pin == name) return field;
        return {};
    };
    for (const auto& p : board.parts) {
        const CatalogEntry* e = catalog.part(p.type);
        if (p.pins.size() < 2) continue;
        if (isSwitch(e)) {
            if (p.closed) connect(p.pins[0].second, p.pins[1].second, p.id);
        } else if (isChangeover(e)) {
            connect(pinField(p, "mitte"), pinField(p, p.closed ? "b" : "a"), p.id);
        } else {
            for (std::size_t i = 1; i < p.pins.size(); ++i) connect(p.pins[0].second, p.pins[i].second, p.id);
        }
    }
    for (const auto& m : board.modules)
        if (const ModuleEntry* e = catalog.module(m.type))
            for (std::size_t i = 1; i < e->pins.size(); ++i)
                connect(moduleSlot(m.slot), moduleSlot(m.slot + static_cast<int>(i)), m.id);
    connect("P:P1a", "P:P1s", "PULT.P1");
    connect("P:P1s", "P:P1b", "PULT.P1");
    connect("P:P2a", "P:P2s", "PULT.P2");
    connect("P:P2s", "P:P2b", "PULT.P2");
    if (board.desk.switchClosed) connect("P:S1", "P:S2", "PULT.S");
    connect("P:LS1", "P:LS2", "PULT.LS");
    if (board.desk.meterVisible) {
        connect("MESS:COM", "MESS:VOHM", "MESS");
        connect("MESS:COM", "MESS:A", "MESS");
    }

    std::vector<CircuitFinding> shorts, broken, reversed, noResistor, overload, bridged, dangling, looseWires, noSupply,
        openSwitch, deadEnd, mainSwitch;
    const std::string plus = nodes.find(kPlus), minus = nodes.find(kMinus);
    const bool anyPart = !board.parts.empty() || !board.modules.empty();

    // ---- Kurzschluss ueber Leitungen
    if (plus == minus)
        shorts.push_back({CircuitFinding::Kind::Short, "", {kPlus, kMinus},
                          msg(QT_TRANSLATE_NOOP("Meldung", "Fließt der Strom hier von +9V direkt nach 0V, ohne durch ein "
                                                           "Bauteil zu gehen?"))});

    // ---- Bauteile mit ueberbrueckten oder losen Beinen
    for (const auto& p : board.parts) {
        const CatalogEntry* e = catalog.part(p.type);
        const std::string model = e ? e->model : std::string();
        if (p.pins.size() == 2 && nodes.find(p.pins[0].second) == nodes.find(p.pins[1].second) && plus != minus)
            bridged.push_back({CircuitFinding::Kind::Bridged, p.id, {p.pins[0].second, p.pins[1].second},
                               msg(QT_TRANSLATE_NOOP("Meldung", "Beide Beine von %1 sind direkt miteinander verbunden. "
                                                                "Kann der Strom dann überhaupt durch %1 fließen?"),
                                   {p.id})});
        // ein Bein ist lose, wenn an seinem Knoten sonst nichts angeschlossen ist
        auto isLoose = [&](const std::string& pin) {
            const std::string f = pinField(p, pin);
            return !f.empty() && terminals[nodes.find(f)].size() <= 1;
        };
        std::vector<std::string> report;
        if (model == "bjt_npn") {
            for (const char* pin : {"b", "c"})
                if (isLoose(pin)) report.push_back(pin);
            if (isLoose("e") && isLoose("e2")) report.push_back("e");
        } else if (model == "relay") {
            if (isLoose("a") != isLoose("b")) report.push_back(isLoose("a") ? "a" : "b"); // Spule halb angeschlossen
            if (isLoose("com") != isLoose("no")) report.push_back(isLoose("com") ? "com" : "no");
        } else if (model == "changeover_switch") {
            if (isLoose("mitte")) report.push_back("mitte");
            else if (isLoose("a") && isLoose("b")) report.push_back("a");
        } else {
            for (const auto& [pin, field] : p.pins)
                if (isLoose(pin)) report.push_back(pin);
        }
        for (const auto& pin : report)
            dangling.push_back({CircuitFinding::Kind::Dangling, p.id, {pinField(p, pin)},
                                msg(QT_TRANSLATE_NOOP("Meldung", "Das Bein von %1 auf Feld %2 ist mit nichts verbunden. "
                                                                 "Wohin soll der Strom von dort fließen?"),
                                    {p.id, pinField(p, pin)})});
    }

    // ---- Leitungen, die nichts verbinden
    for (const auto& w : board.wires)
        if (terminals[nodes.find(canon(w.from))].empty())
            looseWires.push_back({CircuitFinding::Kind::LooseWire, "", {w.from, w.to},
                             msg(QT_TRANSLATE_NOOP("Meldung", "Die Leitung von %1 nach %2 verbindet nichts. Wo soll sie "
                                                              "hinführen?"),
                                 {w.from, w.to})});

    // ---- offener Kreis: vom Pluspol ueber Bauteile bis zum Minuspol?
    auto reaches = [&](const std::string& extraA, const std::string& extraB) {
        std::set<std::string> seen{plus};
        std::deque<std::string> todo{plus};
        while (!todo.empty()) {
            const std::string n = todo.front();
            todo.pop_front();
            std::vector<std::string> next;
            for (const auto& ed : edges[n]) next.push_back(ed.to);
            if (!extraA.empty() && n == extraA) next.push_back(extraB);
            if (!extraA.empty() && n == extraB) next.push_back(extraA);
            for (const auto& t : next)
                if (seen.insert(t).second) todo.push_back(t);
        }
        return seen;
    };
    if (anyPart && plus != minus) {
        const std::set<std::string> fromPlus = reaches("", "");
        const bool closed = fromPlus.count(minus) > 0;
        if (closed) {
            if (!board.desk.mainSwitch)
                mainSwitch.push_back({CircuitFinding::Kind::MainSwitchOff, "", {kPlus, kMinus},
                                      msg(QT_TRANSLATE_NOOP("Meldung", "Ist der Hauptschalter an der Batterie "
                                                                       "eingeschaltet?"))});
        } else {
            // frei: am Pol ist ausser dem Pult selbst nichts angeschlossen (ein offener Schalter zaehlt als angeschlossen)
            const bool plusFree = terminals[plus].size() <= 1, minusFree = terminals[minus].size() <= 1;
            // Widerstandsmessung: das Messgeraet ist selbst die Quelle, die Batterie wird nicht gebraucht
            const bool ohmOnly = plusFree && minusFree && board.desk.meterVisible &&
                                 board.desk.meterMode == core::MeterMode::Ohm;
            if (plusFree && !ohmOnly)
                noSupply.push_back({CircuitFinding::Kind::NoSupply, "", {kPlus},
                                    msg(QT_TRANSLATE_NOOP("Meldung", "Ist dein Aufbau mit dem Pluspol +9V verbunden?"))});
            if (minusFree && !ohmOnly)
                noSupply.push_back({CircuitFinding::Kind::NoSupply, "", {kMinus},
                                    msg(QT_TRANSLATE_NOOP("Meldung", "Ist dein Aufbau mit dem Minuspol 0V verbunden?"))});
            if (!ohmOnly && !plusFree && !minusFree) {
                // Wuerde ein offener Schalter den Kreis schliessen?
                for (const auto& p : board.parts) {
                    const CatalogEntry* e = catalog.part(p.type);
                    std::string a, b;
                    if (isSwitch(e) && !p.closed && p.pins.size() >= 2) {
                        a = p.pins[0].second;
                        b = p.pins[1].second;
                    } else if (isChangeover(e)) {
                        a = pinField(p, "mitte");
                        b = pinField(p, p.closed ? "a" : "b");
                    }
                    if (a.empty() || !reaches(nodes.find(a), nodes.find(b)).count(minus)) continue;
                    const bool button = p.type == "taster";
                    openSwitch.push_back(
                        {CircuitFinding::Kind::OpenSwitch, p.id, {a, b},
                         button ? msg(QT_TRANSLATE_NOOP("Meldung", "Drückst du den Taster %1? Erst dann ist der Kreis "
                                                                   "geschlossen."),
                                      {p.id})
                                : msg(QT_TRANSLATE_NOOP("Meldung", "Ist %1 in der richtigen Stellung? Dort kommt der "
                                                                   "Strom gerade nicht weiter."),
                                      {p.id})});
                }
                if (!board.desk.switchClosed && reaches(nodes.find("P:S1"), nodes.find("P:S2")).count(minus))
                    openSwitch.push_back({CircuitFinding::Kind::OpenSwitch, "", {"P:S1", "P:S2"},
                                          msg(QT_TRANSLATE_NOOP("Meldung", "Ist der Schalter S am Pult "
                                                                           "eingeschaltet?"))});
                if (openSwitch.empty()) {
                    // Sackgassen: vom Pluspol erreichbare Knoten, von denen es nicht weitergeht
                    for (const auto& n : fromPlus) {
                        if (n == plus || edges[n].size() > 1) continue;
                        std::vector<std::string> fields;
                        for (const auto& mbr : nodes.members(n))
                            if (classifyAnchor(mbr) == AnchorKind::Field || classifyAnchor(mbr) == AnchorKind::ModuleSlot)
                                fields.push_back(mbr);
                        if (fields.empty()) continue;
                        std::sort(fields.begin(), fields.end());
                        deadEnd.push_back({CircuitFinding::Kind::DeadEnd, "", fields,
                                           msg(QT_TRANSLATE_NOOP("Meldung", "Von Feld %1 aus geht es nicht weiter. Wie "
                                                                            "kommt der Strom von hier zurück zum "
                                                                            "Minuspol (0V)?"),
                                               {fields.front()})});
                        if (deadEnd.size() >= 2) break;
                    }
                }
            }
        }
    }

    // ---- laufende Simulation
    if (board.desk.mainSwitch && in.reading) {
        if (const core::ComponentReading* bat = in.reading(desk_id::kBattery);
            bat && shorts.empty() && std::fabs(bat->currentA) > 2.0)
            shorts.push_back({CircuitFinding::Kind::Short, "", {kPlus, kMinus},
                              msg(QT_TRANSLATE_NOOP("Meldung", "Aus der Batterie fließt sehr viel Strom. Gibt es einen "
                                                               "Weg von +9V nach 0V, auf dem kein Bauteil den Strom "
                                                               "bremst?"))});
        for (const auto& p : board.parts) {
            const core::ComponentReading* r = in.reading(p.id);
            if (!r) continue;
            const CatalogEntry* e = catalog.part(p.type);
            const std::string model = e ? e->model : std::string();
            std::vector<std::string> where;
            for (const auto& [pin, field] : p.pins) where.push_back(field);
            if (r->destroyed) {
                broken.push_back(
                    {CircuitFinding::Kind::Broken, p.id, where,
                     model == "led" ? msg(QT_TRANSLATE_NOOP("Meldung", "%1 ist kaputt gegangen. Was hätte den Strom "
                                                                      "durch %1 begrenzen können?"),
                                          {p.id})
                                    : msg(QT_TRANSLATE_NOOP("Meldung", "%1 ist kaputt gegangen. Was hat %1 zu viel "
                                                                      "Strom oder Spannung gegeben?"),
                                          {p.id})});
            } else if (model == "led" && r->voltV < -1.0) {
                reversed.push_back({CircuitFinding::Kind::Reversed, p.id, where,
                                    msg(QT_TRANSLATE_NOOP("Meldung", "Steckt das lange Bein (Anode) von %1 auf der "
                                                                     "Seite zum Pluspol?"),
                                        {p.id})});
            } else if (model == "capacitor_polar" && r->voltV < -0.5) {
                reversed.push_back({CircuitFinding::Kind::Reversed, p.id, where,
                                    msg(QT_TRANSLATE_NOOP("Meldung", "Zeigt das Plus-Bein von %1 zum Pluspol?"), {p.id})});
            } else if (model == "led" && (r->overloaded || std::fabs(r->currentA) > 0.03)) {
                noResistor.push_back({CircuitFinding::Kind::NoResistor, p.id, where,
                                      msg(QT_TRANSLATE_NOOP("Meldung", "Durch %1 fließt sehr viel Strom. Was könnte den "
                                                                       "Strom begrenzen?"),
                                          {p.id})});
            } else if (r->overloaded) {
                overload.push_back(
                    {CircuitFinding::Kind::Overload, p.id, where,
                     model == "lamp" ? msg(QT_TRANSLATE_NOOP("Meldung", "%1 ist überlastet. Passt die Spannung zum "
                                                                       "Lämpchen?"),
                                           {p.id})
                                     : msg(QT_TRANSLATE_NOOP("Meldung", "%1 wird sehr warm. Fließt durch %1 zu viel "
                                                                       "Strom?"),
                                           {p.id})});
            }
        }
    }

    std::vector<CircuitFinding> out;
    for (auto* list : {&shorts, &broken, &reversed, &noResistor, &overload, &bridged, &dangling, &looseWires, &noSupply,
                       &openSwitch, &deadEnd, &mainSwitch})
        out.insert(out.end(), list->begin(), list->end());
    return out;
}

} // namespace volta::model

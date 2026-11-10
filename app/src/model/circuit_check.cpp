#include "circuit_check.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <deque>
#include <map>
#include <set>

#include "anchor.hpp"
#include "board_edit.hpp"
#include "netlist_builder.hpp"
#include "volta/core/components/thyristor.hpp"

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
    // Pultteile ab Volta 2 (nur, wenn sie im Pult stehen); G0 liegt im Generator fest an 0V
    std::vector<std::string> extraDesk;
    for (const auto& d : extraDeskFields())
        if (hasDeskPart(board, deskGroupOf(d))) extraDesk.push_back(d);
    if (hasDeskPart(board, "generator")) nodes.join("P:G0", kMinus);
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
    for (const auto& d : extraDesk) addTerminal(d, "PULT", d);
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
    // weitere Quellen ab Volta 2 leiten fuer die Suche nach offenen Kreisen (Batterie 2 in Reihe, Generator, ...)
    if (hasDeskPart(board, "batterie2")) connect("P:+B2", "P:-B2", "PULT.B2");
    if (hasDeskPart(board, "wechselquelle")) connect("P:W~a", "P:W~b", "PULT.W");
    if (hasDeskPart(board, "kennlinienschreiber")) connect("P:KS+", "P:KS-", "PULT.KS");
    if (board.desk.meterVisible) {
        connect("MESS:COM", "MESS:VOHM", "MESS");
        connect("MESS:COM", "MESS:A", "MESS");
    }

    std::vector<CircuitFinding> shorts, broken, reversed, noResistor, overload, bridged, dangling, looseWires, noSupply,
        openSwitch, deadEnd, mainSwitch;
    const std::string plus = nodes.find(kPlus), minus = nodes.find(kMinus);
    const bool anyPart = !board.parts.empty() || !board.modules.empty();

    // Eigene Quellen ab Volta 2 (Kennlinienschreiber, Wechselquelle, Generator; ab Volta 3 die Antenne der
    // Senderanlage): Knoten, die sie ueber Bauteile erreichen. Dort gelten die Batterie-Hinweise nicht (Polung
    // wechselt, V+ wird nicht gebraucht).
    std::set<std::string> ownSource;
    {
        std::deque<std::string> todo;
        for (const char* a : {"P:KS+", "P:KS-", "P:W~a", "P:W~b", "P:G", "P:ANT"})
            if (std::find(extraDesk.begin(), extraDesk.end(), a) != extraDesk.end() &&
                terminals[nodes.find(a)].size() > 1 && ownSource.insert(nodes.find(a)).second)
                todo.push_back(nodes.find(a));
        while (!todo.empty()) {
            const std::string n = todo.front();
            todo.pop_front();
            for (const auto& ed : edges[n])
                if (ed.to != minus && ownSource.insert(ed.to).second) todo.push_back(ed.to);
        }
    }
    auto intended = [&](const std::string& id) {
        return std::find(in.intendedReversed.begin(), in.intendedReversed.end(), id) != in.intendedReversed.end();
    };
    auto onOwnSource = [&](const PartPlacement& p) {
        for (const auto& [pin, field] : p.pins)
            if (ownSource.count(nodes.find(field))) return true;
        return false;
    };

    // ---- Kurzschluss ueber Leitungen
    if (plus == minus)
        shorts.push_back({CircuitFinding::Kind::Short, "", {kPlus, kMinus},
                          msg(QT_TRANSLATE_NOOP("Meldung", "Fließt der Strom hier von V+ direkt nach 0V, ohne durch ein "
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
        if (model == "bjt_npn" || model == "bjt_pnp") {
            for (const char* pin : {"b", "c"})
                if (isLoose(pin)) report.push_back(pin);
            if (isLoose("e") && isLoose("e2")) report.push_back("e");
        } else if (model == "mosfet_n" || model == "mosfet_p") {
            for (const char* pin : {"g", "d"})
                if (isLoose(pin)) report.push_back(pin);
            if (isLoose("s") && isLoose("s2")) report.push_back("s");
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
            // Aufbau an einer eigenen Quelle (z. B. Kennlinienschreiber): die Batterie wird nicht gebraucht
            const bool ownOnly = plusFree && !ownSource.empty();
            if (plusFree && !ohmOnly && !ownOnly)
                noSupply.push_back({CircuitFinding::Kind::NoSupply, "", {kPlus},
                                    msg(QT_TRANSLATE_NOOP("Meldung", "Ist dein Aufbau mit dem Pluspol V+ verbunden?"))});
            if (minusFree && !ohmOnly && !ownOnly)
                noSupply.push_back({CircuitFinding::Kind::NoSupply, "", {kMinus},
                                    msg(QT_TRANSLATE_NOOP("Meldung", "Ist dein Aufbau mit dem Minuspol 0V verbunden?"))});
            if (!ohmOnly && !ownOnly && !plusFree && !minusFree) {
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
                                                               "Weg von V+ nach 0V, auf dem kein Bauteil den Strom "
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
            } else if (model == "led" && r->voltV < -1.0 && !onOwnSource(p) && !intended(p.id)) {
                reversed.push_back({CircuitFinding::Kind::Reversed, p.id, where,
                                    msg(QT_TRANSLATE_NOOP("Meldung", "Steckt das lange Bein (Anode) von %1 auf der "
                                                                     "Seite zum Pluspol?"),
                                        {p.id})});
            } else if (model == "capacitor_polar" && r->voltV < -0.5 && !onOwnSource(p) && !intended(p.id)) {
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

    // Pultbauteile: durchgebrannter oder heisser Drehregler, kaputter Lautsprecher (keine Bauteil-ID, Stelle = Pultfelder)
    if (board.desk.mainSwitch && in.reading) {
        for (int k = 1; k <= 2; ++k) {
            const core::ComponentReading* r = in.reading(k == 1 ? desk_id::kPot1 : desk_id::kPot2);
            if (!r) continue;
            const std::string n = std::to_string(k), p = "P:P" + n;
            if (r->destroyed)
                broken.push_back({CircuitFinding::Kind::Broken, "", {p + "a", p + "s", p + "b"},
                                  msg(QT_TRANSLATE_NOOP("Meldung", "Drehregler P%1 ist durchgebrannt. Was hätte den "
                                                                   "Strom durch ihn begrenzen können? Tippe auf den "
                                                                   "Drehknopf, um ihn zu reparieren."),
                                      {n})});
            else if (r->overloaded)
                overload.push_back({CircuitFinding::Kind::Overload, "", {p + "a", p + "s", p + "b"},
                                    msg(QT_TRANSLATE_NOOP("Meldung", "Drehregler P%1 wird sehr warm. Fließt durch ihn "
                                                                     "zu viel Strom?"),
                                        {n})});
        }
        if (const core::ComponentReading* r = in.reading(desk_id::kSpeaker); r && r->destroyed)
            broken.push_back({CircuitFinding::Kind::Broken, "", {"P:LS1", "P:LS2"},
                              msg(QT_TRANSLATE_NOOP("Meldung", "Der Lautsprecher ist kaputt gegangen. Was hätte den "
                                                               "Strom durch ihn begrenzen können? Tippe auf den "
                                                               "Lautsprecher, um ihn zu reparieren."))});
    }

    // ---- ab Volta 3: typische Fehler fortgeschrittener Schaltungen (Thyristor, MOSFET, Spule, Logik, Schwingkreis).
    // Nur Hinweise als Frage; jeder Musteraufbau besteht sie (templatesPassCircuitCheck).
    std::vector<CircuitFinding> hints;
    if (catalog.secondOrder) {
        std::map<std::string, std::string> partModel, moduleType;
        for (const auto& p : board.parts)
            if (const CatalogEntry* e = catalog.part(p.type)) partModel[p.id] = e->model;
        for (const auto& m : board.modules) moduleType[m.id] = m.type;
        auto modelOf = [&](const std::string& owner) {
            const auto it = partModel.find(owner);
            return it == partModel.end() ? std::string() : it->second;
        };
        auto upper = [](std::string s) {
            for (auto& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            return s;
        };

        // Thyristor gezuendet, aber der Laststrom bleibt unter dem Haltestrom
        if (board.desk.mainSwitch && in.reading)
            for (const auto& p : board.parts) {
                if (modelOf(p.id) != "thyristor") continue;
                const core::ComponentReading* r = in.reading(p.id);
                if (!r || r->destroyed) continue;
                const double holdA = core::ThyristorParams{}.holdA;
                const double gateA = r->terminalCurrentA[2], anodeA = r->terminalCurrentA[0];
                if (gateA > 1e-3 && anodeA > -1e-4 && anodeA < 0.9 * holdA && r->voltV > 0.3) {
                    std::vector<std::string> where;
                    for (const auto& [pin, field] : p.pins) where.push_back(field);
                    hints.push_back({CircuitFinding::Kind::NoHoldingCurrent, p.id, where,
                                     msg(QT_TRANSLATE_NOOP("Meldung", "Am Gate von %1 fließt Strom, durch %1 selbst "
                                                                      "aber weniger als der Haltestrom (etwa 3 mA). "
                                                                      "Bleibt %1 eingeschaltet, wenn das Gate keinen "
                                                                      "Strom mehr bekommt?"),
                                         {p.id})});
                }
            }

        // MOSFET-Gate nur an Taster oder Kondensator: ist der Taster offen, hat das Gate keine feste Spannung
        for (const auto& p : board.parts) {
            const std::string model = modelOf(p.id);
            if (model != "mosfet_n" && model != "mosfet_p") continue;
            const std::string g = pinField(p, "g");
            if (g.empty()) continue;
            bool fixed = false, loose = false;
            for (const auto& t : terminals[nodes.find(g)]) {
                if (t.owner == p.id && t.pin == "g") continue;
                const std::string m = modelOf(t.owner);
                if (m == "switch" || m == "capacitor" || m == "capacitor_polar" || m == "capacitor_variable")
                    loose = true;
                else
                    fixed = true;
            }
            if (loose && !fixed)
                hints.push_back({CircuitFinding::Kind::FloatingGate, p.id, {g},
                                 msg(QT_TRANSLATE_NOOP("Meldung", "Das Gate von %1 hängt nur an einem Taster oder "
                                                                  "Kondensator. Welche Spannung hat das Gate, wenn der "
                                                                  "Taster offen ist?"),
                                     {p.id})});
        }

        // Relais- oder Magnetspule, von einem Transistor geschaltet, ohne Bauteil parallel zur Spule
        for (const auto& p : board.parts) {
            const std::string model = modelOf(p.id);
            if (model != "relay" && model != "electromagnet") continue;
            const std::string fa = pinField(p, "a"), fb = pinField(p, "b");
            if (fa.empty() || fb.empty()) continue;
            const std::string na = nodes.find(fa), nb = nodes.find(fb);
            if (na == nb) continue;
            std::string driver;
            for (const std::string& n : {na, nb})
                for (const auto& t : terminals[n]) {
                    const std::string m = modelOf(t.owner);
                    if (((m == "bjt_npn" || m == "bjt_pnp") && t.pin == "c") ||
                        ((m == "mosfet_n" || m == "mosfet_p") && t.pin == "d"))
                        driver = t.owner;
                }
            if (driver.empty()) continue;
            bool across = false;
            for (const auto& q : board.parts) {
                if (q.id == p.id) continue;
                bool atA = false, atB = false;
                for (const auto& [pin, field] : q.pins) {
                    atA = atA || nodes.find(field) == na;
                    atB = atB || nodes.find(field) == nb;
                }
                across = across || (atA && atB);
            }
            if (!across)
                hints.push_back({CircuitFinding::Kind::NoFreewheel, p.id, {fa, fb},
                                 msg(QT_TRANSLATE_NOOP("Meldung", "Schaltet %2 den Strom durch die Spule von %1 ab, "
                                                                  "entsteht dort eine hohe Spannungsspitze. Was fängt "
                                                                  "sie ab, damit %2 heil bleibt?"),
                                     {p.id, driver})});
        }

        // Logik: Ausgang verdrahtet, Eingang offen (Gatter-Modul; Takt der Zaehler)
        for (const auto& m : board.modules) {
            const ModuleEntry* e = catalog.module(m.type);
            if (!e) continue;
            auto slotOf = [&](const std::string& pin) -> std::string {
                for (std::size_t i = 0; i < e->pins.size(); ++i)
                    if (e->pins[i] == pin) return moduleSlot(m.slot + static_cast<int>(i));
                return {};
            };
            auto wired = [&](const std::string& pin) {
                const std::string s = slotOf(pin);
                return !s.empty() && terminals[nodes.find(s)].size() > 1;
            };
            std::vector<std::pair<std::string, std::vector<std::string>>> groups; // Eingang -> zugehoerige Ausgaenge
            if (m.type == "gattermodul")
                for (const char* k : {"1", "2", "3", "4"}) {
                    groups.push_back({std::string("a") + k, {std::string("y") + k}});
                    groups.push_back({std::string("b") + k, {std::string("y") + k}});
                }
            else if (m.type == "zaehlermodul")
                groups.push_back({"clk", {"q0", "q1", "q2", "q3"}});
            else if (m.type == "dekadenzaehler")
                groups.push_back({"clk", {"q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9"}});
            for (const auto& [input, outputs] : groups) {
                if (wired(input) || slotOf(input).empty()) continue;
                if (std::none_of(outputs.begin(), outputs.end(), wired)) continue;
                hints.push_back({CircuitFinding::Kind::OpenInput, m.id, {slotOf(input)},
                                 msg(QT_TRANSLATE_NOOP("Meldung", "Eingang %2 von %1 ist mit nichts verbunden, sein "
                                                                  "Ausgang aber schon. Soll dieser Eingang H oder L "
                                                                  "sein?"),
                                     {m.id, upper(input)})});
            }
        }

        // Taster zwischen V+ und dem Takteingang eines Zaehlers, ohne Kondensator: jeder Druck soll einmal zaehlen,
        // aber der Taster prellt (schaltet der Taster einen Takt durch, ist das gewollt und keine Meldung)
        for (const auto& p : board.parts) {
            if (p.type != "taster" || p.pins.size() != 2) continue;
            for (const auto& [pin, field] : p.pins) {
                const std::string& other = field == p.pins[0].second ? p.pins[1].second : p.pins[0].second;
                if (nodes.find(other) != plus) continue;
                const auto& ts = terminals[nodes.find(field)];
                std::string counter;
                bool capacitor = false;
                for (const auto& t : ts) {
                    const auto it = moduleType.find(t.owner);
                    if (it != moduleType.end() && t.pin == "clk" &&
                        (it->second == "zaehlermodul" || it->second == "dekadenzaehler"))
                        counter = t.owner;
                    const std::string mo = modelOf(t.owner);
                    capacitor = capacitor || mo == "capacitor" || mo == "capacitor_polar";
                }
                if (counter.empty() || capacitor) continue;
                hints.push_back({CircuitFinding::Kind::Bouncing, p.id, {field},
                                 msg(QT_TRANSLATE_NOOP("Meldung", "Taster %1 führt direkt an den Takteingang von %2. "
                                                                  "Ein Taster prellt beim Drücken. Zählt %2 dann "
                                                                  "einmal oder mehrmals?"),
                                     {p.id, counter})});
                break;
            }
        }

        // Parallelschwingkreis (Spule und Kondensator mit beiden Enden an denselben Knoten) direkt am Generator, ohne
        // Widerstand am Ausgang G: der Generator haelt die Spannung fest, die Resonanz ist kaum zu sehen
        // (Reihenschwingkreise am Generator sind gewollt; seine 50 Ohm daempfen sie)
        if (hasDeskPart(board, "generator")) {
            const std::string g = nodes.find("P:G");
            bool resistor = false;
            std::vector<const PartPlacement*> coils, caps;
            for (const auto& t : terminals[g]) {
                const std::string m = modelOf(t.owner);
                resistor = resistor || m == "resistor" || m == "potentiometer";
                for (const auto& p : board.parts)
                    if (p.id == t.owner && p.pins.size() == 2) {
                        if (m == "inductor") coils.push_back(&p);
                        if (m == "capacitor" || m == "capacitor_variable") caps.push_back(&p);
                    }
            }
            auto otherEnd = [&](const PartPlacement* p) {
                const std::string a = nodes.find(p->pins[0].second);
                return a == g ? nodes.find(p->pins[1].second) : a;
            };
            std::string coil, cap;
            for (const auto* l : coils)
                for (const auto* c : caps)
                    if (otherEnd(l) == otherEnd(c) && otherEnd(l) != g) {
                        coil = l->id;
                        cap = c->id;
                    }
            if (!resistor) {
                if (!coil.empty() && !cap.empty())
                    hints.push_back({CircuitFinding::Kind::Undamped, coil, {"P:G"},
                                     msg(QT_TRANSLATE_NOOP("Meldung", "Zwischen dem Generator G und dem Schwingkreis "
                                                                      "aus %1 und %2 liegt kein Widerstand. Was "
                                                                      "begrenzt dann den Strom, und woran erkennst du "
                                                                      "die Resonanz?"),
                                         {coil, cap})});
            }
        }
    }

    std::vector<CircuitFinding> out;
    for (auto* list : {&shorts, &broken, &reversed, &noResistor, &overload, &bridged, &dangling, &looseWires, &noSupply,
                       &openSwitch, &deadEnd, &hints, &mainSwitch})
        out.insert(out.end(), list->begin(), list->end());
    return out;
}

} // namespace volta::model

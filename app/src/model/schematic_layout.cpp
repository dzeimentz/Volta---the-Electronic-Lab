#include "schematic_layout.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <queue>
#include <set>

#include "anchor.hpp"

namespace volta::model {

namespace {

// Rastermasse (Einheiten): Zeilenabstand der Knoten, Abstand der Spalten, Platz fuer die Kennzeichnung
constexpr double kRowGap = 3.5;
constexpr double kTop = 1.0;
constexpr double kLeft = 0.6;
constexpr double kColumnGap = 0.4;
constexpr double kStub = 0.5;     // Abstand der seitlichen Anschlussleitungen
constexpr double kLabelWidth = 1.3;
constexpr double kEps = 1e-6;
constexpr double kBridge = 2.0; // Brueckengleichrichter: halbe Diagonale der Raute
constexpr double kStageBase = 5.0; // Emitterschaltung: Basisleitung unter der Plusleitung

// Vereinigung von Anschlusspunkten zu Knoten
struct Union {
    std::map<std::string, std::string> parent;
    std::string find(const std::string& a) {
        auto it = parent.find(a);
        if (it == parent.end()) {
            parent[a] = a;
            return a;
        }
        if (it->second == a) return a;
        const std::string root = find(it->second);
        parent[a] = root;
        return root;
    }
    void join(const std::string& a, const std::string& b) { parent[find(a)] = find(b); }
};

// Modulanschluss "BL1.aus" -> Feld der Modulleiste "M:<n>" (Reihenfolge der Katalog-Anschluesse ab dem Startfeld)
std::string canonicalAnchor(const std::string& a, const Board& board, const Catalog& catalog) {
    if (classifyAnchor(a) != AnchorKind::ModulePin) return a;
    const auto dot = a.find('.');
    const std::string id = a.substr(0, dot);
    const std::string pin = a.substr(dot + 1);
    for (const auto& m : board.modules) {
        if (m.id != id) continue;
        const auto e = catalog.modules.find(m.type);
        if (e == catalog.modules.end()) return a;
        const auto& pins = e->second.pins;
        const auto pos = std::find(pins.begin(), pins.end(), pin) - pins.begin();
        if (pos < static_cast<long>(pins.size())) return moduleSlot(m.slot + static_cast<int>(pos));
    }
    return a;
}

// Sortierung: Pultfelder in Pultreihenfolge, dann Felder in Lesereihenfolge, dann Rest
int anchorOrder(const std::string& a) {
    const auto& desk = deskFields();
    const auto d = std::find(desk.begin(), desk.end(), a);
    if (d != desk.end()) return static_cast<int>(d - desk.begin());
    const auto& extra = extraDeskFields(); // Pultteile ab Volta 2 nach dem Pult von Volta 1
    const auto x = std::find(extra.begin(), extra.end(), a);
    if (x != extra.end()) return 50 + static_cast<int>(x - extra.begin());
    if (classifyAnchor(a) == AnchorKind::Field) {
        const int col = std::stoi(a.substr(1));
        return 100 + (a[0] - 'A') * 100 + col;
    }
    return 10000;
}

std::string displayName(const std::string& a) {
    if (a == "P:+9V") return "V+"; // Pluspol heisst auf dem Pult V+
    if (a.rfind("P:", 0) == 0) return a.substr(2);
    if (a.rfind("MESS:", 0) == 0) return a.substr(5) == "VOHM" ? "V/Ω" : a.substr(5);
    if (a.rfind("M:", 0) == 0) return "M" + a.substr(2);
    return a;
}

// Spalten in Signalrichtung (DIN EN 61082-1: von links nach rechts): Quelle, Eingaenge, Regler, passive Bauteile,
// aktive Bauteile und Module, Verbraucher, Lautsprecher, Messgeraet
int signalRank(const SchematicElement& e) {
    const std::string& t = e.type;
    if (t == "batterie" || t == "wechselquelle" || t == "generator" || t == "antenne") return 0;
    if (t == "pultschalter" || t == "taster" || t == "schalter" || t == "reedkontakt" || t == "wechselschalter")
        return 1;
    if (t == "potentiometer" || t.rfind("trimmer", 0) == 0) return 2;
    if (t == "brueckengleichrichter") return 2; // gleich nach der Quelle, vor der Last
    if (t == "widerstand" || t == "fotowiderstand" || t == "kondensator" || t == "elko" || t.rfind("diode", 0) == 0 ||
        t.rfind("zdiode", 0) == 0 || t.rfind("spule", 0) == 0)
        return 3;
    if (t == "transistor_npn" || t == "transistor_pnp" || t == "darlington" || t.rfind("leistung_", 0) == 0 ||
        t.rfind("mosfet_", 0) == 0 || e.module)
        return 4;
    if (t == "lautsprecher") return 6;
    if (t == "messgeraet" || t == "kennlinienschreiber") return 7;
    return 5; // LED, Gluehlampe, Motor, Summer
}

enum class Exit { Up, Down, Left, Right };

struct PinGeo {
    double x = 0.0, y = 0.0; // relativ zur Mitte des Symbols
    Exit exit = Exit::Up;
    std::string label;      // Block: Anschlussname
    bool skip = false;      // nicht zeichnen (zweiter Emitter am selben Knoten)
};

// Lage der Anschluesse eines Elements (relativ zur Mitte) und Drehung, abhaengig von den Zeilen seiner Knoten
struct ElementGeo {
    std::vector<PinGeo> pins;
    int rotation = 0;
    bool block = false;
    double w = 2.0, h = 1.0; // Symbol bzw. Block
};

ElementGeo geometry(const SchematicElement& e, const std::vector<double>& rowOf, const std::vector<bool>& drawn,
                    const std::vector<std::string>& pinNames) {
    ElementGeo g;
    auto row = [&](std::size_t k) { return k < e.nodes.size() ? rowOf[static_cast<std::size_t>(e.nodes[k])] : 0.0; };
    // Anschluss fuehrt zu einer gezeichneten Knotenleitung (offene Anschluesse bestimmen die Lage nicht)
    auto has = [&](std::size_t k) { return k < e.nodes.size() && drawn[static_cast<std::size_t>(e.nodes[k])]; };
    // a liegt unter b (nur wenn beide gezeichnet)
    auto below = [&](std::size_t a, std::size_t b) { return has(a) && has(b) && row(a) > row(b) + kEps; };
    const std::string& t = e.type;
    if (t == "brueckengleichrichter") {
        // Raute: Plus oben, Wechselspannung links und rechts, Minus unten (Anschluesse P, A, B, N)
        g.w = g.h = 2 * kBridge;
        g.pins = {{0.0, -kBridge, Exit::Up}, {-kBridge, 0.0, Exit::Left}, {kBridge, 0.0, Exit::Right},
                  {0.0, kBridge, Exit::Down}};
        return g;
    }
    if (t == "darlington") {
        // zwei Transistoren schraeg versetzt: B des ersten links, gemeinsamer Kollektor oben, E des zweiten unten
        g.w = 3.2;
        g.h = 2.8;
        g.pins = {{-1.35, -0.5, Exit::Left}, {0.85, -1.4, Exit::Up}, {0.85, 1.0, Exit::Down}};
        return g;
    }
    if (e.module || t == "messgeraet" || t == "relais" || t == "optokoppler" || t == "uebertrager" ||
        t == "spannungsregler_5v" || t == "kraftsensor" || t == "hallsensor") {
        // Block: Plus oben, Minus unten, Ausgaenge rechts, uebrige Eingaenge links
        g.block = true;
        g.w = 4.0;
        g.h = t == "timer555" ? 4.0 : 2.0; // 555: fuenf Anschluesse links
        std::vector<std::size_t> left, right;
        g.pins.resize(e.nodes.size());
        for (std::size_t k = 0; k < e.nodes.size(); ++k) {
            const std::string n = k < pinNames.size() ? pinNames[k] : std::string();
            PinGeo& p = g.pins[k];
            p.label = n == "plus" ? "+" : n == "minus" ? "−" : n == "VOHM" ? "V/Ω" : t == "relais" ? (n == "com" ? "COM" : n == "no" ? "NO" : n == "a" ? "A" : "B") : n;
            const bool relayTop = t == "relais" && n == "a", relayBottom = t == "relais" && n == "b";
            const bool relayRight = (t == "relais" && n == "no") || (t == "optokoppler" && (n == "c" || n == "e")) ||
                                    (t == "uebertrager" && (n == "a2" || n == "e2"));
            if (t == "uebertrager") p.label = n == "a1" ? "•1" : n == "e1" ? "1" : n == "a2" ? "•2" : "2";
            if (t == "optokoppler") p.label = n == "anode" ? "A" : n == "kathode" ? "K" : n == "c" ? "C" : "E";
            if (t == "kraftsensor" && (n == "outp" || n == "outn")) { // Brueckenausgang rechts
                p.label = n == "outp" ? "A+" : "A−";
                right.push_back(k);
                continue;
            }
            if (t == "hallsensor" && n == "aus") p.label = "A";
            if (t == "opmodul" && (n == "nichtinv" || n == "inv")) p.label = n == "nichtinv" ? "IN+" : "IN−";
            if (t == "mwmodul" && (n == "ant" || n == "abst")) p.label = n == "ant" ? "ANT" : "ABST";
            if (t == "ukwmodul" && (n == "ant" || n == "abst" || n == "laut"))
                p.label = n == "ant" ? "ANT" : n == "abst" ? "ABST" : "LAUT";
            if (t == "timer555" && n != "plus" && n != "minus" && n != "aus")
                p.label = n == "tr" ? "TR" : n == "reset" ? "R" : n == "cv" ? "CV" : n == "thr" ? "THR" : "DIS";
            if (t == "spannungsregler_5v") { // Ein links, Aus rechts, Masse unten (wie im Datenblatt)
                p.label = n == "ein" ? "IN" : n == "aus" ? "OUT" : "GND";
                if (n == "masse") {
                    p.y = g.h / 2;
                    p.exit = Exit::Down;
                    continue;
                }
                if (n == "aus") {
                    right.push_back(k);
                    continue;
                }
            }
            if (n == "plus" || n == "VOHM" || relayTop) {
                p.y = -g.h / 2;
                p.exit = Exit::Up;
            } else if (n == "minus" || n == "COM" || relayBottom) {
                p.y = g.h / 2;
                p.exit = Exit::Down;
            } else if (n == "aus" || n == "A" || relayRight) {
                right.push_back(k);
            } else {
                left.push_back(k);
            }
        }
        auto side = [&](const std::vector<std::size_t>& idx, double x, Exit ex) {
            for (std::size_t j = 0; j < idx.size(); ++j) {
                PinGeo& p = g.pins[idx[j]];
                p.x = x;
                p.y = (static_cast<double>(j) + 1.0) / (static_cast<double>(idx.size()) + 1.0) * g.h - g.h / 2;
                p.exit = ex;
            }
        };
        side(left, -g.w / 2, Exit::Left);
        side(right, g.w / 2, Exit::Right);
        return g;
    }
    if ((t == "transistor_npn" || t == "transistor_pnp" || t.rfind("leistung_", 0) == 0 || t.rfind("mosfet_", 0) == 0) &&
        e.nodes.size() >= 3) {
        // B links, C oben, E unten (Schaltzeichen 64 x 32: B bei 8|16, C bei 40|0, E bei 40|32)
        const bool flip = below(1, 2); // Kollektor unter dem Emitter: halbe Drehung
        g.rotation = flip ? 180 : 0;
        const double s = flip ? -1.0 : 1.0;
        g.pins.push_back({-0.75 * s, 0.0, flip ? Exit::Right : Exit::Left});
        g.pins.push_back({0.25 * s, -0.5 * s, flip ? Exit::Down : Exit::Up});
        g.pins.push_back({0.25 * s, 0.5 * s, flip ? Exit::Up : Exit::Down});
        if (e.nodes.size() >= 4) {
            PinGeo e2{0.25 * s, 0.5 * s, flip ? Exit::Left : Exit::Right};
            e2.skip = e.nodes[3] == e.nodes[2];
            g.pins.push_back(e2);
        }
        return g;
    }
    if (t == "wechselschalter" && e.nodes.size() == 3) {
        // senkrecht: Mitte oben, Stellungen a und b unten nebeneinander (wie im Treppenhausplan); liegt die Mitte
        // unter den Stellungen, umgekehrt
        const bool flip = below(0, 1) || below(0, 2) || (!has(0) && (has(1) || has(2)));
        g.rotation = flip ? 270 : 90;
        if (flip)
            g.pins = {{0.0, 1.0, Exit::Down}, {-0.25, -1.0, Exit::Up}, {0.25, -1.0, Exit::Up}};
        else
            g.pins = {{0.0, -1.0, Exit::Up}, {0.25, 1.0, Exit::Down}, {-0.25, 1.0, Exit::Down}};
        return g;
    }
    if ((t == "potentiometer" || t.rfind("trimmer", 0) == 0) && e.nodes.size() == 3) {
        // senkrecht, Schleifer (Pfeil) von der Seite
        // a oben; fehlt a oder b, entscheidet die Lage gegenueber dem Schleifer
        const bool flip = has(0) && has(2) ? below(0, 2) : has(0) ? below(0, 1) : has(2) ? below(1, 2) : false;
        g.rotation = flip ? 270 : 90;
        g.pins = {{0.0, flip ? 1.0 : -1.0, flip ? Exit::Down : Exit::Up},
                  {flip ? 0.5 : -0.5, 0.0, flip ? Exit::Right : Exit::Left},
                  {0.0, flip ? -1.0 : 1.0, flip ? Exit::Up : Exit::Down}};
        return g;
    }
    if ((t == "thyristor" || t == "triac") && e.nodes.size() == 3) {
        // wie ein Zweipol senkrecht (Anode bzw. A2 oben), Gate seitlich unten an der Kathode bzw. A1
        // (Schaltzeichen 64 x 32: Gate bei 46|32 bzw. 50|32)
        const bool flip = below(0, 1) || (!has(0) && has(1));
        g.rotation = flip ? 270 : 90;
        const double gx = t == "triac" ? 0.5625 : 0.4375;
        g.pins.push_back({0.0, flip ? 1.0 : -1.0, flip ? Exit::Down : Exit::Up});
        g.pins.push_back({0.0, flip ? -1.0 : 1.0, flip ? Exit::Up : Exit::Down});
        g.pins.push_back({flip ? 0.5 : -0.5, flip ? -gx : gx, flip ? Exit::Right : Exit::Left});
        return g;
    }
    // Zweipol senkrecht: Anschluss 0 oben (Drehung 90), sonst unten (270)
    const bool flip = e.nodes.size() == 2 && (below(0, 1) || (!has(0) && has(1)));
    g.rotation = flip ? 270 : 90;
    g.pins.push_back({0.0, flip ? 1.0 : -1.0, flip ? Exit::Down : Exit::Up});
    if (e.nodes.size() >= 2) g.pins.push_back({0.0, flip ? -1.0 : 1.0, flip ? Exit::Up : Exit::Down});
    return g;
}

} // namespace

Schematic buildSchematic(const Board& board, const Catalog& catalog, bool standardForms) {
    Schematic out;
    Union u;
    std::map<std::string, int> uses; // Anschlusspunkt -> Zahl der Beine/Leitungsenden
    auto canon = [&](const std::string& a) { return canonicalAnchor(a, board, catalog); };

    for (const auto& w : board.wires) {
        const std::string a = canon(w.from), b = canon(w.to);
        u.join(a, b);
        ++uses[a];
        ++uses[b];
    }
    // Elemente (Anschluesse noch als Anschlusspunkte) mit Anschlussnamen
    struct Raw {
        SchematicElement e;
        std::vector<std::string> anchors;
        std::vector<std::string> pinNames;
    };
    std::vector<Raw> raw;
    for (const auto& p : board.parts) {
        Raw r;
        r.e.id = p.id;
        r.e.type = p.type;
        r.e.value = p.value;
        for (const auto& pin : p.pins) {
            r.anchors.push_back(pin.second);
            r.pinNames.push_back(pin.first);
            u.find(pin.second);
            ++uses[pin.second];
        }
        raw.push_back(r);
    }
    for (const auto& m : board.modules) {
        const auto e = catalog.modules.find(m.type);
        if (e == catalog.modules.end()) continue;
        Raw r;
        r.e.id = m.id;
        r.e.type = m.type;
        r.e.module = true;
        for (std::size_t k = 0; k < e->second.pins.size(); ++k) {
            const std::string a = moduleSlot(m.slot + static_cast<int>(k));
            r.anchors.push_back(a);
            r.pinNames.push_back(e->second.pins[k]);
            u.find(a);
        }
        raw.push_back(r);
    }
    // Pultelemente, sofern angeschlossen
    auto used = [&](const std::string& a) { return uses.count(a) && uses[a] > 0; };
    auto deskElement = [&](const char* id, const char* type, std::vector<std::string> anchors,
                           std::vector<std::string> names) {
        bool any = false;
        for (const auto& a : anchors) any = any || used(a);
        if (!any) return;
        Raw r;
        r.e.id = id;
        r.e.type = type;
        r.e.desk = true;
        r.anchors = std::move(anchors);
        r.pinNames = std::move(names);
        for (const auto& a : r.anchors) u.find(a);
        raw.push_back(r);
    };
    deskElement("PULT.B", "batterie", {kPlusAnchor, kGroundAnchor}, {"plus", "minus"});
    deskElement("PULT.S", "pultschalter", {"P:S1", "P:S2"}, {"a", "b"});
    deskElement("PULT.P1", "potentiometer", {"P:P1a", "P:P1s", "P:P1b"}, {"a", "s", "b"});
    deskElement("PULT.P2", "potentiometer", {"P:P2a", "P:P2s", "P:P2b"}, {"a", "s", "b"});
    deskElement("PULT.LS", "lautsprecher", {"P:LS1", "P:LS2"}, {"a", "b"});
    // Pultteile ab Volta 2 (Pult nach Bedarf, Konzept 5.1)
    deskElement("PULT.B2", "batterie", {"P:+B2", "P:-B2"}, {"plus", "minus"});
    deskElement("PULT.W", "wechselquelle", {"P:W~a", "P:W~b"}, {"a", "b"});
    deskElement("PULT.G", "generator", {"P:G", "P:G0"}, {"g", "g0"});
    deskElement("PULT.KS", "kennlinienschreiber", {"P:KS+", "P:KS-"}, {"plus", "minus"});
    deskElement("PULT.ANT", "antenne", {"P:ANT"}, {"ant"}); // Senderanlage ab Volta 3
    if (board.desk.meterVisible)
        deskElement("PULT.MESS", "messgeraet", {"MESS:COM", "MESS:VOHM", "MESS:A"}, {"COM", "VOHM", "A"});

    // Knoten: Gruppen von Anschlusspunkten, die an Elementen haengen
    std::map<std::string, int> nodeOf; // Wurzel -> Knotenindex
    std::map<std::string, std::vector<std::string>> members;
    for (const auto& [a, parent] : u.parent) members[u.find(a)].push_back(a);
    std::vector<int> pinCount; // Element-Anschluesse je Knoten
    for (auto& r : raw) {
        for (const auto& a : r.anchors) {
            const std::string root = u.find(a);
            auto it = nodeOf.find(root);
            if (it == nodeOf.end()) {
                SchematicNode n;
                n.anchors = members[root];
                std::sort(n.anchors.begin(), n.anchors.end(),
                          [](const std::string& x, const std::string& y) { return anchorOrder(x) < anchorOrder(y); });
                for (std::size_t i = 0; i < n.anchors.size(); ++i)
                    n.label += (i ? " · " : "") + displayName(n.anchors[i]);
                it = nodeOf.emplace(root, static_cast<int>(out.nodes.size())).first;
                out.nodes.push_back(std::move(n));
                pinCount.push_back(0);
            }
            r.e.nodes.push_back(it->second);
            ++pinCount[static_cast<std::size_t>(it->second)];
        }
        out.elements.push_back(r.e);
    }

    // Zweipole zwischen denselben Knoten (parallel) durchnummerieren
    std::map<std::pair<int, int>, std::vector<std::size_t>> pairs;
    for (std::size_t i = 0; i < out.elements.size(); ++i) {
        const auto& e = out.elements[i];
        if (e.nodes.size() != 2) continue;
        pairs[{std::min(e.nodes[0], e.nodes[1]), std::max(e.nodes[0], e.nodes[1])}].push_back(i);
    }
    for (const auto& [key, idx] : pairs)
        for (std::size_t k = 0; k < idx.size(); ++k) {
            out.elements[idx[k]].parallelIndex = static_cast<int>(k);
            out.elements[idx[k]].parallelCount = static_cast<int>(idx.size());
        }

    const int n = static_cast<int>(out.nodes.size());
    if (n == 0) return out;

    // ---- Brueckengleichrichter (genormte Darstellung): vier Dioden, zwei Kathoden am Plus-Knoten P, zwei Anoden am
    // Minus-Knoten N, an den Wechselspannungs-Knoten A und B je eine Anode und eine Kathode
    struct Bridge {
        int p = -1, a = -1, b = -1, nn = -1;
        std::size_t ap = 0, bp = 0, na = 0, nb = 0; // Dioden A->P, B->P, N->A, N->B (Elementindex)
    };
    std::vector<Bridge> bridges;
    std::vector<bool> inBridge(out.elements.size(), false);
    if (standardForms) {
        std::vector<std::size_t> diodes;
        for (std::size_t i = 0; i < out.elements.size(); ++i)
            if (out.elements[i].type == "diode_si" && out.elements[i].nodes.size() == 2 &&
                out.elements[i].nodes[0] != out.elements[i].nodes[1])
                diodes.push_back(i);
        auto an = [&](std::size_t i) { return out.elements[i].nodes[0]; };
        auto ka = [&](std::size_t i) { return out.elements[i].nodes[1]; };
        for (std::size_t x = 0; x < diodes.size(); ++x)
            for (std::size_t y = x + 1; y < diodes.size(); ++y) {
                const std::size_t i = diodes[x], j = diodes[y];
                if (inBridge[i] || inBridge[j] || ka(i) != ka(j) || an(i) == an(j)) continue;
                const int p = ka(i);
                for (std::size_t k : diodes)
                    for (std::size_t l : diodes) {
                        if (k == i || k == j || l == i || l == j || k == l || inBridge[k] || inBridge[l]) continue;
                        if (an(k) != an(l) || ka(k) != an(i) || ka(l) != an(j) || an(k) == p) continue;
                        if (inBridge[i] || inBridge[j]) continue;
                        bridges.push_back({p, an(i), an(j), an(k), i, j, k, l});
                        inBridge[i] = inBridge[j] = inBridge[k] = inBridge[l] = true;
                    }
            }
    }

    // Anschluesse je Knoten (Elementindex, je Anschluss einmal)
    std::vector<std::vector<int>> at(static_cast<std::size_t>(n));
    for (std::size_t i = 0; i < out.elements.size(); ++i)
        for (int nd : out.elements[i].nodes) at[static_cast<std::size_t>(nd)].push_back(static_cast<int>(i));
    std::vector<bool> internalNode(static_cast<std::size_t>(n), false); // von einer Grundschaltung selbst gezeichnet

    // ---- Darlingtonschaltung: Emitter von T1 nur an der Basis von T2, Kollektoren verbunden
    struct Darlington {
        std::size_t t1 = 0, t2 = 0;
        int b = -1, c = -1, e = -1, d = -1; // Basis T1, Kollektor, Emitter T2, Verbindung E1-B2
    };
    std::vector<Darlington> darlingtons;
    std::vector<bool> inPair(out.elements.size(), false);
    if (standardForms) {
        auto npn = [&](std::size_t i) {
            return out.elements[i].type == "transistor_npn" && out.elements[i].nodes.size() >= 3 && !inPair[i];
        };
        // zweiter Emitteranschluss: am selben Knoten oder offen
        auto e2ok = [&](std::size_t i) {
            const auto& nd = out.elements[i].nodes;
            return nd.size() < 4 || nd[3] == nd[2] || pinCount[static_cast<std::size_t>(nd[3])] == 1;
        };
        for (std::size_t i = 0; i < out.elements.size(); ++i)
            for (std::size_t j = 0; j < out.elements.size(); ++j) {
                if (i == j || !npn(i) || !npn(j) || !e2ok(i) || !e2ok(j)) continue;
                const auto& a = out.elements[i].nodes;
                const auto& b = out.elements[j].nodes;
                const int d = a[2];
                if (b[0] != d || a[1] != b[1] || d == a[1] || d == a[0] || b[2] == d || b[2] == a[1] || a[0] == a[1])
                    continue;
                bool only = true; // an der Verbindung haengt nichts anderes
                for (int el : at[static_cast<std::size_t>(d)])
                    only = only && (el == static_cast<int>(i) || el == static_cast<int>(j));
                if (!only) continue;
                darlingtons.push_back({i, j, a[0], a[1], b[2], d});
                inPair[i] = inPair[j] = true;
                internalNode[static_cast<std::size_t>(d)] = true;
            }
    }

    // ---- Ebenen: Breitensuche ab dem Pluspol, 0 V ganz unten
    auto findAnchor = [&](const std::string& a) {
        const auto it = nodeOf.find(u.find(a));
        return it == nodeOf.end() ? -1 : it->second;
    };
    const int plus = findAnchor(kPlusAnchor);
    const int ground = findAnchor(kGroundAnchor);
    std::vector<int> layer(static_cast<std::size_t>(n), -1);
    std::queue<int> q;
    const int start = plus >= 0 ? plus : 0;
    layer[static_cast<std::size_t>(start)] = 0;
    q.push(start);
    while (!q.empty()) {
        const int cur = q.front();
        q.pop();
        for (const auto& e : out.elements)
            if (std::find(e.nodes.begin(), e.nodes.end(), cur) != e.nodes.end())
                for (int nb : e.nodes)
                    if (layer[static_cast<std::size_t>(nb)] < 0 && nb != ground) {
                        layer[static_cast<std::size_t>(nb)] = layer[static_cast<std::size_t>(cur)] + 1;
                        q.push(nb);
                    }
    }
    int maxLayer = 0;
    for (int l : layer) maxLayer = std::max(maxLayer, l);
    for (int& l : layer)
        if (l < 0) l = std::max(1, maxLayer / 2); // nicht erreichbar: in die Mitte
    if (ground >= 0 && ground != start) layer[static_cast<std::size_t>(ground)] = maxLayer + 1;
    // Bruecke: Plus-Knoten ueber den Wechselspannungs-Knoten, diese ueber dem Minus-Knoten (die Raute passt dazwischen)
    for (const Bridge& br : bridges) {
        auto L = [&](int nd) -> int& { return layer[static_cast<std::size_t>(nd)]; };
        if (br.p == start || br.a == start || br.b == start || br.nn == start || br.p == ground || br.a == ground ||
            br.b == ground)
            continue;
        const int mid = std::max({L(br.a), L(br.b), L(br.p) + 1});
        L(br.a) = L(br.b) = mid;
        if (br.nn != ground) L(br.nn) = std::max(L(br.nn), mid + 1);
        if (ground >= 0 && ground != start) {
            int deepest = 0;
            for (int i = 0; i < n; ++i)
                if (i != ground) deepest = std::max(deepest, layer[static_cast<std::size_t>(i)]);
            L(ground) = std::max(L(ground), deepest + 1);
        }
    }
    // Darlington: Kollektorleitung ueber der Basisleitung, Emitter darunter (Basis laeuft gerade hinein)
    for (const Darlington& d : darlingtons) {
        auto L = [&](int nd) -> int& { return layer[static_cast<std::size_t>(nd)]; };
        if (d.b == start || d.b == ground || d.c == ground || d.c == start) continue;
        L(d.b) = std::max(L(d.b), L(d.c) + 1);
        if (d.e != ground && d.e != start) L(d.e) = std::max(L(d.e), L(d.b) + 1);
        if (ground >= 0 && ground != start) {
            int deepest = 0;
            for (int i = 0; i < n; ++i)
                if (i != ground) deepest = std::max(deepest, layer[static_cast<std::size_t>(i)]);
            L(ground) = std::max(L(ground), deepest + 1);
        }
    }
    for (int l : layer) maxLayer = std::max(maxLayer, l);
    out.layers = maxLayer + 1;

    // ---- Emitterschaltung (genormte Darstellung, nur bei genau einem Transistor)
    struct Stage {
        int t = -1, rc = -1, re = -1, ce = -1, up = -1, low = -1, pot = -1, cin = -1, rin = -1, cout = -1;
        int nb = -1, nc = -1, ne = -1, nw = -1, nm = -1, nend = -1, nout = -1;
        int probeC = -1, probeB = -1, probeM = -1;
    };
    Stage stage;
    if (standardForms && plus >= 0 && ground >= 0 && plus != ground) {
        auto E = [&](int i) -> const SchematicElement& { return out.elements[static_cast<std::size_t>(i)]; };
        auto At = [&](int nd) -> const std::vector<int>& { return at[static_cast<std::size_t>(nd)]; };
        int transistors = 0, candidate = -1;
        for (std::size_t i = 0; i < out.elements.size(); ++i) {
            const std::string& t = out.elements[i].type;
            if (t == "transistor_npn" || t == "transistor_pnp" || t == "fototransistor" || t.rfind("leistung_", 0) == 0 ||
                t.rfind("mosfet_", 0) == 0) {
                ++transistors;
                candidate = static_cast<int>(i);
            }
        }
        auto isRes = [&](int i) { return E(i).type == "widerstand" && E(i).nodes.size() == 2; };
        auto isCap = [&](int i) { return (E(i).type == "kondensator" || E(i).type == "elko") && E(i).nodes.size() == 2; };
        auto isProbe = [&](int i) { return E(i).type == "tastkopf" && E(i).nodes.size() == 1; };
        auto other = [&](int i, int nd) { return E(i).nodes[0] == nd ? E(i).nodes[1] : E(i).nodes[0]; };
        auto pinNode = [&](int i, const char* name) {
            const auto& names = raw[static_cast<std::size_t>(i)].pinNames;
            const auto it = std::find(names.begin(), names.end(), name);
            return it == names.end() ? -1 : E(i).nodes[static_cast<std::size_t>(it - names.begin())];
        };
        auto rail = [&](int nd) { return nd == plus || nd == ground; };
        Stage s;
        bool ok = transistors == 1 && E(candidate).type == "transistor_npn" && E(candidate).nodes.size() >= 3;
        if (ok) {
            const auto& tn = E(candidate).nodes;
            s.t = candidate;
            s.nb = tn[0];
            s.nc = tn[1];
            s.ne = tn[2];
            const int ne2 = tn.size() >= 4 ? tn[3] : s.ne;
            ok = !rail(s.nb) && !rail(s.nc) && s.ne != plus && s.nb != s.nc && s.nb != s.ne && s.nc != s.ne &&
                 (ne2 == s.ne || pinCount[static_cast<std::size_t>(ne2)] == 1);
        }
        // Kollektor: Kollektorwiderstand nach Plus; Tastkopf oder Koppelkondensator zum Ausgang
        if (ok)
            for (int i : At(s.nc)) {
                if (i == s.t) continue;
                if (isRes(i) && other(i, s.nc) == plus && s.rc < 0) {
                    s.rc = i;
                } else if (isProbe(i) && s.probeC < 0) {
                    s.probeC = i;
                } else if (isCap(i) && !rail(other(i, s.nc)) && other(i, s.nc) != s.nb && other(i, s.nc) != s.ne &&
                           s.cout < 0) {
                    s.cout = i;
                    s.nout = other(i, s.nc);
                } else {
                    ok = false;
                }
            }
        ok = ok && s.rc >= 0 && !(s.cout >= 0 && s.probeC >= 0);
        // Emitter: direkt an 0 V oder ueber Emitterwiderstand (mit Kondensator parallel)
        if (ok && s.ne != ground)
            for (int i : At(s.ne)) {
                if (i == s.t) continue;
                if (isRes(i) && other(i, s.ne) == ground && s.re < 0)
                    s.re = i;
                else if (isCap(i) && other(i, s.ne) == ground && s.ce < 0)
                    s.ce = i;
                else
                    ok = false;
            }
        ok = ok && (s.ne == ground || s.re >= 0);
        // Basis: Widerstand nach Plus oder vom Schleifer (Potentiometer zwischen 0 V und Plus), nach 0 V,
        // Koppelkondensator, Tastkopf
        auto wiperPot = [&](int nd) {
            if (rail(nd) || nd == s.nc || nd == s.ne || At(nd).size() != 2) return -1;
            for (int i : At(nd)) {
                const std::string& t = E(i).type;
                if ((t == "potentiometer" || t.rfind("trimmer", 0) == 0) && E(i).nodes.size() == 3 &&
                    pinNode(i, "s") == nd && pinNode(i, "a") == ground && pinNode(i, "b") == plus)
                    return i;
            }
            return -1;
        };
        if (ok)
            for (int i : At(s.nb)) {
                if (i == s.t) continue;
                if (isRes(i)) {
                    const int o = other(i, s.nb);
                    if (o == plus && s.up < 0) {
                        s.up = i;
                    } else if (o == ground && s.low < 0) {
                        s.low = i;
                    } else if (s.up < 0 && wiperPot(o) >= 0) {
                        s.up = i;
                        s.nw = o;
                        s.pot = wiperPot(o);
                    } else {
                        ok = false;
                    }
                } else if (isCap(i) && !rail(other(i, s.nb)) && other(i, s.nb) != s.nc && other(i, s.nb) != s.ne &&
                           s.cin < 0) {
                    s.cin = i;
                    s.nend = other(i, s.nb);
                } else if (isProbe(i) && s.probeB < 0) {
                    s.probeB = i;
                } else {
                    ok = false;
                }
            }
        ok = ok && s.up >= 0;
        // Eingang: zwischen Koppelkondensator und Vorwiderstand nichts als diese beiden (und ein Tastkopf)
        if (ok && s.cin >= 0) {
            const int y = s.nend;
            int rin = -1, probe = -1;
            bool chain = true;
            for (int i : At(y)) {
                if (i == s.cin) continue;
                if (isRes(i) && rin < 0)
                    rin = i;
                else if (isProbe(i) && probe < 0)
                    probe = i;
                else
                    chain = false;
            }
            if (chain && rin >= 0) {
                const int z = other(rin, y);
                if (!rail(z) && z != s.nb && z != s.nc && z != s.ne && z != s.nw && z != s.nout && z != y) {
                    s.rin = rin;
                    s.nm = y;
                    s.nend = z;
                    s.probeM = probe;
                }
            }
            ok = s.nend != s.nout && s.nend != s.nw;
        }
        ok = ok && (s.nout < 0 || (s.nout != s.nw && s.nout != s.nb && s.nout != s.ne));
        if (ok) {
            stage = s;
            for (int nd : {s.nb, s.nc, s.nw, s.nm, s.ne != ground ? s.ne : -1})
                if (nd >= 0) internalNode[static_cast<std::size_t>(nd)] = true;
        }
    }

    // ---- Zeilen: jeder gezeichnete Knoten bekommt eine eigene Zeile (keine zwei Knoten auf einer Linie);
    // offene Anschluesse (nur ein Bein) bekommen keine
    std::vector<int> order;
    for (int i = 0; i < n; ++i) {
        SchematicNode& node = out.nodes[static_cast<std::size_t>(i)];
        node.layer = layer[static_cast<std::size_t>(i)];
        node.drawn = pinCount[static_cast<std::size_t>(i)] >= 2 || i == plus || i == ground;
        const bool placed = internalNode[static_cast<std::size_t>(i)] || (stage.t >= 0 && (i == stage.nend || i == stage.nout));
        if (node.drawn && !placed) order.push_back(i);
    }
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        const auto& na = out.nodes[static_cast<std::size_t>(a)];
        const auto& nb = out.nodes[static_cast<std::size_t>(b)];
        if (na.layer != nb.layer) return na.layer < nb.layer;
        return anchorOrder(na.anchors.front()) < anchorOrder(nb.anchors.front());
    });
    std::vector<double> rowOf(static_cast<std::size_t>(n), 0.0);
    std::vector<bool> drawnNode(static_cast<std::size_t>(n), false);
    for (int i : order) drawnNode[static_cast<std::size_t>(i)] = true;
    for (std::size_t k = 0; k < order.size(); ++k) rowOf[static_cast<std::size_t>(order[k])] = kTop + kRowGap * k;
    // Emitterschaltung: genug Platz zwischen Plus und 0 V; Eingang auf Hoehe der Basis, Ausgang auf Hoehe des
    // Kollektorabzweigs
    if (stage.t >= 0) {
        auto R = [&](int nd) -> double& { return rowOf[static_cast<std::size_t>(nd)]; };
        // unter der Basis: Emitterwiderstand, Basisteiler, haengende Tastkoepfe, Quelle am Eingang
        const double below = stage.re >= 0 ? 4.0 : 3.5;
        const double delta = R(plus) + kStageBase + below - R(ground);
        if (delta > 0) {
            const double from = R(ground) - kEps;
            for (int i : order)
                if (R(i) >= from) R(i) += delta;
        }
        for (int nd : {stage.nend, stage.nout})
            if (nd >= 0 && out.nodes[static_cast<std::size_t>(nd)].drawn) {
                drawnNode[static_cast<std::size_t>(nd)] = true;
                R(nd) = R(plus) + (nd == stage.nend ? kStageBase : kStageBase - 1.4);
            }
    }
    // Raute nur, wenn Plus oben, Wechselspannung dazwischen und Minus unten liegen; sonst allgemeine Anordnung
    bridges.erase(std::remove_if(bridges.begin(), bridges.end(), [&](const Bridge& br) {
        auto R = [&](int nd) { return rowOf[static_cast<std::size_t>(nd)]; };
        const bool ok = R(br.p) < std::min(R(br.a), R(br.b)) && std::max(R(br.a), R(br.b)) < R(br.nn) &&
                        R(br.nn) - R(br.p) >= 2 * kBridge;
        if (!ok) inBridge[br.ap] = inBridge[br.bp] = inBridge[br.na] = inBridge[br.nb] = false;
        return !ok;
    }), bridges.end());

    // ---- Spalten in Signalrichtung; jede Grundschaltung als ein Element statt ihrer Bauteile
    const std::size_t realElements = out.elements.size();
    auto group = [&](const char* type, std::vector<int> nodes) {
        SchematicElement e;
        e.id = type;
        e.type = type;
        e.nodes = std::move(nodes);
        out.elements.push_back(e);
        raw.push_back({e, {}, {}});
    };
    for (const Bridge& br : bridges) group("brueckengleichrichter", {br.p, br.a, br.b, br.nn});
    const std::size_t firstPair = out.elements.size();
    for (const Darlington& d : darlingtons) group("darlington", {d.b, d.c, d.e});
    const std::size_t stageIndex = out.elements.size();
    std::vector<bool> inStage(realElements, false);
    if (stage.t >= 0) {
        group("emitterschaltung", {plus, ground});
        for (int i : {stage.t, stage.rc, stage.re, stage.ce, stage.up, stage.low, stage.pot, stage.cin, stage.rin,
                      stage.cout, stage.probeC, stage.probeB, stage.probeM})
            if (i >= 0) inStage[static_cast<std::size_t>(i)] = true;
    }
    std::vector<std::size_t> cols;
    for (std::size_t i = 0; i < out.elements.size(); ++i)
        if (i >= realElements || (!inBridge[i] && !inPair[i] && !inStage[i])) cols.push_back(i);
    // Reihenfolge: Signalrichtung; was am Eingang der Emitterschaltung haengt, links von ihr, was am Ausgang, rechts
    std::vector<int> rank(out.elements.size());
    for (std::size_t i = 0; i < rank.size(); ++i) rank[i] = signalRank(out.elements[i]) * 10;
    if (stage.t >= 0) {
        rank[stageIndex] = 45;
        for (std::size_t i = 0; i < realElements; ++i) {
            const auto& nd = out.elements[i].nodes;
            if (std::find(nd.begin(), nd.end(), stage.nend) != nd.end()) rank[i] = std::min(rank[i], 44);
            if (stage.nout >= 0 && std::find(nd.begin(), nd.end(), stage.nout) != nd.end()) rank[i] = std::max(rank[i], 46);
        }
    }
    auto topRow = [&](const SchematicElement& e) {
        double t = 1e9;
        for (int nd : e.nodes)
            if (out.nodes[static_cast<std::size_t>(nd)].drawn) t = std::min(t, rowOf[static_cast<std::size_t>(nd)]);
        return t < 1e8 ? t : kTop;
    };
    auto bottomRow = [&](const SchematicElement& e) {
        double b = -1.0;
        for (int nd : e.nodes)
            if (out.nodes[static_cast<std::size_t>(nd)].drawn) b = std::max(b, rowOf[static_cast<std::size_t>(nd)]);
        return b;
    };
    std::stable_sort(cols.begin(), cols.end(), [&](std::size_t a, std::size_t b) {
        const auto& ea = out.elements[a];
        const auto& eb = out.elements[b];
        if (rank[a] != rank[b]) return rank[a] < rank[b];
        if (topRow(ea) != topRow(eb)) return topRow(ea) < topRow(eb);
        return bottomRow(ea) < bottomRow(eb);
    });

    // ---- Elemente setzen und Anschlussleitungen fuehren
    std::vector<std::vector<double>> attach(static_cast<std::size_t>(n)); // x der Anschluesse je Knotenzeile
    auto line = [&](double x1, double y1, double x2, double y2, int node) {
        if (std::fabs(x1 - x2) < kEps && std::fabs(y1 - y2) < kEps) return;
        out.lines.push_back({x1, y1, x2, y2, node});
    };
    double cursor = kLeft;
    double maxY = kTop;
    // Emitterschaltung an der Stelle x0 zeichnen; liefert den Beginn der naechsten Spalte
    auto placeStage = [&](double x0) {
        const Stage& s = stage;
        auto Y = [&](int nd) { return rowOf[static_cast<std::size_t>(nd)]; };
        const double y9 = Y(plus), yG = Y(ground), yb = y9 + kStageBase, yw = y9 + 2.0, yo = yb - 1.4;
        auto el = [&](int i) -> SchematicElement& { return out.elements[static_cast<std::size_t>(i)]; };
        auto attachTo = [&](int nd, double x) { attach[static_cast<std::size_t>(nd)].push_back(x); };
        auto nodeAt = [&](int nd, double xa, double xb, double y) {
            SchematicNode& node = out.nodes[static_cast<std::size_t>(nd)];
            node.x1 = xa;
            node.x2 = xb;
            node.y = y;
        };
        // senkrechter Zweipol (Anschluss am Knoten top oben), Kennzeichnung rechts oder links
        auto vertical = [&](int i, double x, double y, int top, bool labelLeft) {
            SchematicElement& e = el(i);
            e.x = x;
            e.y = y;
            e.rotation = e.nodes[0] == top ? 90 : 270;
            e.labelX = labelLeft ? x - 1.4 : x + 0.6;
            e.labelY = y;
        };
        // waagrechter Zweipol (Anschluss am Knoten left links), Kennzeichnung darueber
        auto horizontal = [&](int i, double x, double y, int left) {
            SchematicElement& e = el(i);
            e.x = x;
            e.y = y;
            e.rotation = e.nodes[0] == left ? 0 : 180;
            e.labelX = x - 0.45;
            e.labelY = y - 0.85;
        };
        // Tastkopf haengt unter der Basis- bzw. Eingangsleitung
        auto probeDown = [&](int i, double x, int nd) {
            SchematicElement& e = el(i);
            e.x = x;
            e.y = yb + 1.5;
            e.rotation = 90;
            e.labelX = x + 0.6;
            e.labelY = e.y;
            line(x, yb, x, yb + 0.5, nd);
            out.dots.push_back({x, yb, nd});
        };
        double xIn = x0, xP = 0.0;
        if (s.pot >= 0) { // Potentiometer links: b oben (Plus), a unten (0 V), Schleifer rechts
            xP = x0 + 1.4;
            SchematicElement& p = el(s.pot);
            p.x = xP;
            p.y = yw;
            p.rotation = 270;
            p.labelX = xP - 1.3;
            p.labelY = yw;
            line(xP, y9, xP, yw - 1.0, plus);
            line(xP, yw + 1.0, xP, yG, ground);
            attachTo(plus, xP);
            attachTo(ground, xP);
            xIn = xP + 0.9;
        }
        double xJ = xIn + 0.6; // Abzweigung an der Basisleitung
        if (s.cin >= 0) {
            double xc = xIn + 1.4;
            if (s.rin >= 0) {
                const double xr = xIn + 1.4;
                horizontal(s.rin, xr, yb, s.nend);
                line(xIn, yb, xr - 1.0, yb, s.nend);
                xc = xr + 2.5;
                line(xr + 1.0, yb, xc - 1.0, yb, s.nm);
                nodeAt(s.nm, xr + 1.0, xc - 1.0, yb);
                if (s.probeM >= 0) probeDown(s.probeM, xr + 1.25, s.nm);
            } else {
                line(xIn, yb, xc - 1.0, yb, s.nend);
            }
            attachTo(s.nend, xIn);
            horizontal(s.cin, xc, yb, s.rin >= 0 ? s.nm : s.nend);
            xJ = xc + 1.6;
            line(xc + 1.0, yb, xJ, yb, s.nb);
        }
        const double xT = xJ + 2.4, xC = xT + 0.25;
        line(xJ, yb, xT - 0.75, yb, s.nb);
        nodeAt(s.nb, xJ, xT - 0.75, yb);
        if (s.pot >= 0) { // Schleifer waagrecht zur Abzweigung, Basiswiderstand senkrecht herab
            line(xP + 0.5, yw, xJ, yw, s.nw);
            nodeAt(s.nw, xP + 0.5, xJ, yw);
            vertical(s.up, xJ, yb - 1.5, s.nw, false);
            line(xJ, yw, xJ, yb - 2.5, s.nw);
            line(xJ, yb - 0.5, xJ, yb, s.nb);
        } else {
            const double ym = (y9 + yb) / 2;
            vertical(s.up, xJ, ym, plus, false);
            line(xJ, y9, xJ, ym - 1.0, plus);
            line(xJ, ym + 1.0, xJ, yb, s.nb);
            attachTo(plus, xJ);
        }
        if (s.low >= 0) {
            vertical(s.low, xJ, yb + 2.0, s.nb, true);
            line(xJ, yb, xJ, yb + 1.0, s.nb);
            line(xJ, yb + 3.0, xJ, yG, ground);
            attachTo(ground, xJ);
        }
        if (s.cin >= 0 || s.low >= 0) out.dots.push_back({xJ, yb, s.nb});
        if (s.probeB >= 0) probeDown(s.probeB, (xJ + xT - 0.75) / 2, s.nb);
        // Transistor, Kollektorwiderstand darueber, Ausgang rechts am Kollektorabzweig
        SchematicElement& t = el(s.t);
        t.x = xT;
        t.y = yb;
        t.rotation = 0;
        t.labelX = xT + 1.3;
        t.labelY = yb;
        vertical(s.rc, xC, y9 + 1.75, plus, false);
        line(xC, y9, xC, y9 + 0.75, plus);
        line(xC, y9 + 2.75, xC, yb - 0.5, s.nc);
        attachTo(plus, xC);
        nodeAt(s.nc, xC, xC, yo);
        double right = xC + 1.8;
        if (s.probeC >= 0) {
            SchematicElement& k = el(s.probeC);
            k.x = xC + 2.0;
            k.y = yo;
            k.rotation = 0;
            k.labelX = k.x + 1.3;
            k.labelY = yo;
            line(xC, yo, xC + 1.0, yo, s.nc);
            out.dots.push_back({xC, yo, s.nc});
            right = xC + 4.0;
        }
        if (s.cout >= 0) {
            horizontal(s.cout, xC + 1.6, yo, s.nc);
            line(xC, yo, xC + 0.6, yo, s.nc);
            line(xC + 2.6, yo, xC + 3.0, yo, s.nout);
            out.dots.push_back({xC, yo, s.nc});
            attachTo(s.nout, xC + 3.0);
            right = xC + 3.0;
        }
        // Emitter direkt an 0 V oder ueber den Emitterwiderstand (Kondensator rechts daneben)
        if (s.re < 0) {
            line(xC, yb + 0.5, xC, yG, ground);
        } else {
            vertical(s.re, xC, yb + 2.5, s.ne, false);
            line(xC, yb + 0.5, xC, yb + 1.5, s.ne);
            line(xC, yb + 3.5, xC, yG, ground);
            nodeAt(s.ne, xC, xC, yb + 1.0);
            if (s.ce >= 0) {
                const double xe = xC + 2.6;
                vertical(s.ce, xe, yb + 2.5, s.ne, false);
                line(xC, yb + 1.0, xe, yb + 1.0, s.ne);
                line(xe, yb + 1.0, xe, yb + 1.5, s.ne);
                line(xe, yb + 3.5, xe, yG, ground);
                out.dots.push_back({xC, yb + 1.0, s.ne});
                attachTo(ground, xe);
                right = std::max(right, xe + 1.8);
            }
        }
        attachTo(ground, xC);
        return right + kColumnGap;
    };
    for (std::size_t c : cols) {
        if (stage.t >= 0 && c == stageIndex) {
            cursor = placeStage(cursor);
            continue;
        }
        SchematicElement& e = out.elements[c];
        const ElementGeo g = geometry(e, rowOf, drawnNode, raw[c].pinNames);
        e.rotation = g.rotation;
        e.block = g.block;
        e.w = g.w;
        e.h = g.h;
        // Hoehe: unter der tiefsten Knotenleitung der oberen Anschluesse (sonst ueber der hoechsten der unteren),
        // damit moeglichst keine Leitung um das Symbol herumgefuehrt werden muss
        double upMax = -1.0, downMin = 1e9;
        for (std::size_t k = 0; k < g.pins.size() && k < e.nodes.size(); ++k) {
            const int nd = e.nodes[k];
            if (g.pins[k].skip || !drawnNode[static_cast<std::size_t>(nd)]) continue;
            const double r = rowOf[static_cast<std::size_t>(nd)];
            if (g.pins[k].exit == Exit::Up) upMax = std::max(upMax, r);
            if (g.pins[k].exit == Exit::Down) downMin = std::min(downMin, r);
        }
        e.y = upMax >= 0.0 ? upMax + kRowGap / 2 : downMin < 1e8 ? downMin - kRowGap / 2 : topRow(e) + kRowGap / 2;
        if (e.type == "brueckengleichrichter") // Raute mitten zwischen Plus- und Minus-Knoten
            e.y = (rowOf[static_cast<std::size_t>(e.nodes[0])] + rowOf[static_cast<std::size_t>(e.nodes[3])]) / 2;
        if (e.type == "darlington") { // Basis auf Hoehe ihrer Leitung, wenn Kollektor darueber und Emitter darunter
            const double rb = rowOf[static_cast<std::size_t>(e.nodes[0])];
            const double rc = rowOf[static_cast<std::size_t>(e.nodes[1])];
            const double re = rowOf[static_cast<std::size_t>(e.nodes[2])];
            if (rc < rb - 0.9 - kEps && re > rb + 1.5 + kEps) e.y = rb + 0.5;
        }
        // seitliche Anschlussleitungen: links und rechts je eigene Spur
        const double symHalfW = g.block ? g.w / 2 : (g.rotation % 180 == 0 ? g.w / 2 : g.h / 2);
        double minX = -symHalfW, maxX = symHalfW;
        std::vector<double> stub(g.pins.size(), 0.0);
        int leftStubs = 0, rightStubs = 0;
        for (std::size_t k = 0; k < g.pins.size(); ++k) {
            const PinGeo& p = g.pins[k];
            if (p.skip || k >= e.nodes.size()) continue;
            const int nd = e.nodes[k];
            if (!out.nodes[static_cast<std::size_t>(nd)].drawn) continue;
            const double target = rowOf[static_cast<std::size_t>(nd)];
            const double py = e.y + p.y;
            const bool wrongWay = (p.exit == Exit::Up && target > py + kEps) || (p.exit == Exit::Down && target < py - kEps);
            if (p.exit == Exit::Left) {
                stub[k] = std::min(p.x, -symHalfW) - kStub * ++leftStubs;
            } else if (p.exit == Exit::Right || wrongWay) {
                stub[k] = std::max(p.x, symHalfW) + kStub * ++rightStubs;
            } else {
                stub[k] = p.x;
            }
            minX = std::min(minX, stub[k]);
            maxX = std::max(maxX, stub[k]);
        }
        const double labelRight = g.block ? 0.0 : kLabelWidth;
        e.x = cursor - minX;
        e.labelX = e.x + (g.block ? -g.w / 2 : maxX + 0.3);
        e.labelY = g.block ? e.y - g.h / 2 - 0.35 : e.y;
        cursor = e.x + std::max(maxX, (g.block ? 0.0 : maxX) + labelRight) + kColumnGap;
        maxY = std::max(maxY, e.y + g.h / 2 + 0.5);

        for (std::size_t k = 0; k < g.pins.size(); ++k) {
            const PinGeo& p = g.pins[k];
            if (g.block) e.pinLabels.push_back({p.label, e.x + p.x * 0.78, e.y + p.y * 0.62});
            if (p.skip || k >= e.nodes.size()) continue;
            const int nd = e.nodes[k];
            const double px = e.x + p.x, py = e.y + p.y;
            if (!out.nodes[static_cast<std::size_t>(nd)].drawn) { // offener Anschluss: kurzer Stummel
                const double d = 0.4;
                line(px, py, px + (p.exit == Exit::Left ? -d : p.exit == Exit::Right ? d : 0.0),
                     py + (p.exit == Exit::Up ? -d : p.exit == Exit::Down ? d : 0.0), nd);
                continue;
            }
            const double target = rowOf[static_cast<std::size_t>(nd)];
            const double sx = e.x + stub[k];
            if (p.exit == Exit::Left || p.exit == Exit::Right) {
                line(px, py, sx, py, nd);
                line(sx, py, sx, target, nd);
            } else if (std::fabs(sx - px) < kEps) {
                line(px, py, px, target, nd);
            } else { // Knoten auf der falschen Seite: kurz heraus, seitlich vorbei, dann zur Zeile
                const double qy = py + (p.exit == Exit::Up ? -0.5 : 0.5);
                line(px, py, px, qy, nd);
                line(px, qy, sx, qy, nd);
                line(sx, qy, sx, target, nd);
            }
            attach[static_cast<std::size_t>(nd)].push_back(sx);
        }
    }

    // ---- Rauten: die vier Dioden schraeg auf den Seiten (Anode -> Kathode zeigt zum Plus-Knoten), dazwischen kurze
    // Leitungen zu den Ecken, Verbindungspunkte an den Ecken
    for (std::size_t r = 0; r < bridges.size(); ++r) {
        const Bridge& br = bridges[r];
        const SchematicElement& d = out.elements[realElements + r];
        const double s = kBridge;
        struct Corner { double x, y; int node; };
        const Corner top{d.x, d.y - s, br.p}, left{d.x - s, d.y, br.a}, right{d.x + s, d.y, br.b},
            bottom{d.x, d.y + s, br.nn};
        auto place = [&](std::size_t idx, const Corner& from, const Corner& to, bool leftSide) {
            SchematicElement& e = out.elements[idx];
            e.x = (from.x + to.x) / 2;
            e.y = (from.y + to.y) / 2;
            const double dx = to.x - from.x, dy = to.y - from.y, len = std::hypot(dx, dy);
            const int deg = static_cast<int>(std::lround(std::atan2(dy, dx) * 180.0 / 3.14159265358979323846));
            e.rotation = (deg + 360) % 360;
            // Schaltzeichen 2 Einheiten lang: Rest der Seite als Leitung zur Ecke
            const double ux = dx / len, uy = dy / len;
            line(from.x, from.y, e.x - ux, e.y - uy, from.node);
            line(e.x + ux, e.y + uy, to.x, to.y, to.node);
            // Kennzeichnung aussen neben der Diode, abseits der Anschlussleitungen an den Ecken
            e.labelX = leftSide ? e.x - 0.9 : e.x + 0.4;
            e.labelY = e.y + (e.y < d.y ? -0.8 : 0.8);
        };
        place(br.ap, left, top, true);
        place(br.bp, right, top, false);
        place(br.na, bottom, left, true);
        place(br.nb, bottom, right, false);
        for (const Corner& c : {top, left, right, bottom}) out.dots.push_back({c.x, c.y, c.node});
    }
    // Darlington: T1 oben links, T2 unten rechts; Emitter T1 -> Basis T2, Kollektoren oben verbunden
    for (std::size_t r = 0; r < darlingtons.size(); ++r) {
        const Darlington& dl = darlingtons[r];
        const SchematicElement& g = out.elements[firstPair + r];
        SchematicElement& t1 = out.elements[dl.t1];
        SchematicElement& t2 = out.elements[dl.t2];
        t1.x = g.x - 0.6;
        t1.y = g.y - 0.5;
        t2.x = g.x + 0.6;
        t2.y = g.y + 0.5;
        t1.rotation = t2.rotation = 0;
        t1.labelX = t1.x - 1.2;
        t1.labelY = t1.y - 0.95;
        t2.labelX = t2.x + 1.1;
        t2.labelY = t2.y;
        line(g.x - 0.35, g.y, g.x - 0.35, g.y + 0.5, dl.d);
        line(g.x - 0.35, g.y + 0.5, g.x - 0.15, g.y + 0.5, dl.d);
        line(g.x - 0.35, g.y - 1.0, g.x - 0.35, g.y - 1.4, dl.c);
        line(g.x - 0.35, g.y - 1.4, g.x + 0.85, g.y - 1.4, dl.c);
        line(g.x + 0.85, g.y, g.x + 0.85, g.y - 1.4, dl.c);
        out.dots.push_back({g.x + 0.85, g.y - 1.4, dl.c});
        SchematicNode& node = out.nodes[static_cast<std::size_t>(dl.d)];
        node.x1 = node.x2 = g.x - 0.35;
        node.y = g.y + 0.5;
    }
    out.elements.resize(realElements);

    // ---- Knotenleitungen und Verbindungspunkte (an jeder Abzweigung)
    for (int i = 0; i < n; ++i) {
        SchematicNode& node = out.nodes[static_cast<std::size_t>(i)];
        if (!node.drawn || internalNode[static_cast<std::size_t>(i)]) continue; // innere Knoten: schon gezeichnet
        node.y = rowOf[static_cast<std::size_t>(i)];
        auto& xs = attach[static_cast<std::size_t>(i)];
        if (xs.empty()) {
            node.x1 = node.x2 = kLeft;
            continue;
        }
        std::sort(xs.begin(), xs.end());
        node.x1 = xs.front();
        node.x2 = xs.back();
        line(node.x1, node.y, node.x2, node.y, i);
        for (std::size_t k = 0; k < xs.size(); ++k) {
            const bool inner = xs[k] > node.x1 + kEps && xs[k] < node.x2 - kEps;
            const bool twice = k > 0 && std::fabs(xs[k] - xs[k - 1]) < kEps;
            if (inner || twice) out.dots.push_back({xs[k], node.y, i});
        }
        maxY = std::max(maxY, node.y);
    }
    out.width = cursor + kLeft;
    out.height = maxY + 1.0;
    return out;
}

} // namespace volta::model

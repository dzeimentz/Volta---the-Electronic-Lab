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
    if (classifyAnchor(a) == AnchorKind::Field) {
        const int col = std::stoi(a.substr(1));
        return 100 + (a[0] - 'A') * 100 + col;
    }
    return 10000;
}

std::string displayName(const std::string& a) {
    if (a.rfind("P:", 0) == 0) return a.substr(2);
    if (a.rfind("MESS:", 0) == 0) return a.substr(5) == "VOHM" ? "V/Ω" : a.substr(5);
    if (a.rfind("M:", 0) == 0) return "M" + a.substr(2);
    return a;
}

// Spalten in Signalrichtung (DIN EN 61082-1: von links nach rechts): Quelle, Eingaenge, Regler, passive Bauteile,
// aktive Bauteile und Module, Verbraucher, Lautsprecher, Messgeraet
int signalRank(const SchematicElement& e) {
    const std::string& t = e.type;
    if (t == "batterie") return 0;
    if (t == "pultschalter" || t == "taster" || t == "schalter" || t == "reedkontakt" || t == "wechselschalter")
        return 1;
    if (t == "potentiometer") return 2;
    if (t == "widerstand" || t == "fotowiderstand" || t == "kondensator" || t == "elko" || t.rfind("diode", 0) == 0)
        return 3;
    if (t == "transistor_npn" || e.module) return 4;
    if (t == "lautsprecher") return 6;
    if (t == "messgeraet") return 7;
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
    if (e.module || t == "messgeraet") {
        // Block: Plus oben, Minus unten, Ausgaenge rechts, uebrige Eingaenge links
        g.block = true;
        g.w = 4.0;
        g.h = 2.0;
        std::vector<std::size_t> left, right;
        g.pins.resize(e.nodes.size());
        for (std::size_t k = 0; k < e.nodes.size(); ++k) {
            const std::string n = k < pinNames.size() ? pinNames[k] : std::string();
            PinGeo& p = g.pins[k];
            p.label = n == "plus" ? "+" : n == "minus" ? "−" : n == "VOHM" ? "V/Ω" : n;
            if (n == "plus" || n == "VOHM") {
                p.y = -g.h / 2;
                p.exit = Exit::Up;
            } else if (n == "minus" || n == "COM") {
                p.y = g.h / 2;
                p.exit = Exit::Down;
            } else if (n == "aus" || n == "A") {
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
    if (t == "transistor_npn" && e.nodes.size() >= 3) {
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
    if (t == "potentiometer" && e.nodes.size() == 3) {
        // senkrecht, Schleifer (Pfeil) von der Seite
        // a oben; fehlt a oder b, entscheidet die Lage gegenueber dem Schleifer
        const bool flip = has(0) && has(2) ? below(0, 2) : has(0) ? below(0, 1) : has(2) ? below(1, 2) : false;
        g.rotation = flip ? 270 : 90;
        g.pins = {{0.0, flip ? 1.0 : -1.0, flip ? Exit::Down : Exit::Up},
                  {flip ? 0.5 : -0.5, 0.0, flip ? Exit::Right : Exit::Left},
                  {0.0, flip ? -1.0 : 1.0, flip ? Exit::Up : Exit::Down}};
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

Schematic buildSchematic(const Board& board, const Catalog& catalog) {
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
    for (int l : layer) maxLayer = std::max(maxLayer, l);
    out.layers = maxLayer + 1;

    // ---- Zeilen: jeder gezeichnete Knoten bekommt eine eigene Zeile (keine zwei Knoten auf einer Linie);
    // offene Anschluesse (nur ein Bein) bekommen keine
    std::vector<int> order;
    for (int i = 0; i < n; ++i) {
        SchematicNode& node = out.nodes[static_cast<std::size_t>(i)];
        node.layer = layer[static_cast<std::size_t>(i)];
        node.drawn = pinCount[static_cast<std::size_t>(i)] >= 2 || i == plus || i == ground;
        if (node.drawn) order.push_back(i);
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

    // ---- Spalten in Signalrichtung
    std::vector<std::size_t> cols(out.elements.size());
    for (std::size_t i = 0; i < cols.size(); ++i) cols[i] = i;
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
        if (signalRank(ea) != signalRank(eb)) return signalRank(ea) < signalRank(eb);
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
    for (std::size_t c : cols) {
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

    // ---- Knotenleitungen und Verbindungspunkte (an jeder Abzweigung)
    for (int i = 0; i < n; ++i) {
        SchematicNode& node = out.nodes[static_cast<std::size_t>(i)];
        if (!node.drawn) continue;
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

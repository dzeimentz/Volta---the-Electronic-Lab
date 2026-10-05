#include "schematic_layout.hpp"

#include <algorithm>
#include <map>
#include <queue>
#include <set>

#include "anchor.hpp"

namespace volta::model {

namespace {

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
    // Elemente (Anschluesse noch als Anschlusspunkte)
    struct Raw {
        SchematicElement e;
        std::vector<std::string> anchors;
    };
    std::vector<Raw> raw;
    for (const auto& p : board.parts) {
        Raw r;
        r.e.id = p.id;
        r.e.type = p.type;
        for (const auto& pin : p.pins) {
            r.anchors.push_back(pin.second);
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
            u.find(a);
        }
        raw.push_back(r);
    }
    // Pultelemente, sofern angeschlossen
    auto used = [&](const std::string& a) { return uses.count(a) && uses[a] > 0; };
    auto deskElement = [&](const char* id, const char* type, std::vector<std::string> anchors) {
        bool any = false;
        for (const auto& a : anchors) any = any || used(a);
        if (!any) return;
        Raw r;
        r.e.id = id;
        r.e.type = type;
        r.e.desk = true;
        r.anchors = std::move(anchors);
        for (const auto& a : r.anchors) u.find(a);
        raw.push_back(r);
    };
    deskElement("PULT.B", "batterie", {kPlusAnchor, kGroundAnchor});
    deskElement("PULT.S", "pultschalter", {"P:S1", "P:S2"});
    deskElement("PULT.P1", "potentiometer", {"P:P1a", "P:P1s", "P:P1b"});
    deskElement("PULT.P2", "potentiometer", {"P:P2a", "P:P2s", "P:P2b"});
    deskElement("PULT.LS", "lautsprecher", {"P:LS1", "P:LS2"});
    if (board.desk.meterVisible) deskElement("PULT.MESS", "messgeraet", {"MESS:COM", "MESS:VOHM", "MESS:A"});

    // Knoten: Gruppen von Anschlusspunkten, die an Elementen haengen
    std::map<std::string, int> nodeOf; // Wurzel -> Knotenindex
    std::map<std::string, std::vector<std::string>> members;
    for (const auto& [a, parent] : u.parent) members[u.find(a)].push_back(a);
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
            }
            r.e.nodes.push_back(it->second);
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

    // Ebenen: Breitensuche ab dem Pluspol, 0 V ganz unten
    const int n = static_cast<int>(out.nodes.size());
    if (n == 0) return out;
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

    // Positionen: Ebene -> y, innerhalb der Ebene gleichmaessig verteilt (nach Feldreihenfolge)
    std::map<int, std::vector<int>> byLayer;
    for (int i = 0; i < n; ++i) byLayer[layer[static_cast<std::size_t>(i)]].push_back(i);
    for (auto& [l, idx] : byLayer) {
        std::sort(idx.begin(), idx.end(), [&](int a, int b) {
            return anchorOrder(out.nodes[static_cast<std::size_t>(a)].anchors.front()) <
                   anchorOrder(out.nodes[static_cast<std::size_t>(b)].anchors.front());
        });
        for (std::size_t k = 0; k < idx.size(); ++k) {
            SchematicNode& node = out.nodes[static_cast<std::size_t>(idx[k])];
            node.layer = l;
            node.y = out.layers > 1 ? static_cast<double>(l) / (out.layers - 1) : 0.5;
            node.x = (static_cast<double>(k) + 1.0) / (static_cast<double>(idx.size()) + 1.0);
        }
    }
    return out;
}

} // namespace volta::model

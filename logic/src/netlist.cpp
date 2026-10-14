#include "volta/logic/netlist.hpp"

#include <algorithm>
#include <numeric>

namespace volta::logic {

namespace {

constexpr std::size_t kMaxDepth = 32; // Schachtelungstiefe der Chips (Schutz vor Rekursion)

struct Endpoint {
    std::string name;
    bool driver = false;  // Eingang des Chips oder Ausgang eines Teils
    bool chipPort = false;
    Direction portDirection = Direction::In;
};

class Elaborator {
public:
    Elaborator(const Library& lib, Netlist& out) : m_lib(lib), m_out(out) {}

    ElaborateResult result;

    // Loest eine Ebene auf; given: Leitung je Anschluss des Chips
    void run(const ChipDef& def, const std::map<std::string, NetId>& given, std::size_t scopeIndex) {
        if (!result.ok()) return;
        if (m_stack.size() >= kMaxDepth || std::find(m_stack.begin(), m_stack.end(), def.kuerzel) != m_stack.end()) {
            fail(ElaborateError::Recursion, def.kuerzel);
            return;
        }
        m_stack.push_back(def.kuerzel);

        // Endpunkte dieser Ebene
        std::vector<Endpoint> eps;
        std::map<std::string, std::size_t> index;
        auto addEp = [&](const std::string& name, bool driver, bool port, Direction dir) {
            index[name] = eps.size();
            eps.push_back({name, driver, port, dir});
        };
        for (const Port& p : def.ports) addEp(p.name, p.direction == Direction::In, true, p.direction);
        std::vector<std::string> ins, outs;
        for (const Part& part : def.parts) {
            if (!m_lib.pinsOf(part.type, ins, outs)) {
                fail(ElaborateError::UnknownType, part.type);
                m_stack.pop_back();
                return;
            }
            for (const std::string& pin : ins) addEp(part.id + "." + pin, false, false, Direction::In);
            for (const std::string& pin : outs) addEp(part.id + "." + pin, true, false, Direction::In);
        }

        // Gruppen verbundener Endpunkte (Union-Find)
        std::vector<std::size_t> parent(eps.size());
        std::iota(parent.begin(), parent.end(), std::size_t{0});
        auto root = [&](std::size_t i) {
            while (parent[i] != i) i = parent[i] = parent[parent[i]];
            return i;
        };
        for (const Connection& c : def.connections) {
            const auto f = index.find(c.from), t = index.find(c.to);
            if (f == index.end() || t == index.end()) {
                fail(ElaborateError::UnknownEndpoint, f == index.end() ? c.from : c.to);
                m_stack.pop_back();
                return;
            }
            parent[root(f->second)] = root(t->second);
        }

        // Leitung je Gruppe; Anschluesse des Chips bringen ihre Leitung von aussen mit
        std::vector<std::vector<std::size_t>> groups(eps.size());
        for (std::size_t i = 0; i < eps.size(); ++i) groups[root(i)].push_back(i);
        Scope& scope = m_out.scopes[scopeIndex];
        scope.kuerzel = def.kuerzel;
        for (const auto& members : groups) {
            if (members.empty()) continue;
            int drivers = 0;
            const Endpoint* secondDriver = nullptr;
            bool haveIn = false;
            NetId inNet = 0;
            std::vector<NetId> outNets;
            for (std::size_t i : members) {
                const Endpoint& e = eps[i];
                if (e.driver && ++drivers == 2) secondDriver = &e;
                if (!e.chipPort) continue;
                const NetId n = given.at(e.name);
                if (e.portDirection == Direction::In) {
                    haveIn = true;
                    inNet = n;
                } else if (std::find(outNets.begin(), outNets.end(), n) == outNets.end()) {
                    outNets.push_back(n);
                }
            }
            if (secondDriver) {
                fail(ElaborateError::MultipleDrivers, secondDriver->name);
                m_stack.pop_back();
                return;
            }
            const NetId net = haveIn ? inNet : !outNets.empty() ? outNets.front() : m_out.netCount++;
            for (NetId o : outNets)
                if (o != net) m_out.gates.push_back({Primitive::Buf, net, net, o}); // Durchverbindung zum Ausgang
            for (std::size_t i : members) m_out.scopes[scopeIndex].endpoints[eps[i].name] = net;
        }

        // Teile: Gatter direkt, Chips rekursiv
        for (const Part& part : def.parts) {
            const std::map<std::string, NetId>& ep = m_out.scopes[scopeIndex].endpoints;
            if (const auto prim = primitiveFromName(part.type)) {
                Gate g;
                g.kind = *prim;
                g.a = ep.at(part.id + ".a");
                g.b = *prim == Primitive::Not ? g.a : ep.at(part.id + ".b");
                g.y = ep.at(part.id + ".y");
                m_out.gates.push_back(g);
                continue;
            }
            const ChipDef* child = m_lib.find(part.type);
            std::map<std::string, NetId> childGiven;
            for (const Port& p : child->ports) childGiven[p.name] = ep.at(part.id + "." + p.name);
            const std::size_t childIndex = m_out.scopes.size();
            m_out.scopes.emplace_back();
            m_out.scopes[scopeIndex].children[part.id] = childIndex;
            run(*child, childGiven, childIndex);
            if (!result.ok()) break;
        }
        m_stack.pop_back();
    }

private:
    void fail(ElaborateError e, const std::string& where) {
        if (!result.ok()) return;
        result.error = e;
        result.where = where;
    }

    const Library& m_lib;
    Netlist& m_out;
    std::vector<std::string> m_stack;
};

} // namespace

ElaborateResult elaborate(const ChipDef& def, const Library& lib, Netlist& out) {
    out = Netlist{};
    out.scopes.emplace_back();
    std::map<std::string, NetId> given;
    for (const Port& p : def.ports) {
        const NetId n = out.netCount++;
        given[p.name] = n;
        (p.direction == Direction::In ? out.inputs : out.outputs).push_back(n);
    }
    Elaborator e(lib, out);
    e.run(def, given, 0);
    return e.result;
}

ElaborateResult validate(const ChipDef& def, const Library& lib) {
    Netlist scratch;
    return elaborate(def, lib, scratch);
}

} // namespace volta::logic

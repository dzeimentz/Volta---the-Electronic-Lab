#include "netlist_builder.hpp"

#include <unordered_map>

#include "anchor.hpp"
#include "environment.hpp"
#include "volta/core/components/magnet.hpp"
#include "volta/core/components/sensors.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/buzzer.hpp"
#include "volta/core/components/capacitor.hpp"
#include "volta/core/components/diode.hpp"
#include "volta/core/components/lamp.hpp"
#include "volta/core/components/meter.hpp"
#include "volta/core/components/motor.hpp"
#include "volta/core/components/potentiometer.hpp"
#include "volta/core/components/speaker.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/switch.hpp"
#include "volta/core/components/transistor.hpp"
#include "volta/core/components/wire.hpp"

namespace volta::model {

using core::NodeId;

const PartPlacement* Board::findPart(const std::string& id) const {
    for (const auto& p : parts)
        if (p.id == id) return &p;
    return nullptr;
}

bool Netlist::nodeOf(const std::string& anchor, NodeId& out) const {
    const auto it = nodes.find(anchor);
    if (it == nodes.end()) return false;
    out = it->second;
    return true;
}

namespace {

class Builder {
public:
    Builder(const Board& board, const Catalog& catalog, Netlist& net) : m_board(board), m_catalog(catalog), m_net(net) {}

    void run() {
        m_net.nodes[kGroundAnchor] = core::kGround;
        mapModulePins();
        buildDesk();
        for (const auto& p : m_board.parts) buildPart(p);
        for (const auto& w : m_board.wires) buildWire(w);
        for (const auto& m : m_board.modules) buildModule(m);
    }

private:
    // Kanonischer Anschlusspunkt: Modulanschluss "BL1.takt" -> Feld der Modulleiste "M:n".
    std::string canonical(const std::string& anchor) const {
        const auto it = m_pinToSlot.find(anchor);
        return it == m_pinToSlot.end() ? anchor : it->second;
    }

    // Knoten eines Anschlusspunkts, bei erster Verwendung angelegt; false bei ungueltigem Bezeichner.
    bool node(const std::string& anchor, NodeId& out, const std::string& context) {
        const std::string key = canonical(anchor);
        const auto kind = classifyAnchor(key);
        if (kind == AnchorKind::Invalid || kind == AnchorKind::ModulePin) {
            m_net.errors.push_back(context + ": unbekannter Anschlusspunkt '" + anchor + "'");
            return false;
        }
        const auto it = m_net.nodes.find(key);
        if (it != m_net.nodes.end()) {
            out = it->second;
        } else {
            out = m_net.circuit.addNode();
            m_net.nodes[key] = out;
        }
        if (key != anchor) m_net.nodes[anchor] = out; // auch unter dem Modulnamen auffindbar
        return true;
    }

    NodeId internalNode() { return m_net.circuit.addNode(); }

    void mapModulePins() {
        for (const auto& m : m_board.modules) {
            const ModuleEntry* entry = m_catalog.module(m.type);
            if (!entry) continue; // Fehler meldet buildModule
            for (std::size_t k = 0; k < entry->pins.size(); ++k)
                m_pinToSlot[m.id + "." + entry->pins[k]] = moduleSlot(m.slot + static_cast<int>(k));
        }
    }

    void buildDesk() {
        auto& c = m_net.circuit;
        const DeskState& d = m_board.desk;
        NodeId plus = 0, p1a = 0, p1s = 0, p1b = 0, p2a = 0, p2s = 0, p2b = 0, s1 = 0, s2 = 0, ls1 = 0, ls2 = 0;
        NodeId com = 0, vo = 0, amp = 0;
        node(kPlusAnchor, plus, "Pult");
        node("P:P1a", p1a, "Pult");
        node("P:P1s", p1s, "Pult");
        node("P:P1b", p1b, "Pult");
        node("P:P2a", p2a, "Pult");
        node("P:P2s", p2s, "Pult");
        node("P:P2b", p2b, "Pult");
        node("P:S1", s1, "Pult");
        node("P:S2", s2, "Pult");
        node("P:LS1", ls1, "Pult");
        node("P:LS2", ls2, "Pult");
        node("MESS:COM", com, "Pult");
        node("MESS:VOHM", vo, "Pult");
        node("MESS:A", amp, "Pult");

        const NodeId cell = internalNode(); // Pluspol der Batterie vor dem Hauptschalter
        m_net.desk[desk_id::kBattery] = c.componentCount();
        c.add<core::Battery>(desk_id::kBattery, cell, core::kGround, core::BatteryParams{d.cells});
        m_net.desk[desk_id::kMainSwitch] = c.componentCount();
        c.add<core::Switch>(desk_id::kMainSwitch, cell, plus, d.mainSwitch);
        m_net.desk[desk_id::kPot1] = c.componentCount();
        c.add<core::Potentiometer>(desk_id::kPot1, p1a, p1s, p1b, core::PotentiometerParams{}, d.p1);
        m_net.desk[desk_id::kPot2] = c.componentCount();
        c.add<core::Potentiometer>(desk_id::kPot2, p2a, p2s, p2b, core::PotentiometerParams{}, d.p2);
        m_net.desk[desk_id::kSwitch] = c.componentCount();
        c.add<core::Switch>(desk_id::kSwitch, s1, s2, d.switchClosed);
        // Lautsprecher: 8 Ohm, 0,5 W, mit Auswertung fuer die Klanganzeige (Tonhoehe, Signalstaerke, Impulse)
        m_net.desk[desk_id::kSpeaker] = c.componentCount();
        c.add<core::Speaker>(desk_id::kSpeaker, ls1, ls2);
        m_net.desk[desk_id::kMeter] = c.componentCount();
        core::Meter& meter = c.add<core::Meter>(desk_id::kMeter, com, vo, amp);
        meter.setMode(d.meterMode);
        meter.setAnalog(d.meterAnalog);
    }

    // Anschlusspunkt eines Bauteilanschlusses nach Katalogname
    bool pin(const PartPlacement& p, const std::string& name, NodeId& out) {
        for (const auto& [pinName, anchor] : p.pins)
            if (pinName == name) return node(anchor, out, "Bauteil " + p.id);
        m_net.errors.push_back("Bauteil " + p.id + ": Anschluss '" + name + "' fehlt");
        return false;
    }

    bool pins(const PartPlacement& p, const CatalogEntry& e, std::vector<NodeId>& out) {
        out.clear();
        bool ok = true;
        for (const auto& name : e.pins) {
            NodeId n = 0;
            ok = pin(p, name, n) && ok;
            out.push_back(n);
        }
        return ok;
    }

    void buildPart(const PartPlacement& p) {
        const CatalogEntry* e = m_catalog.part(p.type);
        if (!e) {
            m_net.errors.push_back("Bauteil " + p.id + ": unbekannter Typ '" + p.type + "'");
            return;
        }
        std::vector<NodeId> n;
        if (!pins(p, *e, n)) return;
        auto& c = m_net.circuit;
        const std::size_t index = c.componentCount();
        const std::string& m = e->model;

        if (m == "resistor" && n.size() == 2) {
            core::ResistorParams rp;
            rp.ohm = p.value > 0.0 ? p.value : 1000.0;
            rp.pmaxW = e->param("pmax_W", rp.pmaxW);
            c.add<core::Resistor>(p.id, n[0], n[1], rp);
        } else if ((m == "led" || m == "diode") && n.size() == 2) {
            c.add<core::Diode>(p.id, n[0], n[1], diodeParams(*e));
        } else if (m == "lamp" && n.size() == 2) {
            // Variante ueber die Nennspannung (Wert des Bauteils); ohne Wert: Katalogvorgabe
            core::LampParams lp;
            lp.uNomV = p.value > 0.0 ? p.value : e->param("u_nenn_V", lp.uNomV);
            lp.iNomA = e->variantParam("u_nenn_V", lp.uNomV, "i_nenn_mA", lp.iNomA * 1e3) * 1e-3;
            c.add<core::Lamp>(p.id, n[0], n[1], lp);
        } else if (m == "switch" && n.size() == 2) {
            c.add<core::Switch>(p.id, n[0], n[1], p.closed);
        } else if (m == "changeover_switch" && n.size() == 3) {
            c.add<core::ChangeoverSwitch>(p.id, n[0], n[1], n[2], p.closed);
        } else if (m == "motor" && n.size() == 2) {
            core::MotorParams mp;
            mp.raOhm = e->param("ra_ohm", mp.raOhm);
            c.add<core::Motor>(p.id, n[0], n[1], mp);
        } else if (m == "buzzer" && n.size() == 2) {
            core::BuzzerParams bp;
            bp.frequencyHz = e->param("f_Hz", bp.frequencyHz);
            c.add<core::Buzzer>(p.id, n[0], n[1], bp);
        } else if (m == "capacitor" && n.size() == 2) {
            c.add<core::Capacitor>(p.id, n[0], n[1], core::CapacitorParams::film(p.value > 0.0 ? p.value : 100e-9));
        } else if (m == "capacitor_polar" && n.size() == 2) {
            c.add<core::Capacitor>(p.id, n[0], n[1],
                                   core::CapacitorParams::electrolytic(p.value > 0.0 ? p.value : 100e-6,
                                                                       e->param("umax_V", 16.0)));
        } else if (m == "bjt_npn" && (n.size() == 3 || n.size() == 4)) {
            core::BjtParams bp;
            bp.betaF = e->param("beta", bp.betaF);
            c.add<core::Npn>(p.id, n[0], n[1], n[2], bp);
            // 2-x-2-Platine: der vierte Fuss (e2) ist ueber eine Leiterbahn mit E verbunden
            if (n.size() == 4) c.add<core::Wire>(p.id + ".E2", n[2], n[3]);
        } else if (m == "ldr" && n.size() == 2) {
            core::PhotoresistorParams lp;
            lp.r10Ohm = e->param("r10_ohm", lp.r10Ohm);
            lp.gamma = e->param("gamma", lp.gamma);
            lp.darkOhm = e->param("r_dunkel_ohm", lp.darkOhm);
            c.add<core::Photoresistor>(p.id, n[0], n[1], lp, illuminanceAt(m_board, p));
        } else if (m == "ntc" && n.size() == 2) {
            core::ThermistorParams tp;
            tp.r25Ohm = e->param("r25_ohm", tp.r25Ohm);
            tp.betaK = e->param("b_K", tp.betaK);
            c.add<core::Thermistor>(p.id, n[0], n[1], tp, airTemperatureAt(m_board, p));
        } else if (m == "electromagnet" && n.size() == 2) {
            core::ElectromagnetParams ep;
            ep.ohm = e->param("r_ohm", ep.ohm);
            c.add<core::Electromagnet>(p.id, n[0], n[1], ep);
        } else if (m == "relay" && n.size() == 4) {
            core::RelayParams rp;
            rp.coilOhm = e->param("r_spule_ohm", rp.coilOhm);
            c.add<core::Relay>(p.id, n[0], n[1], n[2], n[3], rp);
        } else if (m == "potentiometer" && n.size() == 3) {
            core::PotentiometerParams pp;
            pp.ohm = p.value > 0.0 ? p.value : pp.ohm;
            c.add<core::Potentiometer>(p.id, n[0], n[1], n[2], pp, p.position);
        } else {
            m_net.errors.push_back("Bauteil " + p.id + ": Modell '" + m + "' noch nicht im Simulationskern");
            return;
        }
        m_net.parts[p.id] = index;
    }

    static core::DiodeParams diodeParams(const CatalogEntry& e) {
        core::DiodeParams d = core::DiodeParams::silicon();
        if (e.type == "diode_ge") d = core::DiodeParams::germanium();
        else if (e.type == "led_gruen") d = core::DiodeParams::ledGreen();
        else if (e.type == "led_gelb") d = core::DiodeParams::ledYellow();
        else if (e.type == "led_blau") d = core::DiodeParams::ledBlue();
        else if (e.model == "led") d = core::DiodeParams::ledRed();
        d.ufV = e.param("uf_V", d.ufV); // Katalogwert bestimmt die Kennlinie
        d.imaxA = e.param("imax_mA", d.imaxA * 1e3) * 1e-3;
        return d;
    }

    void buildWire(const WirePlacement& w) {
        NodeId a = 0, b = 0;
        const std::string ctx = "Leitung " + w.id;
        if (!node(w.from, a, ctx) || !node(w.to, b, ctx)) return;
        m_net.wires[w.id] = m_net.circuit.componentCount();
        m_net.circuit.add<core::Wire>("LTG." + w.id, a, b);
    }

    void buildModule(const ModulePlacement& m) {
        const ModuleEntry* e = m_catalog.module(m.type);
        if (!e) {
            m_net.errors.push_back("Modul " + m.id + ": unbekannter Typ '" + m.type + "'");
            return;
        }
        std::vector<NodeId> n;
        for (const auto& pinName : e->pins) {
            NodeId x = 0;
            if (!node(m.id + "." + pinName, x, "Modul " + m.id)) return;
            n.push_back(x);
        }
        if (m.type == "blinkmodul" && n.size() == 4) {
            // Katalog: plus, minus, aus, takt
            const core::SubcircuitRange r = core::addBlinkModule(m_net.circuit, m.id, {n[0], n[1], n[2], n[3]});
            m_net.modules[m.id] = r;
            for (std::size_t i = r.first; i < r.first + r.count; ++i)
                if (m_net.circuit.component(i).id() == m.id + ".T3") m_net.moduleMain[m.id] = i;
        } else if (m.type == "tonmodul" && n.size() == 4) {
            // Katalog: plus, minus, ein, aus
            const core::SubcircuitRange r = core::addToneModule(m_net.circuit, m.id, {n[0], n[1], n[2], n[3]});
            m_net.modules[m.id] = r;
            m_net.moduleMain[m.id] = r.first;
        } else {
            m_net.errors.push_back("Modul " + m.id + ": '" + m.type + "' noch nicht im Simulationskern");
        }
    }

    const Board& m_board;
    const Catalog& m_catalog;
    Netlist& m_net;
    std::unordered_map<std::string, std::string> m_pinToSlot;
};

} // namespace

Netlist buildNetlist(const Board& board, const Catalog& catalog) {
    Netlist net;
    Builder(board, catalog, net).run();
    net.componentIds.reserve(net.circuit.componentCount());
    for (std::size_t i = 0; i < net.circuit.componentCount(); ++i) net.componentIds.push_back(net.circuit.component(i).id());
    return net;
}

void transferStates(const core::Circuit& from, core::Circuit& to) {
    std::unordered_map<std::string, std::size_t> byId;
    for (std::size_t i = 0; i < from.componentCount(); ++i) byId[from.component(i).id()] = i;
    for (std::size_t i = 0; i < to.componentCount(); ++i) {
        const auto it = byId.find(to.component(i).id());
        if (it != byId.end()) to.component(i).restoreState(from.component(it->second).saveState());
    }
}

} // namespace volta::model

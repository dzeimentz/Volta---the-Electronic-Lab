#include "netlist_builder.hpp"

#include <algorithm>
#include <set>
#include <unordered_map>

#include "anchor.hpp"
#include "board_edit.hpp"
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
#include "volta/core/components/sources.hpp"
#include "volta/core/components/speaker.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/switch.hpp"
#include "volta/core/components/transistor.hpp"
#include "volta/core/components/inductor.hpp"
#include "volta/core/components/mosfet.hpp"
#include "volta/core/components/thyristor.hpp"
#include "volta/core/components/transformer.hpp"
#include "volta/core/components/optics.hpp"
#include "volta/core/components/regulator.hpp"
#include "volta/core/modules/opamp_module.hpp"
#include "volta/core/modules/fm_module.hpp"
#include "volta/core/modules/radio_module.hpp"
#include "volta/core/modules/timer555_module.hpp"
#include "volta/core/modules/gate_module.hpp"
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
        coupleInfrared();
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

    static constexpr double kFloatingLeakOhm = 100e6; // Konzept Volta 2, 5.1

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
        if (hasDeskPart(m_board, "batterie2")) {
            NodeId plus2 = 0, minus2 = 0;
            node("P:+B2", plus2, "Pult");
            node("P:-B2", minus2, "Pult");
            // Zellen zwischen zwei inneren Knoten; der Hauptschalter trennt beide Pole (zweipolig), damit die
            // ausgeschaltete Batterie keine Scheinspannung ueber die Isolationsleitwerte zeigt
            const NodeId cell2 = internalNode(), cell2m = internalNode();
            m_net.desk[desk_id::kBattery2] = c.componentCount();
            c.add<core::Battery>(desk_id::kBattery2, cell2, cell2m, core::BatteryParams{d.cells2});
            m_net.desk[desk_id::kMainSwitch2] = c.componentCount();
            c.add<core::Switch>(desk_id::kMainSwitch2, cell2, plus2, d.mainSwitch);
            m_net.desk[desk_id::kMainSwitch2Minus] = c.componentCount();
            c.add<core::Switch>(desk_id::kMainSwitch2Minus, cell2m, minus2, d.mainSwitch);
            // Ableitwiderstand 10 MOhm von -B2 nach 0V: haelt die potentialfreie Batterie fuer den Kern rechenbar und
            // bei offenem Hauptschalter nahe 0 V (< 0,05 V); als Last 0,45 uA bei 4,5 V, also ohne sichtbare Wirkung
            // (offengelegt unter »So rechnet Volta«, Konzept 5.1)
            core::ResistorParams leak;
            leak.ohm = 10e6;
            c.add<core::Resistor>(desk_id::kBattery2Leak, minus2, core::kGround, leak);
        }
        if (hasDeskPart(m_board, "wechselquelle")) {
            // Potentialfrei wie die Wicklung eines Transformators (Konzept 5.1). Der Hauptschalter trennt wie beim
            // echten Geraet die Primaerseite: ausgeschaltet liegt nur die Sekundaerwicklung zwischen W~a und W~b.
            // Der Ableitwiderstand von W~b nach 0V haelt sie rechenbar (offengelegt unter »So rechnet Volta«).
            NodeId wa = 0, wb = 0;
            node("P:W~a", wa, "Pult");
            node("P:W~b", wb, "Pult");
            m_net.desk[desk_id::kAcSource] = c.componentCount();
            core::AcSourceParams ac;
            ac.rmsV = d.acRmsV;
            c.add<core::AcSource>(desk_id::kAcSource, wa, wb, ac).setEnabled(d.mainSwitch);
            core::ResistorParams leak;
            leak.ohm = kFloatingLeakOhm;
            c.add<core::Resistor>(desk_id::kAcLeak, wb, core::kGround, leak);
        }
        if (hasDeskPart(m_board, "generator")) {
            // laeuft mit dem Hauptschalter (entschieden 02.11.2026: ein Schalter fuer alles); G0 fest an 0V (Konzept 5.1)
            NodeId g = 0, g0 = 0;
            node("P:G", g, "Pult");
            node("P:G0", g0, "Pult");
            core::GeneratorParams gp;
            gp.waveform = static_cast<core::Waveform>(std::clamp(d.genWaveform, 0, 2));
            gp.frequencyHz = d.genFrequencyHz;
            gp.amplitudeV = d.genAmplitudeV;
            gp.offsetV = d.genOffsetV;
            gp.amDepth = d.genAmDepth;
            gp.amToneHz = d.genAmToneHz;
            m_net.desk[desk_id::kGenerator] = c.componentCount();
            c.add<core::FunctionGenerator>(desk_id::kGenerator, g, g0, gp).setEnabled(d.mainSwitch);
            c.add<core::Wire>(desk_id::kGeneratorGround, g0, core::kGround);
        }
        if (hasDeskPart(m_board, "senderanlage")) {
            // Antenneneinspeisung der Senderanlage gegen 0V (ab Volta 3): im Zeitbereich nur ihr Innenwiderstand, die
            // Sender wirken ueber das Abstimmen (core::tuneRadio)
            NodeId ant = 0;
            node("P:ANT", ant, "Pult");
            m_net.desk[desk_id::kAntenna] = c.componentCount();
            c.add<core::AntennaFeed>(desk_id::kAntenna, ant, core::kGround);
        }
        if (hasDeskPart(m_board, "kennlinienschreiber")) {
            // Laborgeraet mit eigenem Netzteil, potentialfrei wie ein echter Kennlinienschreiber: das Bauteil darf
            // irgendwo in der Schaltung haengen. Der Ableitwiderstand von KS- nach 0V haelt ihn rechenbar.
            NodeId ksPlus = 0, ksMinus = 0;
            node("P:KS+", ksPlus, "Pult");
            node("P:KS-", ksMinus, "Pult");
            core::TracerParams tp;
            tp.minV = d.tracer.minV();
            tp.maxV = d.tracer.maxV();
            tp.seriesOhm = d.tracer.seriesOhm();
            tp.periodS = kTracerPeriodS;
            m_net.desk[desk_id::kTracer] = c.componentCount();
            c.add<core::CurveTracer>(desk_id::kTracer, ksPlus, ksMinus, tp);
            core::ResistorParams leak;
            leak.ohm = kFloatingLeakOhm;
            c.add<core::Resistor>(desk_id::kTracerLeak, ksMinus, core::kGround, leak);
        }
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
        } else if (m == "diode" && p.type == "diode_ge" && n.size() == 2 && antennaWired(m_board)) {
            // Radio (ab Volta 3): mit angeschlossener Antenne rechnet die Ge-Diode als Detektordiode (ohne Traeger
            // dieselbe Kennlinie); tuneRadio gibt ihr den Traeger
            c.add<core::DetectorDiode>(p.id, n[0], n[1], diodeParams(*e));
        } else if ((m == "led" || m == "diode") && n.size() == 2) {
            auto& d = c.add<core::Diode>(p.id, n[0], n[1], diodeParams(*e));
            if (p.type == "ir_led") m_irLeds.push_back({&d, &p});
            // Diode als Thermometer (ab Volta 3): mit Foehn auf der Grundplatte folgt die Kennlinie der Temperatur
            if (m == "diode" && m_catalog.secondOrder && toolPlaced(m_board, kToolDryer))
                d.setAmbient(airTemperatureAt(m_board, p));
        } else if (m == "lamp" && n.size() == 2) {
            // Variante ueber die Nennspannung (Wert des Bauteils); ohne Wert: Katalogvorgabe
            core::LampParams lp;
            lp.uNomV = p.value > 0.0 ? p.value : e->param("u_nenn_V", lp.uNomV);
            lp.iNomA = e->variantParam("u_nenn_V", lp.uNomV, "i_nenn_mA", lp.iNomA * 1e3) * 1e-3;
            c.add<core::Lamp>(p.id, n[0], n[1], lp);
        } else if (m == "switch" && n.size() == 2) {
            core::Switch& sw = c.add<core::Switch>(p.id, n[0], n[1], p.closed);
            // ab Volta 3 mit Logik auf der Platte (Gatter-Modul oder Logik-Tastkopf): der Taster prellt
            if (p.type == "taster" && m_catalog.secondOrder && logicOnBoard(m_board)) sw.setBounce(true);
        } else if (m == "changeover_switch" && n.size() == 3) {
            c.add<core::ChangeoverSwitch>(p.id, n[0], n[1], n[2], p.closed);
        } else if (m == "motor" && n.size() == 2) {
            core::MotorParams mp;
            mp.raOhm = e->param("ra_ohm", mp.raOhm);
            c.add<core::Motor>(p.id, n[0], n[1], mp);
        } else if (m == "buzzer" && n.size() == 2) {
            core::BuzzerParams bp;
            bp.frequencyHz = e->param("f_Hz", bp.frequencyHz);
            bp.resistanceOhm = e->param("r_ohm", bp.resistanceOhm); // Piezo: wenig Strom
            bp.vMinV = e->param("u_min_V", bp.vMinV);
            c.add<core::Buzzer>(p.id, n[0], n[1], bp);
        } else if (m == "capacitor" && n.size() == 2) {
            c.add<core::Capacitor>(p.id, n[0], n[1], core::CapacitorParams::film(p.value > 0.0 ? p.value : 100e-9));
        } else if (m == "earpiece" && n.size() == 2) {
            // Kristallhoerer (ab Volta 3): Piezoscheibe als Kapazitaet mit Parallelwiderstand (Katalog c_F, r_ohm;
            // typisch einige nF und sehr hochohmig). Hoerbar ist die Spannung an ihm (LabSession::updateScope).
            core::CapacitorParams cp = core::CapacitorParams::film(e->param("c_F", 10e-9));
            cp.leakOhm = e->param("r_ohm", 1e6);
            c.add<core::Capacitor>(p.id, n[0], n[1], cp);
        } else if (m == "capacitor_variable" && n.size() == 2) {
            // Drehkondensator (ab Volta 3): Kapazitaet aus der Stellung des Drehknopfs
            c.add<core::Capacitor>(p.id, n[0], n[1], core::CapacitorParams::film(variableCapacitanceF(*e, p.position)));
        } else if (m == "capacitor_polar" && n.size() == 2) {
            c.add<core::Capacitor>(p.id, n[0], n[1],
                                   core::CapacitorParams::electrolytic(p.value > 0.0 ? p.value : 100e-6,
                                                                       e->param("umax_V", 16.0)));
        } else if (m == "bjt_npn" && (n.size() == 3 || n.size() == 4)) {
            // Leistungstransistor (ab Volta 3, Katalog `leistung` = 1): Kennwerte angelehnt an BD139
            core::BjtParams bp = e->param("leistung", 0.0) > 0.0 ? core::BjtParams::bd139() : core::BjtParams{};
            bp.betaF = e->param("beta", bp.betaF);
            c.add<core::Npn>(p.id, n[0], n[1], n[2], bp);
            // 2-x-2-Platine: der vierte Fuss (e2) ist ueber eine Leiterbahn mit E verbunden
            if (n.size() == 4) c.add<core::Wire>(p.id + ".E2", n[2], n[3]);
        } else if (m == "bjt_pnp" && (n.size() == 3 || n.size() == 4)) {
            core::BjtParams bp = e->param("leistung", 0.0) > 0.0 ? core::BjtParams::bd140() : core::BjtParams::bc557();
            bp.betaF = e->param("beta", bp.betaF);
            c.add<core::Npn>(p.id, n[0], n[1], n[2], bp);
            if (n.size() == 4) c.add<core::Wire>(p.id + ".E2", n[2], n[3]);
        } else if ((m == "mosfet_n" || m == "mosfet_p") && (n.size() == 3 || n.size() == 4)) {
            // MOSFET (ab Volta 3, Stufe 5); 2-x-2-Platine wie der Transistor, der vierte Fuss (s2) ist die Source
            core::MosfetParams mp = m == "mosfet_p" ? core::MosfetParams::pChannelDefault() : core::MosfetParams::nChannel();
            mp.vthV = e->param("u_th_V", mp.vthV);
            c.add<core::Mosfet>(p.id, n[0], n[1], n[2], mp);
            if (n.size() == 4) c.add<core::Wire>(p.id + ".S2", n[2], n[3]);
        } else if (m == "phototransistor" && n.size() == 2) {
            core::PhototransistorParams tp;
            tp.luxSensitivityA = e->param("s_uA_lx", tp.luxSensitivityA * 1e6) * 1e-6;
            auto& q = c.add<core::Phototransistor>(p.id, n[0], n[1], tp, illuminanceAt(m_board, p));
            q.setShade(shadeAt(m_board, p));
            m_receivers.push_back({&q, &p});
        } else if (m == "transformer" && n.size() == 4) {
            // Uebertrager (ab Volta 3): zwei gekoppelte Wicklungen, Anschluesse a1, e1, a2, e2
            core::TransformerParams tp;
            tp.inductance1H = tp.inductance2H = e->param("l_H", tp.inductance1H);
            tp.coupling = e->param("k", tp.coupling);
            c.add<core::Transformer>(p.id, n[0], n[1], n[2], n[3], tp);
        } else if (m == "thyristor" && n.size() == 3) {
            // Thyristor (ab Volta 3): Anschluesse anode, kathode, gate wie im Kern
            c.add<core::Thyristor>(p.id, n[0], n[1], n[2], core::ThyristorParams::scr());
        } else if (m == "triac" && n.size() == 3) {
            // Triac (ab Volta 3): mt2, mt1, gate
            c.add<core::Thyristor>(p.id, n[0], n[1], n[2], core::ThyristorParams::triac());
        } else if (m == "diac" && n.size() == 2) {
            // Diac (ab Volta 3): Kippdiode fuer Kleinspannung
            core::ThyristorParams dp = core::ThyristorParams::diac();
            dp.breakoverV = e->param("u_kipp_V", dp.breakoverV);
            c.add<core::Thyristor>(p.id, n[0], n[1], dp);
        } else if (m == "optocoupler" && n.size() == 4) {
            core::OptocouplerParams op;
            op.ctr = e->param("ctr", op.ctr);
            c.add<core::Optocoupler>(p.id, n[0], n[1], n[2], n[3], op);
        } else if (m == "regulator" && n.size() == 3) {
            c.add<core::VoltageRegulator>(p.id, n[0], n[1], n[2]); // 78L05-artig (Kennwerte im Kern)
        } else if (m == "inductor" && n.size() == 2) {
            // 220 uH, 1 mH, 10 mH (ab Volta 3), 100 mH oder 1 H (Katalog l_H); Wicklungswiderstand und Grenzstrom wie
            // die Vorgaben des Kerns
            // Speicherdrossel (ab Volta 3, Katalog `drossel` = 1): eigene Kennwerte mit wenig Wicklungswiderstand
            const double l = e->param("l_H", 0.1);
            c.add<core::Inductor>(p.id, n[0], n[1],
                                  e->param("drossel", 0.0) > 0.0 ? (l < 5e-3 ? core::InductorParams::choke1mH()
                                                                             : core::InductorParams::choke10mH())
                                  : l >= 0.5                     ? core::InductorParams::h1()
                                  : l >= 0.05                    ? core::InductorParams::mH100()
                                  : l >= 5e-3                    ? core::InductorParams::mH10()
                                  : l >= 5e-4                    ? core::InductorParams::mH1()
                                                                 : core::InductorParams::uH220());
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
        } else if (m == "loadcell" && n.size() == 4) {
            // Kraftsensor (ab Volta 3): Vollbruecke, Anschluesse plus, minus, outp, outn
            core::LoadCellParams lp;
            lp.r0Ohm = e->param("r0_ohm", lp.r0Ohm);
            lp.strainPerKg = e->param("mV_V_kg", lp.strainPerKg * 1e3) * 1e-3;
            c.add<core::LoadCell>(p.id, n[0], n[1], n[2], n[3], lp, loadAt(m_board, p));
        } else if (m == "hall" && n.size() == 3) {
            // Hall-Sensor (ab Volta 3): Anschluesse plus, minus, aus
            core::HallSensorParams hp;
            hp.sensPerMt = e->param("s_mV_mT_5V", hp.sensPerMt * 5000.0) / 5000.0;
            c.add<core::HallSensor>(p.id, n[0], n[1], n[2], hp, magneticFieldAt(m_board, p));
        } else if (m == "moisture" && n.size() == 2) {
            core::MoistureProbeParams mp;
            mp.dryOhm = e->param("r_trocken_ohm", mp.dryOhm);
            mp.wetOhm = e->param("r_nass_ohm", mp.wetOhm);
            c.add<core::MoistureProbe>(p.id, n[0], n[1], mp, wetnessAt(m_board, p));
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
            pp.ohm = p.value > 0.0 ? p.value : e->param("r_ohm", pp.ohm); // Trimmer: Katalogwert
            c.add<core::Potentiometer>(p.id, n[0], n[1], n[2], pp, p.position);
        } else if (m == "logic_probe" && n.size() == 1) {
            // Logik-Tastkopf (ab Volta 3): hochohmig gegen 0V wie der Tastkopf; H, L oder ? entscheidet die Anzeige
            core::ResistorParams rp;
            rp.ohm = e->param("r_ein_ohm", 10e6);
            c.add<core::Resistor>(p.id, n[0], core::kGround, rp);
        } else if (m == "scope_probe" && n.size() == 1) {
            // Tastkopf des Oszilloskops (ab Volta 2): Eingangswiderstand 10 MOhm gegen 0V wie ein echter
            // Tastkopf; der Strom dahin ist der Messwert der Spannung am Feld
            core::ResistorParams rp;
            rp.ohm = e->param("r_ein_ohm", 10e6);
            c.add<core::Resistor>(p.id, n[0], core::kGround, rp);
        } else if ((m == "measure_volt" || m == "measure_ampere" || m == "measure_ohm") && n.size() == 2) {
            // Messstelle (ab Volta 2): das Messgeraet des Kerns mit fester Messart zwischen plus (n[0]) und minus
            // (n[1]); der jeweils unbenutzte Eingang liegt auf minus und ist damit wirkungslos. Ohne Sicherung:
            // ein Strommesser parallel zur Batterie ist ein Kurzschluss (Fehlersuche), kein durchgebranntes Teil.
            core::MeterParams mp;
            mp.digitalInputOhm = e->param("r_ein_ohm", mp.digitalInputOhm);
            mp.shuntOhm = e->param("r_shunt_ohm", mp.shuntOhm);
            mp.fuseA = 1e9;
            const bool amp = m == "measure_ampere";
            auto& meter = c.add<core::Meter>(p.id, n[1], amp ? n[1] : n[0], amp ? n[0] : n[1], mp);
            meter.setMode(amp ? core::MeterMode::Ampere : m == "measure_ohm" ? core::MeterMode::Ohm : core::MeterMode::Volt);
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
        else if (e.type == "led_weiss") d = core::DiodeParams::ledWhite(); // ab Volta 3
        else if (e.type == "ir_led") d = core::DiodeParams::irLed();
        else if (e.model == "led") d = core::DiodeParams::ledRed();
        else if (e.type == "zdiode_4v7") d = core::DiodeParams::zener4V7();
        else if (e.type == "zdiode_5v6") d = core::DiodeParams::zener5V6();
        d.ufV = e.param("uf_V", d.ufV); // Katalogwert bestimmt die Kennlinie
        d.imaxA = e.param("imax_mA", d.imaxA * 1e3) * 1e-3;
        return d;
    }

    // IR-Strahlung: jeder Fototransistor empfaengt die IR-LEDs nach Abstand (Abschattung ueber setShade)
    void coupleInfrared() {
        for (const auto& [q, rp] : m_receivers)
            for (const auto& [d, lp] : m_irLeds) q->addEmitter(d, irCoupling(*lp, *rp));
    }

    // Zwei Anschlusspunkte haengen allein ueber Leitungen zusammen (Kontaktfelder und Modulsteckplaetze sind Knoten,
    // Bauteile trennen)
    bool wiredTogether(const std::string& from, const std::string& to) const {
        const std::string goal = canonical(to);
        std::set<std::string> seen{canonical(from)};
        std::vector<std::string> todo{canonical(from)};
        while (!todo.empty()) {
            const std::string at = todo.back();
            todo.pop_back();
            if (at == goal) return true;
            for (const auto& w : m_board.wires) {
                const std::string a = canonical(w.from), b = canonical(w.to);
                const std::string* next = a == at ? &b : b == at ? &a : nullptr;
                if (next && seen.insert(*next).second) todo.push_back(*next);
            }
        }
        return false;
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
        } else if (m.type == "zeitmodul" && n.size() == 4) {
            // Katalog: plus, minus, start, aus
            const core::SubcircuitRange r = core::addTimerModule(m_net.circuit, m.id, {n[0], n[1], n[2], n[3]});
            m_net.modules[m.id] = r;
            for (std::size_t i = r.first; i < r.first + r.count; ++i)
                if (m_net.circuit.component(i).id() == m.id + ".T3") m_net.moduleMain[m.id] = i;
        } else if (m.type == "verstaerkermodul" && n.size() == 4) {
            // Katalog: plus, minus, ein, aus
            const core::SubcircuitRange r = core::addAmplifierModule(m_net.circuit, m.id, {n[0], n[1], n[2], n[3]});
            m_net.modules[m.id] = r;
            for (std::size_t i = r.first; i < r.first + r.count; ++i)
                if (m_net.circuit.component(i).id() == m.id + ".T2") m_net.moduleMain[m.id] = i;
        } else if (m.type == "opmodul" && n.size() == 5) {
            // Katalog: plus, minus, nichtinv, inv, aus (Makromodell, ein Bauteil)
            const core::SubcircuitRange r = core::addOpAmpModule(m_net.circuit, m.id, {n[0], n[1], n[2], n[3], n[4]});
            m_net.modules[m.id] = r;
            m_net.moduleMain[m.id] = r.first;
        } else if (m.type == "timer555" && n.size() == 8) {
            // Katalog: minus, tr, aus, reset, cv, thr, dis, plus (Pins 1 bis 8, Verhaltensmodell mit Ereignisortung)
            const core::SubcircuitRange r =
                core::addTimer555(m_net.circuit, m.id, {n[0], n[1], n[2], n[3], n[4], n[5], n[6], n[7]});
            m_net.modules[m.id] = r;
            m_net.moduleMain[m.id] = r.first;
        } else if (m.type == "gattermodul" && n.size() == 14) {
            // Katalog: plus, minus, a1, b1, y1, ..., a4, b4, y4 (ab Volta 3; vier NAND mit Schmitt-Eingaengen)
            core::GateModulePins gp;
            gp.plus = n[0];
            gp.minus = n[1];
            for (std::size_t k = 0; k < 12; ++k) gp.io[k] = n[2 + k];
            const core::SubcircuitRange r = core::addGateModule(m_net.circuit, m.id, gp);
            m_net.modules[m.id] = r;
            m_net.moduleMain[m.id] = r.first;
        } else if (m.type == "tonmodul" && n.size() == 4) {
            // Katalog: plus, minus, ein, aus
            const core::SubcircuitRange r = core::addToneModule(m_net.circuit, m.id, {n[0], n[1], n[2], n[3]});
            m_net.modules[m.id] = r;
            m_net.moduleMain[m.id] = r.first;
        } else if (m.type == "mwmodul" && n.size() == 5) {
            // Katalog: plus, minus, ant, abst, aus (ab Volta 3; Teilschaltung, Endstufe T1)
            const core::SubcircuitRange r = core::addMwModule(m_net.circuit, m.id, {n[0], n[1], n[2], n[3], n[4]});
            m_net.modules[m.id] = r;
            for (std::size_t i = r.first; i < r.first + r.count; ++i)
                if (m_net.circuit.component(i).id() == m.id + ".T1") m_net.moduleMain[m.id] = i;
        } else if (m.type == "ukwmodul" && n.size() == 6) {
            // Katalog: plus, minus, ant, abst, laut, aus (ab Volta 3; Verhaltensmodell). Empfang nur, wenn ant ueber
            // Leitungen (auch ueber Kontaktfelder) am Antennenfeld der Senderanlage haengt
            const bool antenna = hasDeskPart(m_board, "senderanlage") && wiredTogether("P:ANT", m.id + ".ant");
            const core::SubcircuitRange r = core::addFmModule(m_net.circuit, m.id, {n[0], n[1], n[2], n[3], n[4], n[5]},
                                                              antenna, fmStationMask(m_board));
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
    std::vector<std::pair<const core::Diode*, const PartPlacement*>> m_irLeds;
    std::vector<std::pair<core::Phototransistor*, const PartPlacement*>> m_receivers;
};

} // namespace

double variableCapacitanceF(const CatalogEntry& entry, double position) {
    const double lo = entry.param("c_min_F", 10e-12), hi = entry.param("c_max_F", 500e-12);
    return lo + std::clamp(position, 0.0, 1.0) * (hi - lo);
}

std::vector<core::RadioStation> deskStations(const Board& board) {
    std::vector<core::RadioStation> out;
    if (!hasDeskPart(board, "senderanlage")) return out;
    // Traegeramplituden verschieden, damit die Sender unterschiedlich stark ankommen
    out.push_back({600e3, 1.0, 0.5, core::StationProgram::Morse, 800.0, board.desk.stationsOn[0]});
    out.push_back({1000e3, 0.8, 0.6, core::StationProgram::Carol, 0.0, board.desk.stationsOn[1]}); // historisch
    out.push_back({1400e3, 0.6, 0.5, core::StationProgram::TimeSignal, 1000.0, board.desk.stationsOn[2]});
    return out;
}

unsigned fmStationMask(const Board& board) {
    unsigned mask = 0;
    for (std::size_t i = 0; i < 3; ++i)
        if (board.desk.stationsOn[3 + i]) mask |= 1u << i;
    return mask;
}

Netlist buildNetlist(const Board& board, const Catalog& catalog) {
    Netlist net;
    Builder(board, catalog, net).run();
    net.circuit.setSecondOrder(catalog.secondOrder);
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

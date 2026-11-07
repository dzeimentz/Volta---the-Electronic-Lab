// SimulationBridge: Schnappschuesse, Bedienhandlungen, Topologiewechsel mit Zustandsuebernahme und -vorgaben,
// Simulations-Thread in Echtzeit und Zeitraffer.
#include <QtTest>

#include <chrono>
#include <cmath>
#include <thread>

#include "model/audio_stream.hpp"
#include "model/board_edit.hpp"
#include "model/netlist_builder.hpp"
#include "services/content_repository.hpp"
#include "sim/simulation_bridge.hpp"
#include "volta/core/components/capacitor.hpp"
#include "volta_paths.hpp"

using namespace volta;

class BridgeTest : public QObject {
    Q_OBJECT

private:
    model::Catalog m_catalog;

    // LED mit 470 Ohm an 9 V; Hauptschalter nach Wunsch
    model::Board ledBoard(bool mainSwitch) const {
        model::Board b;
        b.desk.mainSwitch = mainSwitch;
        b.parts.push_back({"R1", "widerstand", {{"a", "B3"}, {"b", "B4"}}, 470.0});
        b.parts.push_back({"D1", "led_rot", {{"anode", "B4"}, {"kathode", "C4"}}});
        b.wires.push_back({"L1", "P:+9V", "B3", "rot"});
        b.wires.push_back({"L2", "C4", "P:0V", "schwarz"});
        return b;
    }

    static double ledCurrent(const model::Netlist& net, const sim::Snapshot& s) {
        return s.readings.at(net.parts.at("D1")).currentA;
    }

private slots:
    void initTestCase() {
        services::ContentRepository repo(QStringLiteral(VOLTA_CONTENT_DIR));
        QVERIFY(repo.loadCatalog(m_catalog));
    }

    void synchronousSnapshotAndAction() {
        sim::SimulationBridge bridge;
        model::Netlist net = model::buildNetlist(ledBoard(false), m_catalog);
        const auto version = bridge.setCircuit(std::move(net.circuit));
        bridge.runFor(0.05);
        sim::Snapshot s;
        QVERIFY(bridge.latest(s));
        QCOMPARE(s.topology, version);
        QVERIFY(s.valid);
        QVERIFY(std::fabs(s.timeS - 0.05) < 1e-9);
        QVERIFY(std::fabs(ledCurrent(net, s)) < 1e-6);
        QCOMPARE(s.states.size(), s.readings.size());

        bridge.post({model::desk_id::kMainSwitch, sim::Action::Type::SetSwitch, 1.0});
        bridge.runFor(0.05);
        QVERIFY(bridge.latest(s));
        QVERIFY(ledCurrent(net, s) > 14e-3);
        QVERIFY(!bridge.latest(s)); // nichts Neues
    }

    // Radio (ab Volta 3): Detektorempfaenger an der Senderanlage; Drehen am Drehkondensator stimmt neu ab
    void radioRetunesWithVariableCapacitor() {
        model::Board b;
        b.deskParts = std::vector<std::string>{"senderanlage"};
        b.desk.stationsOn = {true, true, true};
        b.parts.push_back({"C1", "kondensator", {{"a", "A1"}, {"b", "A2"}}, 10e-12});
        b.parts.push_back({"SP1", "spule_220uh", {{"a", "C1"}, {"b", "C2"}}});
        model::PartPlacement dk{"DK1", "drehkondensator", {{"a", "D1"}, {"b", "D2"}}};
        b.parts.push_back(dk);
        b.parts.push_back({"D1", "diode_ge", {{"anode", "A4"}, {"kathode", "A5"}}});
        b.parts.push_back({"R1", "widerstand", {{"a", "C6"}, {"b", "C7"}}, 47e3});
        const char* wires[][2] = {{"P:ANT", "A1"}, {"A2", "C1"}, {"A2", "D1"}, {"C2", "P:0V"},
                                  {"D2", "P:0V"},  {"A2", "A4"}, {"A5", "C6"}, {"C7", "P:0V"}};
        int n = 0;
        for (const auto& w : wires) b.wires.push_back({"L" + std::to_string(++n), w[0], w[1], "blau"});
        const model::CatalogEntry* e = m_catalog.part("drehkondensator");
        QVERIFY(e);
        QVERIFY(std::fabs(model::variableCapacitanceF(*e, 0.0) - 10e-12) < 1e-15);
        QVERIFY(std::fabs(model::variableCapacitanceF(*e, 1.0) - 500e-12) < 1e-15);
        const auto carrier = [&](double c) {
            model::Board bb = b;
            bb.parts[2].position = 0.5;
            model::Netlist net = model::buildNetlist(bb, m_catalog);
            const std::size_t det = net.parts.at("D1");
            sim::SimulationBridge bridge;
            bridge.setCircuit(std::move(net.circuit));
            bridge.setStations(model::deskStations(bb));
            bridge.runFor(0.002);
            bridge.post({"DK1", sim::Action::Type::SetCapacitance, c}); // wie LabSession::setPartPosition
            bridge.runFor(0.01);
            sim::Snapshot s;
            if (!bridge.latest(s)) return -1.0;
            return s.readings.at(det).displayValue;
        };
        // Schwingkreis 220 uH mit Drehkondensator + Koppelkondensator 10 pF: 1000 kHz bei 105 pF, 800 kHz bei 170 pF
        const double tuned = carrier(105e-12), between = carrier(170e-12);
        QVERIFY(tuned > 0.0);
        QVERIFY(tuned > 3.0 * between);
    }

    // Kristallhoerer (ab Volta 3): am Detektorempfaenger klingt die Spannung am Hoerer (Ton 1 kHz des Senders)
    void earpieceCarriesTheTone() {
        model::Board b;
        b.deskParts = std::vector<std::string>{"senderanlage"};
        b.parts.push_back({"C1", "kondensator", {{"a", "A1"}, {"b", "A2"}}, 10e-12});
        b.parts.push_back({"SP1", "spule_220uh", {{"a", "C1"}, {"b", "C2"}}});
        model::PartPlacement dk{"DK1", "drehkondensator", {{"a", "D1"}, {"b", "D2"}}};
        dk.position = (105e-12 - 10e-12) / 490e-12; // 1000 kHz
        b.parts.push_back(dk);
        b.parts.push_back({"D1", "diode_ge", {{"anode", "A4"}, {"kathode", "A5"}}});
        b.parts.push_back({"KH1", "kristallhoerer", {{"a", "C6"}, {"b", "C7"}}});
        b.parts.push_back({"R1", "widerstand", {{"a", "E6"}, {"b", "E7"}}, 22e3});
        const char* wires[][2] = {{"P:ANT", "A1"}, {"A2", "C1"}, {"A2", "D1"}, {"C2", "P:0V"}, {"D2", "P:0V"},
                                  {"A2", "A4"},    {"A5", "C6"}, {"C7", "P:0V"}, {"C6", "E6"}, {"E7", "P:0V"}};
        int n = 0;
        for (const auto& w : wires) b.wires.push_back({"L" + std::to_string(++n), w[0], w[1], "blau"});
        QCOMPARE(model::earpieceId(b), std::string("KH1"));
        QVERIFY(model::needsAudioStep(b));
        model::Netlist net = model::buildNetlist(b, m_catalog);
        QVERIFY(net.errors.empty());
        auto audio = std::make_shared<model::AudioStream>(48000);
        sim::SimulationBridge bridge;
        sim::ScopeConfig sc;
        sc.stepS = model::kAudioStepS;
        sc.audio = audio;
        sc.audioId = "KH1";
        sc.audioVoltScale = model::AudioStream::kFullScaleA / 0.5;
        bridge.setScope(sc);
        bridge.setCircuit(std::move(net.circuit));
        bridge.setStations({core::RadioStation{1e6, 1.0, 0.5, core::StationProgram::Tone, 1000.0, true}});
        bridge.runFor(0.2);
        QVERIFY(audio->buffered() > 4000);
        // Tonstrom ausgeben (mit Hochpass, Einblenden, Begrenzer) und den Ton suchen
        std::vector<std::int16_t> out(9600, 0);
        audio->mixInto(out.data(), out.size());
        double peak = 0.0, s1 = 0.0, c1 = 0.0;
        for (std::size_t i = 4800; i < out.size(); ++i) { // nach dem Einblenden; 1 kHz als Grundschwingung
            const double v = out[i] / 32768.0, ph = 2.0 * 3.14159265358979 * 1000.0 * double(i) / 48000.0;
            peak = std::max(peak, std::fabs(v));
            s1 += v * std::sin(ph);
            c1 += v * std::cos(ph);
        }
        const double tone = 2.0 * std::hypot(s1, c1) / 4800.0;
        QVERIFY2(tone > 0.01, qPrintable(QString::number(tone)));
        QVERIFY(tone > 0.5 * peak); // ueberwiegend der Ton, kein Rauschen
    }

    void topologyChangeKeepsStateAndAppliesOverrides() {
        sim::SimulationBridge bridge;
        model::Board b;
        b.desk.mainSwitch = true;
        b.parts.push_back({"C1", "elko", {{"plus", "A1"}, {"minus", "A2"}}, 100e-6});
        b.parts.push_back({"R1", "widerstand", {{"a", "B1"}, {"b", "A1"}}, 1000.0});
        b.wires.push_back({"L1", "P:+9V", "B1", "rot"});
        b.wires.push_back({"L2", "A2", "P:0V", "schwarz"});
        model::Netlist net = model::buildNetlist(b, m_catalog);
        bridge.setCircuit(std::move(net.circuit));
        bridge.runFor(1.0);
        sim::Snapshot s;
        QVERIFY(bridge.latest(s));
        const std::size_t c1 = net.parts.at("C1");
        const core::ComponentState charged = s.states.at(c1);
        QVERIFY(charged.internal[0] > 8.9);

        // Topologiewechsel: Leitung zur Batterie entfernt -> Ladung bleibt (Uebernahme im Simulations-Thread)
        b.wires.erase(b.wires.begin());
        model::Netlist net2 = model::buildNetlist(b, m_catalog);
        const auto v2 = bridge.setCircuit(std::move(net2.circuit));
        bridge.runFor(0.001);
        QVERIFY(bridge.latest(s));
        QCOMPARE(s.topology, v2);
        core::NodeId a1 = 0;
        QVERIFY(net2.nodeOf("A1", a1));
        QVERIFY(std::fabs(s.nodeVolt.at(a1) - charged.internal[0]) < 0.01);

        // Undo: Zustandsvorgabe (ungeladen) hat Vorrang vor der Uebernahme
        model::Netlist net3 = model::buildNetlist(b, m_catalog);
        core::ComponentState empty = charged;
        empty.internal[0] = 0.0;
        bridge.setCircuit(std::move(net3.circuit), {{"C1", empty}});
        bridge.runFor(0.001);
        QVERIFY(bridge.latest(s));
        QVERIFY(std::fabs(s.nodeVolt.at(a1)) < 0.01);
    }

    void eventsReachTheUi() {
        sim::SimulationBridge bridge;
        model::Board b = ledBoard(true);
        b.parts.erase(b.parts.begin()); // LED ohne Widerstand
        b.wires[0].to = "B4";
        model::Netlist net = model::buildNetlist(b, m_catalog);
        bridge.setCircuit(std::move(net.circuit));
        bridge.runFor(1.0);
        sim::Snapshot s;
        QVERIFY(bridge.latest(s));
        bool destroyed = false;
        for (const auto& e : s.events)
            destroyed = destroyed || (e.type == core::SimEventType::Destroyed && e.component == net.parts.at("D1"));
        QVERIFY(destroyed);
        QVERIFY(s.readings.at(net.parts.at("D1")).destroyed);
    }

    void threadRunsInRealTime() {
        sim::SimulationBridge bridge;
        model::Netlist net = model::buildNetlist(ledBoard(true), m_catalog);
        const auto version = bridge.setCircuit(std::move(net.circuit));
        bridge.start();
        QVERIFY(bridge.running());
        std::this_thread::sleep_for(std::chrono::milliseconds(600));
        sim::Snapshot s;
        bridge.latest(s);
        QCOMPARE(s.topology, version);
        QVERIFY2(s.timeS > 0.4 && s.timeS < 0.75, qPrintable(QString::number(s.timeS)));
        QVERIFY(ledCurrent(net, s) > 14e-3);

        bridge.setTimeScale(10.0);
        const double t0 = s.timeS;
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        bridge.latest(s);
        QVERIFY2(s.timeS - t0 > 2.0, qPrintable(QString::number(s.timeS - t0))); // ca. 3 s in 0,3 s

        bridge.setPaused(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        bridge.latest(s);
        const double tp = s.timeS;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        bridge.latest(s);
        QVERIFY(std::fabs(s.timeS - tp) < 0.02);
        bridge.stop();
        QVERIFY(!bridge.running());
    }

    void actionsInThreadAndRapidTopologyChanges() {
        sim::SimulationBridge bridge;
        bridge.start();
        model::Netlist net;
        std::uint64_t version = 0;
        for (int i = 0; i < 20; ++i) { // schnelle Folge von Aufbauaenderungen: die neueste gewinnt
            net = model::buildNetlist(ledBoard(false), m_catalog);
            version = bridge.setCircuit(std::move(net.circuit));
        }
        bridge.post({model::desk_id::kMainSwitch, sim::Action::Type::SetSwitch, 1.0});
        sim::Snapshot s;
        bool ok = false;
        for (int i = 0; i < 100 && !ok; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            bridge.latest(s);
            ok = s.topology == version && s.valid && ledCurrent(net, s) > 14e-3;
        }
        QVERIFY(ok);
        bridge.stop();
    }
};

QTEST_GUILESS_MAIN(BridgeTest)
#include "test_bridge.moc"

// Tests fuer Schaltungsmodell und Dienste: Anschlussbezeichner, Inhaltspakete, Netzlistenbildung, Ende-zu-Ende mit
// dem Simulationskern (docs/testing.md, Modell).
#include <QtTest>

#include <algorithm>
#include <cmath>

#include "model/anchor.hpp"
#include "model/board_json.hpp"
#include "model/build_guide.hpp"
#include "model/experiment_flow.hpp"
#include "model/formula.hpp"
#include "model/recorder.hpp"
#include "model/tone_synth.hpp"
#include "model/meter_display.hpp"
#include "model/resistor_code.hpp"
#include "model/schematic_layout.hpp"
#include "model/netlist_builder.hpp"
#include "services/content_repository.hpp"
#include "volta/core/components/capacitor.hpp"
#include "volta/core/components/meter.hpp"
#include "volta/core/simulator.hpp"

using namespace volta;
using model::AnchorKind;

class ModelTest : public QObject {
    Q_OBJECT

private:
    model::Catalog m_catalog;

    // Netzliste bauen und Simulator starten; Anzahl Schritte simulieren
    static bool simulate(const model::Board& board, const model::Catalog& catalog, model::Netlist& net,
                         core::Simulator& sim, double seconds) {
        net = model::buildNetlist(board, catalog);
        if (!net.ok()) return false;
        if (sim.setCircuit(std::move(net.circuit)) != core::Status::Ok) return false;
        const int steps = static_cast<int>(std::lround(seconds / core::kDefaultStepS));
        for (int i = 0; i < steps; ++i)
            if (sim.step() != core::Status::Ok) return false;
        return true;
    }

    static double partCurrent(const model::Netlist& net, const core::Simulator& sim, const std::string& id) {
        return sim.probe().readings[net.parts.at(id)].currentA;
    }

    static double nodeVoltage(const model::Netlist& net, const core::Simulator& sim, const std::string& anchor) {
        core::NodeId n = 0;
        return net.nodeOf(anchor, n) ? sim.probe().voltage(n) : NAN;
    }

private slots:
    // Automatisches Speichern: Aufbau verlustfrei als JSON; Hauptschalter startet aus
    void boardJsonRoundTrip() {
        model::Board b;
        b.parts.push_back({"R1", "widerstand", {{"a", "B3"}, {"b", "B4"}}, 4700.0});
        b.parts.push_back({"S1", "taster", {{"a", "C1"}, {"b", "C2"}}, 0.0, true});
        b.wires.push_back({"L1", "P:+9V", "B3", "gelb", true});
        b.modules.push_back({"BL1", "blinkmodul", 5});
        b.voltageDisplays = {"B4"};
        b.columns = 8;
        b.desk.cells = 3;
        b.desk.mainSwitch = true;
        b.desk.switchClosed = true;
        b.desk.p1 = 0.25;
        b.desk.meterMode = core::MeterMode::Ohm;
        b.desk.meterVisible = true;
        b.desk.roomLight = 0;
        b.desk.tools.push_back({"taschenlampe", 3.5, 1.0, false});
        model::Board back;
        QVERIFY(model::boardFromJson(model::boardToJson(b), back));
        QCOMPARE(back.parts.size(), std::size_t(2));
        QCOMPARE(back.parts[0].value, 4700.0);
        QCOMPARE(back.parts[0].pins[1].second, std::string("B4"));
        QVERIFY(back.parts[1].closed);
        QCOMPARE(back.wires[0].color, std::string("gelb"));
        QVERIFY(back.wires[0].colorFixed);
        QCOMPARE(back.modules[0].slot, 5);
        QCOMPARE(back.voltageDisplays.front(), std::string("B4"));
        QCOMPARE(back.columns, 8);
        QCOMPARE(back.desk.cells, 3);
        QVERIFY(!back.desk.mainSwitch); // Start im Steckmodus
        QVERIFY(back.desk.switchClosed);
        QCOMPARE(back.desk.p1, 0.25);
        QCOMPARE(back.desk.meterMode, core::MeterMode::Ohm);
        QCOMPARE(back.desk.roomLight, 0);
        QCOMPARE(back.desk.tools.size(), std::size_t(1));
        QVERIFY(!back.desk.tools[0].on);
        QVERIFY(!model::boardFromJson(QJsonObject{}, back)); // kein Aufbau
    }

    void flowRestore() {
        using P = model::ExperimentFlow::Phase;
        using C = model::ExperimentFlow::Comparison;
        model::ExperimentFlow f;
        QVERIFY(!f.restore(P::Build, 0, 0, -1, C::None)); // vor start()
        f.start(3, 2);
        QVERIFY(!f.restore(P::Build, -1, 0, -1, C::None)); // Aufbau ohne Vermutung
        QVERIFY(!f.restore(P::Build, 1, 4, -1, C::None));  // mehr Tipps als vorhanden
        QVERIFY(f.restore(P::Build, 1, 2, -1, C::None));
        QCOMPARE(f.phase(), P::Build);
        QCOMPARE(f.hintsShown(), 2);
        QVERIFY(f.restore(P::Explain, 1, 3, 0, C::Matches));
        QCOMPARE(f.observation(), 0);
        QCOMPARE(f.comparison(), C::Matches);
    }

    // Werkzeugkasten: Farbcode in beide Richtungen, E-Reihen mit naechstem Normwert
    void resistorColorCode() {
        namespace rc = model::resistor_code;
        rc::Code c = rc::encode(470.0, 4); // gelb, violett, braun, gold
        QVERIFY(c.valid);
        QCOMPARE(c.rings, (std::vector<int>{4, 7, 1, rc::kGold}));
        QCOMPARE(rc::decode(c.rings), 470.0);
        c = rc::encode(4.7, 4); // gelb, violett, gold (x0,1)
        QCOMPARE(c.rings, (std::vector<int>{4, 7, rc::kGold, rc::kGold}));
        QVERIFY(std::fabs(rc::decode(c.rings) - 4.7) < 1e-12);
        c = rc::encode(10e3, 5); // braun, schwarz, schwarz, rot, braun
        QCOMPARE(c.rings, (std::vector<int>{1, 0, 0, 2, 1}));
        QCOMPARE(rc::decode(c.rings), 10e3);
        c = rc::encode(999.6, 4); // rundet auf 1,0 kOhm
        QCOMPARE(c.rings, (std::vector<int>{1, 0, 2, rc::kGold}));
        QCOMPARE(c.ohm, 1000.0);
        QVERIFY(!rc::encode(0.0, 4).valid);
        QVERIFY(!rc::encode(1e12, 4).valid);
        QCOMPARE(rc::decode({rc::kGold, 0, 1, rc::kGold}), 0.0); // gold ist keine Ziffer
        QCOMPARE(rc::tolerancePercent(rc::kGold), 5.0);
        QCOMPARE(rc::tolerancePercent(1), 1.0);
    }

    // Schaltplan: LED mit Vorwiderstand; Knoten mit Feldnamen, Pluspol oben, 0 V unten
    void schematicLayout() {
        model::Board b;
        b.parts.push_back({"R1", "widerstand", {{"a", "B3"}, {"b", "B4"}}, 470.0});
        b.parts.push_back({"D1", "led_rot", {{"anode", "B4"}, {"kathode", "C4"}}});
        b.parts.push_back({"R2", "widerstand", {{"a", "A3"}, {"b", "A4"}}, 470.0}); // parallel zu R1
        b.wires.push_back({"L1", "P:+9V", "B3", "rot"});
        b.wires.push_back({"L2", "C4", "P:0V", "schwarz"});
        b.wires.push_back({"L3", "A3", "B3", "rot"});
        b.wires.push_back({"L4", "A4", "B4", "rot"});
        const model::Schematic s = model::buildSchematic(b, m_catalog);
        QCOMPARE(s.nodes.size(), std::size_t(3)); // +9V/A3/B3, A4/B4, C4/0V
        QCOMPARE(s.layers, 3);
        auto nodeWith = [&](const std::string& a) {
            for (std::size_t i = 0; i < s.nodes.size(); ++i)
                for (const auto& x : s.nodes[i].anchors)
                    if (x == a) return static_cast<int>(i);
            return -1;
        };
        const int plus = nodeWith("P:+9V"), mid = nodeWith("B4"), gnd = nodeWith("P:0V");
        QVERIFY(plus >= 0 && mid >= 0 && gnd >= 0);
        QCOMPARE(nodeWith("A3"), plus);
        QCOMPARE(nodeWith("C4"), gnd);
        QCOMPARE(s.nodes[static_cast<std::size_t>(plus)].layer, 0);
        QCOMPARE(s.nodes[static_cast<std::size_t>(mid)].layer, 1);
        QCOMPARE(s.nodes[static_cast<std::size_t>(gnd)].layer, 2);
        QVERIFY(s.nodes[static_cast<std::size_t>(plus)].label.find("+9V") != std::string::npos);
        // Elemente: R1, D1, R2 und die Batterie
        int parallel = 0;
        bool battery = false;
        for (const auto& e : s.elements) {
            if (e.id == "PULT.B") battery = (e.nodes == std::vector<int>{plus, gnd});
            if (e.id == "R1" || e.id == "R2") parallel += e.parallelCount;
        }
        QVERIFY(battery);
        QCOMPARE(parallel, 4); // beide melden zwei parallele Zweipole
        QVERIFY(model::buildSchematic(model::Board{}, m_catalog).nodes.empty());

        // DIN EN 61082-1: nur waagrechte und senkrechte Leitungen; Pluspol oben, 0 V unten; Batterie links
        for (const auto& l : s.lines)
            QVERIFY(std::fabs(l.x1 - l.x2) < 1e-9 || std::fabs(l.y1 - l.y2) < 1e-9);
        QVERIFY(s.nodes[static_cast<std::size_t>(plus)].y < s.nodes[static_cast<std::size_t>(mid)].y);
        QVERIFY(s.nodes[static_cast<std::size_t>(mid)].y < s.nodes[static_cast<std::size_t>(gnd)].y);
        double batteryX = 1e9, minX = 1e9;
        for (const auto& e : s.elements) {
            minX = std::min(minX, e.x);
            if (e.id == "PULT.B") batteryX = e.x;
        }
        QCOMPARE(batteryX, minX);
        // R1 und R2 parallel zwischen +9V und B4: Abzweigungen an beiden Knoten mit Verbindungspunkt
        int plusDots = 0, midDots = 0;
        for (const auto& d : s.dots) {
            plusDots += d.node == plus;
            midDots += d.node == mid;
        }
        QVERIFY(plusDots >= 1);
        QVERIFY(midDots >= 1);
        // jede Leitung endet auf ihrem Knoten oder an einem Bauteil: Knotenleitungen liegen auf ihrer Zeile
        for (const auto& l : s.lines)
            if (std::fabs(l.y1 - l.y2) < 1e-9 && l.node >= 0 && std::fabs(l.x2 - l.x1) > 1.0)
                QCOMPARE(l.y1, s.nodes[static_cast<std::size_t>(l.node)].y);

        // Modul als Block: Plus oben, Minus unten, Ausgang rechts, Takt links; Ein- und Ausgaenge mit eigener Leitung
        model::Board m;
        m.modules.push_back({"BL1", "blinkmodul", 1});
        m.parts.push_back({"R1", "widerstand", {{"a", "B3"}, {"b", "B4"}}, 470.0});
        m.parts.push_back({"D1", "led_rot", {{"anode", "B4"}, {"kathode", "C4"}}});
        m.wires.push_back({"L1", "P:+9V", "BL1.plus", "rot"});
        m.wires.push_back({"L2", "BL1.minus", "P:0V", "schwarz"});
        m.wires.push_back({"L3", "P:+9V", "B3", "rot"});
        m.wires.push_back({"L4", "C4", "BL1.aus", "blau"});
        const model::Schematic sm = model::buildSchematic(m, m_catalog);
        const model::SchematicElement* bl = nullptr;
        for (const auto& e : sm.elements)
            if (e.id == "BL1") bl = &e;
        QVERIFY(bl && bl->block);
        QCOMPARE(bl->pinLabels.size(), std::size_t(4));
        for (const auto& l : sm.lines)
            QVERIFY(std::fabs(l.x1 - l.x2) < 1e-9 || std::fabs(l.y1 - l.y2) < 1e-9);
        QVERIFY(sm.width > 0.0 && sm.height > 0.0);
    }

    void parseOhmInput() {
        namespace rc = model::resistor_code;
        QCOMPARE(rc::parseOhm("470"), 470.0);
        QCOMPARE(rc::parseOhm("4,7k"), 4700.0);
        QCOMPARE(rc::parseOhm("4.7 kOhm"), 4700.0);
        QCOMPARE(rc::parseOhm("4k7"), 4700.0);
        QCOMPARE(rc::parseOhm("470R"), 470.0);
        QCOMPARE(rc::parseOhm("4R7"), 4.7);
        QCOMPARE(rc::parseOhm("2M2"), 2.2e6);
        QCOMPARE(rc::parseOhm("1 k\u03A9"), 1000.0);
        QCOMPARE(rc::parseOhm(""), 0.0);
        QCOMPARE(rc::parseOhm("abc"), 0.0);
        QCOMPARE(rc::parseOhm("1.2.3"), 0.0);
    }

    void eSeriesNearest() {
        namespace rc = model::resistor_code;
        QCOMPARE(rc::eSeries(6).size(), std::size_t(6));
        QCOMPARE(rc::eSeries(12).size(), std::size_t(12));
        QCOMPARE(rc::eSeries(24).size(), std::size_t(24));
        rc::Nearest n = rc::nearest(470.0, 12);
        QVERIFY(n.isMember);
        QCOMPARE(n.value, 470.0);
        n = rc::nearest(500.0, 12); // 470 (-6 %) oder 560 (+12 %) -> 470
        QCOMPARE(n.value, 470.0);
        QVERIFY(std::fabs(n.deviationPercent - 6.383) < 0.01);
        n = rc::nearest(950.0, 12); // naechste Dekade: 1 kOhm
        QCOMPARE(n.value, 1000.0);
        n = rc::nearest(500.0, 24); // 510 in E24
        QCOMPARE(n.value, 510.0);
        QVERIFY(!rc::nearest(0.0, 12).isMember);
    }

    // Anzeige des Messgeraets: drei gueltige Stellen, passender Vorsatz
    void meterDisplayFormat() {
        using core::MeterMode;
        auto f = [](MeterMode m, double v) {
            const model::MeterText t = model::formatMeter(m, v);
            return t.value + " " + t.unit;
        };
        QCOMPARE(f(MeterMode::Volt, 9.0), std::string("9.00 V"));
        QCOMPARE(f(MeterMode::Volt, 12.34), std::string("12.3 V"));
        QCOMPARE(f(MeterMode::Volt, 0.4503), std::string("450 mV"));
        QCOMPARE(f(MeterMode::Volt, 0.9998), std::string("1.00 V")); // Rundung wechselt den Vorsatz
        QCOMPARE(f(MeterMode::Volt, -4.5), std::string("-4.50 V"));
        QCOMPARE(f(MeterMode::Volt, 0.0), std::string("0.00 V"));
        QCOMPARE(f(MeterMode::Volt, -1e-9), std::string("0.00 mV"));
        QCOMPARE(f(MeterMode::Ampere, 0.0192), std::string("19.2 mA"));
        QCOMPARE(f(MeterMode::Ampere, 0.25), std::string("250 mA"));
        QCOMPARE(f(MeterMode::Ampere, 5e-6), std::string("5.00 \u00B5A"));
        QCOMPARE(f(MeterMode::Ampere, 0.0), std::string("0.00 mA"));
        QCOMPARE(f(MeterMode::Ohm, 470.0), std::string("470 \u03A9"));
        QCOMPARE(f(MeterMode::Ohm, 1000.0), std::string("1.00 k\u03A9"));
        QCOMPARE(f(MeterMode::Ohm, 2.2e6), std::string("2.20 M\u03A9"));
        QCOMPARE(f(MeterMode::Ohm, 0.5), std::string("0.50 \u03A9"));
    }

    void initTestCase() {
        services::ContentRepository repo(QStringLiteral(VOLTA_CONTENT_DIR));
        QVERIFY2(repo.loadCatalog(m_catalog), qPrintable(repo.lastError()));
    }

    void anchors() {
        QCOMPARE(model::classifyAnchor("A1"), AnchorKind::Field);
        QCOMPARE(model::classifyAnchor("E8"), AnchorKind::Field);
        QCOMPARE(model::classifyAnchor("E12"), AnchorKind::Field);
        QCOMPARE(model::classifyAnchor("A13"), AnchorKind::Invalid);
        QCOMPARE(model::classifyAnchor("F1"), AnchorKind::Invalid);
        QCOMPARE(model::classifyAnchor("A9"), AnchorKind::Field);
        QCOMPARE(model::classifyAnchor("A0"), AnchorKind::Invalid);
        QCOMPARE(model::classifyAnchor("A01"), AnchorKind::Invalid);
        QCOMPARE(model::classifyAnchor("P:+9V"), AnchorKind::Desk);
        QCOMPARE(model::classifyAnchor("P:LS2"), AnchorKind::Desk);
        QCOMPARE(model::classifyAnchor("P:LS3"), AnchorKind::Invalid);
        QCOMPARE(model::classifyAnchor("MESS:VOHM"), AnchorKind::MeterJack);
        QCOMPARE(model::classifyAnchor("M:1"), AnchorKind::ModuleSlot);
        QCOMPARE(model::classifyAnchor("M:16"), AnchorKind::ModuleSlot);
        QCOMPARE(model::classifyAnchor("M:17"), AnchorKind::Invalid);
        QCOMPARE(model::classifyAnchor("BL1.takt"), AnchorKind::ModulePin);
        QCOMPARE(model::classifyAnchor("bl1.takt"), AnchorKind::Invalid);
        QCOMPARE(model::boardFields().size(), std::size_t(60));
        QCOMPARE(model::boardFields().front(), std::string("A1"));
        QCOMPARE(model::boardFields().back(), std::string("E12"));
    }

    void catalogLoads() {
        QVERIFY(m_catalog.part("widerstand") != nullptr);
        QCOMPARE(m_catalog.part("led_rot")->model, std::string("led"));
        QCOMPARE(m_catalog.part("led_rot")->pins, (std::vector<std::string>{"anode", "kathode"}));
        QCOMPARE(m_catalog.part("led_rot")->param("uf_V", 0.0), 1.9);
        QVERIFY(m_catalog.module("blinkmodul") != nullptr);
        QCOMPARE(m_catalog.module("blinkmodul")->pins.size(), std::size_t(4));
    }

    void experimentLoads() {
        services::ContentRepository repo(QStringLiteral(VOLTA_CONTENT_DIR));
        services::Experiment exp;
        QVERIFY2(repo.loadExperiment(QStringLiteral("volta1.2.4"), exp), qPrintable(repo.lastError()));
        QCOMPARE(exp.stage, 2);
        QVERIFY(exp.destructive);
        QCOMPARE(exp.templateBoard.parts.size(), std::size_t(2));
        QCOMPARE(exp.templateBoard.wires.size(), std::size_t(2));
        QCOMPARE(exp.shownVoltages, (QStringList{QStringLiteral("B3"), QStringLiteral("B4")}));
        const model::PartPlacement* led = exp.templateBoard.findPart("D1");
        QVERIFY(led != nullptr);
        QCOMPARE(led->pins.at(0), (std::pair<std::string, std::string>{"anode", "B4"}));
        QVERIFY(repo.loadTexts(QStringLiteral("volta1"), QStringLiteral("de")));
        QVERIFY(repo.text(exp.titleKey).contains(QStringLiteral("Leuchtdiode")));
        QCOMPARE(repo.text(QStringLiteral("gibt_es_nicht")), QStringLiteral("gibt_es_nicht"));
    }

    // Ende-zu-Ende: Vorlage 2.4 -> Netzliste -> Simulation: LED mit 470 Ohm leuchtet mit ca. 15 mA
    void experiment24EndToEnd() {
        services::ContentRepository repo(QStringLiteral(VOLTA_CONTENT_DIR));
        services::Experiment exp;
        QVERIFY(repo.loadExperiment(QStringLiteral("volta1.2.4"), exp));
        model::Board board = exp.templateBoard;

        model::Netlist net;
        core::Simulator sim;
        QVERIFY(simulate(board, m_catalog, net, sim, 0.1)); // Hauptschalter aus
        QVERIFY(std::fabs(partCurrent(net, sim, "D1")) < 1e-6);

        board.desk.mainSwitch = true;
        core::Simulator sim2;
        QVERIFY(simulate(board, m_catalog, net, sim2, 1.0));
        const double i = partCurrent(net, sim2, "D1");
        QVERIFY2(i > 14.5e-3 && i < 15.5e-3, qPrintable(QString::number(i * 1e3) + " mA"));
        QVERIFY(nodeVoltage(net, sim2, "B3") > 8.9);
        QVERIFY(nodeVoltage(net, sim2, "B4") > 1.8 && nodeVoltage(net, sim2, "B4") < 2.0);
        QVERIFY(std::fabs(nodeVoltage(net, sim2, "C4")) < 1e-3);
        QCOMPARE(net.wires.size(), std::size_t(2));
        // Leitungen tragen den Strom (Stromflussdarstellung)
        QVERIFY(std::fabs(sim2.probe().readings[net.wires.at("L1")].currentA - i) < 1e-6);
    }

    void ledWithoutResistorBurnsOut() {
        model::Board board;
        board.desk.mainSwitch = true;
        board.parts.push_back({"D1", "led_rot", {{"anode", "B4"}, {"kathode", "C4"}}});
        board.wires.push_back({"L1", "P:+9V", "B4", "rot"});
        board.wires.push_back({"L2", "C4", "P:0V", "schwarz"});
        model::Netlist net;
        core::Simulator sim;
        QVERIFY(simulate(board, m_catalog, net, sim, 1.0));
        QVERIFY(sim.probe().readings[net.parts.at("D1")].destroyed);
    }

    void deskPotentiometerDivides() {
        model::Board board;
        board.desk.mainSwitch = true;
        board.desk.p1 = 0.25;
        board.wires.push_back({"L1", "P:+9V", "P:P1a", "rot"});
        board.wires.push_back({"L2", "P:P1b", "P:0V", "schwarz"});
        model::Netlist net;
        core::Simulator sim;
        QVERIFY(simulate(board, m_catalog, net, sim, 0.01));
        // Stellung 0,25 von a: U(s) = 9 V * 0,75 (Batterie unbelastet bis auf 10 kOhm)
        QVERIFY(std::fabs(nodeVoltage(net, sim, "P:P1s") - 9.0 * 0.75 * 10000.0 / 10001.5) < 0.01);
    }

    void meterMeasuresThroughJacks() {
        model::Board board;
        board.desk.mainSwitch = true;
        board.parts.push_back({"R1", "widerstand", {{"a", "A1"}, {"b", "A2"}}, 1000.0});
        board.parts.push_back({"R2", "widerstand", {{"a", "A2"}, {"b", "A3"}}, 1000.0});
        board.wires.push_back({"L1", "P:+9V", "A1", "rot"});
        board.wires.push_back({"L2", "A3", "P:0V", "schwarz"});
        board.wires.push_back({"M1", "MESS:VOHM", "A2", "rot"});
        board.wires.push_back({"M2", "MESS:COM", "P:0V", "schwarz"});
        model::Netlist net;
        core::Simulator sim;
        QVERIFY(simulate(board, m_catalog, net, sim, 0.01));
        const auto& reading = sim.probe().readings[net.desk.at(model::desk_id::kMeter)];
        QVERIFY2(std::fabs(reading.displayValue - 4.5) < 0.01, qPrintable(QString::number(reading.displayValue)));
    }

    void blinkModuleViaPinNames() {
        // Blinkmodul in den Feldern M1-M4 (plus, minus, aus, takt); LED mit 470 Ohm von +9V nach BL1.aus
        model::Board board;
        board.desk.mainSwitch = true;
        board.modules.push_back({"BL1", "blinkmodul", 1});
        board.parts.push_back({"R1", "widerstand", {{"a", "A1"}, {"b", "A2"}}, 470.0});
        board.parts.push_back({"D1", "led_rot", {{"anode", "A2"}, {"kathode", "A3"}}});
        board.wires.push_back({"L1", "P:+9V", "BL1.plus", "rot"});
        board.wires.push_back({"L2", "BL1.minus", "P:0V", "schwarz"});
        board.wires.push_back({"L3", "P:+9V", "A1", "rot"});
        board.wires.push_back({"L4", "A3", "M:3", "blau"}); // gleichwertig zu BL1.aus
        model::Netlist net = model::buildNetlist(board, m_catalog);
        QVERIFY2(net.ok(), net.errors.empty() ? "" : net.errors.front().c_str());
        core::NodeId pinNode = 0, slotNode = 0;
        QVERIFY(net.nodeOf("BL1.aus", pinNode));
        QVERIFY(net.nodeOf("M:3", slotNode));
        QCOMPARE(pinNode, slotNode);
        QVERIFY(net.modules.count("BL1") == 1);

        core::Simulator sim;
        QVERIFY(sim.setCircuit(std::move(net.circuit)) == core::Status::Ok);
        int edges = 0;
        bool on = false;
        for (int i = 0; i < 4000; ++i) {
            QVERIFY(sim.step() == core::Status::Ok);
            const bool now = sim.probe().readings[net.parts.at("D1")].currentA > 5e-3;
            if (now != on) ++edges;
            on = now;
        }
        QVERIFY2(edges >= 4, qPrintable(QString::number(edges)));
    }

    // Aufbauhilfe: Schritte aus der Vorlage, erledigt ohne Reihenfolgezwang, Richtung ungepolter Bauteile egal
    void buildGuideSteps() {
        model::Board tmpl;
        tmpl.modules.push_back({"TM1", "tonmodul", 1});
        tmpl.parts.push_back({"R1", "widerstand", {{"a", "B3"}, {"b", "B4"}}, 470.0});
        tmpl.parts.push_back({"D1", "led_rot", {{"anode", "B4"}, {"kathode", "C4"}}});
        tmpl.wires.push_back({"L1", "P:+9V", "B3", "rot"});
        tmpl.wires.push_back({"L2", "C4", "TM1.aus", "blau"});
        const auto steps = model::buildGuide(tmpl, m_catalog);
        QCOMPARE(steps.size(), std::size_t(6)); // Modul, 2 Bauteile, 2 Leitungen, Hauptschalter
        QCOMPARE(steps[0].kind, model::GuideStep::Kind::Module);
        QCOMPARE(steps[0].targets, (std::vector<std::string>{"M:1", "M:2", "M:3", "M:4"}));
        QCOMPARE(steps[4].to, std::string("M:4")); // Modulanschluss als Feld der Leiste
        QCOMPARE(steps[5].kind, model::GuideStep::Kind::MainSwitch);

        model::Board b;
        QCOMPARE(model::currentStep(steps, b, m_catalog), std::size_t(0));
        b.modules.push_back({"TM1", "tonmodul", 1});
        QCOMPARE(model::currentStep(steps, b, m_catalog), std::size_t(1));
        // Widerstand andersherum, aber mit falschem Wert: noch offen; mit 470 Ohm erledigt
        b.parts.push_back({"R7", "widerstand", {{"a", "B4"}, {"b", "B3"}}, 1000.0});
        QCOMPARE(model::currentStep(steps, b, m_catalog), std::size_t(1));
        b.parts.back().value = 470.0;
        QCOMPARE(model::currentStep(steps, b, m_catalog), std::size_t(2));
        // LED verkehrt herum: zaehlt nicht
        b.parts.push_back({"D1", "led_rot", {{"anode", "C4"}, {"kathode", "B4"}}});
        QCOMPARE(model::currentStep(steps, b, m_catalog), std::size_t(2));
        b.parts.back().pins = {{"anode", "B4"}, {"kathode", "C4"}};
        // Leitungen in anderer Reihenfolge und Richtung; zweite Leitung ueber das Feld der Leiste
        b.wires.push_back({"L5", "M:4", "C4", "blau"});
        QCOMPARE(model::currentStep(steps, b, m_catalog), std::size_t(3));
        b.wires.push_back({"L6", "B3", "P:+9V", "gelb"});
        QCOMPARE(model::currentStep(steps, b, m_catalog), std::size_t(5));
        b.desk.mainSwitch = true;
        QCOMPARE(model::currentStep(steps, b, m_catalog), steps.size());
        // Rueckschritt: Bauteil entfernt -> dieser Schritt ist wieder dran
        b.parts.erase(b.parts.begin());
        QCOMPARE(model::currentStep(steps, b, m_catalog), std::size_t(1));
    }

    // Klangerzeuger: Tonhoehe aus den Nulldurchgaengen, Aussteuerung begrenzt, Stille ohne Stimmen, kein Knacken
    void toneSynth() {
        model::ToneSynth synth(48000);
        std::vector<std::int16_t> buf(48000);
        synth.render(buf.data(), buf.size());
        QVERIFY(std::all_of(buf.begin(), buf.end(), [](std::int16_t v) { return v == 0; })); // still
        synth.setVoices({{440.0, 1.0}});
        synth.render(buf.data(), buf.size()); // 1 s
        int rising = 0, peak = 0, maxStep = 0;
        for (std::size_t i = 4800; i < buf.size(); ++i) { // nach dem Einschwingen
            if (buf[i - 1] < 0 && buf[i] >= 0) ++rising;
            peak = std::max(peak, std::abs(static_cast<int>(buf[i])));
        }
        QVERIFY2(std::abs(rising - 396) <= 2, qPrintable(QString::number(rising))); // 440 Hz * 0,9 s
        QVERIFY(peak > 2000);
        QVERIFY(peak <= static_cast<int>(model::ToneSynth::kHeadroom * 32767) + 1);
        // vier laute Stimmen: weich begrenzt, nie ueber dem Kopfraum
        synth.setVoices({{300.0, 1.0}, {450.0, 1.0}, {700.0, 1.0}, {2400.0, 1.0}});
        synth.click(1.0);
        synth.render(buf.data(), buf.size());
        for (std::int16_t v : buf) QVERIFY(std::abs(static_cast<int>(v)) <= model::ToneSynth::kHeadroom * 32767 + 1);
        // Ausschalten gleitet aus: keine Spruenge groesser als im laufenden Ton
        synth.setVoices({{440.0, 1.0}});
        synth.render(buf.data(), buf.size());
        for (std::size_t i = 1; i < buf.size(); ++i) maxStep = std::max(maxStep, std::abs(buf[i] - buf[i - 1]));
        synth.setVoices({});
        std::vector<std::int16_t> tail(4800);
        synth.render(tail.data(), tail.size());
        int tailStep = std::abs(tail[0] - buf.back());
        for (std::size_t i = 1; i < tail.size(); ++i) tailStep = std::max(tailStep, std::abs(tail[i] - tail[i - 1]));
        QVERIFY(tailStep <= maxStep);
        QVERIFY(std::abs(static_cast<int>(tail.back())) < 50); // nach 100 ms praktisch still
    }

    // Zeitschreiber: Fenster, lueckenlose Zeit bei Neustart der Simulation, Ausduennen, Pause, Kanaele
    void recorder() {
        model::Recorder r;
        QVERIFY(!r.active());
        r.push(0.1, {1.0, 2.0}, {true, true});
        QVERIFY(r.samples().empty()); // ohne Kanal keine Aufzeichnung
        r.setChannel(0, "B4");
        r.setWindow(2.0);
        for (int i = 0; i <= 300; ++i) r.push(i * 0.01, {i * 0.01, 0.0}, {true, true}); // 0..3 s, Wert = Zeit
        QCOMPARE(r.samples().back().has[1], false); // Kanal 2 aus
        auto s = r.series(0, 1000);
        QVERIFY(s.any);
        QVERIFY(std::fabs(s.points.front().first) < 0.011);        // Fensteranfang
        QVERIFY(std::fabs(s.points.back().first - 2.0) < 1e-9);    // Fensterende
        QVERIFY(std::fabs(s.min - 1.0) < 0.011 && std::fabs(s.max - 3.0) < 1e-9);
        // Ausduennen: hoechstens maxPoints (+1 fuer den letzten Punkt)
        QVERIFY(r.series(0, 20).points.size() <= 21);
        QCOMPARE(r.series(0, 20).points.back().second, 3.0);
        // neue Netzliste: Simulationszeit beginnt bei 0, die Aufzeichnung laeuft weiter
        r.push(0.01, {5.0, 0.0}, {true, false});
        QVERIFY(r.now() > 3.0);
        QCOMPARE(r.series(0, 1000).points.back().second, 5.0);
        // Pause: Zeit laeuft, aber keine neuen Punkte
        const std::size_t before = r.samples().size();
        r.setPaused(true);
        r.push(0.5, {7.0, 0.0}, {true, false});
        QCOMPARE(r.samples().size(), before);
        r.setPaused(false);
        r.clear();
        QVERIFY(r.samples().empty());
        QVERIFY(!r.series(0, 10).any);
    }

    // Formelrechner: Eingabe mit Vorsatz, Ausgabe mit Vorsatz, Rechenmodus, Rechenblaetter
    void formulaCalculator() {
        namespace f = model::formula;
        bool ok = false;
        auto approx = [](double a, double b) { return std::fabs(a - b) <= 1e-9 * std::max(1.0, std::fabs(b)); };
        QVERIFY(approx(f::parse("4,7k", ok), 4700.0) && ok);
        QVERIFY(approx(f::parse("4k7", ok), 4700.0) && ok);
        QVERIFY(approx(f::parse("470m", ok), 0.47) && ok);
        QVERIFY(approx(f::parse("100\xC2\xB5", ok), 1e-4) && ok); // 100µ
        QVERIFY(approx(f::parse("100u", ok), 1e-4) && ok);
        QVERIFY(approx(f::parse("2,2 M\xCE\xA9", ok), 2.2e6) && ok); // 2,2 MΩ
        QVERIFY(approx(f::parse("15 mA", ok), 0.015) && ok);
        QVERIFY(approx(f::parse("9 V", ok), 9.0) && ok);
        f::parse("abc", ok);
        QVERIFY(!ok);
        f::parse("1,2,3", ok);
        QVERIFY(!ok);
        QCOMPARE(f::format(4700.0, "\xCE\xA9"), std::string("4.7 k\xCE\xA9"));
        QCOMPARE(f::format(0.015, "A"), std::string("15 mA"));
        QCOMPARE(f::format(1e-4, "F"), std::string("100 \xC2\xB5" "F"));
        QCOMPARE(f::format(0.693, "s"), std::string("693 ms"));
        QCOMPARE(f::format(9.0, "V"), std::string("9 V"));
        QCOMPARE(f::format(1000.0, "\xCE\xA9"), std::string("1 k\xCE\xA9"));
        // Rechenmodus
        QVERIFY(approx(f::evaluate("4,7k \xC3\x97 2m", ok), 9.4) && ok); // ×
        QVERIFY(approx(f::evaluate("(1k + 1k) / 2", ok), 1000.0) && ok);
        QVERIFY(approx(f::evaluate("9 \xC3\xB7 1k", ok), 0.009) && ok); // ÷
        QVERIFY(approx(f::evaluate("-3 + 5", ok), 2.0) && ok);
        f::evaluate("1 / 0", ok);
        QVERIFY(!ok);
        f::evaluate("(1 + 2", ok);
        QVERIFY(!ok);
        // Ohmsches Gesetz: genau eine Groesse fehlt
        double u = 9.0, r = 1000.0, i = 0.0;
        QVERIFY(f::ohm(u, r, i) && approx(i, 0.009));
        u = 0.0; r = 470.0; i = 0.015;
        QVERIFY(f::ohm(u, r, i) && approx(u, 7.05));
        u = 9.0; r = 0.0; i = 0.0;
        QVERIFY(!f::ohm(u, r, i));
        // Vorwiderstand: (9 - 1,9) / 15 mA = 473 Ohm -> naechstgroesserer E12-Wert 560 Ohm
        const auto led = f::ledResistor(9.0, 1.9, 0.015);
        QVERIFY(led.ok && approx(led.ohm, 7.1 / 0.015));
        QCOMPARE(led.normOhm, 560.0);
        QVERIFY(led.normA < 0.015 && !led.tooHot);
        QVERIFY(f::ledResistor(9.0, 1.9, 0.2).tooHot); // 200 mA: 35 Ohm, 1,4 W
        QVERIFY(!f::ledResistor(1.5, 1.9, 0.01).ok);
        // Reihe, parallel, Leistung, Teiler, Zeitkonstante, Kippstufe
        QCOMPARE(f::series({1000.0, 1000.0, 470.0}), 2470.0);
        QVERIFY(approx(f::parallel({1000.0, 1000.0}), 500.0));
        double p = 0.0;
        QVERIFY(f::power(9.0, 0.02, 0.0, p) && approx(p, 0.18));
        QVERIFY(f::power(0.0, 0.1, 100.0, p) && approx(p, 1.0));
        QVERIFY(approx(f::divider(9.0, 2000.0, 1000.0), 3.0));
        QVERIFY(approx(f::timeConstant(10000.0, 100e-6), 1.0));
        QVERIFY(std::fabs(f::chargeAt(1.0, 1.0) - 0.632) < 0.001);
        // Blinkmodul-Kippstufe: RB 22 kOhm, C 22 uF -> ca. 1,49 Hz (ideal, ohne Basis-Emitter-Spannung)
        QVERIFY(std::fabs(f::astableHz(22000.0, 22e-6) - 1.0 / (2.0 * std::log(2.0) * 0.484)) < 1e-6);
    }

    void errorsAreReported() {
        model::Board board;
        board.parts.push_back({"X1", "gibt_es_nicht", {{"a", "A1"}, {"b", "A2"}}});
        board.parts.push_back({"R1", "widerstand", {{"a", "Z9"}, {"b", "A2"}}, 100.0});
        board.parts.push_back({"F1", "fotowiderstand", {{"a", "A1"}, {"b", "A2"}}}); // seit Stufe 3 simuliert
        board.wires.push_back({"L1", "A1", "BL9.aus", "rot"});
        board.modules.push_back({"XM1", "gibt_es_nicht", 5});
        const model::Netlist net = model::buildNetlist(board, m_catalog);
        QCOMPARE(net.errors.size(), std::size_t(4));
        // gueltige Teile werden trotzdem aufgebaut: die Schaltung bleibt simulierbar
        core::Simulator sim;
        model::Netlist copy = model::buildNetlist(board, m_catalog);
        QVERIFY(sim.setCircuit(std::move(copy.circuit)) == core::Status::Ok);
    }

    // Topologiewechsel: Ladung des Kondensators bleibt ueber transferStates erhalten
    void stateSurvivesRebuild() {
        model::Board board;
        board.desk.mainSwitch = true;
        board.parts.push_back({"C1", "elko", {{"plus", "A1"}, {"minus", "A2"}}, 100e-6});
        board.parts.push_back({"R1", "widerstand", {{"a", "B1"}, {"b", "A1"}}, 1000.0});
        board.wires.push_back({"L1", "P:+9V", "B1", "rot"});
        board.wires.push_back({"L2", "A2", "P:0V", "schwarz"});
        model::Netlist net;
        core::Simulator sim;
        QVERIFY(simulate(board, m_catalog, net, sim, 1.0)); // tau = 0,1 s: voll geladen
        const double charged = nodeVoltage(net, sim, "A1");
        QVERIFY(charged > 8.9);

        // Leitung zur Batterie entfernt: neuer Aufbau, Zustand uebernehmen
        board.wires.erase(board.wires.begin());
        model::Netlist next = model::buildNetlist(board, m_catalog);
        model::transferStates(sim.circuit(), next.circuit);
        QVERIFY(sim.setCircuit(std::move(next.circuit)) == core::Status::Ok);
        QVERIFY(std::fabs(nodeVoltage(next, sim, "A1") - charged) < 1e-3);
    }
};

QTEST_GUILESS_MAIN(ModelTest)
#include "test_model.moc"

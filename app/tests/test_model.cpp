// Tests fuer Schaltungsmodell und Dienste: Anschlussbezeichner, Inhaltspakete, Netzlistenbildung, Ende-zu-Ende mit
// dem Simulationskern (docs/testing.md, Modell).
#include <QtTest>

#include <cmath>

#include "model/anchor.hpp"
#include "model/build_guide.hpp"
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

    void errorsAreReported() {
        model::Board board;
        board.parts.push_back({"X1", "gibt_es_nicht", {{"a", "A1"}, {"b", "A2"}}});
        board.parts.push_back({"R1", "widerstand", {{"a", "Z9"}, {"b", "A2"}}, 100.0});
        board.parts.push_back({"F1", "fotowiderstand", {{"a", "A1"}, {"b", "A2"}}});
        board.wires.push_back({"L1", "A1", "BL9.aus", "rot"});
        board.modules.push_back({"XM1", "gibt_es_nicht", 5});
        const model::Netlist net = model::buildNetlist(board, m_catalog);
        QCOMPARE(net.errors.size(), std::size_t(5));
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

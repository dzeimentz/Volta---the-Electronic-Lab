// Tests fuer Schaltungsmodell und Dienste: Anschlussbezeichner, Inhaltspakete, Netzlistenbildung, Ende-zu-Ende mit
// dem Simulationskern (docs/testing.md, Modell).
#include <QtTest>

#include <cmath>

#include "model/anchor.hpp"
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

    void errorsAreReported() {
        model::Board board;
        board.parts.push_back({"X1", "gibt_es_nicht", {{"a", "A1"}, {"b", "A2"}}});
        board.parts.push_back({"R1", "widerstand", {{"a", "Z9"}, {"b", "A2"}}, 100.0});
        board.parts.push_back({"F1", "fotowiderstand", {{"a", "A1"}, {"b", "A2"}}});
        board.wires.push_back({"L1", "A1", "BL9.aus", "rot"});
        board.modules.push_back({"TM1", "tonmodul", 5});
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

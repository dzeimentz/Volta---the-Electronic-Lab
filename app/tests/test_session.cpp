// LabSession: Aufbau bearbeiten (Platzierregeln), Rueckgaengig/Wiederholen mit Zustandswiederherstellung (ADR-0005),
// Zusammenfassen beim Ziehen, Reparieren, Bedienhandlungen ausserhalb des Verlaufs.
#include <QtTest>

#include <cmath>

#include "model/experiment_flow.hpp"
#include "model/lab_session.hpp"
#include "services/content_repository.hpp"

using namespace volta;
using model::Orientation;
using model::Span;

class SessionTest : public QObject {
    Q_OBJECT

private:
    model::Catalog m_catalog;

    std::unique_ptr<model::LabSession> session() const { return std::make_unique<model::LabSession>(m_catalog); }

    // Simulieren und Schnappschuss holen (synchron)
    static void run(model::LabSession& s, double seconds) {
        s.bridge().runFor(seconds);
        s.poll();
    }

    static std::vector<std::pair<std::string, std::string>> pins(const model::LabSession& s, const std::string& id) {
        return s.board().findPart(id)->pins;
    }

private slots:
    void initTestCase() {
        services::ContentRepository repo(QStringLiteral(VOLTA_CONTENT_DIR));
        QVERIFY(repo.loadCatalog(m_catalog));
    }

    void placementRules() {
        auto s = session();
        std::string id;
        QVERIFY(s->addPart("widerstand", "B3", Orientation::Horizontal, Span::Short, &id));
        QCOMPARE(id, std::string("R1"));
        QCOMPARE(pins(*s, "R1"), (std::vector<std::pair<std::string, std::string>>{{"a", "B3"}, {"b", "B4"}}));
        QVERIFY(s->addPart("led_rot", "B4", Orientation::Vertical, Span::Short, &id));
        QCOMPARE(id, std::string("D1"));
        QCOMPARE(pins(*s, "D1"),
                 (std::vector<std::pair<std::string, std::string>>{{"anode", "B4"}, {"kathode", "C4"}}));
        QVERIFY(s->addPart("widerstand", "A1", Orientation::Horizontal, Span::Long, &id));
        QCOMPARE(pins(*s, id).back().second, std::string("A3"));

        // passt nicht auf die Platte
        QVERIFY(!s->addPart("widerstand", "E8", Orientation::Vertical, Span::Short));
        QVERIFY(!s->lastError().empty());
        QVERIFY(!s->addPart("widerstand", "A12", Orientation::Horizontal, Span::Short));
        QVERIFY(s->addPart("widerstand", "A11", Orientation::Horizontal, Span::Short)); // A11-A12
        // Motor nur mit langer Spannweite (Katalog)
        QVERIFY(!s->addPart("motor", "D1", Orientation::Horizontal, Span::Short));
        QVERIFY(s->addPart("motor", "D1", Orientation::Horizontal, Span::Long));
        // Traegerplatine: drei benachbarte Felder in Lage-Reihenfolge
        QVERIFY(s->addPart("transistor_npn", "E2", Orientation::Horizontal, Span::Short, &id));
        QCOMPARE(pins(*s, id),
                 (std::vector<std::pair<std::string, std::string>>{{"b", "E2"}, {"c", "E3"}, {"e", "E4"}}));
        QVERIFY(s->addPart("wechselschalter", "C6", Orientation::Vertical, Span::Short, &id));
        QCOMPARE(pins(*s, id), (std::vector<std::pair<std::string, std::string>>{
                                   {"mitte", "D6"}, {"a", "C6"}, {"b", "E6"}}));

        // hoechstens fuenf Beine je Feld (B4 hat schon R1 und D1)
        QVERIFY(s->addPart("widerstand", "B4", Orientation::Horizontal, Span::Short));
        QVERIFY(s->addPart("widerstand", "A4", Orientation::Vertical, Span::Short));
        QVERIFY(s->addPart("widerstand", "B4", Orientation::Horizontal, Span::Long)); // fuenftes Bein
        QVERIFY(!s->addPart("widerstand", "B4", Orientation::Vertical, Span::Short));
        QVERIFY(QString::fromStdString(s->lastError()).contains(QStringLiteral("voll")));
    }

    void flipKeepsPolarityWhenMoving() {
        auto s = session();
        s->setMainSwitch(true);
        QVERIFY(s->addPart("widerstand", "B3", Orientation::Horizontal, Span::Short, nullptr, 470.0)); // B3-B4
        QVERIFY(s->addPart("led_rot", "B4", Orientation::Vertical, Span::Short));     // Anode B4, Kathode C4
        QVERIFY(s->addWire("P:+9V", "B3", "rot"));
        QVERIFY(s->addWire("C4", "P:0V", "schwarz"));
        run(*s, 0.05);
        QVERIFY(s->reading("D1")->currentA > 14e-3);
        QVERIFY(s->flipPart("D1")); // verpolt: leuchtet nicht
        QCOMPARE(pins(*s, "D1"),
                 (std::vector<std::pair<std::string, std::string>>{{"anode", "C4"}, {"kathode", "B4"}}));
        run(*s, 0.05);
        QVERIFY(std::fabs(s->reading("D1")->currentA) < 1e-6);
        // Verschieben und Drehen behalten die Polung
        QVERIFY(s->movePart("D1", "B6", Orientation::Horizontal, Span::Short));
        QCOMPARE(pins(*s, "D1"),
                 (std::vector<std::pair<std::string, std::string>>{{"anode", "B7"}, {"kathode", "B6"}}));
        s->undoStack().undo();
        s->undoStack().undo(); // Umdrehen zurueck
        QCOMPARE(pins(*s, "D1").front().second, std::string("B4"));
        // Traegerplatine umdrehen: e links, b rechts
        QVERIFY(s->addPart("transistor_npn", "E1", Orientation::Horizontal, Span::Short));
        QVERIFY(s->flipPart("T1"));
        QCOMPARE(pins(*s, "T1"), (std::vector<std::pair<std::string, std::string>>{{"b", "E3"}, {"c", "E2"}, {"e", "E1"}}));
    }

    void undoRedoAndMerging() {
        auto s = session();
        QVERIFY(s->addPart("widerstand", "B3", Orientation::Horizontal, Span::Short));
        QVERIFY(s->addWire("P:+9V", "B3", "rot"));
        QCOMPARE(s->undoStack().count(), 2);
        // Ziehen: drei Verschiebungen desselben Bauteils = ein Schritt
        QVERIFY(s->movePart("R1", "C3", Orientation::Horizontal, Span::Short));
        QVERIFY(s->movePart("R1", "D3", Orientation::Horizontal, Span::Short));
        QVERIFY(s->movePart("R1", "D3", Orientation::Vertical, Span::Short));
        QCOMPARE(s->undoStack().count(), 3);
        s->undoStack().undo();
        QCOMPARE(pins(*s, "R1").front().second, std::string("B3"));
        s->undoStack().redo();
        QCOMPARE(pins(*s, "R1").back().second, std::string("E3"));
        // Leitungsende umsetzen (zusammengefasst) und ungueltige Leitung
        QVERIFY(s->moveWireEnd("L1", false, "C3"));
        QVERIFY(s->moveWireEnd("L1", false, "D3"));
        QCOMPARE(s->undoStack().count(), 4);
        QVERIFY(!s->addWire("B3", "B3", "rot"));
        QVERIFY(!s->addWire("Z9", "B3", "rot"));
        QVERIFY(!s->addWire("BL1.aus", "B3", "rot")); // kein Modul BL1 gesteckt
        // Alles zurueck
        while (s->undoStack().canUndo()) s->undoStack().undo();
        QVERIFY(s->board().parts.empty());
        QVERIFY(s->board().wires.empty());
    }

    // Kernfall des Konzepts: Rueckgaengig macht die zerstoerte LED wieder heil (Zeitreise)
    void undoRestoresDestroyedLed() {
        auto s = session();
        s->setMainSwitch(true);
        QVERIFY(s->addPart("led_rot", "B4", Orientation::Vertical, Span::Short));
        QVERIFY(s->addWire("C4", "P:0V", "schwarz"));
        run(*s, 0.01);
        QVERIFY(s->addWire("P:+9V", "B4", "rot")); // ohne Vorwiderstand!
        run(*s, 1.0);
        QVERIFY(s->reading("D1")->destroyed);

        s->undoStack().undo(); // Leitung weg, LED im Zustand von damals: heil
        run(*s, 0.01);
        QVERIFY(!s->reading("D1")->destroyed);
        QVERIFY(s->board().desk.mainSwitch); // Bedienhandlung bleibt

        s->undoStack().redo(); // Leitung wieder da: die LED brennt erneut durch
        run(*s, 1.0);
        QVERIFY(s->reading("D1")->destroyed);
    }

    void repairIsACommand() {
        auto s = session();
        s->setMainSwitch(true);
        QVERIFY(s->addPart("led_rot", "B4", Orientation::Vertical, Span::Short));
        QVERIFY(s->addWire("C4", "P:0V", "schwarz"));
        QVERIFY(s->addWire("P:+9V", "B4", "rot"));
        run(*s, 1.0);
        QVERIFY(s->reading("D1")->destroyed);
        QVERIFY(s->removeWire("L2"));        // Ursache beseitigen
        QVERIFY(s->repairPart("D1"));        // Reparieren
        run(*s, 0.01);
        QVERIFY(!s->reading("D1")->destroyed);
        QCOMPARE(s->undoStack().undoText(), QStringLiteral("D1 reparieren"));
        s->undoStack().undo();               // Reparieren rueckgaengig: wieder kaputt
        run(*s, 0.01);
        QVERIFY(s->reading("D1")->destroyed);
    }

    void commandRightAfterUndoKeepsRestoredState() {
        auto s = session();
        s->setMainSwitch(true);
        QVERIFY(s->addPart("led_rot", "B4", Orientation::Vertical, Span::Short));
        QVERIFY(s->addWire("C4", "P:0V", "schwarz"));
        run(*s, 0.01);
        QVERIFY(s->addWire("P:+9V", "B4", "rot"));
        run(*s, 1.0);
        QVERIFY(s->reading("D1")->destroyed);
        s->undoStack().undo();                  // LED heil (Vorgabe), noch kein neuer Schnappschuss
        QVERIFY(s->toggleVoltageDisplay("B4")); // sofort ein neuer Befehl
        run(*s, 0.01);
        QVERIFY(!s->reading("D1")->destroyed);
        s->undoStack().undo();                  // diesen Befehl zurueck: LED muss heil bleiben
        run(*s, 0.01);
        QVERIFY(!s->reading("D1")->destroyed);
    }

    void undoRestoresCapacitorCharge() {
        auto s = session();
        s->setMainSwitch(true);
        QVERIFY(s->addPart("elko", "A1", Orientation::Vertical, Span::Short, nullptr, 100e-6)); // plus A1, minus B1
        QVERIFY(s->addPart("widerstand", "A2", Orientation::Horizontal, Span::Short, nullptr, 1000.0)); // A2-A3
        QVERIFY(s->addWire("P:+9V", "A3", "rot"));
        QVERIFY(s->addWire("A2", "A1", "rot"));
        QVERIFY(s->addWire("B1", "P:0V", "schwarz"));
        run(*s, 1.0);
        QVERIFY(s->voltage("A1") > 8.9);
        QVERIFY(s->removeWire("L1"));          // Batterie ab: Ladung bleibt
        run(*s, 0.05);
        QVERIFY(s->voltage("A1") > 8.9);
        QVERIFY(s->addWire("A1", "B1", "gelb")); // Kurzschluss: entladen
        run(*s, 0.05);
        QVERIFY(s->voltage("A1") < 0.1);
        s->undoStack().undo();                 // Kurzschluss zurueck: Ladung von damals
        run(*s, 0.001);
        QVERIFY2(s->voltage("A1") > 8.9, qPrintable(QString::number(s->voltage("A1"))));
    }

    void operatingActionsAreNotCommands() {
        auto s = session();
        QVERIFY(s->addPart("taster", "B3", Orientation::Horizontal, Span::Short));
        const int count = s->undoStack().count();
        s->setMainSwitch(true);
        s->setCells(3);
        s->setPartClosed("S1", true);
        s->setPot(1, 0.3);
        s->setDeskSwitch(true);
        QCOMPARE(s->undoStack().count(), count);
        QCOMPARE(s->board().desk.cells, 3);
        QVERIFY(s->board().findPart("S1")->closed);
        // Spannungsanzeige setzen ist dagegen ein Befehl
        QVERIFY(s->toggleVoltageDisplay("B3"));
        QCOMPARE(s->undoStack().count(), count + 1);
        QCOMPARE(s->board().voltageDisplays, (std::vector<std::string>{"B3"}));
        s->undoStack().undo();
        QVERIFY(s->board().voltageDisplays.empty());
    }

    void voltageBarsFollowCells() {
        auto s = session();
        s->setMainSwitch(true);
        QVERIFY(s->addWire("P:+9V", "A1", "rot"));
        run(*s, 0.01);
        QCOMPARE(s->voltageBars("A1"), 4);
        s->setCells(2); // 3 V: Bezug ist die Batteriespannung
        run(*s, 0.01);
        QCOMPARE(s->voltageBars("A1"), 4);
        QVERIFY(std::fabs(s->voltage("A1") - 3.0) < 0.01);
    }

    // Erfolgspruefung im laufenden Aufbau: Bauteilbezug ueber den Typ, nicht ueber die Kennung der Vorlage
    void successMatchesPartsByType() {
        services::ContentRepository repo(QStringLiteral(VOLTA_CONTENT_DIR));
        services::Experiment exp;
        QVERIFY2(repo.loadExperiment(QStringLiteral("volta1.1.1"), exp), qPrintable(repo.lastError()));
        auto s = session();
        s->setCells(exp.templateBoard.desk.cells);
        s->setSuccess(exp.successCondition, exp.templateBoard);
        // zwei Laempchen, das erste (L1) wieder entfernt: das Kind arbeitet mit L2 auf anderen Feldern
        QVERIFY(s->addPart("gluehlampe", "A1", Orientation::Horizontal, Span::Short));
        QVERIFY(s->addPart("gluehlampe", "D5", Orientation::Horizontal, Span::Short));
        QVERIFY(s->removePart("L1"));
        QVERIFY(s->addWire("P:+9V", "D5", "rot"));
        s->setMainSwitch(true);
        run(*s, 3.0);
        QVERIFY(!s->successReached()); // Kreis offen
        QVERIFY(s->addWire("D6", "P:0V", "schwarz"));
        run(*s, 1.0);
        QVERIFY(!s->successReached()); // Haltedauer 2 s noch nicht erreicht
        run(*s, 1.5);
        QVERIFY(s->successReached());
        // Neue Bedingung: Ergebnis beginnt von vorn
        s->setSuccess(exp.successCondition, exp.templateBoard);
        QVERIFY(!s->successReached());
        s->clearSuccess();
        QVERIFY(!s->successActive());
    }

    // 6-V-Laempchen an 9 V: ueberlastet, gilt nicht als geloest (und brennt spaeter durch)
    void lampAtNineVoltIsNoSuccess() {
        services::ContentRepository repo(QStringLiteral(VOLTA_CONTENT_DIR));
        services::Experiment exp;
        QVERIFY(repo.loadExperiment(QStringLiteral("volta1.1.1"), exp));
        auto s = session();
        s->setSuccess(exp.successCondition, exp.templateBoard);
        s->replaceBoard(exp.templateBoard, QStringLiteral("Musteraufbau"));
        s->setCells(6);
        s->setMainSwitch(true);
        run(*s, 4.0);
        QVERIFY(!s->successReached());
        s->setCells(3); // 4,5 V wie vorgesehen
        run(*s, 3.0);
        QVERIFY(s->successReached());
    }

    void experimentFlow() {
        model::ExperimentFlow f;
        QCOMPARE(f.phase(), model::ExperimentFlow::Phase::None);
        QVERIFY(!f.reportSuccess());
        f.start(3);
        QCOMPARE(f.phase(), model::ExperimentFlow::Phase::Question);
        QVERIFY(!f.nextHint());      // Hilfen erst beim Aufbau
        QVERIFY(!f.reportSuccess()); // Erfolg erst nach der Vermutung
        QVERIFY(f.chooseGuess(1));
        QCOMPARE(f.guess(), 1);
        QCOMPARE(f.phase(), model::ExperimentFlow::Phase::Build);
        QVERIFY(f.nextHint() && f.nextHint());
        QVERIFY(!f.solutionShown());
        QVERIFY(f.nextHint());
        QVERIFY(f.solutionShown());
        QVERIFY(!f.nextHint());
        QVERIFY(f.reportSuccess());
        QCOMPARE(f.phase(), model::ExperimentFlow::Phase::Success);
        QVERIFY(!f.reportSuccess());
        f.stop();
        QCOMPARE(f.phase(), model::ExperimentFlow::Phase::None);
    }
};

QTEST_GUILESS_MAIN(SessionTest)
#include "test_session.moc"

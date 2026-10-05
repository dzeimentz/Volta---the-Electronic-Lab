// LabSession: Aufbau bearbeiten (Platzierregeln), Rueckgaengig/Wiederholen mit Zustandswiederherstellung (ADR-0005),
// Zusammenfassen beim Ziehen, Reparieren, Bedienhandlungen ausserhalb des Verlaufs.
#include <QtTest>

#include <algorithm>
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
        // Transistor auf 2-x-2-Platine: B | C oben, E | E unten; unterste Reihe passt nicht
        QVERIFY(!s->addPart("transistor_npn", "E2", Orientation::Horizontal, Span::Short));
        QVERIFY(s->addPart("transistor_npn", "D2", Orientation::Horizontal, Span::Short, &id));
        QCOMPARE(pins(*s, id), (std::vector<std::pair<std::string, std::string>>{
                                   {"b", "D2"}, {"c", "D3"}, {"e", "E2"}, {"e2", "E3"}}));
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
        // 2-x-2-Platine umdrehen = halbe Drehung: E | E oben, C | B unten
        QVERIFY(s->addPart("transistor_npn", "D1", Orientation::Horizontal, Span::Short));
        QVERIFY(s->flipPart("T1"));
        QCOMPARE(pins(*s, "T1"), (std::vector<std::pair<std::string, std::string>>{
                                     {"b", "E2"}, {"c", "E1"}, {"e", "D2"}, {"e2", "D1"}}));
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

    // Steckloecher: Beine zur Seite des Partnerfelds, senkrecht in die Mitte, Leitungen in freie Loecher
    void socketsFollowDirection() {
        model::Board b;
        b.parts.push_back({"R1", "widerstand", {{"a", "B3"}, {"b", "B4"}}, 470.0});   // waagrecht
        b.parts.push_back({"D1", "led_rot", {{"anode", "B4"}, {"kathode", "C4"}}});  // senkrecht
        b.parts.push_back({"R2", "widerstand", {{"a", "B5"}, {"b", "B3"}}, 470.0});  // Partner links (umgedreht)
        b.parts.push_back({"W1", "wechselschalter", {{"mitte", "E3"}, {"a", "E2"}, {"b", "E4"}}});
        b.parts.push_back({"T1", "transistor_npn", {{"b", "C6"}, {"c", "C7"}, {"e", "D6"}, {"e2", "D7"}}});
        b.wires.push_back({"L1", "P:+9V", "B3", "rot"});
        b.wires.push_back({"L2", "B4", "B8", "blau"});
        b.wires.push_back({"L3", "A1", "B4", "gelb"}); // anderes Ende links
        const model::SocketAssignment s = model::assignSockets(b);
        QCOMPARE(s.partPins.at("R1"), (std::vector<int>{3, 1}));
        QCOMPARE(s.partPins.at("D1"), (std::vector<int>{2, 2}));
        QCOMPARE(s.partPins.at("R2"), (std::vector<int>{1, 4})); // B5 nach links: 1; B3 nach rechts: 3 belegt -> 4
        QCOMPARE(s.partPins.at("W1"), (std::vector<int>{2, 3, 1})); // Mitte, aussen nach innen
        QCOMPARE(s.partPins.at("T1"), (std::vector<int>{3, 1, 3, 1})); // Platine: zur Mitte des Blocks
        QCOMPARE(s.wireEnds.at("L1"), (std::pair<int, int>{2, 2}));
        QCOMPARE(s.wireEnds.at("L2").first, 4); // B4: 1 (R1), 2 (D1) belegt; nach rechts von aussen
        QCOMPARE(s.wireEnds.at("L3").second, 0); // B4: von links von aussen
        // Volles Feld: Leitungen teilen sich ein Loch (keine Grenze fuer Leitungen)
        model::Board full;
        for (int i = 0; i < 7; ++i)
            full.wires.push_back({"L" + std::to_string(i + 1), "P:+9V", "A" + std::to_string(i + 1), "rot"});
        const model::SocketAssignment f = model::assignSockets(full);
        std::vector<int> count(5, 0);
        for (const auto& [id, ends] : f.wireEnds) ++count[static_cast<std::size_t>(ends.first)];
        QCOMPARE(*std::min_element(count.begin(), count.end()), 1); // jedes Loch benutzt, dann geteilt
        QCOMPARE(model::socketCount("M:3"), 1);
        QCOMPARE(model::socketCount("P:S1"), 5);
    }

    // 2-x-2-Platine: der zweite E-Fuss ist ueber die Leiterbahn mit E verbunden (Emitter nur ueber e2 an 0V)
    void transistorSecondEmitterConducts() {
        auto s = session();
        s->setMainSwitch(true);
        QVERIFY(s->addPart("transistor_npn", "C3", Orientation::Horizontal, Span::Short)); // B C3, C C4, E D3, E2 D4
        QVERIFY(s->addPart("widerstand", "B2", Orientation::Vertical, Span::Short, nullptr, 10e3)); // B2-C2
        QVERIFY(s->addWire("C2", "C3", "gelb"));                                                  // Basiswiderstand
        QVERIFY(s->addPart("widerstand", "A4", Orientation::Horizontal, Span::Short, nullptr, 470.0)); // A4-A5
        QVERIFY(s->addPart("led_rot", "B4", Orientation::Vertical, Span::Short));                 // B4 -> C4
        QVERIFY(s->addWire("A5", "B4", "rot"));
        QVERIFY(s->addWire("P:+9V", "B2", "rot"));
        QVERIFY(s->addWire("P:+9V", "A4", "rot"));
        QVERIFY(s->addWire("D4", "P:0V", "schwarz")); // nur der zweite E-Fuss
        run(*s, 0.2);
        const core::ComponentReading* led = s->reading("D1");
        QVERIFY(led);
        QVERIFY2(led->currentA > 5e-3, qPrintable(QString::number(led->currentA)));
        QVERIFY(std::fabs(s->voltage("D3") - s->voltage("D4")) < 1e-3); // Leiterbahn E <-> E2
    }

    // 2-x-2-Platine drehen: 90 Grad im Uhrzeigersinn, Verschieben behaelt die Drehung, Rueckgaengig
    void plateRotation() {
        using Pins = std::vector<std::pair<std::string, std::string>>;
        auto s = session();
        QVERIFY(s->addPart("transistor_npn", "D1", Orientation::Horizontal, Span::Short));
        const Pins start{{"b", "D1"}, {"c", "D2"}, {"e", "E1"}, {"e2", "E2"}};
        QCOMPARE(pins(*s, "T1"), start);
        QVERIFY(s->rotatePart("T1")); // B oben rechts, C unten rechts, E oben links, E2 unten links
        QCOMPARE(pins(*s, "T1"), (Pins{{"b", "D2"}, {"c", "E2"}, {"e", "D1"}, {"e2", "E1"}}));
        QVERIFY(s->movePart("T1", "B5", Orientation::Horizontal, Span::Short));
        QCOMPARE(pins(*s, "T1"), (Pins{{"b", "B6"}, {"c", "C6"}, {"e", "B5"}, {"e2", "C5"}}));
        QVERIFY(!s->movePart("T1", "E5", Orientation::Horizontal, Span::Short)); // unterste Reihe: passt nicht
        QVERIFY(!s->movePart("T1", "A12", Orientation::Horizontal, Span::Short)); // rechter Rand
        QVERIFY(s->rotatePart("T1") && s->rotatePart("T1") && s->rotatePart("T1"));
        QCOMPARE(pins(*s, "T1"), (Pins{{"b", "B5"}, {"c", "B6"}, {"e", "C5"}, {"e2", "C6"}})); // viermal = Ausgang
        s->undoStack().undo();
        s->undoStack().undo();
        s->undoStack().undo();
        s->undoStack().undo(); // Verschieben
        s->undoStack().undo(); // erste Drehung
        QCOMPARE(pins(*s, "T1"), start);
        // nur Platinen
        QVERIFY(s->addPart("widerstand", "A5", Orientation::Horizontal, Span::Short));
        QVERIFY(!s->rotatePart("R1"));
        // gedreht weiterhin leitend: Emitter ueber e2 (jetzt unten links)
        QVERIFY(s->rotatePart("T1"));
        const model::SocketAssignment a = model::assignSockets(s->board());
        QCOMPARE(a.partPins.at("T1"), (std::vector<int>{1, 1, 3, 3})); // Fuesse zur Blockmitte
    }

    void boardColumnsAreACommand() {
        auto s = session();
        QCOMPARE(s->board().columns, 12);
        QVERIFY(!s->setColumns(9));
        QVERIFY(!s->setColumns(14));
        QVERIFY(s->addPart("widerstand", "A10", Orientation::Horizontal, Span::Short)); // A10-A11
        QVERIFY(s->toggleVoltageDisplay("C12"));
        QVERIFY(!s->setColumns(10)); // A11 belegt
        QVERIFY(QString::fromStdString(s->lastError()).contains(QStringLiteral("11-12")));
        QVERIFY(s->movePart("R1", "A1", Orientation::Horizontal, Span::Short));
        QVERIFY(s->setColumns(8));
        QCOMPARE(s->board().columns, 8);
        QVERIFY(s->board().voltageDisplays.empty()); // C12 entfaellt
        // ausserhalb der Grundplatte: weder Bauteil noch Leitung
        QVERIFY(!s->addPart("widerstand", "B8", Orientation::Horizontal, Span::Short)); // B8-B9
        QVERIFY(!s->addWire("P:+9V", "A9", "rot"));
        QVERIFY(s->addPart("widerstand", "B7", Orientation::Horizontal, Span::Short));
        s->undoStack().undo(); // Bauteil
        s->undoStack().undo(); // Breite
        QCOMPARE(s->board().columns, 12);
        QCOMPARE(s->board().voltageDisplays, std::vector<std::string>{"C12"});
        s->undoStack().redo();
        QCOMPARE(s->board().columns, 8);
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

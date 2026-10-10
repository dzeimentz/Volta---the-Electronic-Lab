// LabSession: Aufbau bearbeiten (Platzierregeln), Rueckgaengig/Wiederholen mit Zustandswiederherstellung (ADR-0005),
// Zusammenfassen beim Ziehen, Reparieren, Bedienhandlungen ausserhalb des Verlaufs.
#include <QtTest>

#include <algorithm>
#include <cmath>

#include "model/board_edit.hpp"
#include "model/environment.hpp"
#include "model/experiment_flow.hpp"
#include "model/lab_session.hpp"
#include "services/content_repository.hpp"
#include "services/journal_store.hpp"
#include "services/progress_store.hpp"

#include <QJsonArray>
#include <QTemporaryDir>

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
        QVERIFY(s->templateComplete()); // ein Laempchen wie in der Vorlage
        s->setMainSwitch(false);        // Bedienhandlung: Erfolg bleibt
        QVERIFY(s->successReached());
        QVERIFY(s->removePart("L2"));   // Aufbauaenderung: muss erneut funktionieren
        QVERIFY(!s->successReached());
        QVERIFY(!s->templateComplete());
        s->undoStack().undo();
        QVERIFY(s->templateComplete());
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

    // Dreifach-Messgeraet ueber die Sitzung: V, A und Ohm messen ehrlich (Abweichung < 2 %)
    void meterMeasures() {
        auto s = session();
        QVERIFY(!s->addWire("P:+9V", "MESS:VOHM", "rot")); // ausgeblendet: Buchsen nicht steckbar
        QVERIFY(s->setMeterVisible(true));
        s->setMainSwitch(true);
        QVERIFY(s->addWire("P:+9V", "MESS:VOHM", "rot"));
        QVERIFY(s->addWire("P:0V", "MESS:COM", "schwarz"));
        run(*s, 0.05);
        const core::ComponentReading* r = s->deskReading(model::desk_id::kMeter);
        QVERIFY(r);
        QVERIFY2(std::fabs(r->displayValue - 9.0) < 0.18, qPrintable(QString::number(r->displayValue)));
        QVERIFY(!s->setMeterVisible(false)); // Leitungen stecken noch
        // Strom durch 470 Ohm
        QVERIFY(s->removeWire("L1"));
        QVERIFY(s->addPart("widerstand", "B6", Orientation::Horizontal, Span::Short, nullptr, 470.0));
        QVERIFY(s->addWire("P:+9V", "B6", "rot"));
        QVERIFY(s->addWire("B7", "MESS:A", "rot"));
        s->setMeterMode(core::MeterMode::Ampere);
        run(*s, 0.05);
        r = s->deskReading(model::desk_id::kMeter);
        QVERIFY2(std::fabs(r->displayValue - 9.0 / 470.1) < 0.02 * 9.0 / 470.1, qPrintable(QString::number(r->displayValue)));
        // Widerstand 470 Ohm (Batterie getrennt: nur Messquelle)
        // Leitungen jetzt: L1 +9V-B6 (freie Kennung wiederverwendet), L2 0V-COM, L3 B7-A
        QVERIFY(s->removeWire("L1"));
        QVERIFY(s->removeWire("L3"));
        QVERIFY(s->removeWire("L2"));
        QVERIFY(s->addWire("B6", "MESS:VOHM", "rot"));
        QVERIFY(s->addWire("B7", "MESS:COM", "schwarz"));
        s->setMeterMode(core::MeterMode::Ohm);
        run(*s, 0.05);
        r = s->deskReading(model::desk_id::kMeter);
        QVERIFY2(std::fabs(r->displayValue - 470.0) < 9.4, qPrintable(QString::number(r->displayValue)));
        // abziehen, dann ausblenden; Rueckgaengig holt die Leitung und blendet wieder ein
        QVERIFY(s->removeWire("L1"));
        QVERIFY(s->removeWire("L2"));
        QVERIFY(s->setMeterVisible(false));
        s->undoStack().undo();
        QVERIFY(s->board().desk.meterVisible);
    }

    // Sicherung: Kurzschluss ueber den A-Eingang brennt sie durch; tauschen ist ein Befehl
    void meterFuseReplace() {
        auto s = session();
        QVERIFY(s->setMeterVisible(true));
        s->setMainSwitch(true);
        s->setMeterMode(core::MeterMode::Ampere);
        QVERIFY(s->addWire("P:+9V", "MESS:A", "rot"));
        QVERIFY(s->addWire("MESS:COM", "P:0V", "schwarz"));
        run(*s, 0.1);
        const core::ComponentReading* r = s->deskReading(model::desk_id::kMeter);
        QVERIFY(r && (r->displayFlags & core::kDisplayFuseBlown));
        QVERIFY(s->removeWire("L1"));
        QVERIFY(s->replaceMeterFuse());
        run(*s, 0.05);
        r = s->deskReading(model::desk_id::kMeter);
        QVERIFY(r && !(r->displayFlags & core::kDisplayFuseBlown));
        s->undoStack().undo(); // Sicherung wieder durchgebrannt
        run(*s, 0.05);
        r = s->deskReading(model::desk_id::kMeter);
        QVERIFY(r && (r->displayFlags & core::kDisplayFuseBlown));
    }

    // Klanganzeige: Blinkmodul schaltet den Pult-Lautsprecher (Impulse unter 20 Hz); reparieren ist ein Befehl
    void speakerPulsesFromBlinkModule() {
        auto s = session();
        model::Board b;
        b.desk.mainSwitch = true;
        b.modules.push_back({"BL1", "blinkmodul", 1});
        b.wires.push_back({"L1", "P:+9V", "BL1.plus", "rot"});
        b.wires.push_back({"L2", "BL1.minus", "P:0V", "schwarz"});
        b.wires.push_back({"L3", "P:+9V", "P:LS1", "rot"});
        b.parts.push_back({"R1", "widerstand", {{"a", "A1"}, {"b", "A2"}}, 100.0});
        b.wires.push_back({"L4", "P:LS2", "A1", "blau"});
        b.wires.push_back({"L5", "A2", "BL1.aus", "blau"});
        s->loadBoard(b);
        run(*s, 6.0);
        const core::ComponentReading* r = s->deskReading(model::desk_id::kSpeaker);
        QVERIFY(r);
        QVERIFY2(r->displayValue >= 3.0, qPrintable(QString::number(r->displayValue))); // ca. 0,85 Hz
        QVERIFY2(r->frequencyHz > 0.5 && r->frequencyHz < 1.5, qPrintable(QString::number(r->frequencyHz)));
        QVERIFY(!r->destroyed);
        QVERIFY(s->repairSpeaker());
        QCOMPARE(s->undoStack().count(), 1);
    }

    // Farbautomatik: am Pluspol rot, am Minuspol schwarz (auch ueber Leitungen verbunden); von Hand gewaehlte Farbe bleibt
    void wirePoleColors() {
        model::Board b;
        b.wires.push_back({"L1", "P:+9V", "A1", "gelb"});
        b.wires.push_back({"L2", "C3", "P:0V", "blau"});
        QCOMPARE(model::poleColor(b, "P:+9V", "B2"), std::string("rot"));
        QCOMPARE(model::poleColor(b, "B2", "P:0V"), std::string("schwarz"));
        QCOMPARE(model::poleColor(b, "A1", "B5"), std::string("rot"));     // A1 haengt am Pluspol
        QCOMPARE(model::poleColor(b, "B5", "C3"), std::string("schwarz")); // C3 haengt am Minuspol
        QCOMPARE(model::poleColor(b, "B5", "B6"), std::string());          // kein Pol: gewaehlte Farbe
        QCOMPARE(model::poleColor(b, "A1", "C3"), std::string());          // beide Pole (Kurzschluss)
        QCOMPARE(model::poleColor(b, "A1", "B5", "L1"), std::string());    // L1 nicht mitgezaehlt

        auto s = session();
        QVERIFY(s->addWire("B2", "B5", "gruen"));
        QVERIFY(s->moveWireEnd("L1", true, "P:+9V", true)); // Ende an den Pluspol umgesetzt: rot
        QCOMPARE(s->board().wires[0].color, std::string("rot"));
        QVERIFY(s->setWireColor("L1", "gelb")); // von Hand: bleibt
        QVERIFY(s->board().wires[0].colorFixed);
        QVERIFY(s->moveWireEnd("L1", false, "P:0V", false));
        QVERIFY(s->moveWireEnd("L1", false, "C7", true));
        QCOMPARE(s->board().wires[0].color, std::string("gelb"));
    }

    // Stufe 3: Taschenlampe und Foehn wirken ueber den Abstand auf Fotowiderstand und Heissleiter (Bedienhandlungen)
    void lightAndHeatTools() {
        auto s = session();
        QVERIFY(s->addPart("fotowiderstand", "B3", Orientation::Horizontal, Span::Short)); // Mitte bei Spalte 3,5 / B
        QVERIFY(s->addPart("heissleiter", "D3", Orientation::Horizontal, Span::Short));
        const model::PartPlacement* ldr = s->board().findPart("LDR1");
        const model::PartPlacement* ntc = s->board().findPart("NTC1");
        QVERIFY(ldr && ntc);
        QCOMPARE(model::illuminanceAt(s->board(), *ldr), 300.0); // Raumlicht hell
        s->setRoomLight(0);
        QCOMPARE(model::illuminanceAt(s->board(), *ldr), 0.5);
        s->placeTool("taschenlampe", 3.5, 1.0); // direkt darueber
        QVERIFY(std::fabs(model::illuminanceAt(s->board(), *ldr) - 5000.5) < 1e-6);
        s->placeTool("taschenlampe", 4.5, 1.0); // ein Feld weiter: 5000 / (1 + 2^2) = 1000 lx
        QVERIFY(std::fabs(model::illuminanceAt(s->board(), *ldr) - 1000.5) < 1e-6);
        s->setToolOn("taschenlampe", false);
        QCOMPARE(model::illuminanceAt(s->board(), *ldr), 0.5);
        QCOMPARE(model::airTemperatureAt(s->board(), *ntc), core::kAmbientK);
        s->placeTool("foehn", 3.5, 3.0);
        QVERIFY(std::fabs(model::airTemperatureAt(s->board(), *ntc) - (core::kAmbientK + 60.0)) < 1e-6);
        QCOMPARE(s->board().desk.tools.size(), std::size_t(2));

        // Wirkung in der Simulation: Fotowiderstand in Reihe mit 1 kOhm an 9 V
        QVERIFY(s->addWire("P:+9V", "B3", "rot"));
        QVERIFY(s->addPart("widerstand", "B4", Orientation::Horizontal, Span::Long)); // B4-B6
        QVERIFY(s->addWire("B6", "P:0V", "schwarz"));
        s->setMainSwitch(true);
        run(*s, 0.5);
        const double darkmA = s->reading("LDR1")->currentA * 1e3;
        s->setToolOn("taschenlampe", true);
        run(*s, 0.5);
        const double brightmA = s->reading("LDR1")->currentA * 1e3;
        // dunkel (0,5 lx): ca. 75 kOhm -> 0,12 mA; Taschenlampe ein Feld weiter (1000 lx): ca. 0,4 kOhm -> 6,4 mA
        QVERIFY2(darkmA < 0.2 && brightmA > 5.0, qPrintable(QStringLiteral("%1 / %2").arg(darkmA).arg(brightmA)));
        // Heissleiter erwaermt sich vor dem Foehn
        run(*s, 6.0);
        QVERIFY(s->reading("NTC1")->tempK > core::kAmbientK + 35.0);
        s->removeTool("foehn");
        QCOMPARE(s->board().desk.tools.size(), std::size_t(1));
        QCOMPARE(s->undoStack().count(), 5); // nur die Aufbauaenderungen (3 Bauteile, 2 Leitungen) sind Befehle
    }

    // Modulfach: Module setzen, verschieben, herausnehmen sind Befehle; Leitungen folgen dem Modul
    void moduleCommands() {
        auto s = session();
        std::string id;
        QVERIFY(s->addModule("blinkmodul", 1, &id));
        QCOMPARE(id, std::string("BL1"));
        QVERIFY(!s->addModule("tonmodul", 3)); // ueberlappt BL1 (M1-M4)
        QVERIFY(QString::fromStdString(s->lastError()).contains(QStringLiteral("Modul")));
        QVERIFY(!s->addModule("tonmodul", 14)); // M14-M17 passt nicht
        QVERIFY(!s->addModule("gibtsnicht", 6));
        QVERIFY(s->addModule("tonmodul", 6, &id));
        QCOMPARE(id, std::string("TM1"));
        QVERIFY(s->addWire("P:+9V", "BL1.plus", "rot")); // ueber den Anschlussnamen
        QVERIFY(s->addWire("M:2", "P:0V", "schwarz"));   // ueber das Feld der Leiste (BL1.minus)
        QCOMPARE(model::moduleSlotOf(s->board(), m_catalog, "BL1.plus"), std::string("M:1"));
        QVERIFY(s->netlist().ok());

        QVERIFY(!s->moveModule("BL1", 5)); // trifft TM1
        QVERIFY(s->moveModule("BL1", 11));
        QCOMPARE(s->board().wires[1].from, std::string("M:12")); // Leitung auf dem Feld wandert mit
        QCOMPARE(model::moduleSlotOf(s->board(), m_catalog, "BL1.plus"), std::string("M:11"));
        s->undoStack().undo();
        QCOMPARE(s->board().wires[1].from, std::string("M:2"));
        QCOMPARE(s->board().modules[0].slot, 1);

        // Herausnehmen: die Leitung an BL1.plus bleibt auf dem Feld M:1 liegen; Netzliste bleibt gueltig
        QVERIFY(s->removeModule("BL1"));
        QCOMPARE(s->board().modules.size(), std::size_t(1));
        QCOMPARE(s->board().wires[0].to, std::string("M:1"));
        QVERIFY(s->netlist().ok());
        s->undoStack().undo();
        QCOMPARE(s->board().wires[0].to, std::string("BL1.plus"));
        QCOMPARE(s->board().modules.size(), std::size_t(2));
    }

    // Steckvorschau beim Ziehen aus dem Fach: alle Felder des Bauteils, ohne Befehl
    void previewShowsAllFields() {
        auto s = session();
        std::vector<std::pair<std::string, std::string>> pins;
        QVERIFY(s->previewPart("widerstand", "A1", Orientation::Horizontal, Span::Short, pins));
        QCOMPARE(pins.size(), std::size_t(2));
        QCOMPARE(pins[0].second, std::string("A1"));
        QCOMPARE(pins[1].second, std::string("A2"));
        QVERIFY(s->previewPart("transistor_npn", "B3", Orientation::Horizontal, Span::Short, pins));
        QCOMPARE(pins.size(), std::size_t(4)); // 2-x-2-Platine
        QVERIFY(s->previewPart("motor", "C4", Orientation::Horizontal, Span::Long, pins));
        QCOMPARE(pins.back().second, std::string("C6"));
        QVERIFY(!s->previewPart("widerstand", "A12", Orientation::Horizontal, Span::Short, pins)); // ragt hinaus
        QVERIFY(pins.empty());
        QCOMPARE(s->undoStack().count(), 0);
    }

    // Fehlersuche: Auffaelligkeiten als Fragen an der richtigen Stelle; ein funktionierender Aufbau bleibt ohne Befund
    void circuitCheckFindsTypicalFaults() {
        using K = model::CircuitFinding::Kind;
        auto kinds = [](const std::vector<model::CircuitFinding>& f) {
            std::vector<K> k;
            for (const auto& x : f) k.push_back(x.kind);
            return k;
        };
        auto has = [&](const std::vector<model::CircuitFinding>& f, K kind) {
            const auto k = kinds(f);
            return std::find(k.begin(), k.end(), kind) != k.end();
        };
        auto s = session();
        QVERIFY(s->checkCircuit().empty()); // leere Grundplatte

        // Widerstand 1 kOhm an 9 V: nur der Hauptschalter fehlt, danach ohne Befund
        QVERIFY(s->addPart("widerstand", "B3", Orientation::Horizontal, Span::Short)); // R1 B3-B4
        QVERIFY(s->addWire("P:+9V", "B3", "rot"));
        auto f = s->checkCircuit();
        QVERIFY(!f.empty());
        QVERIFY(has(f, K::Dangling)); // B4 lose
        QVERIFY(has(f, K::NoSupply)); // 0V fehlt
        QCOMPARE(f.front().anchors.front(), std::string("B4"));
        QVERIFY(s->addWire("B4", "P:0V", "schwarz"));
        f = s->checkCircuit();
        QCOMPARE(f.size(), std::size_t(1));
        if (f.empty()) return;
        QCOMPARE(f.front().kind, K::MainSwitchOff);
        s->setMainSwitch(true);
        run(*s, 0.2);
        f = s->checkCircuit();
        QVERIFY2(f.empty(), f.empty() ? "" : f.front().message.text().c_str());

        // offener Taster in Reihe: genau der Taster ist die Frage
        QVERIFY(s->addPart("taster", "D3", Orientation::Horizontal, Span::Short)); // S1 D3-D4
        s->undoStack().undo();
        s->undoStack().undo(); // Leitung B4-0V und Taster wieder weg
        QVERIFY(s->addPart("taster", "D3", Orientation::Horizontal, Span::Short));
        QVERIFY(s->addWire("B4", "D3", "blau"));
        QVERIFY(s->addWire("D4", "P:0V", "schwarz"));
        f = s->checkCircuit();
        QCOMPARE(f.size(), std::size_t(1));
        if (f.empty()) return;
        QCOMPARE(f.front().kind, K::OpenSwitch);
        QCOMPARE(f.front().partId, std::string("S1"));

        // Sackgasse: Leitung zum Minuspol fehlt hinter dem geschlossenen Taster
        s->setPartClosed("S1", true);
        s->undoStack().undo(); // Leitung D4-0V weg
        f = s->checkCircuit();
        QVERIFY(has(f, K::NoSupply) || has(f, K::DeadEnd));

        // Kurzschluss ueber eine Leitung
        QVERIFY(s->addWire("P:+9V", "P:0V", "rot"));
        f = s->checkCircuit();
        QVERIFY(!f.empty());
        QCOMPARE(f.front().kind, K::Short);
    }

    // Fehlersuche an der laufenden Simulation: verpolte und ungeschuetzte Leuchtdiode
    void circuitCheckLed() {
        using K = model::CircuitFinding::Kind;
        auto s = session();
        QVERIFY(s->addPart("led_rot", "B3", Orientation::Horizontal, Span::Short)); // D1 Anode B3, Kathode B4
        QVERIFY(s->addPart("widerstand", "C3", Orientation::Horizontal, Span::Short)); // R1 C3-C4
        QVERIFY(s->setPartValue("R1", 470.0));
        // verpolt: Pluspol ueber den Widerstand an die Kathode
        QVERIFY(s->addWire("P:+9V", "C3", "rot"));
        QVERIFY(s->addWire("C4", "B4", "blau"));
        QVERIFY(s->addWire("B3", "P:0V", "schwarz"));
        s->setMainSwitch(true);
        run(*s, 0.2);
        auto f = s->checkCircuit();
        QVERIFY(!f.empty());
        QCOMPARE(f.front().kind, K::Reversed);
        QCOMPARE(f.front().partId, std::string("D1"));

        // ohne Vorwiderstand richtig herum: sehr grosser Strom bzw. zerstoert
        auto t = session();
        QVERIFY(t->addPart("led_rot", "B3", Orientation::Horizontal, Span::Short));
        QVERIFY(t->addWire("P:+9V", "B3", "rot"));
        QVERIFY(t->addWire("B4", "P:0V", "schwarz"));
        t->setMainSwitch(true);
        run(*t, 0.05);
        f = t->checkCircuit();
        QVERIFY(!f.empty());
        QVERIFY(f.front().kind == K::NoResistor || f.front().kind == K::Broken);
        QCOMPARE(f.front().partId, std::string("D1"));
    }

    // Stufe 4: Wassertropfen macht die Feuchtesonde nass (Bedienhandlung, Wirkung ueber den Abstand)
    void waterToolWetsProbe() {
        auto s = session();
        QVERIFY(s->addPart("feuchtesonde", "B1", Orientation::Horizontal, Span::Long)); // B1-B3, Mitte Spalte 2
        const model::PartPlacement* probe = s->board().findPart("F1");
        QVERIFY(probe);
        QCOMPARE(model::wetnessAt(s->board(), *probe), 0.0);
        s->placeTool("wasser", 3.0, 1.0); // ein Feld daneben: 1 / (1 + (1/0,6)^2) = 0,265
        QVERIFY(std::fabs(model::wetnessAt(s->board(), *probe) - 0.2647) < 1e-3);
        s->placeTool("wasser", 2.0, 1.0); // direkt darauf
        QCOMPARE(model::wetnessAt(s->board(), *probe), 1.0);

        // Simulation: Sonde in Reihe mit 1 kOhm an 9 V; nass ca. 20 kOhm -> ca. 0,43 mA, trocken fast nichts
        QVERIFY(s->addWire("P:+9V", "B1", "rot"));
        QVERIFY(s->addPart("widerstand", "B3", Orientation::Horizontal, Span::Short));
        QVERIFY(s->setPartValue("R1", 1000.0));
        QVERIFY(s->addWire("B4", "P:0V", "schwarz"));
        s->setMainSwitch(true);
        run(*s, 2.0);
        const double wetmA = s->reading("F1")->currentA * 1e3;
        s->removeTool("wasser");
        run(*s, 20.0); // trocknet langsam (tau 4 s)
        const double drymA = s->reading("F1")->currentA * 1e3;
        QVERIFY2(wetmA > 0.35 && wetmA < 0.5 && drymA < 0.01, qPrintable(QStringLiteral("%1 / %2").arg(wetmA).arg(drymA)));
    }

    // Stufe 4: Zeitmodul haelt den Ausgang nach einem kurzen Startimpuls ca. 3,2 s eingeschaltet
    void timerModuleHoldsOutput() {
        auto s = session();
        QVERIFY(s->addModule("zeitmodul", 1));
        QVERIFY(s->addWire("P:+9V", "ZM1.plus", "rot"));
        QVERIFY(s->addWire("ZM1.minus", "P:0V", "schwarz"));
        QVERIFY(s->addPart("taster", "A1", Orientation::Horizontal, Span::Short));
        QVERIFY(s->addWire("P:+9V", "A1", "rot"));
        QVERIFY(s->addWire("A2", "ZM1.start", "gelb"));
        QVERIFY(s->addPart("widerstand", "B3", Orientation::Horizontal, Span::Short)); // Last 1 kOhm
        QVERIFY(s->setPartValue("R1", 1000.0));
        QVERIFY(s->addWire("P:+9V", "B3", "rot"));
        QVERIFY(s->addWire("B4", "ZM1.aus", "blau"));
        QVERIFY(s->netlist().ok());
        s->setMainSwitch(true);
        run(*s, 1.0);
        QVERIFY(s->reading("R1")->currentA < 0.5e-3); // Ruhe: aus
        s->setPartClosed("S1", true);
        run(*s, 0.1);
        s->setPartClosed("S1", false);
        run(*s, 1.0);
        QVERIFY(s->reading("R1")->currentA > 7e-3); // haelt nach dem Loslassen
        QVERIFY(s->moduleReading("ZM1"));           // Hauptbauteil T3 (Ausgangstransistor)
        run(*s, 4.0);
        QVERIFY(s->reading("R1")->currentA < 0.5e-3); // nach ca. 3,2 s wieder aus
    }

    // Stufe 4: Verstaerkermodul (Darlington) schaltet schon mit wenigen Mikroampere am Eingang
    void amplifierModuleIsSensitive() {
        auto s = session();
        QVERIFY(s->addModule("verstaerkermodul", 1));
        QVERIFY(s->addWire("P:+9V", "VM1.plus", "rot"));
        QVERIFY(s->addWire("VM1.minus", "P:0V", "schwarz"));
        QVERIFY(s->addPart("widerstand", "A1", Orientation::Horizontal, Span::Short)); // Eingang ueber 1 MOhm
        QVERIFY(s->setPartValue("R1", 1.0e6));
        QVERIFY(s->addWire("A2", "VM1.ein", "gelb"));
        QVERIFY(s->addPart("widerstand", "B3", Orientation::Horizontal, Span::Short)); // Last 1 kOhm
        QVERIFY(s->setPartValue("R2", 1000.0));
        QVERIFY(s->addWire("P:+9V", "B3", "rot"));
        QVERIFY(s->addWire("B4", "VM1.aus", "blau"));
        s->setMainSwitch(true);
        run(*s, 0.2);
        QVERIFY(s->reading("R2")->currentA < 0.1e-3); // Eingang offen: aus
        QVERIFY(s->addWire("P:+9V", "A1", "rot"));      // ca. 8 uA in den Eingang
        s->setMainSwitch(true);
        run(*s, 0.2);
        QVERIFY2(s->reading("R2")->currentA > 7e-3, qPrintable(QString::number(s->reading("R2")->currentA)));
    }

    // Tonmodul: Taster an ein startet die Melodie; der Lautsprecher am Ausgang zeigt den Ton (Klanganzeige)
    void toneModuleDrivesSpeaker() {
        auto s = session();
        QVERIFY(s->addModule("tonmodul", 1));
        QVERIFY(s->addWire("P:+9V", "TM1.plus", "rot"));
        QVERIFY(s->addWire("TM1.minus", "P:0V", "schwarz"));
        QVERIFY(s->addWire("P:+9V", "P:LS1", "rot"));
        QVERIFY(s->addWire("P:LS2", "TM1.aus", "blau"));
        QVERIFY(s->addPart("taster", "A1", Orientation::Horizontal, Span::Short));
        QVERIFY(s->addWire("P:+9V", "A1", "rot"));
        QVERIFY(s->addWire("A2", "TM1.ein", "gelb"));
        QVERIFY(s->netlist().ok());
        s->setMainSwitch(true);
        run(*s, 0.3);
        core::ComponentReading ls;
        QVERIFY(s->speakerReading(ls));
        QCOMPARE(ls.frequencyHz, 0.0); // still
        const core::ComponentReading* tm = s->moduleReading("TM1");
        QVERIFY(tm && tm->displayValue < 0.0);

        s->setPartClosed("S1", true); // Taster gedrueckt (Bedienhandlung)
        run(*s, 0.1);
        tm = s->moduleReading("TM1");
        QVERIFY(tm && tm->frequencyHz > 500.0);
        QVERIFY(s->speakerReading(ls));
        QCOMPARE(ls.frequencyHz, tm->frequencyHz);
        QVERIFY2(ls.level > 0.3 && ls.level < 0.6, qPrintable(QString::number(ls.level))); // ca. 43 mA / 100 mA
        s->setPartClosed("S1", false);
        run(*s, 0.1);
        QVERIFY(s->speakerReading(ls));
        QCOMPARE(ls.frequencyHz, 0.0);
    }

    // Erfolgsbedingung auf ein Modul der Vorlage: Zuordnung ueber den Typ (das Kind setzt das Modul selbst)
    void successMatchesModulesByType() {
        model::SuccessCondition c;
        c.kind = model::SuccessCondition::Kind::Quantity;
        c.part = "TM1";
        c.quantity = model::SuccessCondition::Quantity::FrequencyHz;
        c.hasMin = true;
        c.min = 100.0;
        c.holdS = 0.2;
        model::Board tmpl;
        tmpl.modules.push_back({"TM1", "tonmodul", 1});
        auto s = session();
        s->setSuccess(c, tmpl);
        QVERIFY(s->addModule("blinkmodul", 1));
        QVERIFY(s->addModule("tonmodul", 6));  // TM1
        QVERIFY(s->addModule("tonmodul", 12)); // TM2
        QVERIFY(s->removeModule("TM1"));       // das Kind arbeitet mit TM2
        QVERIFY(s->addWire("P:+9V", "TM2.plus", "rot"));
        QVERIFY(s->addWire("TM2.minus", "P:0V", "schwarz"));
        QVERIFY(s->addWire("P:+9V", "TM2.ein", "gelb"));
        s->setMainSwitch(true);
        run(*s, 0.5);
        QVERIFY(s->successReached());
    }

    // Gluehlampe: Nennspannung 2,5 / 6 / 10 V waehlbar (Befehl); die Variante bestimmt das Modell
    void lampVariants() {
        auto s = session();
        QVERIFY(s->addPart("gluehlampe", "A1", Orientation::Horizontal, Span::Short));
        QCOMPARE(s->board().findPart("L1")->value, 6.0); // Vorgabe aus dem Katalog
        QVERIFY(!s->setPartValue("L1", 12.0));           // nicht im Katalog
        QVERIFY(s->setPartValue("L1", 2.5));
        QVERIFY(s->addWire("P:+9V", "A1", "rot"));
        QVERIFY(s->addWire("A2", "P:0V", "schwarz"));
        s->setCells(2); // 3 V
        s->setMainSwitch(true);
        run(*s, 0.5);
        const core::ComponentReading* r = s->reading("L1");
        QVERIFY(r);
        // 2,5-V-Lampe (200 mA) an 3 V (Innenwiderstand 0,5 Ohm): etwas ueber Nennleistung, hell
        QVERIFY2(r->level > 0.8, qPrintable(QString::number(r->level)));
        QVERIFY(s->setPartValue("L1", 10.0));
        run(*s, 0.5);
        r = s->reading("L1");
        QVERIFY2(r->level < 0.2, qPrintable(QString::number(r->level))); // 10-V-Lampe an 3 V: kaum Licht
        s->undoStack().undo();
        QCOMPARE(s->board().findPart("L1")->value, 2.5);
    }

    // Fortschritt: nur ID und Datum, bleibt nach Neustart erhalten, zuruecksetzbar
    void progressStorePersists() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString file = dir.filePath(QStringLiteral("sub/fortschritt.json"));
        {
            services::ProgressStore p(file);
            QVERIFY(!p.isDone(QStringLiteral("volta1.1.1")));
            QVERIFY(p.markDone(QStringLiteral("volta1.1.1")));
            QVERIFY(p.markDone(QStringLiteral("volta1.1.1"))); // zweimal: bleibt einmal erledigt
        }
        services::ProgressStore again(file);
        QVERIFY(again.isDone(QStringLiteral("volta1.1.1")));
        QCOMPARE(again.doneIds().size(), 1);
        {
            QFile f(file);
            QVERIFY(f.open(QIODevice::ReadOnly));
            const QByteArray json = f.readAll();
            QVERIFY(json.contains("volta1.1.1") && json.contains("datum")); // keine weiteren Angaben
        }
        QVERIFY(again.reset());
        QVERIFY(!services::ProgressStore(file).isDone(QStringLiteral("volta1.1.1")));
        services::ProgressStore memory; // ohne Datei: nur im Speicher
        QVERIFY(memory.markDone(QStringLiteral("x")));
    }

    void boardColumnsAreACommand() {
        auto s = session();
        QCOMPARE(s->board().columns, 12);
        QVERIFY(!s->setColumns(9));
        QVERIFY(!s->setColumns(14));
        QVERIFY(s->addPart("widerstand", "A10", Orientation::Horizontal, Span::Short)); // A10-A11
        QVERIFY(s->toggleVoltageDisplay("C12"));
        QVERIFY(!s->setColumns(10)); // A11 belegt
        QVERIFY(QString::fromStdString(s->lastError()).contains(QStringLiteral("11–12")));
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
        using P = model::ExperimentFlow::Phase;
        using C = model::ExperimentFlow::Comparison;
        model::ExperimentFlow f;
        QCOMPARE(f.phase(), P::None);
        QVERIFY(!f.reportSuccess());
        f.start(3, 2);
        QCOMPARE(f.phase(), P::Question);
        QVERIFY(!f.nextHint());      // Hilfen erst beim Aufbau
        QVERIFY(!f.reportSuccess()); // Erfolg erst nach der Vermutung
        QVERIFY(f.chooseGuess(1));
        QCOMPARE(f.guess(), 1);
        QCOMPARE(f.phase(), P::Build);
        QVERIFY(f.nextHint() && f.nextHint());
        QVERIFY(!f.solutionShown());
        QVERIFY(f.nextHint());
        QVERIFY(f.solutionShown());
        QVERIFY(!f.nextHint());
        QVERIFY(!f.chooseObservation(0, true, false)); // erst nach dem Versuch
        QVERIFY(f.reportSuccess());
        QCOMPARE(f.phase(), P::Observe);
        QVERIFY(!f.reportSuccess());
        QVERIFY(!f.chooseObservation(2, true, false)); // nur angebotene Beobachtungen
        QVERIFY(f.chooseObservation(0, false, false));
        QCOMPARE(f.comparison(), C::Differs);
        QCOMPARE(f.phase(), P::Explain);
        // ohne Beobachtungsangebot direkt zur Erklaerung; offene Vermutung
        f.start(1, 0);
        QVERIFY(f.chooseGuess(0));
        QVERIFY(f.reportSuccess());
        QCOMPARE(f.phase(), P::Explain);
        f.start(1, 2);
        QVERIFY(f.chooseGuess(2) && f.reportSuccess());
        QVERIFY(f.chooseObservation(1, false, true));
        QCOMPARE(f.comparison(), C::Open);
        f.stop();
        QCOMPARE(f.phase(), P::None);
    }

    // Forscherheft: Vermutung, Beobachtung, Vergleich und Notizen bleiben erhalten
    void journalStorePersists() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString file = dir.filePath(QStringLiteral("forscherheft.json"));
        {
            services::JournalStore j(file);
            QVERIFY(j.setGuess(QStringLiteral("volta1.1.1"), QStringLiteral("Zwei Leitungen")));
            QVERIFY(j.setObservation(QStringLiteral("volta1.1.1"), QStringLiteral("Es leuchtet"), QStringLiteral("passt")));
            QVERIFY(j.addNote(QStringLiteral("volta1.1.1"), QStringLiteral("  Bei 3 V dunkler  ")));
            QVERIFY(!j.addNote(QStringLiteral("volta1.1.1"), QStringLiteral("   "))); // leere Notiz nicht
        }
        services::JournalStore again(file);
        const QJsonObject e = again.entry(QStringLiteral("volta1.1.1"));
        QCOMPARE(e.value(QStringLiteral("vermutung")).toString(), QStringLiteral("Zwei Leitungen"));
        QCOMPARE(e.value(QStringLiteral("vergleich")).toString(), QStringLiteral("passt"));
        QCOMPARE(e.value(QStringLiteral("notizen")).toArray().size(), 1);
        QCOMPARE(e.value(QStringLiteral("notizen")).toArray().at(0).toString(), QStringLiteral("Bei 3 V dunkler"));
        // neue Vermutung: neuer Durchgang, Beobachtung offen, Notizen bleiben
        QVERIFY(again.setGuess(QStringLiteral("volta1.1.1"), QStringLiteral("Eine Leitung")));
        QVERIFY(!again.entry(QStringLiteral("volta1.1.1")).contains(QStringLiteral("beobachtung")));
        QCOMPARE(again.entries().size(), 1);
        QVERIFY(again.reset());
        QVERIFY(services::JournalStore(file).entries().isEmpty());
    }
};

QTEST_GUILESS_MAIN(SessionTest)
#include "test_session.moc"

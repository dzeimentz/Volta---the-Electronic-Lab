// Werkbank des Zuse-Prototyps: Stufen, Befehle mit Rueckgaengig, Pruefen, Chip erstellen, Blick hinein, Ablage.
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include "workbench_controller.hpp"

using namespace zuse;
using volta::logic::Level;

namespace {
const QString kStages = QStringLiteral(ZUSE_CONTENT_DIR "/stufen.json");

// baut NICHT aus einem NAND und erstellt den Chip
void buildNot(WorkbenchController& w) {
    const QString n = w.addPart("NAND", 400, 250);
    QVERIFY(!n.isEmpty());
    QVERIFY(w.connectPins("A", n + ".a"));
    QVERIFY(w.connectPins("A", n + ".b"));
    QVERIFY(w.connectPins(n + ".y", "Y"));
    w.check();
    QCOMPARE(w.checkState(), QStringLiteral("passed"));
    QVERIFY(w.createChip());
}
} // namespace

class WorkbenchTest : public QObject {
    Q_OBJECT
private slots:
    void stagesLoadAndUnlockInOrder() {
        QTemporaryDir dir;
        WorkbenchController w(kStages, dir.path(), "de");
        QCOMPARE(w.stages().size(), 5);
        QCOMPARE(w.stageKuerzel(), QStringLiteral("NICHT"));
        QVERIFY(!w.selectStage(1)); // UND erst nach NICHT
        QCOMPARE(w.palette().size(), 1);
        QCOMPARE(w.ports().size(), 2);
        QVERIFY(w.stageQuestion().contains("NAND"));
    }

    void buildCheckAndUndo() {
        QTemporaryDir dir;
        WorkbenchController w(kStages, dir.path(), "de");
        const QString n = w.addPart("NAND", 400, 250);
        QCOMPARE(n, QStringLiteral("n1"));
        QVERIFY(w.connectPins("A", "n1.a"));
        QVERIFY(w.connectPins("n1.y", "Y"));
        // b offen: bei A = 0 entscheidet die 0, bei A = 1 bleibt Y unbestimmt
        QCOMPARE(w.levelAt("Y"), Level::One);
        w.toggleInput("A");
        QCOMPARE(w.levelAt("Y"), Level::X);
        w.check();
        QCOMPARE(w.checkState(), QStringLiteral("failed"));
        QVERIFY(!w.chipReady());
        QVERIFY(w.connectPins("A", "n1.b"));
        QCOMPARE(w.checkState(), QStringLiteral("none")); // Aenderung verlangt neue Pruefung
        QCOMPARE(w.levelAt("Y"), Level::Zero);
        w.check();
        QVERIFY(w.chipReady());
        // Rueckgaengig nimmt die letzte Leitung weg, Wiederholen bringt sie zurueck
        QCOMPARE(w.def().connections.size(), size_t(3));
        w.undo();
        QCOMPARE(w.def().connections.size(), size_t(2));
        QVERIFY(!w.chipReady());
        w.redo();
        QCOMPARE(w.def().connections.size(), size_t(3));
        // doppelte Leitung und zwei Ausgaenge auf einer Leitung werden abgelehnt
        QVERIFY(!w.connectPins("n1.a", "A"));
        const QString n2 = w.addPart("NAND", 600, 400);
        QVERIFY(!w.connectPins(n2 + ".y", "n1.y"));
        QVERIFY(!w.message().isEmpty());
        // Baustein entfernen nimmt seine Leitungen mit
        w.removePart("n1");
        QCOMPARE(w.def().connections.size(), size_t(0));
        w.undo();
        QCOMPARE(w.def().connections.size(), size_t(3));
    }

    void chipLadderAndLookInside() {
        QTemporaryDir dir;
        WorkbenchController w(kStages, dir.path(), "de");
        buildNot(w);
        QVERIFY(w.isDone("NICHT"));
        QVERIFY(w.selectStage(1));
        QCOMPARE(w.stageKuerzel(), QStringLiteral("UND"));
        QCOMPARE(w.palette().size(), 2); // NAND und der eigene NICHT-Chip
        const QString n = w.addPart("NAND", 300, 150);
        const QString i = w.addPart("NICHT", 640, 150);
        QCOMPARE(i, QStringLiteral("n2")); // gleiche Anfangsbuchstaben: laufende Nummer
        QVERIFY(w.connectPins("A", n + ".a"));
        QVERIFY(w.connectPins("B", n + ".b"));
        QVERIFY(w.connectPins(n + ".y", i + ".A"));
        QVERIFY(w.connectPins(i + ".Y", "Y"));
        w.toggleInput("A");
        w.toggleInput("B");
        QCOMPARE(w.levelAt("Y"), Level::One);
        // Blick hinein: im NICHT-Chip liegt am Eingang die 0 aus dem NAND
        QVERIFY(!w.lookInside(n)); // Grundgatter haben kein Inneres
        QVERIFY(w.lookInside(i));
        QVERIFY(w.inside());
        QCOMPARE(w.path(), (QStringList{"UND", "NICHT"}));
        QCOMPARE(w.levelAt("A"), Level::Zero);
        QCOMPARE(w.levelAt("Y"), Level::One);
        QCOMPARE(w.parts().size(), 1);
        QVERIFY(w.addPart("NAND", 100, 100).isEmpty()); // innen nur ansehen
        w.leaveTo(0);
        QVERIFY(!w.inside());
        w.check();
        QVERIFY(w.createChip());
    }

    void solutionsPassEveryStage() {
        QTemporaryDir dir;
        WorkbenchController w(kStages, dir.path(), "de");
        for (int s = 0; s < 5; ++s) {
            QVERIFY(w.selectStage(s));
            w.showSolution();
            w.check();
            QVERIFY2(w.checkState() == "passed", qPrintable(w.stageKuerzel()));
            QVERIFY(w.createChip());
        }
        // Halbaddierer ist bis zum NAND aufgeloest und zeigt 1 + 1 = 10
        w.toggleInput("A");
        w.toggleInput("B");
        QCOMPARE(w.levelAt("S"), Level::Zero);
        QCOMPARE(w.levelAt("C"), Level::One);
        QVERIFY(w.lookInside("x1"));
        QVERIFY(w.lookInside("o1"));
        QCOMPARE(w.path(), (QStringList{"HA", "XODER", "ODER"}));
    }

    void chipsAndDraftsArePersisted() {
        QTemporaryDir dir;
        {
            WorkbenchController w(kStages, dir.path(), "de");
            buildNot(w);
            QVERIFY(w.selectStage(1));
            w.addPart("NAND", 300, 150);
        }
        WorkbenchController w(kStages, dir.path(), "en");
        QVERIFY(w.isDone("NICHT"));
        QVERIFY(w.library().find("NICHT"));
        QCOMPARE(w.stageKuerzel(), QStringLiteral("UND")); // erste offene Stufe
        QCOMPARE(w.def().parts.size(), size_t(1));         // Entwurf ist noch da
        QCOMPARE(w.stageTitle(), QStringLiteral("The AND gate"));
    }
};

QTEST_GUILESS_MAIN(WorkbenchTest)
#include "test_workbench.moc"

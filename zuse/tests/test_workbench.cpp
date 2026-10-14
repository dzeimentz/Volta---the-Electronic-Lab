// Werkbank des Zuse-Prototyps: Geschichte, Raetselkisten (entdecken), Werkzeugkiste, Bauen mit Rueckgaengig,
// Pruefen, Erfindung einpacken, Blick hinein, Ablage.
#include <QTemporaryDir>
#include <QtTest>

#include "workbench_controller.hpp"

using namespace zuse;
using volta::logic::Level;

namespace {
const QString kStages = QStringLiteral(ZUSE_CONTENT_DIR "/stufen.json");

// Stufe 0: Schalter direkt an die LEDs (ergibt keine Erfindung)
void finishBit(WorkbenchController& w) {
    QCOMPARE(w.stageKuerzel(), QStringLiteral("BIT"));
    QVERIFY(w.connectPins("A", "X"));
    QVERIFY(w.connectPins("B", "Y"));
    w.check();
    QCOMPARE(w.checkState(), QStringLiteral("passed"));
    QVERIFY(w.createChip());
    QVERIFY(!w.library().find("BIT"));
}

// Entdeckerstufe mit der richtigen Kiste abschliessen
void discover(WorkbenchController& w, int stage) {
    QVERIFY(w.selectStage(stage));
    QCOMPARE(w.stageKind(), QStringLiteral("entdecken"));
    for (const QVariant& b : w.boxes()) {
        const QVariantMap box = b.toMap();
        if (box["symbol"].toString().isEmpty()) continue; // nur die richtige Kiste hat ein Zeichen
        w.selectBox(box["index"].toInt());
    }
    QVERIFY(w.confirmBox());
}

void solveAndPack(WorkbenchController& w, int stage) {
    QVERIFY(w.selectStage(stage));
    w.showSolution();
    w.check();
    QVERIFY2(w.checkState() == "passed", qPrintable(w.stageKuerzel()));
    QVERIFY(w.createChip());
}
} // namespace

class WorkbenchTest : public QObject {
    Q_OBJECT
private slots:
    void stagesStartWithAStory() {
        QTemporaryDir dir;
        WorkbenchController w(kStages, dir.path(), "de");
        QCOMPARE(w.stages().size(), 7);
        QCOMPARE(w.stageKuerzel(), QStringLiteral("BIT"));
        QVERIFY(!w.stageStory().isEmpty());
        QVERIFY(!w.stageMakesChip());
        QVERIFY(!w.selectStage(1)); // Nachtlicht erst nach Stufe 0
        const QVariantList in = w.panelInputs();
        QCOMPARE(in.size(), 2);
        QCOMPARE(in[0].toMap()["label"].toString(), QStringLiteral("Lichtschalter"));
        QCOMPARE(in[1].toMap()["kind"].toString(), QStringLiteral("taster"));
        QCOMPARE(w.panelOutputs()[1].toMap()["label"].toString(), QStringLiteral("Klingel"));
        QCOMPARE(w.ports()[0].toMap()["label"].toString(), QStringLiteral("Lichtschalter"));
        QCOMPARE(w.tools().size(), 0);
    }

    void pushButtonOnlyWhilePressed() {
        QTemporaryDir dir;
        WorkbenchController w(kStages, dir.path(), "de");
        QVERIFY(w.connectPins("B", "Y"));
        w.setInput("B", true);
        QCOMPARE(w.levelAt("Y"), Level::One);
        w.setInput("B", false);
        QCOMPARE(w.levelAt("Y"), Level::Zero);
    }

    void puzzleBoxesTeachTheRule() {
        QTemporaryDir dir;
        WorkbenchController w(kStages, dir.path(), "de");
        finishBit(w);
        QVERIFY(w.selectStage(1));
        QVERIFY(!w.selectStage(2)); // Tresor erst nach dem Nachtlicht
        discover(w, 1);
        QVERIFY(w.selectStage(2));
        QCOMPARE(w.stageKuerzel(), QStringLiteral("UND"));
        QCOMPARE(w.boxes().size(), 3);
        // ohne Kiste ist die LED nicht angeschlossen
        QCOMPARE(w.levelAt("Y"), Level::X);
        QVERIFY(!w.confirmBox());
        // Kiste 1 (ODER) oeffnet schon mit einem Schluessel: falsch, mit konkreter Erklaerung
        w.selectBox(0);
        w.toggleInput("A");
        QCOMPARE(w.levelAt("Y"), Level::One);
        QVERIFY(!w.confirmBox());
        QVERIFY(w.message().contains("Schlüssel 1 aus, Schlüssel 2 an")); // erste Stellung, die nicht passt
        QCOMPARE(w.zettiPose(), QStringLiteral("nachdenklich"));
        // ausprobierte Zeilen erscheinen in der Tabelle, das Soll bleibt verborgen
        const QVariantList rows = w.checkRows();
        QCOMPARE(rows[2].toMap()["actual"].toString(), QStringLiteral("1"));
        QCOMPARE(rows[3].toMap()["actual"].toString(), QString());
        QCOMPARE(rows[2].toMap()["expected"].toString(), QString());
        // Kiste 3 ist UND
        w.selectBox(2);
        QCOMPARE(w.levelAt("Y"), Level::Zero);
        w.toggleInput("B");
        QCOMPARE(w.levelAt("Y"), Level::One);
        QVERIFY(w.confirmBox());
        QCOMPARE(w.zettiPose(), QStringLiteral("begeistert"));
        // die Werkzeugkiste kennt jetzt NICHT und UND
        const QVariantList tools = w.tools();
        QCOMPARE(tools.size(), 2);
        QCOMPARE(tools[1].toMap()["name"].toString(), QStringLiteral("UND"));
        QCOMPARE(tools[1].toMap()["type"].toString(), QStringLiteral("AND"));
        QVERIFY(!tools[1].toMap()["merksatz"].toString().isEmpty());
        QVERIFY(w.boxes()[2].toMap()["revealed"].toBool());
    }

    void buildInventionFromTools() {
        QTemporaryDir dir;
        WorkbenchController w(kStages, dir.path(), "de");
        finishBit(w);
        discover(w, 1);
        discover(w, 2);
        discover(w, 3);
        QVERIFY(w.selectStage(4));
        QCOMPARE(w.stageKuerzel(), QStringLiteral("ALARM"));
        QCOMPARE(w.palette().size(), 3); // UND, ODER, NICHT aus der Werkzeugkiste
        const QString o = w.addPart("OR", 330, 330);
        const QString u = w.addPart("AND", 560, 220);
        QVERIFY(w.connectPins("F", o + ".a"));
        QVERIFY(w.connectPins("T", o + ".b"));
        QVERIFY(w.connectPins("A", u + ".a"));
        QVERIFY(w.connectPins(o + ".y", u + ".b"));
        QVERIFY(w.connectPins(u + ".y", "S"));
        w.check();
        QCOMPARE(w.checkState(), QStringLiteral("passed"));
        // letzte Leitung rueckgaengig: Pruefung verfaellt und schlaegt fehl
        w.undo();
        QCOMPARE(w.checkState(), QStringLiteral("none"));
        w.check();
        QCOMPARE(w.checkState(), QStringLiteral("failed"));
        QCOMPARE(w.zettiPose(), QStringLiteral("gruebelnd"));
        w.redo();
        w.check();
        QVERIFY(w.chipReady());
        QVERIFY(w.createChip());
        QVERIFY(w.message().contains("Alarmanlage"));
        QCOMPARE(w.tools().size(), 4);
        QVERIFY(w.tools()[3].toMap()["invention"].toBool());
        // Sirene nur, wenn die Anlage an ist und ein Fenster offen
        w.toggleInput("F");
        QCOMPARE(w.levelAt("S"), Level::Zero);
        w.toggleInput("A");
        QCOMPARE(w.levelAt("S"), Level::One);
        // zwei Ausgaenge auf einer Leitung werden abgelehnt
        const QString o2 = w.addPart("OR", 330, 100);
        QVERIFY(!w.connectPins(o2 + ".y", o + ".y"));
    }

    void solutionsPassAndLookInside() {
        QTemporaryDir dir;
        WorkbenchController w(kStages, dir.path(), "de");
        finishBit(w);
        discover(w, 1);
        discover(w, 2);
        discover(w, 3);
        solveAndPack(w, 4);
        solveAndPack(w, 5);
        solveAndPack(w, 6);
        // Halbaddierer mit der selbst gebauten Treppenlicht-Schaltung: 1 + 1 = 10
        w.toggleInput("A");
        w.toggleInput("B");
        QCOMPARE(w.levelAt("S"), Level::Zero);
        QCOMPARE(w.levelAt("C"), Level::One);
        QVERIFY(w.lookInside("x1"));
        QCOMPARE(w.path(), (QStringList{"HA", "XODER"}));
        QCOMPARE(w.parts().size(), 4);
        QVERIFY(w.addPart("AND", 100, 100).isEmpty()); // innen nur ansehen
        w.leaveTo(0);
        QVERIFY(!w.inside());
    }

    void numberSystems() {
        QTemporaryDir dir;
        WorkbenchController w(kStages, dir.path(), "de");
        QVariantMap n = w.inputNumber();
        QCOMPARE(n["bin"].toString(), QStringLiteral("00"));
        w.toggleInput("A"); // erster Schalter = hoechstes Bit
        n = w.inputNumber();
        QCOMPARE(n["bin"].toString(), QStringLiteral("10"));
        QCOMPARE(n["dec"].toString(), QStringLiteral("2"));
        QCOMPARE(w.outputNumber()["dec"].toString(), QStringLiteral("?")); // LEDs noch nicht verbunden
        const QVariantMap big = WorkbenchController::numberOf({Level::One, Level::One, Level::One, Level::One});
        QCOMPARE(big["dec"].toString(), QStringLiteral("15"));
        QCOMPARE(big["hex"].toString(), QStringLiteral("F"));
        QVERIFY(w.numbersVisible());
        w.setNumbersVisible(false);
        WorkbenchController again(kStages, dir.path(), "de");
        QVERIFY(!again.numbersVisible());
    }

    void progressIsPersisted() {
        QTemporaryDir dir;
        {
            WorkbenchController w(kStages, dir.path(), "de");
            finishBit(w);
            discover(w, 1);
            QVERIFY(w.selectStage(2));
            w.setSymbolStyle("us");
        }
        WorkbenchController w(kStages, dir.path(), "en");
        QCOMPARE(w.stageKuerzel(), QStringLiteral("UND")); // erste offene Stufe
        QCOMPARE(w.stageTitle(), QStringLiteral("The safe"));
        QCOMPARE(w.tools().size(), 1);
        QCOMPARE(w.symbolStyle(), QStringLiteral("us"));
    }
};

QTEST_GUILESS_MAIN(WorkbenchTest)
#include "test_workbench.moc"

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
    w.skipGuess();
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
// Zahlenstufe: jede Zielzahl an den Schaltern einstellen
void countThrough(WorkbenchController& w, int stage) {
    QVERIFY(w.selectStage(stage));
    QCOMPARE(w.stageKind(), QStringLiteral("zahlen"));
    if (w.stageSequence()) { // Rechenmaschine: Schritte am Bedienfeld
        w.playSteps();
        QVERIFY(w.stageDone());
        return;
    }
    const QStringList in = w.stageInputs();
    for (const QVariant& t : w.tasks())
        for (qsizetype i = 0; i < in.size(); ++i)
            w.setInput(in[i], (t.toMap().value("value").toInt() >> (in.size() - 1 - i)) & 1);
    QVERIFY(w.stageDone());
}
} // namespace

class WorkbenchTest : public QObject {
    Q_OBJECT
private slots:
    void stagesStartWithAStory() {
        QTemporaryDir dir;
        WorkbenchController w(kStages, dir.path(), "de");
        QCOMPARE(w.stages().size(), 23);
        QCOMPARE(w.stageKuerzel(), QStringLiteral("BIT"));
        QVERIFY(!w.stageStory().isEmpty());
        QVERIFY(!w.stageMakesChip());
        QVERIFY(!w.selectStage(1)); // Nachtlicht erst nach Stufe 0
        // Lernstufen wie bei Volta: Auftakt, Logik entdecken, Erfinden, Zahlen und Rechnen, Speichern und Zaehlen,
        // Rechenmaschine (mit Ausblick auf Zuse 2)
        const QVariantList levels = w.levels();
        QCOMPARE(levels.size(), 6);
        QCOMPARE(levels[0].toMap()["experiments"].toList().size(), 1);
        QCOMPARE(levels[1].toMap()["experiments"].toList().size(), 4);
        QVERIFY(levels[1].toMap()["unlocked"].toBool() == false);
        QCOMPARE(levels[4].toMap()["experiments"].toList().size(), 8);
        QCOMPARE(levels[5].toMap()["experiments"].toList().size(), 3);
        QVERIFY(levels[5].toMap()["outlook"].toString().contains("Zuse 2"));
        QCOMPARE(w.stages()[4].toMap()["kuerzel"].toString(), QStringLiteral("NAND"));
        QCOMPARE(w.stages()[4].toMap()["level"].toInt(), 1);
        QCOMPARE(w.stages()[11].toMap()["level"].toInt(), 3);
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
        // erst vermuten, dann ausprobieren: ohne Vermutung laesst sich keine Kiste einsetzen
        QVERIFY(w.guessing());
        w.selectBox(0);
        QCOMPARE(w.boxIndex(), -1);
        for (int r = 0; r < 4; ++r) w.toggleGuess(r); // vermutet: immer offen (1)
        w.toggleGuess(0);                             // ... ausser ohne Schluessel (0)
        QVERIFY(!w.guessing());
        QCOMPARE(w.checkRows()[0].toMap()["guess"].toString(), QStringLiteral("0"));
        // Kiste 1 (ODER) oeffnet schon mit einem Schluessel: falsch, mit konkreter Erklaerung
        w.selectBox(0);
        w.toggleInput("A");
        QCOMPARE(w.levelAt("Y"), Level::One);
        QVERIFY(!w.confirmBox());
        // erste Situation, die nicht passt, in Alltagssprache
        QVERIFY(w.message().contains("Schlüssel 1 nicht gedreht, Schlüssel 2 gedreht"));
        QVERIFY(w.message().contains("»Tresor geht auf«"));
        QCOMPARE(w.zettiPose(), QStringLiteral("nachdenklich"));
        // ausprobierte Zeilen erscheinen in der Tabelle, das Soll bleibt verborgen
        const QVariantList rows = w.checkRows();
        QCOMPARE(rows[2].toMap()["actual"].toString(), QStringLiteral("1"));
        QCOMPARE(rows[3].toMap()["actual"].toString(), QString());
        QCOMPARE(rows[2].toMap()["expected"].toString(), QString());
        QCOMPARE(rows[2].toMap()["match"].toString(), QStringLiteral("=")); // wie vermutet
        QCOMPARE(rows[3].toMap()["match"].toString(), QString());          // noch nicht ausprobiert
        // Kiste 3 ist UND
        w.selectBox(2);
        QCOMPARE(w.levelAt("Y"), Level::Zero);
        w.toggleInput("B");
        QCOMPARE(w.levelAt("Y"), Level::One);
        QVERIFY(w.confirmBox());
        QCOMPARE(w.zettiPose(), QStringLiteral("begeistert"));
        QVERIFY(w.message().contains("2 Situationen kam es anders")); // Vermutung 0111, UND ist 0001
        QVERIFY(!w.guessing());
        // die Werkzeugkiste kennt jetzt NICHT und UND
        const QVariantList tools = w.tools();
        QCOMPARE(tools.size(), 2);
        QCOMPARE(tools[1].toMap()["name"].toString(), QStringLiteral("UND"));
        QCOMPARE(tools[1].toMap()["type"].toString(), QStringLiteral("AND"));
        QVERIFY(!tools[1].toMap()["merksatz"].toString().isEmpty());
        QVERIFY(w.boxes()[2].toMap()["revealed"].toBool());
    }

    // Forscherheft: Zetti schreibt Vermutung und Ergebnis, das Kind ergaenzt Notizen; gespeichert im Datenordner
    void journal() {
        QTemporaryDir dir;
        {
            WorkbenchController w(kStages, dir.path(), "de");
            QVERIFY(w.journal().isEmpty());
            finishBit(w);
            QVERIFY(w.selectStage(1)); // Nachtlicht: Vermutung »dunkel -> leuchtet, hell -> leuchtet«
            w.setGuess(0, true);
            QCOMPARE(w.journal().size(), 1); // erst, wenn jede Situation vermutet ist
            w.setGuess(1, true);
            QVariantMap e = w.journal()[0].toMap();
            QCOMPARE(e["id"].toString(), QStringLiteral("NICHT"));
            QCOMPARE(e["guess"].toString(),
                     QStringLiteral("Im Zimmer ist es dunkel → Nachtlicht leuchtet\nIm Zimmer ist es hell → Nachtlicht leuchtet"));
            w.selectBox(1);
            QVERIFY(w.confirmBox());
            e = w.journal()[0].toMap();
            QCOMPARE(e["comparison"].toInt(), 2); // anders als vermutet
            QVERIFY(e["observation"].toString().contains("Die passende Kiste heißt NICHT."));
            QVERIFY(e["observation"].toString().contains("Im Zimmer ist es hell → Nachtlicht aus"));
            QVERIFY(w.addJournalNote("NICHT", "  Das Licht geht aus, wenn es hell ist.  "));
            QVERIFY(!w.addJournalNote("NICHT", "   ")); // leere Notiz nicht
        }
        WorkbenchController w(kStages, dir.path(), "de"); // neu geladen
        const QVariantList pages = w.journal();
        QCOMPARE(pages.size(), 2); // Licht an! und Nachtlicht
        const QVariantMap night = pages[0].toMap();
        QCOMPARE(night["id"].toString(), QStringLiteral("NICHT"));
        QCOMPARE(night["notes"].toStringList(), QStringList{"Das Licht geht aus, wenn es hell ist."});
        QVERIFY(pages[1].toMap()["observation"].toString().startsWith("»Licht an!« gebaut und geprüft."));
        // Vorfuehrung schreibt nichts
        QVERIFY(w.startDemo(w.demos()[0].toMap()["index"].toInt()));
        QVERIFY(!w.addJournalNote("NICHT", "x"));
        w.leaveSpecial();
        w.resetJournal();
        QVERIFY(w.journal().isEmpty());
    }

    // Franzoesisch: Texte, Zustaende und Werkzeugnamen der Stufen in der eingestellten Sprache
    void frenchContent() {
        QTemporaryDir dir;
        WorkbenchController w(kStages, dir.path(), "fr");
        finishBit(w);
        discover(w, 1);
        discover(w, 2);
        QCOMPARE(w.tools()[1].toMap()["name"].toString(), QStringLiteral("ET"));
        QCOMPARE(w.tools()[0].toMap()["name"].toString(), QStringLiteral("NON"));
        w.selectStage(2);
        QCOMPARE(w.stageTitle(), QStringLiteral("Le coffre-fort"));
        const QVariantMap row = w.checkRows()[3].toMap();
        QCOMPARE(row["situation"].toList()[0].toMap()["text"].toString(), QStringLiteral("Clé 1 tournée"));
        w.setLanguage("en");
        QCOMPARE(w.tools()[1].toMap()["name"].toString(), QStringLiteral("AND"));
    }

    // Entdecken mit Situationskarten: Alltagssprache statt 0 und 1, Vermutung direkt, Situation per Antippen
    void situationCards() {
        QTemporaryDir dir;
        WorkbenchController w(kStages, dir.path(), "de");
        finishBit(w);
        discover(w, 1);
        discover(w, 2);
        discover(w, 3);
        QVERIFY(w.selectStage(4));
        QCOMPARE(w.stageKuerzel(), QStringLiteral("NAND"));
        const QVariantMap row = w.checkRows()[1].toMap(); // Fenster 1 offen, Fenster 2 zu
        const QVariantList situation = row["situation"].toList();
        QCOMPARE(situation.size(), 2);
        QCOMPARE(situation[0].toMap()["text"].toString(), QStringLiteral("Fenster 1 offen"));
        QCOMPARE(situation[1].toMap()["text"].toString(), QStringLiteral("Fenster 2 zu"));
        QVERIFY(situation[1].toMap()["on"].toBool());
        QCOMPARE(row["outOn"].toString(), QStringLiteral("Warnlampe leuchtet"));
        QCOMPARE(row["outOff"].toString(), QStringLiteral("Warnlampe aus"));
        QCOMPARE(w.panelInputs()[0].toMap()["state"].toString(), QStringLiteral("Fenster 1 offen"));
        QCOMPARE(w.panelOutputs()[0].toMap()["state"].toString(), QString()); // ohne Kiste unbestimmt

        for (int r = 0; r < 4; ++r) w.setGuess(r, r != 3);
        QVERIFY(!w.guessing());
        QCOMPARE(w.checkRows()[3].toMap()["guess"].toString(), QStringLiteral("0"));
        w.selectBox(1);
        w.trySituation(2); // Fenster 1 zu, Fenster 2 offen: die Schalter stellen sich ein
        QCOMPARE(w.levelAt("A"), Level::One);
        QCOMPARE(w.levelAt("B"), Level::Zero);
        QCOMPARE(w.levelAt("Y"), Level::One);
        QCOMPARE(w.panelOutputs()[0].toMap()["state"].toString(), QStringLiteral("Warnlampe leuchtet"));
        const QVariantMap tried = w.checkRows()[2].toMap();
        QVERIFY(tried["current"].toBool());
        QCOMPARE(tried["actual"].toString(), QStringLiteral("1"));
        QCOMPARE(tried["match"].toString(), QStringLiteral("="));
        QVERIFY(w.confirmBox());
        QVERIFY(w.message().contains("Deine Vermutung hat gestimmt"));
    }

    // Bauen mit Situationskarten: Soll und Ist je Lampe in Worten; Ablauf- und Probenstufen bleiben Tabellen
    void buildInWords() {
        QTemporaryDir dir;
        WorkbenchController w(kStages, dir.path(), "de");
        QVERIFY(w.stageInWords()); // Bit: Lichtschalter und Klingel
        const QVariantList ok = w.demos();
        int alarm = -1, adder = -1, memory = -1;
        for (const QVariant& d : ok) {
            const QString label = d.toMap()["label"].toString();
            if (label == "2.1") alarm = d.toMap()["index"].toInt();
            if (label == "3.4") adder = d.toMap()["index"].toInt();
            if (label == "4.1") memory = d.toMap()["index"].toInt();
        }
        QVERIFY(w.startDemo(alarm));
        QCOMPARE(w.stageKuerzel(), QStringLiteral("ALARM"));
        QVERIFY(w.stageInWords());
        const QVariantMap row = w.checkRows()[5].toMap(); // Anlage an, Fenster zu, Tuer offen
        QStringList texts;
        for (const QVariant& s : row["situation"].toList()) texts << s.toMap()["text"].toString();
        QCOMPARE(texts, QStringList({"Anlage an", "Fenster zu", "Tür offen"}));
        QCOMPARE(row["outStates"].toList()[0].toStringList(), QStringList({"Sirene still", "Sirene heult"}));
        QCOMPARE(row["expected"].toString(), QStringLiteral("1"));
        QCOMPARE(row["actual"].toString(), QStringLiteral("1")); // Vorfuehrung ist geprueft
        w.trySituation(5);
        QVERIFY(w.checkRows()[5].toMap()["current"].toBool());
        QCOMPARE(w.levelAt("S"), Level::One);
        bool found = false;
        for (const QVariant& p : w.ports())
            if (p.toMap()["name"] == "S") found = p.toMap()["state"] == "Sirene heult";
        QVERIFY(found);
        w.leaveSpecial();
        QVERIFY(w.startDemo(adder));
        QVERIFY(!w.stageInWords()); // 4-Bit-Addierer: Rechnungen statt Situationen
        w.leaveSpecial();
        QVERIFY(w.startDemo(memory));
        QVERIFY(!w.stageInWords()); // Speicher: Ablauf in Schritten
        w.trySituation(1);          // im Ablauf ohne Wirkung
        w.leaveSpecial();
    }

    void buildInventionFromTools() {
        QTemporaryDir dir;
        WorkbenchController w(kStages, dir.path(), "de");
        finishBit(w);
        discover(w, 1);
        discover(w, 2);
        discover(w, 3);
        discover(w, 4); // NAND gehoert zur Stufe »Logik entdecken«
        QVERIFY(w.selectStage(5));
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
        QCOMPARE(w.tools().size(), 5); // NICHT, UND, ODER, NAND und die Erfindung
        QVERIFY(w.tools()[4].toMap()["invention"].toBool());
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
        discover(w, 4);
        solveAndPack(w, 5);
        solveAndPack(w, 6);
        solveAndPack(w, 7);
        countThrough(w, 8);
        solveAndPack(w, 9);
        QCOMPARE(w.stageKuerzel(), QStringLiteral("HA"));
        // Halbaddierer mit der selbst gebauten Treppenlicht-Schaltung: 1 + 1 = 10
        w.toggleInput("A");
        w.toggleInput("B");
        QCOMPARE(w.levelAt("S"), Level::Zero);
        QCOMPARE(w.levelAt("C"), Level::One);
        QVERIFY(w.lookInside("x1"));
        QCOMPARE(w.path(), (QStringList{"Halbaddierer", "Treppenlicht-Schaltung"})); // Namen statt Kuerzel
        QCOMPARE(w.parts().size(), 4);
        QVERIFY(w.addPart("AND", 100, 100).isEmpty()); // innen nur ansehen
        w.leaveTo(0);
        QVERIFY(!w.inside());
    }

    // NAND, alles aus NAND, Zahlen am Bedienfeld, Volladdierer, 4-Bit-Addierer mit Ueberlauf
    void countingNandAndAdders() {
        QTemporaryDir dir;
        WorkbenchController w(kStages, dir.path(), "de");
        finishBit(w);
        for (int s : {1, 2, 3}) discover(w, s);
        // 4: NAND entdecken
        discover(w, 4);
        QCOMPARE(w.stageKuerzel(), QStringLiteral("NAND"));
        for (int s : {5, 6}) solveAndPack(w, s);
        // 7: UND nur aus NAND (keine Erfindung)
        QVERIFY(w.selectStage(7));
        QCOMPARE(w.palette().size(), 1);
        QVERIFY(!w.addPart("NAND", 300, 200).isEmpty());
        QVERIFY(w.addPart("AND", 300, 400).isEmpty()); // nur NAND erlaubt
        w.clearBench();
        solveAndPack(w, 7);
        QVERIFY(!w.library().find("NANDUND"));

        // 8: fest verdrahtetes Bedienfeld, Zahlen nacheinander einstellen
        QVERIFY(w.selectStage(8));
        QCOMPARE(w.stageKind(), QStringLiteral("zahlen"));
        QVERIFY(w.benchLocked());
        QVERIFY(!w.connectPins("D0", "L1")); // Verdrahtung gesperrt
        QVERIFY(w.addPart("AND", 300, 300).isEmpty());
        QCOMPARE(w.tasks().size(), 5);
        QVERIFY(w.ports()[0].toMap()["compact"].toBool()); // vier Schalter: flache Form
        auto show = [&](int value) {
            for (int bit = 0; bit < 4; ++bit) {
                const QString name = QStringLiteral("D%1").arg(bit);
                const bool want = (value >> bit) & 1;
                if ((w.levelAt(name) == Level::One) != want) w.toggleInput(name);
            }
        };
        show(3); // falsche Zahl: nichts passiert
        QVERIFY(w.tasks()[0].toMap()["current"].toBool());
        show(2);
        QVERIFY(w.tasks()[0].toMap()["done"].toBool());
        QVERIFY(w.message().contains("0010"));
        for (int v : {5, 8, 11}) show(v);
        QVERIFY(!w.stageDone());
        show(15);
        QVERIFY(w.stageDone());

        // 9: Halbaddierer
        solveAndPack(w, 9);
        // 10: Volladdierer aus zwei Halbaddierern
        solveAndPack(w, 10);
        QVERIFY(w.library().find("VA"));
        // 11: 4-Bit-Addierer; gezeigt werden nur die Proben, geprueft werden alle 256 Zeilen
        QVERIFY(w.selectStage(11));
        const QVariantList numbers = w.inputNumbers();
        QCOMPARE(numbers.size(), 2);
        QCOMPARE(numbers[1].toMap()["title"].toString(), QStringLiteral("B"));
        QCOMPARE(w.checkRows().size(), 8);
        QCOMPARE(w.checkRows()[2].toMap()["note"].toString(), QStringLiteral("3 + 5 = 8"));
        QCOMPARE(w.checkRows()[2].toMap()["expected"].toString(), QStringLiteral("01000"));
        solveAndPack(w, 11);
        // 9 + 9 = 18: passt nicht in vier Bit, der Uebertrag leuchtet
        for (const QString& in : {"A3", "A0", "B3", "B0"}) w.toggleInput(in);
        QCOMPARE(w.inputNumbers()[0].toMap()["dec"].toString(), QStringLiteral("9"));
        QCOMPARE(w.outputNumber()["dec"].toString(), QStringLiteral("18"));
        QCOMPARE(w.levelAt("C"), Level::One);
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

    // Lernstufe 4: Merker, Speicherbit, Taktbit, Register, Lauflicht, Umschalter, Zaehler, Taktgeber – als Ablauf;
    // Lernstufe 5: Vergleicher, Plus und Minus, »Du bist das Steuerwerk«
    void memoryAndCalculatingMachine() {
        QTemporaryDir dir;
        WorkbenchController w(kStages, dir.path(), "de");
        finishBit(w);
        for (int s : {1, 2, 3, 4}) discover(w, s);
        for (int s : {5, 6, 7}) solveAndPack(w, s);
        countThrough(w, 8);
        for (int s : {9, 10, 11}) solveAndPack(w, s);

        QVERIFY(w.selectStage(12));
        QCOMPARE(w.stageKuerzel(), QStringLiteral("MERKER"));
        QVERIFY(w.stageSequence());
        QCOMPARE(w.checkRows().size(), 8);
        QVERIFY(w.checkRows()[1].toMap()["note"].toString().startsWith("2. "));
        w.check(); // leere Werkbank: Ablauf stimmt nicht
        QCOMPARE(w.checkState(), QStringLiteral("failed"));
        solveAndPack(w, 12);
        // gemerkt: nach dem Loslassen bleibt die Lampe an
        QVERIFY(w.selectStage(12));
        w.showSolution();
        QCOMPARE(w.levelAt("Q"), Level::Zero); // eingeschaltet: Speicher leer
        w.setInput("S", true);
        w.setInput("S", false);
        QCOMPARE(w.levelAt("Q"), Level::One);
        w.setInput("R", true);
        QCOMPARE(w.levelAt("Q"), Level::Zero);
        solveAndPack(w, 13);

        // Taktbit: uebernimmt nur im Moment des Takts, Loeschen gewinnt
        QVERIFY(w.selectStage(14));
        QCOMPARE(w.stageKuerzel(), QStringLiteral("TBIT"));
        w.showSolution();
        w.toggleInput("D");
        w.setInput("T", true);
        QCOMPARE(w.levelAt("Q"), Level::One);
        w.toggleInput("D"); // Schalter aus, Takt noch gedrueckt
        QCOMPARE(w.levelAt("Q"), Level::One);
        w.setInput("T", false);
        w.setInput("L", true);
        QCOMPARE(w.levelAt("Q"), Level::Zero);
        w.setInput("L", false);
        solveAndPack(w, 14);
        for (int s : {15, 16, 17}) solveAndPack(w, s);
        // Umschalter: wechselt beim Druecken
        QVERIFY(w.selectStage(17));
        QCOMPARE(w.stageKuerzel(), QStringLiteral("TFF"));
        w.showSolution();
        w.setInput("T", true);
        QCOMPARE(w.levelAt("Q"), Level::One);
        w.setInput("T", false);
        w.setInput("T", true);
        QCOMPARE(w.levelAt("Q"), Level::Zero);
        // Zaehler: 17 Takte, Loeschen und ein Takt danach (Loslassen wird mitgeprueft, aber nicht gezeigt)
        QVERIFY(w.selectStage(18));
        QCOMPARE(w.checkRows().size(), 19);
        solveAndPack(w, 18);
        // Taktgeber: tickt von allein, der Teiler halbiert
        QVERIFY(w.selectStage(19));
        QCOMPARE(w.stageKuerzel(), QStringLiteral("TAKT"));
        QVERIFY(w.hasClock());
        QVERIFY(w.clockRunning());
        QCOMPARE(w.panelInputs()[0].toMap()["kind"].toString(), QStringLiteral("takt"));
        w.setInput("C", true); // von Hand nicht schaltbar
        QCOMPARE(w.panelInputs()[0].toMap()["level"].toString(), QStringLiteral("0"));
        w.showSolution();
        for (int k = 0; k < 4; ++k) w.tickClock(); // zwei volle Takte
        QCOMPARE(w.levelAt("C2"), Level::Zero);
        QCOMPARE(w.levelAt("C4"), Level::One);
        w.toggleClock();
        QVERIFY(!w.clockRunning());
        solveAndPack(w, 19);
        QVERIFY(w.levels()[4].toMap()["done"].toBool());

        // Vergleicher und Rechenwerk: grosse Tabellen nur mit Proben, geprueft wird alles
        QVERIFY(w.selectStage(20));
        QCOMPARE(w.stageKuerzel(), QStringLiteral("GLEICH"));
        QVERIFY(!w.clockRunning());
        solveAndPack(w, 20);
        solveAndPack(w, 21);
        QVERIFY(w.selectStage(21));
        w.showSolution();
        for (const QString& n : {"A3", "A0", "B2", "M"}) w.toggleInput(n); // 9 - 4
        QCOMPARE(w.levelAt("C"), Level::One);
        QCOMPARE(w.outputNumber()["value"].toInt(), 16 + 5);

        // Steuerwerk: fest verdrahtet aus eigenem Rechenwerk und Register, das Kind gibt die Schritte
        QVERIFY(w.selectStage(22));
        QCOMPARE(w.stageKuerzel(), QStringLiteral("STEUER"));
        QVERIFY(w.benchLocked());
        QCOMPARE(w.tasks().size(), 4);
        QVERIFY(w.tasks()[1].toMap()["text"].toString().contains("3 + 5"));
        QCOMPARE(w.outputNumber()["value"].toInt(), 0); // Akku startet leer
        for (const QString& n : {"D1", "D0"}) w.toggleInput(n);
        w.setInput("T", true);
        w.setInput("T", false);
        QCOMPARE(w.outputNumber()["value"].toInt(), 3);
        QVERIFY(w.tasks()[1].toMap()["current"].toBool());
        w.setInput("L", true); // Loeschen: wieder 0
        w.setInput("L", false);
        QCOMPARE(w.outputNumber()["value"].toInt(), 0);
        countThrough(w, 22);
        QVERIFY(w.message().contains("13"));
        QVERIFY(w.levels()[5].toMap()["done"].toBool());
    }

    // Feldgroesse wie Volta: groesseres Feld = mehr Raster; kleiner geht nur, wenn der Aufbau hineinpasst
    void boardSize() {
        QTemporaryDir dir;
        {
            WorkbenchController w(kStages, dir.path(), "de");
            QCOMPARE(w.boardSize(), 0);
            QVERIFY(!w.canShrinkBoard());
            w.setBoardSize(2);
            QCOMPARE(w.frameWidth(), 896.0 + 16 * 28);
            QCOMPARE(w.logicRight(), w.frameWidth() - w.logicLeft());
            QVERIFY(!w.canGrowBoard());
            QVERIFY(w.connectPins("A", "X")); // Schalter A am rechten Rand des groesseren Felds
            w.setBoardSize(1);
        }
        WorkbenchController again(kStages, dir.path(), "de");
        QCOMPARE(again.boardSize(), 1); // gespeichert
    }

    // aeltere Ablage: Erfindung mit anderen Anschluessen (z. B. Register ohne Loeschen) wird verworfen
    void staleChipsAreDropped() {
        QTemporaryDir dir;
        {
            QFile f(dir.filePath("werkbank.json"));
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write(R"({"schema_version": 1, "geschafft": ["BIT", "REG4"], "chips": [{"kuerzel": "REG4",
                "anschluesse": [{"name": "D0", "richtung": "ein"}, {"name": "T", "richtung": "ein"},
                                {"name": "Q0", "richtung": "aus"}], "bausteine": [], "verbindungen": []}]})");
        }
        WorkbenchController w(kStages, dir.path(), "de");
        QVERIFY(w.isDone("BIT"));
        QVERIFY(!w.isDone("REG4"));
        QVERIFY(!w.library().find("REG4"));
    }

    // wie Volta: Freies Labor (alle eigenen Werkzeuge, nicht im Lernpfad) und Vorfuehrung (nichts wird gespeichert)
    // Schaltplan aller Musterloesungen: nur rechte Winkel, alles im Plan, keine Leitung quer durch einen Baustein
    void schematicOfAllSolutions() {
        QTemporaryDir dir;
        WorkbenchController w(kStages, dir.path(), "de");
        for (const QVariant& d : w.demos()) {
            QVERIFY(w.startDemo(d.toMap()["index"].toInt()));
            const QVariantMap plan = w.schematic();
            const qreal width = plan["width"].toReal(), height = plan["height"].toReal();
            QList<QRectF> boxes;
            int parts = 0;
            for (const QVariant& e : plan["elements"].toList()) {
                const QVariantMap m = e.toMap();
                if (m["kind"] != "part") continue;
                ++parts;
                boxes << QRectF(m["x"].toReal(), m["y"].toReal(), m["w"].toReal(), m["h"].toReal());
            }
            QCOMPARE(parts, int(w.def().parts.size()));
            for (const QVariant& l : plan["lines"].toList()) {
                const QVariantList pts = l.toMap()["points"].toList();
                for (int i = 0; i + 3 < pts.size(); i += 2) {
                    const QPointF a(pts[i].toReal(), pts[i + 1].toReal()), b(pts[i + 2].toReal(), pts[i + 3].toReal());
                    QVERIFY2(qFuzzyCompare(a.x() + 1, b.x() + 1) || qFuzzyCompare(a.y() + 1, b.y() + 1),
                             qPrintable(d.toMap()["label"].toString()));
                    for (const QPointF& p : {a, b})
                        QVERIFY(p.x() >= 0 && p.x() <= width && p.y() >= 0 && p.y() <= height);
                    const QRectF seg = QRectF(a, b).normalized().adjusted(-0.1, -0.1, 0.1, 0.1);
                    for (const QRectF& box : boxes)
                        QVERIFY2(!seg.intersects(box.adjusted(2, 2, -2, -2)),
                                 qPrintable(d.toMap()["label"].toString() + ": Leitung durch einen Baustein"));
                }
            }
            QVERIFY(w.faultFindings().isEmpty()); // Musterloesungen sind vollstaendig verdrahtet
            int count = 0;
            for (const QVariant& p : w.partsList()) count += p.toMap()["count"].toInt();
            QCOMPARE(count, int(w.def().parts.size()));
            w.leaveSpecial();
        }
    }

    // Fehlersuche: offener Eingang, Ausgang ohne Ziel, Lampe ohne Leitung
    void faultFinder() {
        QTemporaryDir dir;
        WorkbenchController w(kStages, dir.path(), "de");
        finishBit(w);
        discover(w, 1);
        w.startFreeLab();
        const QString id = w.addPart("NOT", 300, 300);
        QVERIFY(!id.isEmpty());
        QStringList kinds;
        for (const QVariant& f : w.faultFindings()) kinds << f.toMap()["kind"].toString();
        QVERIFY(kinds.contains("offen"));
        QVERIFY(kinds.contains("unbenutzt"));
        QCOMPARE(w.partsList().size(), 1);
        QCOMPARE(w.partsList()[0].toMap()["count"].toInt(), 1);
    }

    void freeLabAndDemo() {
        QTemporaryDir dir;
        {
            WorkbenchController w(kStages, dir.path(), "de");
            QCOMPARE(w.mode(), QStringLiteral("lernpfad"));
            for (const QVariant& s : w.stages()) QVERIFY(s.toMap()["kuerzel"].toString() != "FREI");
            w.startFreeLab();
            QCOMPARE(w.mode(), QStringLiteral("frei"));
            QCOMPARE(w.palette().size(), 0); // noch nichts entdeckt
            QVERIFY(w.addPart("AND", 300, 300).isEmpty()); // UND ist noch nicht entdeckt
            w.leaveSpecial();
            QCOMPARE(w.mode(), QStringLiteral("lernpfad"));
            QCOMPARE(w.stageKuerzel(), QStringLiteral("BIT"));

            // Vorfuehrung: Zaehler laeuft mit allen Musterloesungen, obwohl noch nichts gebaut ist
            const QVariantList demos = w.demos();
            QVERIFY(demos.size() >= 10);
            int counter = -1;
            for (const QVariant& d : demos)
                if (d.toMap()["label"].toString() == "4.7") counter = d.toMap()["index"].toInt();
            QVERIFY(w.startDemo(counter));
            QCOMPARE(w.mode(), QStringLiteral("vorfuehrung"));
            QVERIFY(w.benchLocked());
            QCOMPARE(w.checkState(), QStringLiteral("passed"));
            QVERIFY(!w.chipReady()); // nichts einpacken
            w.setInput("T", true);
            QCOMPARE(w.levelAt("Q0"), Level::One);
            QVERIFY(w.addPart("NOT", 300, 300).isEmpty()); // Aufbau gesperrt
            w.leaveSpecial();
            QVERIFY(!w.library().find("ZAEHLER")); // eigene Werkzeugkiste wieder da
            QCOMPARE(w.resume()["label"].toString(), QStringLiteral("0.1"));
        }
        WorkbenchController again(kStages, dir.path(), "de");
        QVERIFY(!again.isDone("ZAEHLER"));
        QVERIFY(!again.library().find("ZAEHLER"));
    }
};

QTEST_GUILESS_MAIN(WorkbenchTest)
#include "test_workbench.moc"

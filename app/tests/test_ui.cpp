// Oberflaechentests (Qt Quick, ohne sichtbares Fenster): echte Bedienung mit simulierter Maus und Tastatur
// gegen die Experimentansicht. Prueft das Zusammenspiel QML <-> LabController <-> LabSession.
#include <QQmlApplicationEngine>
#include <QTemporaryDir>
#include <algorithm>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWindow>
#include <QtQml/qqmlextensionplugin.h>
#include <QtTest>

#include "lab_controller.hpp"
#include "app_settings.hpp"
#include "language_service.hpp"
#include "services/progress_store.hpp"

Q_IMPORT_QML_PLUGIN(VoltaPlugin)

using namespace volta;

class UiTest : public QObject {
    Q_OBJECT

private:
    std::unique_ptr<LabController> m_lab;
    std::unique_ptr<QQmlApplicationEngine> m_engine;
    std::unique_ptr<LanguageService> m_language;
    std::unique_ptr<AppSettings> m_settings;
    QQuickWindow* m_window = nullptr;

    // Repeater-Elemente haengen im visuellen Baum (childItems), nicht zwingend im QObject-Baum
    static QQuickItem* findItem(QQuickItem* root, const QString& name) {
        if (root->objectName() == name) return root;
        for (QQuickItem* child : root->childItems())
            if (QQuickItem* found = findItem(child, name)) return found;
        return nullptr;
    }

    // Erstes sichtbares Element mit diesem Namen (Startseite und Labor teilen sich z. B. die Knoepfe oben rechts)
    static QQuickItem* findVisible(QQuickItem* root, const QString& name) {
        if (root->objectName() == name && root->isVisible()) return root;
        for (QQuickItem* child : root->childItems())
            if (QQuickItem* found = findVisible(child, name)) return found;
        return nullptr;
    }

    QQuickItem* item(const QString& name) const {
        QQuickItem* found = findVisible(m_window->contentItem(), name);
        if (!found) found = findItem(m_window->contentItem(), name);
        if (!found) qWarning() << "Element nicht gefunden:" << name;
        return found;
    }

    // Knopf in einer ScrollView: Neuanordnung abwarten, in den sichtbaren Bereich rollen, dann klicken
    void clickInPanel(const QString& name) {
        QQuickItem* it = item(name);
        QVERIFY(it);
        QTRY_VERIFY(it->isVisible());
        QTest::qWait(80); // Spalte ordnet beim naechsten Bild neu an; Mitrollen des Experimentfelds (30 ms)
        QQuickItem* flick = it->parentItem();
        while (flick && !flick->inherits("QQuickFlickable")) flick = flick->parentItem();
        QVERIFY(flick);
        QQuickItem* content = flick->property("contentItem").value<QQuickItem*>();
        const QPointF inContent = it->mapToItem(content, QPointF(0, 0));
        const double viewTop = flick->property("contentY").toDouble();
        if (inContent.y() < viewTop || inContent.y() + it->height() > viewTop + flick->height())
            flick->setProperty("contentY", std::max(0.0, inContent.y() - 16));
        QTest::qWait(20);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(it));
    }

    QPoint centerOf(QQuickItem* it) const {
        return it->mapToScene(QPointF(it->width() / 2, it->height() / 2)).toPoint();
    }

    // Ziehen mit mehreren Zwischenschritten (DragHandler braucht Bewegung ueber die Schwelle)
    void drag(QPoint from, QPoint to, Qt::MouseButton button = Qt::LeftButton) {
        QTest::mousePress(m_window, button, {}, from);
        for (int i = 1; i <= 10; ++i) {
            const QPoint p = from + (to - from) * i / 10;
            QTest::mouseMove(m_window, p, 10);
        }
        QTest::mouseRelease(m_window, button, {}, to, 10);
        QCoreApplication::processEvents();
    }

    const model::Board& board() const { return m_lab->session().board(); }
    const model::PartPlacement* part(const std::string& id) const { return board().findPart(id); }

    // Gefuehrtes Experiment 1.1 ueber das Experimentfeld: Vermutung, Tipps bis zur Loesung, Musteraufbau, Erfolg
    void experimentFlowInPanel() {
        clickInPanel(QStringLiteral("guess_1"));
        QTRY_COMPARE(m_lab->phase(), 2);
        QCOMPARE(m_lab->guess(), 1);
        for (int i = 0; i < 3; ++i) {
            clickInPanel(QStringLiteral("hintButton"));
            QTRY_COMPARE(m_lab->hints().size(), i + 1);
        }
        QVERIFY(m_lab->solutionShown());
        clickInPanel(QStringLiteral("templateButton"));
        QTRY_COMPARE(board().parts.size(), std::size_t(1));
        QVERIFY(m_lab->canUndo()); // Musteraufbau ist ein Befehl
        QQuickItem* mainSwitch = item(QStringLiteral("mainSwitch"));
        QVERIFY(mainSwitch);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(mainSwitch));
        QTRY_VERIFY(board().desk.mainSwitch);
        QTRY_COMPARE_WITH_TIMEOUT(m_lab->phase(), 3, 10000); // Haltedauer 2 s in Echtzeit
        clickInPanel(QStringLiteral("nextButton"));
        QTRY_COMPARE(m_lab->experimentId(), QStringLiteral("volta1.1.2"));
        QCOMPARE(m_lab->phase(), 1);
        QVERIFY(!board().desk.mainSwitch);
    }

private slots:
    void initTestCase() {
        m_lab = std::make_unique<LabController>(QStringLiteral(VOLTA_CONTENT_DIR));
        m_engine = std::make_unique<QQmlApplicationEngine>();
        m_engine->rootContext()->setContextProperty(QStringLiteral("labController"), m_lab.get());
        m_language = std::make_unique<LanguageService>(m_engine.get(), false); // ohne Speichern
        connect(m_language.get(), &LanguageService::languageChanged, m_lab.get(), &LabController::setContentLanguage);
        m_engine->rootContext()->setContextProperty(QStringLiteral("volta_languageService"), m_language.get());
        m_settings = std::make_unique<AppSettings>(false); // ohne Speichern
        m_engine->rootContext()->setContextProperty(QStringLiteral("volta_settings"), m_settings.get());
        m_engine->loadFromModule("Volta", "Main");
        QVERIFY(!m_engine->rootObjects().isEmpty());
        m_window = qobject_cast<QQuickWindow*>(m_engine->rootObjects().first());
        QVERIFY(m_window);
        QVERIFY(QTest::qWaitForWindowExposed(m_window));
        // Start auf der Startseite: Lernpfad mit Stufe 1 offen, Stufe 2 gesperrt
        QQuickItem* home = item(QStringLiteral("home"));
        QVERIFY(home && home->isVisible());
        QVERIFY(m_lab->isUnlocked(QStringLiteral("volta1.1.1")));
        QVERIFY(!m_lab->isUnlocked(QStringLiteral("volta1.2.4")));
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("card_volta1.1.1"))));
        QTRY_COMPARE(m_lab->phase(), 1);
        QTRY_VERIFY(!home->isVisible());
        QVERIFY(board().parts.empty());
        // Bauteilfach des Experiments: nur die freigegebenen Bauteile
        QTRY_VERIFY(item(QStringLiteral("tray_gluehlampe")) != nullptr);
        QTRY_VERIFY(findItem(m_window->contentItem(), QStringLiteral("tray_motor")) == nullptr);
        experimentFlowInPanel();
        QVERIFY(m_lab->isDone(QStringLiteral("volta1.1.1"))); // Lernpfad: geschafft
        // die folgenden Ablaeufe arbeiten mit der Vorlage von 2.4 und allen Bauteilen
        m_settings->setUnlockAll(true);
        QTRY_VERIFY(m_lab->unlockAll());
        QVERIFY(m_lab->loadExperiment(QStringLiteral("volta1.2.4")));
        QTRY_COMPARE(board().parts.size(), std::size_t(2));
        QTest::qWait(100); // Bauteilfach neu aufgebaut (alte Kacheln werden verzoegert geloescht)
        QCOMPARE(m_lab->phase(), 0);
    }

    void dragPartFromTray() {
        QQuickItem* tile = item(QStringLiteral("tray_led_gruen"));
        QQuickItem* field = item(QStringLiteral("field_D2"));
        QVERIFY(tile && field);
        drag(centerOf(tile), centerOf(field));
        QTRY_COMPARE(board().parts.size(), std::size_t(3));
        const model::PartPlacement* led = part("D2");
        QVERIFY(led);
        QCOMPARE(led->type, std::string("led_gruen"));
        QCOMPARE(led->pins.front().second, std::string("D2")); // waagrecht, kurz: D2-D3
        QCOMPARE(led->pins.back().second, std::string("D3"));
    }

    void dropOutsideBoardDoesNothing() {
        QQuickItem* tile = item(QStringLiteral("tray_widerstand"));
        QVERIFY(tile);
        const std::size_t before = board().parts.size();
        drag(centerOf(tile), centerOf(tile) + QPoint(40, 0)); // im Fach losgelassen
        QCOMPARE(board().parts.size(), before);
    }

    void drawWireBetweenFields() {
        QQuickItem* a = item(QStringLiteral("field_A1"));
        QQuickItem* b = item(QStringLiteral("field_A5"));
        QVERIFY(a && b);
        const std::size_t before = board().wires.size();
        drag(centerOf(a), centerOf(b));
        QTRY_COMPARE(board().wires.size(), before + 1);
        const auto& w = board().wires.back();
        QCOMPARE(w.from, std::string("A1"));
        QCOMPARE(w.to, std::string("A5"));
    }

    void movePartByDragging() {
        QQuickItem* d2 = item(QStringLiteral("field_D2"));
        QQuickItem* d3 = item(QStringLiteral("field_D3"));
        QQuickItem* e2 = item(QStringLiteral("field_E2"));
        QVERIFY(d2 && d3 && e2);
        // Koerper der LED D2 liegt zwischen D2 und D3; ziehen um eine Zeile nach unten
        const QPoint body = (centerOf(d2) + centerOf(d3)) / 2;
        drag(body, body + (centerOf(e2) - centerOf(d2)));
        QTRY_COMPARE(part("D2")->pins.front().second, std::string("E2"));
    }

    void undoRedoWithKeyboard() {
        QTest::keyClick(m_window, Qt::Key_Z, Qt::ControlModifier);
        QTRY_COMPARE(part("D2")->pins.front().second, std::string("D2"));
        QTest::keyClick(m_window, Qt::Key_Y, Qt::ControlModifier);
        QTRY_COMPARE(part("D2")->pins.front().second, std::string("E2"));
    }

    void moveWireEnd() {
        // Leitung L1 der Vorlage: +9V -> B3; Ende auf A3 umsetzen
        QQuickItem* end = item(QStringLiteral("wireEnd_L1_nach"));
        QQuickItem* a3 = item(QStringLiteral("field_A3"));
        QVERIFY(end && a3);
        drag(centerOf(end), centerOf(a3));
        QTRY_COMPARE(board().wires.front().to, std::string("A3"));
    }

    void invalidMoveShowsHint() {
        // LED D2 (E2-E3) senkrecht drehen geht nicht: unter Zeile E ist kein Platz
        QVERIFY(!m_lab->movePart(QStringLiteral("D2"), QStringLiteral("E2"), true, false));
        auto* toast = item(QStringLiteral("toast"));
        QVERIFY(toast);
        QTRY_VERIFY(toast->property("opacity").toDouble() > 0.5);
        QVERIFY(!toast->property("text").toString().isEmpty());
    }

    void deleteSelectedPartWithKey() {
        const std::size_t before = board().parts.size();
        QQuickItem* e2 = item(QStringLiteral("field_E2"));
        QQuickItem* e3 = item(QStringLiteral("field_E3"));
        QVERIFY(e2 && e3);
        const QPoint body = (centerOf(e2) + centerOf(e3)) / 2;
        QTest::mouseClick(m_window, Qt::LeftButton, {}, body); // auswaehlen
        QTest::keyClick(m_window, Qt::Key_Delete);
        QTRY_COMPARE(board().parts.size(), before - 1);
        QVERIFY(part("D2") == nullptr);
    }

    void tasterClosesOnlyWhilePressed() {
        QVERIFY(m_lab->addPart(QStringLiteral("taster"), QStringLiteral("A6"), false, false)); // A6-A7
        QQuickItem* a6 = item(QStringLiteral("field_A6"));
        QQuickItem* a7 = item(QStringLiteral("field_A7"));
        QVERIFY(a6 && a7);
        const QPoint body = (centerOf(a6) + centerOf(a7)) / 2;
        QTest::mousePress(m_window, Qt::LeftButton, {}, body);
        QTRY_VERIFY(part("S1")->closed);
        QTest::mouseRelease(m_window, Qt::LeftButton, {}, body, 50);
        QTRY_VERIFY(!part("S1")->closed);
    }

    void rightClickOpensPartMenu() {
        QObject* menu = m_engine->rootObjects().first()->findChild<QObject*>(QStringLiteral("partMenu"));
        QVERIFY(menu);
        QVERIFY(!menu->property("opened").toBool());
        const QPoint body = (centerOf(item(QStringLiteral("field_A6"))) + centerOf(item(QStringLiteral("field_A7")))) / 2;
        QTest::mouseClick(m_window, Qt::RightButton, {}, body);
        QTRY_VERIFY(menu->property("opened").toBool());
        QTest::keyClick(m_window, Qt::Key_Escape);
        QTRY_VERIFY(!menu->property("opened").toBool());
    }

    void knobTurnsByCircularDrag() {
        QQuickItem* knob = item(QStringLiteral("knobP1"));
        QVERIFY(knob);
        // vom oberen Ringrand (Mitte) im Uhrzeigersinn nach rechts: 90 Grad -> (90 + 135) / 270
        const QPointF c(knob->width() / 2, knob->height() / 2);
        const QPoint top = knob->mapToScene(c + QPointF(0, -40)).toPoint();
        const QPoint right = knob->mapToScene(c + QPointF(40, 0)).toPoint();
        drag(top, right);
        QTRY_VERIFY(std::abs(board().desk.p1 - 225.0 / 270.0) < 0.03);
        const QString undoBefore = m_lab->undoText();
        // Doppeltippen stellt auf Mitte
        QTest::mouseDClick(m_window, Qt::LeftButton, {}, centerOf(knob));
        QTRY_VERIFY(std::abs(board().desk.p1 - 0.5) < 1e-9);
        QCOMPARE(m_lab->undoText(), undoBefore);
    }

    void deskSwitchToggles() {
        QQuickItem* sw = item(QStringLiteral("deskSwitch"));
        QVERIFY(sw);
        const bool before = board().desk.switchClosed;
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(sw));
        QTRY_COMPARE(board().desk.switchClosed, !before);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(sw));
        QTRY_COMPARE(board().desk.switchClosed, before);
    }

    // +/- in der Werkzeugleiste: 12 -> 10 -> 8 Felder je Reihe und zurueck (Befehl, rueckgaengig machbar)
    void boardColumnsPlusMinus() {
        m_lab->startFreeLab();
        QTRY_VERIFY(board().parts.empty());
        QVERIFY(m_lab->addPart(QStringLiteral("widerstand"), QStringLiteral("A11"), false, false)); // A11-A12
        QQuickItem* minus = item(QStringLiteral("columnsMinus"));
        QQuickItem* plus = item(QStringLiteral("columnsPlus"));
        QVERIFY(minus && plus);
        QVERIFY(!plus->isEnabled());
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(minus));
        QTRY_VERIFY(!m_lab->lastEditError().isEmpty()); // A11/A12 belegt: Hinweis statt Loeschen
        QCOMPARE(board().columns, 12);
        m_lab->undo();
        QTRY_VERIFY(board().parts.empty());
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(minus));
        QTRY_COMPARE(board().columns, 10);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(minus));
        QTRY_COMPARE(board().columns, 8);
        QTRY_VERIFY(!minus->isEnabled());
        QTRY_VERIFY(findItem(m_window->contentItem(), QStringLiteral("field_A9")) == nullptr);
        QVERIFY(item(QStringLiteral("field_E8")));
        QTest::keyClick(m_window, Qt::Key_Z, Qt::ControlModifier);
        QTRY_COMPARE(board().columns, 10);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(plus));
        QTRY_COMPARE(board().columns, 12);
        QTRY_VERIFY(item(QStringLiteral("field_E12")));
    }

    // Sprache oben rechts: Menue mit runden Flaggen, LU noch nicht waehlbar; Oberflaeche und Inhalte wechseln
    void languageSwitch() {
        m_lab->startExperiment(QStringLiteral("volta1.1.1"));
        QQuickItem* button = item(QStringLiteral("languageButton"));
        QVERIFY(button && button->isVisible());
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(button));
        QQuickItem* fr = nullptr;
        QTRY_VERIFY((fr = item(QStringLiteral("language_fr"))) != nullptr && fr->isVisible());
        QQuickItem* lb = item(QStringLiteral("language_lb"));
        QVERIFY(lb && !lb->isEnabled()); // Luxemburgisch folgt
        QTest::qWait(50);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(fr));
        QTRY_COMPARE(m_language->language(), QStringLiteral("fr"));
        QCOMPARE(m_lab->title(), QStringLiteral("Comment allumer une ampoule ?"));
        QCOMPARE(QCoreApplication::translate("TopBarActions", "Einstellungen"), QStringLiteral("Réglages"));
        QCOMPARE(QLocale().decimalPoint(), QStringLiteral(","));
        QVERIFY(!m_language->setLanguage(QStringLiteral("lb")));
        QVERIFY(m_language->setLanguage(QStringLiteral("en")));
        QCOMPARE(m_lab->title(), QStringLiteral("How do you make a bulb light up?"));
        QCOMPARE(QLocale().decimalPoint(), QStringLiteral("."));
        QVERIFY(m_language->setLanguage(QStringLiteral("de")));
        QCOMPARE(QCoreApplication::translate("TopBarActions", "Einstellungen"), QStringLiteral("Einstellungen"));
        QCOMPARE(m_lab->title(), QStringLiteral("Wie bringst du ein Lämpchen zum Leuchten?"));
    }

    void infoDialogShowsPublisher() {
        QObject* dlg = m_engine->rootObjects().first()->findChild<QObject*>(QStringLiteral("infoDialog"));
        QVERIFY(dlg);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("infoButton"))));
        QTRY_VERIFY(dlg->property("opened").toBool());
        const QVariantMap info = m_lab->appInfo();
        QVERIFY(!info.value(QStringLiteral("qtVersion")).toString().isEmpty());
        QTest::keyClick(m_window, Qt::Key_Escape);
        QTRY_VERIFY(!dlg->property("opened").toBool());
    }

    // Experimentfeld einklappen schafft Platz fuer die Grundplatte
    void experimentPanelCollapses() {
        QQuickItem* panel = item(QStringLiteral("experimentPanel"));
        QVERIFY(panel);
        const double wide = panel->width();
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("panelToggle"))));
        QTRY_VERIFY(panel->width() < wide / 3);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("panelToggle"))));
        QTRY_COMPARE(panel->width(), wide);
    }

    // Messgeraet: ein-/ausblenden, Messart am Drehschalter, analog nur im V-Bereich
    void meterPanelControls() {
        m_lab->startFreeLab();
        QVERIFY(!m_lab->meterVisible());
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("meterToggle"))));
        QTRY_VERIFY(m_lab->meterVisible());
        QTRY_VERIFY(item(QStringLiteral("field_MESS:COM")) != nullptr);
        QTest::qWait(50);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("meterMode_1"))));
        QTRY_COMPARE(m_lab->meterMode(), 1);
        QVERIFY(item(QStringLiteral("meterAnalog"))->isEnabled()); // Zeiger in allen Messarten
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("meterAnalog"))));
        QTRY_VERIFY(m_lab->meterAnalog());
        QTRY_COMPARE(item(QStringLiteral("meterUnit"))->property("text").toString(), QStringLiteral("mA"));
        m_lab->setMeterMode(2); // Ohm: Zeiger bleibt, Skala in Ohm
        QVERIFY(m_lab->meterAnalog());
        QTRY_COMPARE(item(QStringLiteral("meterUnit"))->property("text").toString(), QStringLiteral("Ω"));
        m_lab->setMeterAnalog(false);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("meterToggle"))));
        QTRY_VERIFY(!m_lab->meterVisible());
        QTRY_VERIFY(findItem(m_window->contentItem(), QStringLiteral("field_MESS:COM")) == nullptr);
    }

    // Experimentauswahl als Aufklappfeld; Einstellungsmenue ganz rechts
    void experimentBoxAndSettings() {
        QObject* box = item(QStringLiteral("experimentBox"));
        QVERIFY(box);
        m_lab->startExperiment(QStringLiteral("volta1.1.2"));
        QTRY_COMPARE(box->property("currentIndex").toInt(), 2); // 0 Freies Labor, 1 = 1.1, 2 = 1.2
        QMetaObject::invokeMethod(box, "activated", Q_ARG(int, 0));
        QTRY_VERIFY(m_lab->freeLab());
        QCOMPARE(box->property("currentIndex").toInt(), 0);
        QMetaObject::invokeMethod(box, "activated", Q_ARG(int, 1));
        QTRY_COMPARE(m_lab->experimentId(), QStringLiteral("volta1.1.1"));
        QVERIFY(!m_lab->freeLab());

        QQuickItem* settingsButton = item(QStringLiteral("settingsButton"));
        QVERIFY(settingsButton);
        // ganz rechts in der Werkzeugleiste
        QVERIFY(centerOf(settingsButton).x() > centerOf(item(QStringLiteral("infoButton"))).x());
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(settingsButton));
        QQuickItem* flow = nullptr;
        QTRY_VERIFY((flow = item(QStringLiteral("settingFlow"))) != nullptr && flow->isVisible());
        QTest::qWait(50);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(flow));
        QTRY_VERIFY(!m_settings->showCurrentFlow());
        m_settings->setShowCurrentFlow(true);
    }

    // Startseite: zurueck aus dem Labor, freies Labor; Freischaltung der Stufe 2 nach Stufe 1
    void learningPathAndHome() {
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("homeButton"))));
        QQuickItem* home = item(QStringLiteral("home"));
        QTRY_VERIFY(home->isVisible());
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("tileFreeLab"))));
        QTRY_VERIFY(m_lab->freeLab());
        QTRY_VERIFY(!home->isVisible());

        m_settings->setUnlockAll(false);
        QTRY_VERIFY(!m_lab->unlockAll());
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString file = dir.filePath(QStringLiteral("fortschritt.json"));
        {
            services::ProgressStore store(file);
            QVERIFY(store.markDone(QStringLiteral("volta1.1.1")));
            QVERIFY(store.markDone(QStringLiteral("volta1.1.2")));
        }
        m_lab->useProgressFile(file);
        QVERIFY(!m_lab->isUnlocked(QStringLiteral("volta1.2.4"))); // 1.3 fehlt noch
        {
            services::ProgressStore store(file);
            QVERIFY(store.markDone(QStringLiteral("volta1.1.3")));
        }
        m_lab->useProgressFile(file);
        QVERIFY(m_lab->isUnlocked(QStringLiteral("volta1.2.4")));
        const QVariantList stages = m_lab->stages();
        const QVariantMap s1 = stages.at(1).toMap();
        QCOMPARE(s1.value(QStringLiteral("done")).toInt(), 3);
        QVERIFY(stages.at(2).toMap().value(QStringLiteral("unlocked")).toBool());
        QVERIFY(!stages.at(3).toMap().value(QStringLiteral("unlocked")).toBool());
        m_lab->resetProgress();
        QVERIFY(!m_lab->isUnlocked(QStringLiteral("volta1.2.4")));
    }

    void cleanupTestCase() {
        m_engine.reset();
        m_language.reset();
        m_settings.reset();
        m_lab.reset();
    }
};

QTEST_MAIN(UiTest)
#include "test_ui.moc"

// Oberflaechentests (Qt Quick, ohne sichtbares Fenster): echte Bedienung mit simulierter Maus und Tastatur
// gegen die Experimentansicht. Prueft das Zusammenspiel QML <-> LabController <-> LabSession.
#include <QDir>
#include <QJsonArray>
#include <QJSValue>
#include <QQmlApplicationEngine>
#include <QSettings>
#include <QStyleHints>
#include <QFile>
#include <QTemporaryDir>
#include <algorithm>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWindow>
#include <QtQml/qqmlextensionplugin.h>
#include <QtTest>

#include "lab_controller.hpp"
#include "services/audio_route.hpp"
#include "services/managed_config.hpp"
#include "app_settings.hpp"
#include "language_service.hpp"
#include "services/progress_store.hpp"
#include "services/journal_store.hpp"
#include <QImage>
#include <QImageReader>

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

    // Sichtbare Popups (Dialoge, Menues, Werkzeugkasten), auch solche, die gerade noch ausblenden
    QList<QObject*> visiblePopups() const {
        QList<QObject*> list;
        for (QObject* o : m_engine->rootObjects().first()->findChildren<QObject*>())
            if (o->inherits("QQuickPopup") && o->property("visible").toBool()) list << o;
        return list;
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
        QVERIFY(m_lab->buildComplete());
        QQuickItem* finish = item(QStringLiteral("finishButton"));
        QVERIFY(finish && !finish->isEnabled()); // grau, bis die Schaltung funktioniert
        QTRY_VERIFY_WITH_TIMEOUT(m_lab->canFinish(), 30000); // Haltedauer 2 s in Echtzeit (Reserve fuer Last)
        QTRY_VERIFY(finish->isEnabled());
        QCOMPARE(m_lab->phase(), 2); // kein automatischer Abschluss
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(finish)); // fest unten im Feld, nicht im Rollbereich
        QTRY_COMPARE(m_lab->phase(), 3); // Beobachtung
        clickInPanel(QStringLiteral("observation_0"));            // »mit zwei Leitungen hat es geleuchtet«
        QTRY_COMPARE(m_lab->phase(), 4);
        QCOMPARE(m_lab->comparison(), 1); // Vermutung »zwei Leitungen« passt
        QVERIFY(m_lab->addNote(QStringLiteral("Bei 3 V leuchtet es schwaecher")));
        const QVariantList journal = m_lab->journal();
        QCOMPARE(journal.size(), 1);
        const QVariantMap entry = journal.first().toMap();
        QCOMPARE(entry.value(QStringLiteral("comparison")).toInt(), 1);
        QVERIFY(!entry.value(QStringLiteral("guess")).toString().isEmpty());
        QCOMPARE(entry.value(QStringLiteral("notes")).toStringList().size(), 1);
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
        m_settings->setAutoBoardColumns(false); // feste Breite; die Automatik prueft autoBoardColumns()
        m_engine->rootContext()->setContextProperty(QStringLiteral("volta_settings"), m_settings.get());
        // Langes Druecken oeffnet auf Bauteilen, Feldern und Leitungsenden das Menue (Schwelle 0,8 s Echtzeit). Kein
        // Test will das; unter Last kann aber zwischen Druecken und Ziehen bzw. Loslassen so viel Zeit vergehen.
        // Schwelle vor dem Laden der Oberflaeche hochsetzen, damit sie fuer alle TapHandler gilt.
        QGuiApplication::styleHints()->setMousePressAndHoldInterval(60000);
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

    // Vor jedem Test: kein Popup mehr offen oder im Ausblenden. Der Werkzeugkasten faehrt nach close() noch eine
    // Weile hinaus (unter Last deutlich laenger); solange greifen die Tastenkuerzel des Labors nicht, und am Ende gibt
    // Qt den Fokus an das Element zurueck, das ihn vor dem Oeffnen hatte - mitten im naechsten Test. Ein nach einem
    // Fehlschlag offen gebliebenes modales Fenster wuerde ausserdem alle folgenden Klicks schlucken.
    void init() {
        for (QObject* popup : visiblePopups()) {
            if (!popup->property("opened").toBool()) continue; // blendet schon aus
            qWarning() << "Popup aus dem vorigen Test noch offen, wird geschlossen:" << popup->objectName();
            QMetaObject::invokeMethod(popup, "close");
        }
        QTRY_VERIFY_WITH_TIMEOUT(visiblePopups().isEmpty(), 10000);
    }

    void dragPartFromTray() {
        QQuickItem* tile = item(QStringLiteral("tray_led_gruen"));
        QQuickItem* field = item(QStringLiteral("field_D2"));
        QVERIFY(tile && field);
        // waehrend des Ziehens: Schemen an Ort und Stelle, beide Felder hervorgehoben
        const QPoint from = centerOf(tile), to = centerOf(field);
        QTest::mousePress(m_window, Qt::LeftButton, {}, from);
        for (int i = 1; i <= 10; ++i) QTest::mouseMove(m_window, from + (to - from) * i / 10, 10);
        QQuickItem* ghost = item(QStringLiteral("trayGhostPart"));
        QTRY_VERIFY(ghost && ghost->isVisible());
        QTRY_VERIFY(item(QStringLiteral("field_D3"))->property("highlighted").toBool());
        QVERIFY(item(QStringLiteral("field_D2"))->property("highlighted").toBool());
        QVERIFY(!item(QStringLiteral("field_D4"))->property("highlighted").toBool());
        QTest::mouseRelease(m_window, Qt::LeftButton, {}, to, 10);
        QTRY_COMPARE(board().parts.size(), std::size_t(3));
        QVERIFY(!ghost->isVisible());
        const model::PartPlacement* led = part("D2");
        QVERIFY(led);
        QCOMPARE(led->type, std::string("led_gruen"));
        QCOMPARE(led->pins.front().second, std::string("D2")); // waagrecht, kurz: D2-D3
        QCOMPARE(led->pins.back().second, std::string("D3"));
    }

    // Muelleimer: Ausgewaehltes antippen bzw. Bauteil darauf ziehen, immer mit Rueckfrage, rueckgaengig machbar
    void trashDeletesWithConfirmation() {
        const std::size_t before = board().parts.size();
        QVERIFY(part("D2"));
        // Mitte des Bauteils: zwischen seinen Feldern D2 und D3
        const QPoint body = (centerOf(item(QStringLiteral("field_D2"))) + centerOf(item(QStringLiteral("field_D3")))) / 2;
        QTest::mouseClick(m_window, Qt::LeftButton, {}, body);
        QObject* lab = m_engine->rootObjects().first()->findChild<QObject*>(QStringLiteral("lab"));
        QTRY_COMPARE(lab->property("selectedPart").toString(), QStringLiteral("D2"));
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("trashButton"))));
        QQuickItem* cancel = nullptr;
        QTRY_VERIFY((cancel = item(QStringLiteral("trashCancel"))) && cancel->isVisible());
        QTest::qWait(150); // Dialog fertig angeordnet
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(cancel));
        QTRY_VERIFY(!cancel->isVisible());
        QCOMPARE(board().parts.size(), before); // abgebrochen: nichts geloescht
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("trashButton"))));
        QQuickItem* confirm = nullptr;
        QTRY_VERIFY((confirm = item(QStringLiteral("trashConfirm"))) && confirm->isVisible());
        QTest::qWait(150);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(confirm));
        QTRY_COMPARE(board().parts.size(), before - 1);
        QVERIFY(!part("D2"));
        m_lab->undo();
        QTRY_VERIFY(part("D2"));
        // Ziehen auf den Muelleimer
        QTest::qWait(50);
        drag(body, centerOf(item(QStringLiteral("trashButton"))));
        QTRY_VERIFY(confirm->isVisible());
        QCOMPARE(board().parts.size(), before); // erst nach der Rueckfrage
        QTest::qWait(150);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(confirm));
        QTRY_VERIFY(!part("D2"));
        m_lab->undo();
        QTRY_VERIFY(part("D2"));
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

    // Steck- und Betriebsmodus: Hauptschalter ein = Betrieb (Teile fest); Leitung ziehen schaltet automatisch aus
    void operatingAndPlugMode() {
        QQuickItem* mode = item(QStringLiteral("modeButton"));
        QVERIFY(mode);
        QVERIFY(!m_lab->mainSwitch());
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(mode));
        QTRY_VERIFY(m_lab->mainSwitch()); // Knopf schaltet ein: Betrieb
        QObject* lab = m_engine->rootObjects().first()->findChild<QObject*>(QStringLiteral("lab"));
        QVERIFY(lab->property("operating").toBool());
        // ein Bauteil ist im Betrieb nicht verschiebbar
        QQuickItem* anyPart = nullptr;
        for (QQuickItem* it : m_window->contentItem()->findChildren<QQuickItem*>())
            if (it->objectName().startsWith(QStringLiteral("part_")) && it->isVisible()) anyPart = it;
        if (anyPart) QVERIFY(!anyPart->property("movable").toBool());
        // Leitung ziehen: Strom aus, Steckmodus, Leitung entsteht trotzdem
        const std::size_t before = board().wires.size();
        drag(centerOf(item(QStringLiteral("field_E1"))), centerOf(item(QStringLiteral("field_E4"))));
        QTRY_COMPARE(board().wires.size(), before + 1);
        QVERIFY(!m_lab->mainSwitch());
        QVERIFY(!lab->property("operating").toBool());
        if (anyPart) QVERIFY(anyPart->property("movable").toBool());
        m_lab->undo();
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

    // Sprache oben rechts: Menue mit runden Flaggen (DE, FR, EN, LU); Oberflaeche und Inhalte wechseln
    void languageSwitch() {
        m_lab->startExperiment(QStringLiteral("volta1.1.1"));
        QQuickItem* button = item(QStringLiteral("languageButton"));
        QVERIFY(button && button->isVisible());
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(button));
        QQuickItem* fr = nullptr;
        QTRY_VERIFY((fr = item(QStringLiteral("language_fr"))) != nullptr && fr->isVisible());
        QQuickItem* lb = item(QStringLiteral("language_lb"));
        QVERIFY(lb && lb->isEnabled());
        QTest::qWait(50);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(fr));
        QTRY_COMPARE(m_language->language(), QStringLiteral("fr"));
        QCOMPARE(m_lab->title(), QStringLiteral("Comment allumer une ampoule ?"));
        QCOMPARE(QCoreApplication::translate("TopBarActions", "Einstellungen"), QStringLiteral("Réglages"));
        QCOMPARE(QLocale().decimalPoint(), QStringLiteral(","));
        QVERIFY(m_language->setLanguage(QStringLiteral("lb")));
        QCOMPARE(m_lab->title(), QStringLiteral("Wéi bréngs du e Bierchend zum Liichten?"));
        QCOMPARE(QCoreApplication::translate("TopBarActions", "Einstellungen"), QStringLiteral("Astellungen"));
        QVERIFY(m_language->setLanguage(QStringLiteral("en")));
        QCOMPARE(m_lab->title(), QStringLiteral("How do you make a bulb light up?"));
        QCOMPARE(QLocale().decimalPoint(), QStringLiteral("."));
        // Meldungen des Modells und Befehlsnamen in der Anzeigesprache
        QVERIFY(!m_lab->addPart(QStringLiteral("gluehlampe"), QStringLiteral("E12"), true, false));
        QCOMPARE(m_lab->lastEditError(), QStringLiteral("The part doesn't fit on the board there."));
        QVERIFY(m_lab->addWire(QStringLiteral("P:+9V"), QStringLiteral("C7"), QStringLiteral("rot")));
        QVERIFY2(m_lab->undoText().startsWith(QStringLiteral("Draw wire")), qPrintable(m_lab->undoText()));
        m_lab->undo();
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
        // Platz wechseln: oben ins Pult - die Buchsen wandern in die Reihe der Pultleiste, Tasten statt Drehschalter
        QQuickItem* com = item(QStringLiteral("field_MESS:COM"));
        const qreal besideY = com->mapToScene(QPointF(0, 0)).y();
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("meterDockButton"))));
        QTRY_VERIFY(m_settings->meterDocked());
        const qreal deskY = item(QStringLiteral("field_P:+9V"))->mapToScene(QPointF(0, 0)).y();
        QTRY_VERIFY(std::abs(com->mapToScene(QPointF(0, 0)).y() - deskY) < 1.0);
        QVERIFY(deskY < besideY);
        QTRY_VERIFY(item(QStringLiteral("meterKey_0"))->isVisible());
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("meterKey_0"))));
        QTRY_COMPARE(m_lab->meterMode(), 0);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("meterDockButton"))));
        QTRY_VERIFY(!m_settings->meterDocked());
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("meterToggle"))));
        QTRY_VERIFY(!m_lab->meterVisible());
        QTRY_VERIFY(findItem(m_window->contentItem(), QStringLiteral("field_MESS:COM")) == nullptr);
    }

    // Aufbauhilfe: Knopf im Experimentfeld, Geisterbild und Zielfelder folgen dem Aufbau
    void buildGuideInPanel() {
        QVERIFY(m_lab->startExperiment(QStringLiteral("volta1.1.1")));
        m_lab->chooseGuess(0);
        QTRY_COMPARE(m_lab->phase(), 2);
        clickInPanel(QStringLiteral("guideButton"));
        QTRY_VERIFY(m_lab->guideActive());
        QCOMPARE(m_lab->guideStep().value(QStringLiteral("kind")).toString(), QStringLiteral("bauteil"));
        QTRY_VERIFY(item(QStringLiteral("guideGhostPart"))->isVisible());
        QTRY_VERIFY(item(QStringLiteral("field_A1"))->property("target").toBool());
        // Lampe wie in der Vorlage gesteckt: weiter zur ersten Leitung
        QVERIFY(m_lab->addPart(QStringLiteral("gluehlampe"), QStringLiteral("A1"), false, false));
        QTRY_COMPARE(m_lab->guideStep().value(QStringLiteral("kind")).toString(), QStringLiteral("leitung"));
        QTRY_VERIFY(item(QStringLiteral("guideGhostWire"))->isVisible());
        QVERIFY(!item(QStringLiteral("guideGhostPart"))->isVisible());
        const QString text = item(QStringLiteral("guideText"))->property("text").toString();
        QVERIFY2(text.contains(QStringLiteral("+9V")), qPrintable(text));
        clickInPanel(QStringLiteral("guideButton"));
        QTRY_VERIFY(!m_lab->guideActive());
        QTRY_VERIFY(!item(QStringLiteral("field_A1"))->property("target").toBool());
    }

    // Antippen waehlt nur aus (nichts wird geloescht); Rechtsklick bzw. langer Druck oeffnet das Leitungsmenue mit
    // Farben und abgesetztem Loeschen. Farbautomatik: Leitung von +9V rot, nach 0V schwarz.
    void wireContextMenu() {
        m_settings->setUnlockAll(true);
        m_lab->startFreeLab();
        QVERIFY(m_lab->addPart(QStringLiteral("widerstand"), QStringLiteral("C5"), false, false));
        QVERIFY(m_lab->addWire(QStringLiteral("P:+9V"), QStringLiteral("C5"), QStringLiteral("gruen")));
        QCOMPARE(board().wires[0].color, std::string("rot"));
        QTest::qWait(100);
        QObject* lab = m_engine->rootObjects().first()->findChild<QObject*>(QStringLiteral("lab"));
        QObject* menu = lab->findChild<QObject*>(QStringLiteral("wireMenu"));
        QVERIFY(menu);
        QQuickItem* end = item(QStringLiteral("wireEnd_L1_nach"));
        QVERIFY(end);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(end));
        QTRY_COMPARE(lab->property("selectedWire").toString(), QStringLiteral("L1"));
        QTest::qWait(50);
        QVERIFY(!menu->property("opened").toBool()); // Antippen: kein Menue
        QCOMPARE(board().wires.size(), std::size_t(1));
        QTest::mouseClick(m_window, Qt::RightButton, {}, centerOf(end));
        QTRY_VERIFY(menu->property("opened").toBool());
        QCOMPARE(menu->property("wireColor").toString(), QStringLiteral("rot"));
        QMetaObject::invokeMethod(menu, "close");
        QVERIFY(m_lab->setWireColor(QStringLiteral("L1"), QStringLiteral("blau"))); // wie Auswahl im Menue
        QVERIFY(board().wires[0].colorFixed);
        QVERIFY(m_lab->removeWire(QStringLiteral("L1")));
        QVERIFY(m_lab->addWire(QStringLiteral("C6"), QStringLiteral("P:0V"), QStringLiteral("gelb")));
        QCOMPARE(board().wires.back().color, std::string("schwarz"));
    }

    // Experimentauswahl als Aufklappfeld; Einstellungsmenue ganz rechts
    void experimentBoxAndSettings() {
        QObject* box = item(QStringLiteral("experimentBox"));
        QVERIFY(box);
        m_lab->startExperiment(QStringLiteral("volta1.1.2"));
        // Eintraege: 0 Freies Labor, danach die freigeschalteten Experimente in Lernpfad-Reihenfolge
        auto indexOf = [box](const QString& id) {
            const QVariantList entries = box->property("entries").value<QJSValue>().toVariant().toList();
            for (int i = 0; i < entries.size(); ++i)
                if (entries.at(i).toMap().value(QStringLiteral("id")).toString() == id) return i;
            return -1;
        };
        QVERIFY(indexOf(QStringLiteral("volta1.1.2")) > 1);
        QTRY_COMPARE(box->property("currentIndex").toInt(), indexOf(QStringLiteral("volta1.1.2")));
        QMetaObject::invokeMethod(box, "activated", Q_ARG(int, 0));
        QTRY_VERIFY(m_lab->freeLab());
        QCOMPARE(box->property("currentIndex").toInt(), 0);
        QMetaObject::invokeMethod(box, "activated", Q_ARG(int, indexOf(QStringLiteral("volta1.1.1"))));
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

    // Forscherheft-Seite: Eintrag aus dem gefuehrten Experiment 1.1 (initTestCase) ist sichtbar
    void journalPage() {
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("homeButton"))));
        QTRY_VERIFY(item(QStringLiteral("home"))->isVisible());
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("tileJournal"))));
        QTRY_VERIFY(item(QStringLiteral("journal"))->isVisible());
        QTRY_VERIFY(item(QStringLiteral("journal_volta1.1.1")) != nullptr);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("journalBack"))));
        QTRY_VERIFY(item(QStringLiteral("home"))->isVisible());
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("tileFreeLab"))));
        QTRY_VERIFY(m_lab->freeLab());
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
        // freies Labor: alle Bauteile sichtbar, gesperrte (z. B. Transistor, Stufe 4) nicht setzbar
        m_lab->resetProgress();
        QTRY_VERIFY(item(QStringLiteral("tray_transistor_npn")) != nullptr);
        // Umschalter Bauteilfach / Modulfach: immer nur ein Fach sichtbar
        QQuickItem* trayToggle = item(QStringLiteral("trayToggle"));
        QVERIFY(trayToggle && trayToggle->isEnabled());
        QVERIFY(item(QStringLiteral("tray_widerstand"))->isVisible());
        QVERIFY(!item(QStringLiteral("trayModule_blinkmodul"))->isVisible());
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(trayToggle));
        QTRY_VERIFY(item(QStringLiteral("trayModule_blinkmodul"))->isVisible());
        QVERIFY(!item(QStringLiteral("tray_widerstand"))->isVisible());
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(trayToggle));
        QTRY_VERIFY(item(QStringLiteral("tray_widerstand"))->isVisible());
        bool transistorLocked = false;
        for (const QVariant& v : m_lab->trayParts())
            if (v.toMap().value(QStringLiteral("type")).toString() == QStringLiteral("transistor_npn"))
                transistorLocked = v.toMap().value(QStringLiteral("locked")).toBool();
        QVERIFY(transistorLocked);
        QVERIFY(!m_lab->addPart(QStringLiteral("transistor_npn"), QStringLiteral("B2"), false, false));
        QVERIFY(m_lab->addPart(QStringLiteral("gluehlampe"), QStringLiteral("B2"), false, false));
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
        QVERIFY(!m_lab->isUnlocked(QStringLiteral("volta1.2.4"))); // Stufe 1 noch nicht ganz geschafft
        {
            services::ProgressStore store(file); // alle uebrigen Experimente der Stufe 1
            for (int nr = 3; nr <= 10; ++nr) QVERIFY(store.markDone(QStringLiteral("volta1.1.%1").arg(nr)));
        }
        m_lab->useProgressFile(file);
        QVERIFY(m_lab->isUnlocked(QStringLiteral("volta1.2.4")));
        const QVariantList stages = m_lab->stages();
        const QVariantMap s1 = stages.at(1).toMap();
        QCOMPARE(s1.value(QStringLiteral("done")).toInt(), s1.value(QStringLiteral("total")).toInt());
        QVERIFY(stages.at(2).toMap().value(QStringLiteral("unlocked")).toBool());
        QVERIFY(!stages.at(3).toMap().value(QStringLiteral("unlocked")).toBool());
        m_lab->resetProgress();
        QVERIFY(!m_lab->isUnlocked(QStringLiteral("volta1.2.4")));
    }

    // Schriftgroesse in drei Stufen ueber A- / A+ (Vorgabe: groesste)
    void fontSizeButtons() {
        QCOMPARE(m_settings->fontLevel(), 2);
        QQuickItem* larger = item(QStringLiteral("fontLarger"));
        QQuickItem* smaller = item(QStringLiteral("fontSmaller"));
        QVERIFY(larger && smaller);
        QVERIFY(!larger->isEnabled()); // schon am groessten
        QTest::qWait(100);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(smaller));
        QTRY_COMPARE(m_settings->fontLevel(), 1);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(smaller));
        QTRY_COMPARE(m_settings->fontLevel(), 0);
        QTRY_VERIFY(!smaller->isEnabled());
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(larger));
        QTRY_COMPARE(m_settings->fontLevel(), 1);
        m_settings->setFontLevel(2);
    }

    // Werkzeugkasten: Wert eingeben -> Ringe; auf den ausgewaehlten Widerstand uebernehmen (Befehl)
    void toolboxColorCode() {
        m_settings->setUnlockAll(true);
        m_lab->startFreeLab();
        QVERIFY(m_lab->addPart(QStringLiteral("widerstand"), QStringLiteral("B3"), false, false)); // R1, 1 kOhm
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("toolboxButton"))));
        QObject* box = m_engine->rootObjects().first()->findChild<QObject*>(QStringLiteral("toolbox"));
        QVERIFY(box);
        QTRY_VERIFY(box->property("opened").toBool());
        QQuickItem* input = item(QStringLiteral("colorInput"));
        QVERIFY(input);
        input->setProperty("text", QStringLiteral("4k7"));
        QMetaObject::invokeMethod(item(QStringLiteral("colorShow")), "clicked");
        QTRY_VERIFY(item(QStringLiteral("colorValue"))->property("text").toString().startsWith(QStringLiteral("4,7 k")));
        QCOMPARE(m_lab->decodeColors({4, 7, 2, 10}), 4700.0);
        // Widerstand auswaehlen (Lab.selectedPart) und uebernehmen
        QObject* lab = m_engine->rootObjects().first()->findChild<QObject*>(QStringLiteral("lab"));
        lab->setProperty("selectedPart", QStringLiteral("R1"));
        QTRY_VERIFY(item(QStringLiteral("colorApply"))->isEnabled());
        QMetaObject::invokeMethod(item(QStringLiteral("colorApply")), "clicked");
        QTRY_COMPARE(board().findPart("R1")->value, 4700.0);
        QVERIFY(m_lab->canUndo());
        QMetaObject::invokeMethod(box, "close");
        QTRY_VERIFY(!box->property("opened").toBool());
    }

    // Forscherprofile: getrennte Daten je Profil (Fortschritt, Laborstand), Loeschen entfernt den Ordner
    void profilesKeepDataApart() {
        QTemporaryDir dir;
        LabController lab(QStringLiteral(VOLTA_CONTENT_DIR));
        lab.useDataRoot(dir.path());
        lab.setUnlockAll(true);
        lab.setProfilesEnabled(true);
        const QString a = lab.addProfile(QStringLiteral("🦊"), QStringLiteral("  Funkenfuchs  "));
        const QString b = lab.addProfile(QStringLiteral("🐼"), QStringLiteral("Panda"));
        QVERIFY(!a.isEmpty() && !b.isEmpty() && a != b);
        QCOMPARE(lab.profiles().first().toMap().value(QStringLiteral("name")).toString(), QStringLiteral("Funkenfuchs"));
        QVERIFY(lab.addProfile(QStringLiteral("🦉"), QStringLiteral("   ")).isEmpty()); // ohne Spitzname
        QVERIFY(lab.selectProfile(a));
        QCOMPARE(lab.currentProfile().value(QStringLiteral("id")).toString(), a);
        lab.startFreeLab();
        QVERIFY(lab.addPart(QStringLiteral("widerstand"), QStringLiteral("B3"), false, false));
        lab.saveSession();
        QVERIFY(lab.selectProfile(b));
        QVERIFY(lab.session().board().parts.empty()); // neues Profil: leeres Labor
        QVERIFY(!lab.resumeSession());                // und kein gespeicherter Stand
        QVERIFY(lab.selectProfile(a));
        QVERIFY(lab.resumeSession());
        QCOMPARE(lab.session().board().parts.size(), std::size_t(1));
        // Neustart: Profile bleiben, Loeschen entfernt Profil und Ordner
        LabController again(QStringLiteral(VOLTA_CONTENT_DIR));
        again.useDataRoot(dir.path());
        again.setProfilesEnabled(true);
        QCOMPARE(again.profiles().size(), 2);
        QVERIFY(QDir(dir.filePath(QStringLiteral("profile/") + a)).exists());
        QVERIFY(again.removeProfile(a));
        QVERIFY(!QDir(dir.filePath(QStringLiteral("profile/") + a)).exists());
        again.removeAllProfiles();
        QCOMPARE(again.profiles().size(), 0);
    }

    // Verwaltete Konfiguration: gueltige Werte uebernehmen, ungueltige ignorieren; INI-Quelle; Wirkung im Labor
    void managedConfiguration() {
        using services::ManagedConfig;
        const ManagedConfig none(QVariantMap{});
        QVERIFY(!none.managed());
        QCOMPARE(none.unlockStage(), -1);
        QCOMPARE(none.profiles(), -1);
        QVERIFY(none.parentArea());
        QVERIFY(none.soundMode().isEmpty());
        const ManagedConfig school(QVariantMap{{"sprache", "FR"}, {"sprache_gesperrt", "true"}, {"stufen_frei", "3"},
                                               {"elternbereich", "false"}, {"klassenmodus_ton", "kopfhoerer"},
                                               {"profile", "12"}});
        QVERIFY(school.managed());
        QCOMPARE(school.language(), QStringLiteral("fr"));
        QVERIFY(school.languageLocked());
        QCOMPARE(school.unlockStage(), 3);
        QVERIFY(!school.parentArea());
        QCOMPARE(school.soundMode(), QStringLiteral("kopfhoerer"));
        QCOMPARE(school.profiles(), 12);
        const ManagedConfig odd(QVariantMap{{"sprache", "xx"}, {"klassenmodus_ton", "laut"}, {"profile", "99"}});
        QVERIFY(odd.language().isEmpty() && odd.soundMode().isEmpty());
        QCOMPARE(odd.profiles(), 30); // begrenzt
        // INI-Quelle (wie volta.ini neben der EXE)
        QTemporaryDir dir;
        const QString ini = dir.filePath(QStringLiteral("volta.ini"));
        {
            QSettings s(ini, QSettings::IniFormat);
            s.setValue(QStringLiteral("klassenmodus_ton"), QStringLiteral("aus"));
            s.setValue(QStringLiteral("stufen_frei"), 2);
        }
        qputenv("VOLTA_MANAGED_INI", ini.toLocal8Bit());
        const ManagedConfig fromIni;
        qunsetenv("VOLTA_MANAGED_INI");
        QCOMPARE(fromIni.soundMode(), QStringLiteral("aus"));
        QCOMPARE(fromIni.unlockStage(), 2);
        // Kopfhoerer am Geraetenamen (Desktop)
        QVERIFY(services::AudioRoute::looksLikeHeadphones(QStringLiteral("Kopfhörer (Realtek Audio)")));
        QVERIFY(services::AudioRoute::looksLikeHeadphones(QStringLiteral("Headset Earphone")));
        QVERIFY(!services::AudioRoute::looksLikeHeadphones(QStringLiteral("Lautsprecher (Realtek Audio)")));
        // Wirkung: Stufen bis 3 offen, Profilzahl begrenzt
        LabController lab(QStringLiteral(VOLTA_CONTENT_DIR));
        lab.resetProgress();
        QCOMPARE(lab.unlockedStage(), 1);
        lab.setManagedUnlockStage(3);
        QCOMPARE(lab.unlockedStage(), 3);
        lab.setProfilesEnabled(true);
        lab.setProfileLimit(2);
        QVERIFY(!lab.addProfile(QStringLiteral("🦊"), QStringLiteral("A")).isEmpty());
        QVERIFY(!lab.addProfile(QStringLiteral("🐼"), QStringLiteral("B")).isEmpty());
        QVERIFY(lab.addProfile(QStringLiteral("🦉"), QStringLiteral("C")).isEmpty()); // Grenze erreicht
    }

    // Mitnahme per Datei: Profil speichern und auf einem "anderen Geraet" als neues Profil laden
    void profileFileTransfer() {
        QTemporaryDir a, b;
        LabController from(QStringLiteral(VOLTA_CONTENT_DIR));
        from.useDataRoot(a.path());
        from.setProfilesEnabled(true);
        QVERIFY(from.selectProfile(from.addProfile(QStringLiteral("🦉"), QStringLiteral("Eule"))));
        QCOMPARE(from.importProgressLink(QStringLiteral("volta1://fortschritt?d=1.1,1.2")), 2);
        const QUrl file = QUrl::fromLocalFile(a.filePath(QStringLiteral("eule.volta1profil")));
        QVERIFY(from.exportProfile(file));
        const QVariantMap info = from.inspectProfileFile(file);
        QVERIFY(info.value(QStringLiteral("ok")).toBool());
        QCOMPARE(info.value(QStringLiteral("name")).toString(), QStringLiteral("Eule"));
        QCOMPARE(info.value(QStringLiteral("done")).toInt(), 2);
        // anderes Geraet mit Profilen: neues Profil mit dem Fortschritt
        LabController to(QStringLiteral(VOLTA_CONTENT_DIR));
        to.useDataRoot(b.path());
        to.setProfilesEnabled(true);
        QVERIFY(to.importProfileFile(file));
        QCOMPARE(to.profiles().size(), 1);
        QVERIFY(to.selectProfile(to.profiles().first().toMap().value(QStringLiteral("id")).toString()));
        QVERIFY(to.isDone(QStringLiteral("volta1.1.2")));
        // anderes Geraet ohne Profile: ersetzt den Stand
        LabController plain(QStringLiteral(VOLTA_CONTENT_DIR));
        QTemporaryDir c;
        plain.useDataRoot(c.path());
        QVERIFY(!plain.isDone(QStringLiteral("volta1.1.1")));
        QVERIFY(plain.importProfileFile(file));
        QVERIFY(plain.isDone(QStringLiteral("volta1.1.1")));
        // keine Profildatei
        QFile junk(a.filePath(QStringLiteral("x.volta1profil")));
        QVERIFY(junk.open(QIODevice::WriteOnly));
        junk.write("{}");
        junk.close();
        QVERIFY(!plain.inspectProfileFile(QUrl::fromLocalFile(junk.fileName())).value(QStringLiteral("ok")).toBool());
    }

    // Fortschritts-Link aus der Kamera-App: Rueckfrage, dann uebernommen
    void progressLinkOpensDialog() {
        m_lab->resetProgress();
        QObject* dialog = m_engine->rootObjects().first()->findChild<QObject*>(QStringLiteral("linkDialog"));
        QVERIFY(dialog);
        m_lab->handleUrl(QUrl(QStringLiteral("volta1://fortschritt?d=1.1,1.3")));
        QTRY_VERIFY(dialog->property("opened").toBool());
        QCOMPARE(dialog->property("count").toInt(), 2);
        QVERIFY(!m_lab->isDone(QStringLiteral("volta1.1.1"))); // erst nach der Rueckfrage
        QMetaObject::invokeMethod(dialog, "accept");
        QTRY_VERIFY(m_lab->isDone(QStringLiteral("volta1.1.3")));
        QTRY_VERIFY(!dialog->property("opened").toBool());
        m_lab->resetProgress();
    }

    // Profilauswahl beim Start: neues Profil anlegen, Abzeichen auf der Startseite, Wechsel zurueck zur Auswahl
    void profileChooserFlow() {
        QObject* chooser = m_engine->rootObjects().first()->findChild<QObject*>(QStringLiteral("profileChooser"));
        QVERIFY(chooser);
        QVERIFY(!chooser->property("visible").toBool());
        m_settings->setProfilesEnabled(true);
        QTRY_VERIFY(chooser->property("visible").toBool());
        QTest::qWait(50);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("profileNew"))));
        QQuickItem* symbol = nullptr;
        QTRY_VERIFY((symbol = item(QStringLiteral("profileSymbol_0"))) && symbol->isVisible());
        QTest::qWait(100); // Anordnung abwarten
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(symbol));
        QTRY_COMPARE(chooser->property("newSymbol").toString(), QStringLiteral("🦊"));
        item(QStringLiteral("profileName"))->setProperty("text", QStringLiteral("Blitz"));
        QQuickItem* create = item(QStringLiteral("profileCreate"));
        QTRY_VERIFY(create->isEnabled());
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(create));
        QTRY_VERIFY(!chooser->property("visible").toBool());
        QCOMPARE(m_lab->currentProfile().value(QStringLiteral("name")).toString(), QStringLiteral("Blitz"));
        // zur Startseite: Abzeichen mit Profil, Antippen fuehrt zur Auswahl
        QMetaObject::invokeMethod(item(QStringLiteral("homeButton")), "clicked");
        QQuickItem* badge = nullptr;
        QTRY_VERIFY((badge = item(QStringLiteral("profileBadge"))) && badge->isVisible());
        QTest::qWait(50);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(badge));
        QTRY_VERIFY(chooser->property("visible").toBool());
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("profile_") +
                                                                       m_lab->profiles().first().toMap().value(QStringLiteral("id")).toString())));
        QTRY_VERIFY(!chooser->property("visible").toBool());
        m_lab->removeAllProfiles();
        m_settings->setProfilesEnabled(false);
        QTRY_VERIFY(!chooser->property("visible").toBool());
        m_engine->rootObjects().first()->setProperty("inLab", true); // folgende Tests arbeiten im Labor
    }

    // Elternschranke und Elternbereich: falsche Antwort haelt auf, richtige oeffnet; Freigabe und Zuruecksetzen
    void parentGateAndArea() {
        m_settings->setUnlockAll(false);
        QObject* gate = m_engine->rootObjects().first()->findChild<QObject*>(QStringLiteral("parentGate"));
        QObject* area = m_engine->rootObjects().first()->findChild<QObject*>(QStringLiteral("parentArea"));
        QVERIFY(gate && area);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("settingsButton"))));
        QQuickItem* entry = nullptr;
        QTRY_VERIFY((entry = item(QStringLiteral("settingParentArea"))) && entry->isVisible());
        QTest::qWait(100);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(entry));
        QTRY_VERIFY(gate->property("opened").toBool());
        QVERIFY(m_settings->parentPinIsDefault());
        auto type = [&](const QString& digits) {
            for (QChar ch : digits)
                QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("gateKey_") + ch)));
            QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("gateKey_ok"))));
        };
        QTest::qWait(100);
        type(QStringLiteral("4321")); // falsch
        QVERIFY(gate->property("opened").toBool());
        QVERIFY(gate->property("wrong").toBool());
        QVERIFY(!area->property("opened").toBool());
        type(QStringLiteral("1234")); // Start-PIN
        QTRY_VERIFY(area->property("opened").toBool());
        QTRY_VERIFY(!gate->property("opened").toBool());
        // Freigabe im Elternbereich
        QTest::qWait(100);
        clickInPanel(QStringLiteral("parentUnlockAll"));
        QTRY_VERIFY(m_settings->unlockAll());
        QMetaObject::invokeMethod(area, "close");
        QTRY_VERIFY(!area->property("opened").toBool());
        // Info: Link nach aussen nur ueber die Schranke (hier abbrechen)
        QObject* info = m_engine->rootObjects().first()->findChild<QObject*>(QStringLiteral("infoDialog"));
        QMetaObject::invokeMethod(info, "open");
        QTRY_VERIFY(info->property("opened").toBool());
        QTest::qWait(100);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("infoWebsite"))));
        QTRY_VERIFY(gate->property("opened").toBool());
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("gateCancel"))));
        QTRY_VERIFY(!gate->property("opened").toBool());
    }

    // PIN fuer den Elternbereich: aendern (nur Ziffern, 4-8), Pruefwert statt PIN, Sperre nach fuenf Fehlversuchen
    void parentPin() {
        QTemporaryDir dir;
        const QString file = dir.filePath(QStringLiteral("einstellungen.ini"));
        {
            AppSettings s(true, file);
            QVERIFY(s.parentPinIsDefault());
            QVERIFY(s.checkParentPin(QStringLiteral("1234")));
            QVERIFY(!s.setParentPin(QStringLiteral("12a4")));
            QVERIFY(!s.setParentPin(QStringLiteral("123")));
            QVERIFY(s.setParentPin(QStringLiteral("80734")));
            QVERIFY(!s.parentPinIsDefault());
            QVERIFY(!s.checkParentPin(QStringLiteral("1234")));
        }
        QFile f(file);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QVERIFY(!f.readAll().contains("80734")); // nur der Pruefwert steht in der Datei
        f.close();
        AppSettings again(true, file); // Neustart
        QVERIFY(again.checkParentPin(QStringLiteral("80734")));
        for (int i = 0; i < 5; ++i) QVERIFY(!again.checkParentPin(QStringLiteral("0000")));
        QVERIFY(again.parentPinLockSeconds() > 0);
        QVERIFY(!again.checkParentPin(QStringLiteral("80734"))); // gesperrt, auch mit richtiger PIN
        AppSettings restarted(true, file);
        QVERIFY(restarted.parentPinLockSeconds() > 0); // Sperre bleibt ueber den Neustart
    }

    // Teileliste im Werkzeugkasten: Zeilen zur Vorlage, Aufbaukarte als PDF; zerstoerende Versuche ohne Karte
    void partsListAndBuildCard() {
        m_settings->setUnlockAll(true);
        QVERIFY(m_lab->startExperiment(QStringLiteral("volta1.4.4")));
        QObject* box = m_engine->rootObjects().first()->findChild<QObject*>(QStringLiteral("toolbox"));
        QVERIFY(box);
        QMetaObject::invokeMethod(box, "open");
        QTRY_VERIFY(box->property("opened").toBool());
        box->findChild<QObject*>(QStringLiteral("toolboxTabs"))->setProperty("currentIndex", 5);
        QTRY_VERIFY(item(QStringLiteral("partsLine_0")) && item(QStringLiteral("partsLine_0"))->isVisible());
        QObject* panel = box->findChild<QObject*>(QStringLiteral("partsList"));
        QVERIFY(panel);
        QVariant texts;
        QVERIFY(QMetaObject::invokeMethod(panel, "cardTexts", Q_RETURN_ARG(QVariant, texts)));
        const QVariantMap map = texts.toMap();
        QVERIFY(map.value(QStringLiteral("parts")).toStringList().size() >= 5);
        QVERIFY(map.value(QStringLiteral("steps")).toStringList().first().contains(QStringLiteral("B1")));
        QTemporaryDir dir;
        const QString file = dir.filePath(QStringLiteral("karte.pdf"));
        QVERIFY(m_lab->exportBuildCard(QUrl::fromLocalFile(file), map));
        QFile f(file);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QVERIFY(f.read(5) == "%PDF-");
        QVERIFY(f.size() > 5000);
        f.close();
        QCOMPARE(m_lab->buildCardFileName(), QStringLiteral("Volta1-Aufbaukarte-4.4.pdf"));
        // »Nur in der App!«: Hinweis statt Knopf
        QVERIFY(m_lab->startExperiment(QStringLiteral("volta1.2.4")));
        QTRY_VERIFY(item(QStringLiteral("partsAppOnly")) && item(QStringLiteral("partsAppOnly"))->isVisible());
        QVERIFY(!item(QStringLiteral("saveBuildCard"))->isVisible());
        QMetaObject::invokeMethod(box, "close");
        QTRY_VERIFY(!box->property("opened").toBool());
    }

    // Hilfe mit Tick: Fragezeichen oeffnet sie, Kapitelknopf springt, Erwachsenenteil fuehrt ueber die PIN-Abfrage
    void helpWithTick() {
        QObject* root = m_engine->rootObjects().first();
        QObject* help = root->findChild<QObject*>(QStringLiteral("helpDialog"));
        QVERIFY(help);
        // im Labor steht die Hilfe im Menue (Leiste voll), auf der Startseite als Fragezeichen
        QVERIFY(!item(QStringLiteral("helpButton"))->isVisible() || !root->property("inLab").toBool());
        root->setProperty("inLab", false);
        QTRY_VERIFY(item(QStringLiteral("home"))->isVisible());
        QQuickItem* button = item(QStringLiteral("helpButton"));
        QVERIFY(button && button->isVisible());
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(button));
        QTRY_VERIFY(help->property("opened").toBool());
        QQuickItem* chip = item(QStringLiteral("helpChip_fehler"));
        QVERIFY(chip);
        QTest::qWait(100);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(chip));
        QQuickItem* section = item(QStringLiteral("helpSection_fehler"));
        QVERIFY(section);
        QTRY_VERIFY(section->mapToItem(m_window->contentItem(), QPointF(0, 0)).y() < 200); // nach oben gerollt
        QMetaObject::invokeMethod(help, "jumpTo", Q_ARG(QVariant, QStringLiteral("eltern")));
        QQuickItem* parentButton = item(QStringLiteral("helpParentArea"));
        QVERIFY(parentButton);
        QTest::qWait(400);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(parentButton));
        QTRY_VERIFY(!help->property("opened").toBool());
        QObject* gate = root->findChild<QObject*>(QStringLiteral("parentGate"));
        QTRY_VERIFY(gate->property("opened").toBool()); // erst die PIN
        QMetaObject::invokeMethod(gate, "close");
        QTRY_VERIFY(!gate->property("opened").toBool());
        root->setProperty("inLab", true);
    }

    // Kurzanleitung neben der Hilfe: Knopf auf der Startseite oeffnet sie mit Bild der eigenen Sprache; von dort geht
    // es in die ausfuehrliche Hilfe
    void quickGuide() {
        QObject* root = m_engine->rootObjects().first();
        QObject* guide = root->findChild<QObject*>(QStringLiteral("quickGuide"));
        QObject* help = root->findChild<QObject*>(QStringLiteral("helpDialog"));
        QVERIFY(guide && help);
        root->setProperty("inLab", false);
        QTRY_VERIFY(item(QStringLiteral("home"))->isVisible());
        QVERIFY(item(QStringLiteral("quickGuideButton"))->isVisible()); // auch oben neben dem Fragezeichen
        QQuickItem* button = item(QStringLiteral("homeQuickGuide"));
        QVERIFY(button && button->isVisible());
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(button));
        QTRY_VERIFY(guide->property("opened").toBool());
        QQuickItem* image = item(QStringLiteral("quickGuideImage"));
        QVERIFY(image);
        QTRY_COMPARE(image->property("status").toInt(), 1); // Image.Ready: Bild ist in den Ressourcen
        QMetaObject::invokeMethod(guide, "close");
        QTRY_VERIFY(!guide->property("opened").toBool());
        QMetaObject::invokeMethod(guide, "helpRequested");
        QTRY_VERIFY(help->property("opened").toBool());
        QMetaObject::invokeMethod(help, "close");
        QTRY_VERIFY(!help->property("opened").toBool());
        root->setProperty("inLab", true);
    }

    // Werkzeugkasten: verdeckte Reiter rechts zeigt ein Pfeil; Antippen rollt die Leiste, dann zeigt er nach links
    void toolboxTabArrows() {
        QObject* box = m_engine->rootObjects().first()->findChild<QObject*>(QStringLiteral("toolbox"));
        QMetaObject::invokeMethod(box, "open");
        QTRY_VERIFY(box->property("opened").toBool());
        box->findChild<QObject*>(QStringLiteral("toolboxTabs"))->setProperty("currentIndex", 0);
        QQuickItem* right = item(QStringLiteral("tabsMoreRight"));
        QVERIFY(right);
        QTRY_VERIFY(right->isVisible());
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(right));
        QTRY_VERIFY(!right->isVisible());
        QTRY_VERIFY(item(QStringLiteral("tabsMoreLeft"))->isVisible());
        QMetaObject::invokeMethod(box, "close");
        QTRY_VERIFY(!box->property("opened").toBool());
    }

    // Kaputtes Bauteil: Tick meldet sich, »Zur Fehlersuche« oeffnet sie; antippen daneben schliesst den Hinweis
    void troublePopupOnDestroyed() {
        m_settings->setUnlockAll(true);
        m_lab->startFreeLab();
        QObject* popup = m_engine->rootObjects().first()->findChild<QObject*>(QStringLiteral("troublePopup"));
        QVERIFY(popup);
        QVERIFY(m_lab->addPart(QStringLiteral("led_rot"), QStringLiteral("B3"), false, false)); // B3-B4, ohne Vorwiderstand
        QVERIFY(m_lab->addWire(QStringLiteral("P:+9V"), QStringLiteral("B3"), QStringLiteral("rot")));
        QVERIFY(m_lab->addWire(QStringLiteral("B4"), QStringLiteral("P:0V"), QStringLiteral("schwarz")));
        m_lab->setMainSwitch(true);
        QTRY_VERIFY_WITH_TIMEOUT(popup->property("opened").toBool(), 10000);
        QVERIFY(popup->property("message").toString().contains(QStringLiteral("D1")));
        // daneben tippen schliesst
        QTest::mouseClick(m_window, Qt::LeftButton, {}, QPoint(m_window->width() / 2, 290));
        QTRY_VERIFY(!popup->property("opened").toBool());
        // erneut zeigen und den Weg zur Fehlersuche nehmen
        QMetaObject::invokeMethod(popup, "showDestroyed", Q_ARG(QVariant, QStringLiteral("LED rot (D1)")));
        QTRY_VERIFY(popup->property("opened").toBool());
        QTest::qWait(100);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("troubleFinder"))));
        QObject* box = m_engine->rootObjects().first()->findChild<QObject*>(QStringLiteral("toolbox"));
        QTRY_VERIFY(box->property("opened").toBool());
        QCOMPARE(box->findChild<QObject*>(QStringLiteral("toolboxTabs"))->property("currentIndex").toInt(), 4);
        QTRY_VERIFY(item(QStringLiteral("finding_0")) != nullptr);
        m_lab->setMainSwitch(false);
        QMetaObject::invokeMethod(box, "close");
        QTRY_VERIFY(!box->property("opened").toBool());
        m_lab->clearBoard();
    }

    // Breite der Grundplatte automatisch: Lernpfad 8, freies Labor 12, Messgeraet daneben 10, im Pult 12
    void autoBoardColumns() {
        m_settings->setUnlockAll(true);
        m_settings->setMeterDocked(false);
        m_settings->setAutoBoardColumns(true);
        QTRY_VERIFY(m_lab->autoColumns());
        QVERIFY(m_lab->startExperiment(QStringLiteral("volta1.1.1")));
        QCOMPARE(m_lab->boardColumns(), 8);
        m_lab->setMeterVisible(true);
        QCOMPARE(m_lab->boardColumns(), 10);
        m_settings->setMeterDocked(true);
        QTRY_COMPARE(m_lab->boardColumns(), 12);
        m_lab->setMeterVisible(false);
        QCOMPARE(m_lab->boardColumns(), 8);
        m_lab->startFreeLab();
        QCOMPARE(m_lab->boardColumns(), 12);
        m_lab->setMeterVisible(true); // im Pult
        QCOMPARE(m_lab->boardColumns(), 12);
        m_settings->setMeterDocked(false);
        QTRY_COMPARE(m_lab->boardColumns(), 10);
        m_lab->setMeterVisible(false);
        QCOMPARE(m_lab->boardColumns(), 12);
        // Vorlage breiter als 8 Spalten: passt die Breite an
        QVERIFY(m_lab->startExperiment(QStringLiteral("volta1.4.4")));
        QVERIFY(m_lab->boardColumns() >= 8);
        m_settings->setAutoBoardColumns(false);
        QTRY_VERIFY(!m_lab->autoColumns());
        m_lab->startFreeLab();
    }

    // Kontaktfelder silbern oder messingfarben (Einstellung, Menue und Theme folgen)
    void silverFields() {
        QObject* item = m_window->findChild<QObject*>(QStringLiteral("settingSilverFields"));
        QVERIFY(item);
        QVERIFY(item->property("checked").toBool());
        m_settings->setSilverFields(false);
        QTRY_VERIFY(!item->property("checked").toBool());
        m_settings->setSilverFields(true);
        QTRY_VERIFY(item->property("checked").toBool());
    }

    // Summer und Piezo-Schallgeber klingen beide (gleiches Modell »buzzer«)
    void piezoSounds() {
        m_settings->setUnlockAll(true);
        m_lab->startFreeLab();
        QVERIFY(m_lab->addPart(QStringLiteral("piezo"), QStringLiteral("B3"), false, false)); // plus B3, minus B4
        QVERIFY(m_lab->addWire(QStringLiteral("P:+9V"), QStringLiteral("B3"), QStringLiteral("rot")));
        QVERIFY(m_lab->addWire(QStringLiteral("B4"), QStringLiteral("P:0V"), QStringLiteral("schwarz")));
        m_lab->setMainSwitch(true);
        QTRY_COMPARE(m_lab->soundingParts(), 1);
        m_lab->setMainSwitch(false);
        QTRY_COMPARE(m_lab->soundingParts(), 0);
        m_lab->clearBoard();
    }

    // Aufgabe der Lehrkraft: Code eintippen -> Aufgabe mit Experimenten; Link mit eigenem Aufbau oeffnet ihn im Labor
    void teacherTask() {
        QObject* root = m_engine->rootObjects().first();
        const QStringList ids{QStringLiteral("volta1.2.4"), QStringLiteral("volta1.3.1")};
        const QString code = m_lab->makeTaskCode(ids);
        QCOMPARE(code.size(), 9);
        QObject* dlg = root->findChild<QObject*>(QStringLiteral("taskDialog"));
        QVERIFY(dlg);
        QMetaObject::invokeMethod(dlg, "openTask");
        QTRY_VERIFY(dlg->property("opened").toBool());
        QQuickItem* field = item(QStringLiteral("taskCodeField"));
        QVERIFY(field && field->isVisible());
        field->setProperty("text", QString(code).toLower()); // klein getippt
        QTest::qWait(100);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("taskCodeOpen"))));
        QTRY_VERIFY(m_lab->task().value(QStringLiteral("ok")).toBool());
        QTRY_VERIFY(item(QStringLiteral("taskExperiment_1")) && item(QStringLiteral("taskExperiment_1"))->isVisible());
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("taskExperiment_1"))));
        QTRY_COMPARE(m_lab->experimentId(), QStringLiteral("volta1.3.1"));
        QTRY_VERIFY(!dlg->property("opened").toBool());
        // eigener Aufbau mit Text: nur als Link (kein Code)
        m_lab->startFreeLab();
        QVERIFY(m_lab->addPart(QStringLiteral("widerstand"), QStringLiteral("B3"), false, false));
        const QString link = m_lab->makeTaskLink({}, QStringLiteral("Miss den Widerstand"), true, QStringLiteral("Messen"));
        QVERIFY(link.contains(QStringLiteral("?d=")));
        m_lab->clearBoard();
        QVERIFY(m_lab->acceptTask(link));
        QVERIFY(m_lab->task().value(QStringLiteral("hasBoard")).toBool());
        QVERIFY(m_lab->startTaskBoard());
        QCOMPARE(m_lab->parts().size(), 1);
        QCOMPARE(m_lab->title(), QStringLiteral("Messen"));
        m_lab->clearTask();
        QVERIFY(!m_lab->task().value(QStringLiteral("ok")).toBool());
        m_lab->clearBoard();
    }

    // PIN-Abfrage per Tastatur (Rechner): Ziffern, Ruecktaste, Eingabe
    void parentGateKeyboard() {
        QObject* root = m_engine->rootObjects().first();
        QObject* gate = root->findChild<QObject*>(QStringLiteral("parentGate"));
        QVERIFY(gate);
        auto typeKeys = [this](const char* digits) {
            for (const char* c = digits; *c; ++c) QTest::keyClick(m_window, *c);
        };
        QMetaObject::invokeMethod(gate, "ask", Q_ARG(QVariant, QVariant()));
        QTRY_VERIFY(gate->property("opened").toBool());
        QTest::qWait(100);
        QTest::keyClick(m_window, Qt::Key_9);
        QCOMPARE(gate->property("pin").toString(), QStringLiteral("9"));
        QTest::keyClick(m_window, Qt::Key_Backspace);
        QCOMPARE(gate->property("pin").toString(), QString());
        typeKeys("4321");
        QTest::keyClick(m_window, Qt::Key_Return);
        QTRY_VERIFY(gate->property("wrong").toBool()); // falsche PIN, Abfrage bleibt offen
        QVERIFY(gate->property("opened").toBool());
        typeKeys("1234");
        QTest::keyClick(m_window, Qt::Key_Enter);
        QTRY_VERIFY(!gate->property("opened").toBool()); // Start-PIN
        // Abfrage oeffnet, waehrend ein anderes Fenster noch ausblendet (wie Info -> Webseite): endet das Ausblenden
        // danach, gibt Qt den Fokus an das vorher fokussierte Element - die Abfrage muss ihn sich zurueckholen
        QObject* box = root->findChild<QObject*>(QStringLiteral("toolbox"));
        QVERIFY(box);
        QMetaObject::invokeMethod(box, "open");
        QTRY_VERIFY(box->property("opened").toBool());
        QMetaObject::invokeMethod(box, "close");
        QMetaObject::invokeMethod(gate, "ask", Q_ARG(QVariant, QVariant()));
        QTRY_VERIFY(gate->property("opened").toBool());
        QTRY_VERIFY(!box->property("visible").toBool()); // Werkzeugkasten ganz ausgeblendet
        QTest::qWait(50);
        QTest::keyClick(m_window, Qt::Key_7);
        QCOMPARE(gate->property("pin").toString(), QStringLiteral("7"));
        QTest::keyClick(m_window, Qt::Key_Escape);
        QTRY_VERIFY(!gate->property("opened").toBool());
    }

    // Doppeltippen dreht ein Bauteil (Abkuerzung zum Menue »Drehen«), rueckgaengig machbar
    void doubleTapRotatesPart() {
        m_settings->setUnlockAll(true);
        m_lab->startFreeLab();
        QVERIFY(m_lab->addPart(QStringLiteral("widerstand"), QStringLiteral("B3"), false, false)); // waagerecht
        auto vertical = [this] { return m_lab->parts().first().toMap().value(QStringLiteral("vertical")).toBool(); };
        QVERIFY(!vertical());
        QTRY_VERIFY(item(QStringLiteral("part_R1")) != nullptr);
        QTest::qWait(100);
        // Mitte des Bauteils: zwischen seinen Feldern B3 und B4
        const QPoint body = (centerOf(item(QStringLiteral("field_B3"))) + centerOf(item(QStringLiteral("field_B4")))) / 2;
        // zwei Tipps binnen 400 ms Echtzeit (PartItem misst selbst): kein qWait dazwischen, das dauert unter Last laenger
        QTest::mouseClick(m_window, Qt::LeftButton, {}, body);
        QCoreApplication::processEvents();
        QTest::mouseClick(m_window, Qt::LeftButton, {}, body);
        QTRY_VERIFY(vertical());
        m_lab->undo();
        QVERIFY(!vertical());
        m_lab->clearBoard();
    }

    // Lang gestecktes Bauteil (drei Felder): in der Mitte greifen verschiebt das Bauteil, statt eine Leitung zu ziehen
    void longPartCanBeGrabbed() {
        m_settings->setUnlockAll(true);
        m_lab->startFreeLab();
        QVERIFY(m_lab->addPart(QStringLiteral("widerstand"), QStringLiteral("B3"), false, true)); // B3-B5
        QTRY_VERIFY(item(QStringLiteral("part_R1")) != nullptr);
        QTest::qWait(100);
        const int wires = m_lab->wires().size();
        drag(centerOf(item(QStringLiteral("field_B4"))), centerOf(item(QStringLiteral("field_D4"))));
        QTRY_COMPARE(m_lab->parts().first().toMap().value(QStringLiteral("start")).toString(), QStringLiteral("D3"));
        QCOMPARE(m_lab->wires().size(), wires); // keine Leitung
        m_lab->clearBoard();
    }

    // Fotoimport ins Forscherheft: verkleinert, als JPEG neu gespeichert (ohne Kameradaten), entfernbar, beim
    // Zuruecksetzen geloescht
    void journalPhotos() {
        QTemporaryDir dir;
        const QString source = dir.filePath(QStringLiteral("kamera.png"));
        QImage big(3200, 2400, QImage::Format_RGB32);
        big.fill(Qt::darkGreen);
        QVERIFY(big.save(source));
        services::JournalStore store(dir.filePath(QStringLiteral("profil/forscherheft.json")));
        const QString name = store.addPhoto(QStringLiteral("volta1.2.4"), source);
        QVERIFY(!name.isEmpty());
        const QString saved = store.photoDir() + QLatin1Char('/') + name;
        QVERIFY(QFile::exists(saved));
        {
            QImageReader reader(saved); // eigener Block: Windows sperrt die Datei, solange sie offen ist
            QCOMPARE(reader.format(), QByteArray("jpeg"));
            QCOMPARE(reader.size(), QSize(1600, 1200));
        }
        QCOMPARE(store.entry(QStringLiteral("volta1.2.4")).value(QStringLiteral("fotos")).toArray().size(), 1);
        QVERIFY(store.addPhoto(QStringLiteral("volta1.2.4"), dir.filePath(QStringLiteral("gibtsnicht.jpg"))).isEmpty());
        QVERIFY(store.removePhoto(QStringLiteral("volta1.2.4"), name));
        QVERIFY(!QFile::exists(saved));
        QVERIFY(!store.addPhoto(QStringLiteral("volta1.2.4"), source).isEmpty());
        QVERIFY(store.reset());
        QVERIFY(!QDir(store.photoDir()).exists());
    }

    // Praesentationsmodus: alle Stufen offen, nichts wird gespeichert; danach ist der Stand wie vorher
    void presentationKeepsProfileData() {
        QTemporaryDir dir;
        LabController lab(QStringLiteral(VOLTA_CONTENT_DIR));
        lab.useDataRoot(dir.path());
        QVERIFY(lab.importProgressLink(QStringLiteral("volta1://fortschritt?d=1.1")) == 1);
        lab.startFreeLab();
        QVERIFY(lab.addPart(QStringLiteral("gluehlampe"), QStringLiteral("B3"), false, false));
        lab.saveSession();
        const int stageBefore = lab.unlockedStage();
        lab.setPresentation(true);
        QVERIFY(lab.unlockedStage() > 10);
        QVERIFY(!lab.isDone(QStringLiteral("volta1.1.1"))); // leerer Stand in der Praesentation
        QVERIFY(lab.importProgressLink(QStringLiteral("volta1://fortschritt?d=2.1")) == 1);
        lab.startFreeLab();
        QVERIFY(lab.addPart(QStringLiteral("led_rot"), QStringLiteral("C3"), false, false));
        lab.setPresentation(false);
        QVERIFY(lab.isDone(QStringLiteral("volta1.1.1")));
        QVERIFY(!lab.isDone(QStringLiteral("volta1.2.1"))); // nichts aus der Praesentation uebernommen
        QCOMPARE(lab.unlockedStage(), stageBefore);
        QVERIFY(lab.resumeSession());
        QCOMPARE(lab.parts().size(), 1); // der eigene Aufbau, nicht der aus der Praesentation
        QCOMPARE(lab.parts().first().toMap().value(QStringLiteral("type")).toString(), QStringLiteral("gluehlampe"));
    }

    // Code der Lehrkraft: zwei Haelften mit festem Strich, nach vier Zeichen geht es im zweiten Feld weiter
    void taskCodeTyping() {
        QObject* dlg = m_engine->rootObjects().first()->findChild<QObject*>(QStringLiteral("taskDialog"));
        m_lab->clearTask();
        QMetaObject::invokeMethod(dlg, "openTask");
        QTRY_VERIFY(dlg->property("opened").toBool());
        QQuickItem* first = item(QStringLiteral("taskCodeField"));
        QQuickItem* second = item(QStringLiteral("taskCodeField2"));
        QTRY_VERIFY(first && first->isVisible());
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(first));
        const QString code = m_lab->makeTaskCode({QStringLiteral("volta1.1.1")}).remove(QLatin1Char('-'));
        for (QChar c : code) QTest::keyClick(m_window, c.toLatin1());
        QCOMPARE(first->property("text").toString().toUpper(), code.left(4));
        QCOMPARE(second->property("text").toString().toUpper(), code.mid(4));
        QTest::keyClick(m_window, Qt::Key_Return);
        QTRY_VERIFY(m_lab->task().value(QStringLiteral("ok")).toBool());
        QMetaObject::invokeMethod(dlg, "close");
        m_lab->clearTask();
    }

    // Maskottchen Tick: alle Stimmungsbilder eingebunden, Begruessung auf der Startseite, im Elternbereich
    // abschaltbar - dann bleibt der Text als Kasten stehen
    void tickMascot() {
        for (const char* mood : {"erklaerend", "fragend", "freundlich", "froehlich", "gut", "motivierend",
                                 "nachdenklich", "neugierig", "ueberrascht", "warnend"})
            QVERIFY2(QFile::exists(QStringLiteral(":/tick/tick_%1.png").arg(QLatin1String(mood))), mood);
        QQuickItem* tick = item(QStringLiteral("homeTick"));
        QVERIFY(tick);
        QVERIFY(tick->property("showFigure").toBool());
        QVERIFY(tick->property("text").toString().contains(QStringLiteral("Tick")));
        m_settings->setTickEnabled(false);
        QTRY_VERIFY(!tick->property("showFigure").toBool());
        QVERIFY(tick->height() > 0); // Hinweis bleibt
        m_settings->setTickEnabled(true);
        QTRY_VERIFY(tick->property("showFigure").toBool());
    }

    // Automatisches Speichern: Stand sichern, neuer Controller (Neustart) stellt Aufbau und Ablauf wieder her
    void sessionSaveAndResume() {
        QTemporaryDir dir;
        const QString file = dir.filePath(QStringLiteral("laborstand.json"));
        {
            LabController a(QStringLiteral(VOLTA_CONTENT_DIR));
            a.useSessionFile(file);
            a.setUnlockAll(true);
            QVERIFY(a.startExperiment(QStringLiteral("volta1.1.1")));
            a.chooseGuess(1);
            a.nextHint();
            QVERIFY(a.addPart(QStringLiteral("gluehlampe"), QStringLiteral("C5"), false, false));
            QVERIFY(a.addWire(QStringLiteral("P:+9V"), QStringLiteral("C5"), QStringLiteral("rot")));
            a.setLabOpen(true);
            a.saveSession();
        }
        LabController b(QStringLiteral(VOLTA_CONTENT_DIR));
        b.useSessionFile(file);
        QVERIFY(b.savedLabOpen());
        QVERIFY(b.resumeSession());
        QCOMPARE(b.experimentId(), QStringLiteral("volta1.1.1"));
        QCOMPARE(b.phase(), 2);
        QCOMPARE(b.guess(), 1);
        QCOMPARE(b.hints().size(), 1);
        QCOMPARE(b.session().board().parts.size(), std::size_t(1));
        QCOMPARE(b.session().board().wires.size(), std::size_t(1));
        QVERIFY(!b.session().board().desk.mainSwitch);
        // freies Labor ohne Inhalte bleibt freies Labor
        b.startFreeLab();
        b.setLabOpen(false);
        b.saveSession();
        LabController c(QStringLiteral(VOLTA_CONTENT_DIR));
        c.useSessionFile(file);
        QVERIFY(c.resumeSession());
        QVERIFY(c.freeLab());
        QVERIFY(!c.savedLabOpen());
    }

    // Fehlersuche: Reiter im Werkzeugkasten, Fragen mit Nummer, Marke an der Stelle; Aenderung am Aufbau loescht sie
    void toolboxFaultFinder() {
        m_settings->setUnlockAll(true);
        m_lab->startFreeLab();
        QVERIFY(m_lab->addPart(QStringLiteral("widerstand"), QStringLiteral("B3"), false, false)); // R1 B3-B4
        QVERIFY(m_lab->addWire(QStringLiteral("P:+9V"), QStringLiteral("B3"), QStringLiteral("rot")));
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("toolboxButton"))));
        QObject* box = m_engine->rootObjects().first()->findChild<QObject*>(QStringLiteral("toolbox"));
        QTRY_VERIFY(box->property("opened").toBool());
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("tabCheck"))));
        QQuickItem* checkButton = item(QStringLiteral("checkCircuitButton"));
        QTRY_VERIFY(checkButton && checkButton->isVisible());
        QTest::qWait(50);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(checkButton));
        QTRY_VERIFY(item(QStringLiteral("finding_0")) != nullptr);
        QQuickItem* mark = item(QStringLiteral("checkMark_0"));
        QVERIFY(mark && mark->isVisible());
        // Antippen hebt die Stelle hervor (B4: loses Bein)
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("finding_0"))));
        QTRY_VERIFY(item(QStringLiteral("field_B4"))->property("highlighted").toBool());
        // Aufbau geaendert: Befunde und Marken verschwinden
        QVERIFY(m_lab->addWire(QStringLiteral("B4"), QStringLiteral("P:0V"), QStringLiteral("schwarz")));
        QTRY_VERIFY(findItem(m_window->contentItem(), QStringLiteral("checkMark_0")) == nullptr);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(checkButton));
        QTRY_VERIFY(item(QStringLiteral("finding_0")) != nullptr); // Hauptschalter aus
        QMetaObject::invokeMethod(box, "close");
        QTRY_VERIFY(!box->property("opened").toBool());
        QTRY_VERIFY(findItem(m_window->contentItem(), QStringLiteral("checkMark_0")) == nullptr);
    }

    // Schaltplan: einblenden, Bauteil im Plan antippen waehlt es auf der Grundplatte; Knoten hebt Felder hervor
    void schematicView() {
        m_settings->setUnlockAll(true);
        m_lab->startFreeLab();
        QVERIFY(m_lab->addPart(QStringLiteral("widerstand"), QStringLiteral("B3"), false, false));
        QVERIFY(m_lab->addWire(QStringLiteral("P:+9V"), QStringLiteral("B3"), QStringLiteral("rot")));
        QVERIFY(m_lab->addWire(QStringLiteral("B4"), QStringLiteral("P:0V"), QStringLiteral("schwarz")));
        QQuickItem* toggle = item(QStringLiteral("schematicToggle"));
        QVERIFY(toggle && toggle->isVisible());
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(toggle));
        QQuickItem* panel = item(QStringLiteral("schematicPanel"));
        QTRY_VERIFY(panel->isVisible() && panel->width() > 100);
        QTest::qWait(50);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("schem_R1"))));
        QObject* lab = m_engine->rootObjects().first()->findChild<QObject*>(QStringLiteral("lab"));
        QTRY_COMPARE(lab->property("selectedPart").toString(), QStringLiteral("R1"));
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("node_0"))));
        auto fieldCount = [lab] {
            const QVariant v = lab->property("highlightFields");
            return v.canConvert<QJSValue>() ? v.value<QJSValue>().toVariant().toList().size() : v.toList().size();
        };
        QTRY_VERIFY(fieldCount() > 0);
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(toggle));
        QTRY_VERIFY(!panel->isVisible());
        QTRY_COMPARE(fieldCount(), 0);
    }

    // Einstellungen bleiben ueber Neustarts erhalten (eigene Datei)
    void settingsPersist() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString file = dir.filePath(QStringLiteral("einstellungen.ini"));
        {
            AppSettings s(true, file);
            s.setFontLevel(0);
            s.setBoardColumns(8);
            s.setMeterVisible(true);
            s.setWireColor(QStringLiteral("blau"));
            s.setSchematicVisible(true);
            s.setExperimentPanelCollapsed(true);
            s.setBoardColumns(9);            // ungueltig: bleibt 8
            s.setWireColor(QStringLiteral("pink")); // ungueltig: bleibt blau
        }
        AppSettings again(true, file);
        QCOMPARE(again.fontLevel(), 0);
        QCOMPARE(again.boardColumns(), 8);
        QVERIFY(again.meterVisible());
        QCOMPARE(again.wireColor(), QStringLiteral("blau"));
        QVERIFY(again.schematicVisible());
        QVERIFY(again.experimentPanelCollapsed());
        // neue Aufbauten uebernehmen die gespeicherte Breite und das Messgeraet
        m_settings->setUnlockAll(true);
        m_settings->setBoardColumns(8);
        m_settings->setMeterVisible(true);
        QTRY_COMPARE(m_lab->property("preferredColumns").toInt(), 8);
        m_lab->startFreeLab();
        QCOMPARE(board().columns, 8);
        QVERIFY(board().desk.meterVisible);
        QVERIFY(m_lab->startExperiment(QStringLiteral("volta1.1.1"))); // Vorlage passt in 8 Spalten
        QCOMPARE(board().columns, 8);
        m_settings->setBoardColumns(12);
        m_settings->setMeterVisible(false);
    }

    // Zeitschreiber: Kanal ueber den Controller, Feld bekommt das Schild, Daten kommen an; Leiste fuer Licht und
    // Waerme klappt beim Antippen aus und von selbst wieder ein
    void recorderAndFlyout() {
        m_settings->setUnlockAll(true);
        m_lab->startFreeLab();
        QVERIFY(m_lab->recorderAvailable());
        QVERIFY(m_lab->addWire(QStringLiteral("P:+9V"), QStringLiteral("D7"), QStringLiteral("rot")));
        m_lab->setRecorderChannel(0, QStringLiteral("D7"));
        QTRY_COMPARE(item(QStringLiteral("field_D7"))->property("recorderChannel").toInt(), 0);
        m_lab->setMainSwitch(true);
        auto last = [this] {
            const QVariantMap ch0 = m_lab->recorderData(50).value(QStringLiteral("channels")).toList().at(0).toMap();
            return ch0.value(QStringLiteral("any")).toBool() ? ch0.value(QStringLiteral("last")).toDouble() : -1.0;
        };
        QTRY_VERIFY(last() > 8.0); // +9V
        m_lab->setMainSwitch(false);
        // Leiste fuer Licht und Waerme (Fotowiderstand verfuegbar): eingeklappt, Antippen klappt aus
        QQuickItem* bar = item(QStringLiteral("environmentBar"));
        QVERIFY(bar && bar->isVisible());
        QVERIFY(!bar->property("expanded").toBool());
        QTest::mouseClick(m_window, Qt::LeftButton, {}, centerOf(item(QStringLiteral("flyoutHandle"))));
        QTRY_VERIFY(bar->property("expanded").toBool());
        QTRY_VERIFY(item(QStringLiteral("roomLightButton"))->isVisible());
        bar->setProperty("collapseDelay", 50);
        QTest::mouseMove(m_window, centerOf(item(QStringLiteral("field_A6")))); // Maus weg von der Leiste
        QTRY_VERIFY(!bar->property("expanded").toBool()); // klappt von selbst ein
        m_lab->setRecorderChannel(0, QString());
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

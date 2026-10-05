// Oberflaechentests (Qt Quick, ohne sichtbares Fenster): echte Bedienung mit simulierter Maus und Tastatur
// gegen die Experimentansicht. Prueft das Zusammenspiel QML <-> LabController <-> LabSession.
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWindow>
#include <QtQml/qqmlextensionplugin.h>
#include <QtTest>

#include "lab_controller.hpp"

Q_IMPORT_QML_PLUGIN(VoltaPlugin)

using namespace volta;

class UiTest : public QObject {
    Q_OBJECT

private:
    std::unique_ptr<LabController> m_lab;
    std::unique_ptr<QQmlApplicationEngine> m_engine;
    QQuickWindow* m_window = nullptr;

    // Repeater-Elemente haengen im visuellen Baum (childItems), nicht zwingend im QObject-Baum
    static QQuickItem* findItem(QQuickItem* root, const QString& name) {
        if (root->objectName() == name) return root;
        for (QQuickItem* child : root->childItems())
            if (QQuickItem* found = findItem(child, name)) return found;
        return nullptr;
    }

    QQuickItem* item(const QString& name) const {
        QQuickItem* found = findItem(m_window->contentItem(), name);
        if (!found) qWarning() << "Element nicht gefunden:" << name;
        return found;
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

private slots:
    void initTestCase() {
        m_lab = std::make_unique<LabController>(QStringLiteral(VOLTA_CONTENT_DIR));
        m_engine = std::make_unique<QQmlApplicationEngine>();
        m_engine->rootContext()->setContextProperty(QStringLiteral("labController"), m_lab.get());
        m_engine->loadFromModule("Volta", "Main");
        QVERIFY(!m_engine->rootObjects().isEmpty());
        m_window = qobject_cast<QQuickWindow*>(m_engine->rootObjects().first());
        QVERIFY(m_window);
        QVERIFY(QTest::qWaitForWindowExposed(m_window));
        QTRY_COMPARE(board().parts.size(), std::size_t(2)); // Vorlage 2.4 geladen
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

    void cleanupTestCase() {
        m_engine.reset();
        m_lab.reset();
    }
};

QTEST_MAIN(UiTest)
#include "test_ui.moc"

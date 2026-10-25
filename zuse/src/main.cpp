// Zuse-Prototyp Z0 (docs/Konzept_Zuse.md): Stufen mit Geschichte, Raetselkisten, Werkbank und Werkzeugkiste.
// Offline, keine Datenerhebung: eigene Chips und Entwuerfe liegen nur in AppDataLocation.
// Entwicklerhilfen: ZUSE_SCREENSHOT=<bild.png> (Bildschirmfoto statt Bedienung), ZUSE_SIZE=<b>x<h>,
// ZUSE_DATA_DIR=<ordner> (Ablage), ZUSE_LANG=de|en, ZUSE_DEMO=<n> (n Stufen loesen und zur letzten gehen),
// ZUSE_DEMO_GUESS=10 (Vermutung je Zeile), ZUSE_DEMO_BOX=<i> (Raetselkiste i einsetzen), ZUSE_DEMO_POPUP=stagePicker|numbers|toolbox (Fenster oeffnen),
// ZUSE_DEMO_ON=A,B (Eingaenge einschalten), ZUSE_DEMO_INSIDE=x1/o1 (Blick hinein entlang der Teil-IDs),
// ZUSE_DEMO_PRESS=x,y[,los] (Maus druecken, mit »los« wieder loslassen), ZUSE_SYMBOLS=din|us (Schaltzeichen),
// ZUSE_PAGE=home|lab (Startseite bzw. Experiment; mit ZUSE_DEMO sonst lab), ZUSE_DEMO_FREE=1 (Freies Labor),
// ZUSE_DEMO_SHOW=<n> (Vorfuehrung des Experiments n).
#include <QDir>
#include <QGuiApplication>
#include <QIcon>
#include <QLocale>
#include <QMouseEvent>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTimer>

#include "workbench_controller.hpp"

int main(int argc, char* argv[]) {
    QQuickStyle::setStyle(QStringLiteral("Basic")); // eigene Gestaltung; Release liefert nur diesen Stil aus
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName(QStringLiteral("DC Software Engineering"));
    QGuiApplication::setApplicationName(QStringLiteral("Zuse 1"));
    QIcon icon;
    icon.addFile(QStringLiteral(":/zuse/zuse1_64.png"));
    icon.addFile(QStringLiteral(":/zuse/zuse1_256.png"));
    QGuiApplication::setWindowIcon(icon);

    QString language = qEnvironmentVariable("ZUSE_LANG");
    if (language.isEmpty()) language = QLocale().language() == QLocale::German ? "de" : "en";
    QString dataDir = qEnvironmentVariable("ZUSE_DATA_DIR");
    if (dataDir.isEmpty()) dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    zuse::WorkbenchController workbench(QStringLiteral(":/zuse/stufen.json"), dataDir, language);
    // Vorfuehrung: die ersten n Stufen mit der Musterloesung durchspielen
    const int demo = qEnvironmentVariableIntValue("ZUSE_DEMO");
    for (int s = 0; s < demo && s < workbench.stages().size(); ++s) {
        workbench.selectStage(s);
        if (workbench.isDone(workbench.stageKuerzel()) || s + 1 >= demo) continue;
        if (workbench.stageKind() == QStringLiteral("entdecken")) {
            workbench.skipGuess();
            for (const QVariant& b : workbench.boxes())
                if (!b.toMap().value("symbol").toString().isEmpty()) workbench.selectBox(b.toMap().value("index").toInt());
            workbench.confirmBox();
        } else if (workbench.stageKind() == QStringLiteral("zahlen") && workbench.stageSequence()) {
            workbench.playSteps(); // Rechenschritte am Bedienfeld nachspielen
        } else if (workbench.stageKind() == QStringLiteral("zahlen")) { // jede Aufgabe an den Schaltern einstellen
            const QStringList in = workbench.stageInputs();
            for (const QVariant& t : workbench.tasks())
                for (qsizetype i = 0; i < in.size(); ++i)
                    workbench.setInput(in[i], (t.toMap().value("value").toInt() >> (in.size() - 1 - i)) & 1);
        } else {
            workbench.showSolution();
            workbench.check();
            workbench.createChip();
        }
    }

    if (qEnvironmentVariableIsSet("ZUSE_SYMBOLS")) workbench.setSymbolStyle(qEnvironmentVariable("ZUSE_SYMBOLS"));
    QString startPage = qEnvironmentVariable("ZUSE_PAGE");
    if (startPage.isEmpty()) startPage = demo > 0 ? QStringLiteral("lab") : QStringLiteral("home");
    if (qEnvironmentVariableIsSet("ZUSE_DEMO_FREE")) {
        workbench.startFreeLab();
        startPage = QStringLiteral("lab");
    }
    if (qEnvironmentVariableIsSet("ZUSE_DEMO_SHOW") && workbench.startDemo(qEnvironmentVariableIntValue("ZUSE_DEMO_SHOW")))
        startPage = QStringLiteral("lab");
    const QString guess = qEnvironmentVariable("ZUSE_DEMO_GUESS"); // z. B. "10": Vermutung je Zeile, "-" = offen
    for (qsizetype r = 0; r < guess.size(); ++r)
        for (int t = guess[r] == '1' ? 1 : guess[r] == '0' ? 2 : 0; t > 0; --t) workbench.toggleGuess(int(r));
    if (qEnvironmentVariableIsSet("ZUSE_DEMO_BOX")) {
        workbench.skipGuess();
        workbench.selectBox(qEnvironmentVariableIntValue("ZUSE_DEMO_BOX"));
    }
    for (const QString& name : qEnvironmentVariable("ZUSE_DEMO_ON").split(',', Qt::SkipEmptyParts))
        workbench.toggleInput(name);
    for (const QString& id : qEnvironmentVariable("ZUSE_DEMO_INSIDE").split('/', Qt::SkipEmptyParts))
        workbench.lookInside(id);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("workbench"), &workbench);
    engine.rootContext()->setContextProperty(QStringLiteral("startPage"), startPage);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); },
                     Qt::QueuedConnection);
    engine.loadFromModule("Zuse", "Main");
    if (engine.rootObjects().isEmpty()) return 1;
    auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());

    const QStringList size = qEnvironmentVariable("ZUSE_SIZE").split('x');
    if (window && size.size() == 2) {
        window->setWidth(size[0].toInt());
        window->setHeight(size[1].toInt());
    }
    const QString popup = qEnvironmentVariable("ZUSE_DEMO_POPUP");
    if (window && !popup.isEmpty())
        if (QObject* p = window->findChild<QObject*>(popup)) QMetaObject::invokeMethod(p, "open");
    // ZUSE_DEMO_PRESS=x,y[,los]: Maus an Fensterposition druecken (und wieder loslassen) – prueft z. B. den Taster
    const QStringList press = qEnvironmentVariable("ZUSE_DEMO_PRESS").split(',');
    if (window && press.size() >= 2) {
        QTimer::singleShot(600, window, [window, press] {
            const QPointF at(press[0].toDouble(), press[1].toDouble());
            QMouseEvent down(QEvent::MouseButtonPress, at, window->mapToGlobal(at), Qt::LeftButton, Qt::LeftButton,
                             Qt::NoModifier);
            QGuiApplication::sendEvent(window, &down);
            if (press.size() >= 3) {
                QMouseEvent up(QEvent::MouseButtonRelease, at, window->mapToGlobal(at), Qt::LeftButton, Qt::NoButton,
                               Qt::NoModifier);
                QGuiApplication::sendEvent(window, &up);
            }
        });
    }
    const QString shot = qEnvironmentVariable("ZUSE_SCREENSHOT");
    if (window && !shot.isEmpty()) {
        QTimer::singleShot(1200, &app, [window, shot] {
            window->grabWindow().save(shot);
            QCoreApplication::quit();
        });
    }
    return app.exec();
}

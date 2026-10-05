// Einstiegspunkt der Qt-Anwendung. Modell, Dienste und Lernpfad folgen gemaess docs/architecture.md.
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTimer>
#include <QtQml/qqmlextensionplugin.h>

#include "lab_controller.hpp"
#include "volta/core/version.hpp"

Q_IMPORT_QML_PLUGIN(VoltaPlugin)

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName(QStringLiteral("DC Software Engineering"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("dc-software-engineering.com"));
    QGuiApplication::setApplicationName(QStringLiteral(VOLTA_APP_NAME));
    QGuiApplication::setApplicationVersion(QStringLiteral(VOLTA_VERSION));

    // Inhaltspakete: in Phase 0 direkt aus dem Quellbaum; Einbettung als Ressource folgt mit der Paketierung.
    volta::LabController lab(QStringLiteral(VOLTA_CONTENT_DIR));

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("labController"), &lab);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);
    engine.loadFromModule("Volta", "Main");

    // Entwicklerhilfe: VOLTA_SCREENSHOT=<datei.png> speichert nach 3 s ein Bild des Fensters und beendet die App
    // (Sichtpruefung ohne Bedienung). Optional VOLTA_SCREENSHOT_MAINSWITCH=1 schaltet vorher den Hauptschalter ein.
    // Entwicklerhilfe: VOLTA_DEMO_GALLERY=1 legt eine Mustertafel mit allen Bauteiltypen im Betrieb an (Sichtpruefung
    // der Gestaltung: Leuchten, Drehen, Toenen, Ueberlast, Zerstoerung).
    if (qEnvironmentVariableIntValue("VOLTA_DEMO_GALLERY") == 1) {
        QTimer::singleShot(200, &lab, [&lab] {
            lab.loadExperiment(QStringLiteral("volta1.2.4"));
            lab.removePart(QStringLiteral("D1"));
            lab.addPart(QStringLiteral("led_gruen"), QStringLiteral("B4"), true, false); // 470 Ohm -> leuchtet
            lab.addWire(QStringLiteral("C4"), QStringLiteral("P:0V"), QStringLiteral("schwarz"));
            struct P { const char* type; const char* field; bool vertical; bool longSpan; };
            const P parts[] = {{"gluehlampe", "A1", false, false}, {"motor", "A4", false, true},
                               {"summer", "C1", false, false},     {"taster", "C6", false, false},
                               {"reedkontakt", "D1", false, false}, {"wechselschalter", "D4", false, false},
                               {"kondensator", "E1", false, false}, {"elko", "E3", false, false},
                               {"diode_si", "E6", false, false},     {"transistor_npn", "B6", false, false},
                               {"led_blau", "D7", true, false},      {"led_rot", "A7", true, false}};
            for (const auto& p : parts)
                lab.addPart(QString::fromLatin1(p.type), QString::fromLatin1(p.field), p.vertical, p.longSpan);
            const char* wires[][2] = {{"P:+9V", "A1"}, {"A2", "P:0V"}, {"P:+9V", "A4"}, {"A6", "P:0V"},
                                      {"P:+9V", "C1"}, {"C2", "P:0V"}, {"P:+9V", "A7"}, {"B7", "P:0V"}};
            for (const auto& w : wires)
                lab.addWire(QString::fromLatin1(w[0]), QString::fromLatin1(w[1]), QStringLiteral("blau"));
            lab.setPartClosed(QStringLiteral("RK1"), true);
            lab.setMainSwitch(true);
        });
    }

    const QString shot = qEnvironmentVariable("VOLTA_SCREENSHOT");
    if (!shot.isEmpty()) {
        if (qEnvironmentVariableIntValue("VOLTA_SCREENSHOT_MAINSWITCH") == 1)
            QTimer::singleShot(500, &lab, [&lab] { lab.setMainSwitch(true); });
        QTimer::singleShot(3000, &app, [&engine, shot] {
            const auto roots = engine.rootObjects();
            if (auto* window = roots.isEmpty() ? nullptr : qobject_cast<QQuickWindow*>(roots.first()))
                window->grabWindow().save(shot);
            QCoreApplication::quit();
        });
    }
    return QGuiApplication::exec();
}

// Einstiegspunkt der Qt-Anwendung. Modell, Dienste und Lernpfad folgen gemaess docs/architecture.md.
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTimer>

#include "lab_controller.hpp"
#include "volta/core/version.hpp"

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

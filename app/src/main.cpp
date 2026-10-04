// Einstiegspunkt der Qt-Anwendung. Modell, Dienste und Lernpfad folgen gemaess docs/architecture.md.
#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "volta/core/version.hpp"

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName(QStringLiteral("DC Software Engineering"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("dc-software-engineering.com"));
    QGuiApplication::setApplicationName(QStringLiteral(VOLTA_APP_NAME));
    QGuiApplication::setApplicationVersion(QStringLiteral(VOLTA_VERSION));

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);
    engine.loadFromModule("Volta", "Main");
    return QGuiApplication::exec();
}

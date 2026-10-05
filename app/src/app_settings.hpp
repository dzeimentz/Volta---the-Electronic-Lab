#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace volta {

// Einstellungen der Oberflaeche (Einstellungsmenue oben rechts): lokal gespeichert in einstellungen.ini unter
// QStandardPaths::AppConfigLocation (keine Datenerhebung). Bedienzustaende des Aufbaus gehoeren nicht hierher.
class AppSettings : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(AppSettings)
    QML_UNCREATABLE("Wird von main.cpp bereitgestellt")
    Q_PROPERTY(bool showCurrentFlow READ showCurrentFlow WRITE setShowCurrentFlow NOTIFY changed)
    Q_PROPERTY(bool reduceMotion READ reduceMotion WRITE setReduceMotion NOTIFY changed)
    Q_PROPERTY(bool showSplash READ showSplash WRITE setShowSplash NOTIFY changed)
    Q_PROPERTY(bool splashAvailable READ splashAvailable CONSTANT) // nur Desktop-Ausgabe

public:
    explicit AppSettings(bool persist = true, QObject* parent = nullptr);

    bool showCurrentFlow() const { return m_showCurrentFlow; }
    void setShowCurrentFlow(bool on);
    bool reduceMotion() const { return m_reduceMotion; }
    void setReduceMotion(bool on);
    bool showSplash() const { return m_showSplash; }
    void setShowSplash(bool on);
    static bool splashAvailable();

signals:
    void changed();

private:
    void save(const QString& key, bool value);

    bool m_persist;
    bool m_showCurrentFlow = true;
    bool m_reduceMotion = false;
    bool m_showSplash = true;
};

} // namespace volta

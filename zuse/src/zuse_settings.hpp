#pragma once

// Einstellungen von Zuse 1 wie bei Volta: Anzeigesprache (DE, FR, EN; LU folgt spaeter), Schriftgroesse (drei Stufen,
// groesste ist die Vorgabe) und die Einstellungen des Elternbereichs (PIN, Freigaben, Forscherprofile, Vorfuehrung).
// Gespeichert in <Ablage>/einstellungen.ini fuer das ganze Geraet, nichts verlaesst das Geraet.
#include <QObject>
#include <QString>
#include <QTranslator>
#include <QVariantList>

namespace zuse {

class ZuseSettings : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString language READ language NOTIFY languageChanged)
    Q_PROPERTY(QVariantList languages READ languages CONSTANT)
    Q_PROPERTY(int fontLevel READ fontLevel WRITE setFontLevel NOTIFY fontLevelChanged)
    Q_PROPERTY(qreal fontScale READ fontScale NOTIFY fontLevelChanged)
    Q_PROPERTY(bool zettiVisible READ zettiVisible WRITE setZettiVisible NOTIFY zettiVisibleChanged)
    Q_PROPERTY(bool schematicVisible READ schematicVisible WRITE setSchematicVisible NOTIFY schematicVisibleChanged)
    // Elternbereich
    Q_PROPERTY(bool parentPinIsDefault READ parentPinIsDefault NOTIFY parentChanged)
    Q_PROPERTY(bool unlockAll READ unlockAll WRITE setUnlockAll NOTIFY parentChanged)
    Q_PROPERTY(bool profilesEnabled READ profilesEnabled WRITE setProfilesEnabled NOTIFY parentChanged)
    Q_PROPERTY(bool demoVisible READ demoVisible WRITE setDemoVisible NOTIFY parentChanged)

public:
    // dataDir leer: nichts laden und speichern (Tests); language: Vorgabe, falls noch nichts gewaehlt wurde
    ZuseSettings(const QString& dataDir, const QString& language, QObject* parent = nullptr);
    ~ZuseSettings() override;

    QString language() const { return m_language; }
    QVariantList languages() const; // {code, short, name}
    static bool isAvailable(const QString& code);
    Q_INVOKABLE bool setLanguage(const QString& code);

    int fontLevel() const { return m_fontLevel; }
    void setFontLevel(int level);
    qreal fontScale() const { return 0.8 + 0.1 * m_fontLevel; } // 0,8 / 0,9 / 1,0
    bool zettiVisible() const { return m_zettiVisible; } // Zetti mit Sprechblase im Experiment
    void setZettiVisible(bool on);
    bool schematicVisible() const { return m_schematicVisible; } // Schaltplan neben der Werkbank
    void setSchematicVisible(bool on);

    // PIN der Elternschranke (wie Volta): Start-PIN 1234, eigene PIN 4 bis 8 Ziffern, gespeichert nur als Pruefwert
    // mit Salz; nach fuenf Fehlversuchen 30 s Sperre
    bool parentPinIsDefault() const { return m_pinHash.isEmpty(); }
    Q_INVOKABLE bool checkParentPin(const QString& pin);
    Q_INVOKABLE int parentPinLockSeconds() const;
    Q_INVOKABLE bool setParentPin(const QString& pin);
    static constexpr const char* kDefaultPin = "1234";
    bool unlockAll() const { return m_unlockAll; }            // alle Experimente sofort offen
    void setUnlockAll(bool on);
    bool profilesEnabled() const { return m_profilesEnabled; } // Forscherprofile fuer geteilte Geraete
    void setProfilesEnabled(bool on);
    bool demoVisible() const { return m_demoVisible; }         // Vorfuehrung auf der Startseite
    void setDemoVisible(bool on);

signals:
    void languageChanged(const QString& code);
    void fontLevelChanged();
    void schematicVisibleChanged();
    void zettiVisibleChanged();
    void parentChanged();

private:
    void save() const;
    void installTranslator();

    QString m_file;
    QString m_language = QStringLiteral("de");
    int m_fontLevel = 2;
    bool m_schematicVisible = false;
    bool m_zettiVisible = true;
    bool m_unlockAll = false;
    bool m_profilesEnabled = false;
    bool m_demoVisible = true;
    QString m_pinHash; // leer: Start-PIN
    QString m_pinSalt;
    int m_pinFailures = 0;
    qint64 m_pinLockedUntil = 0; // ms seit Epoche
    static QString hashPin(const QString& salt, const QString& pin);
    void savePinState() const;
    QTranslator m_translator;
    bool m_installed = false;
};

} // namespace zuse

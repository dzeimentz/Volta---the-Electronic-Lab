#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

namespace volta::services {

// Verwaltete Konfiguration fuer Schulen (docs/platforms.md): Vorgaben der Geraeteverwaltung, die die App nur liest.
// Quellen je Plattform:
//  - Android: RestrictionsManager (res/xml/app_restrictions.xml, Managed Google Play / MDM)
//  - Windows: Registry HKLM\Software\DC Software Engineering\Volta1, sonst volta.ini neben der EXE
//  - Linux: /etc/xdg/dc-software-engineering/volta1.ini
//  - Entwicklung/Tests: Umgebungsvariable VOLTA_MANAGED_INI=<Datei> (hat Vorrang)
// Nicht gesetzte Schluessel lassen die Einstellung beim Kind bzw. im Elternbereich.
class ManagedConfig : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool managed READ managed CONSTANT)             // irgendein Schluessel gesetzt
    Q_PROPERTY(QString language READ language CONSTANT)        // "" = nicht vorgegeben
    Q_PROPERTY(bool languageLocked READ languageLocked CONSTANT)
    Q_PROPERTY(int unlockStage READ unlockStage CONSTANT)      // -1 = nicht vorgegeben
    Q_PROPERTY(bool parentArea READ parentArea CONSTANT)       // Elternbereich anzeigen
    Q_PROPERTY(bool photoImport READ photoImport CONSTANT)
    Q_PROPERTY(QString soundMode READ soundMode CONSTANT)      // "" | "aus" | "kopfhoerer" | "normal"
    Q_PROPERTY(int profiles READ profiles CONSTANT)            // -1 = nicht vorgegeben, 0 = aus, sonst Hoechstzahl
    Q_PROPERTY(bool presentation READ presentation CONSTANT)

public:
    explicit ManagedConfig(QObject* parent = nullptr);
    // aus Schluessel-Wert-Paaren (Tests und alle Quellen)
    explicit ManagedConfig(const QVariantMap& values, QObject* parent = nullptr);

    bool managed() const { return m_managed; }
    QString language() const { return m_language; }
    bool languageLocked() const { return m_languageLocked; }
    int unlockStage() const { return m_unlockStage; }
    bool parentArea() const { return m_parentArea; }
    bool photoImport() const { return m_photoImport; }
    QString soundMode() const { return m_soundMode; }
    int profiles() const { return m_profiles; }
    bool presentation() const { return m_presentation; }

    // Rohwerte der Plattformquelle (leer, wenn nichts vorgegeben ist)
    static QVariantMap readPlatform();

private:
    void apply(const QVariantMap& values);

    bool m_managed = false;
    QString m_language;
    bool m_languageLocked = false;
    int m_unlockStage = -1;
    bool m_parentArea = true;
    bool m_photoImport = true;
    QString m_soundMode;
    int m_profiles = -1;
    bool m_presentation = false;
};

} // namespace volta::services

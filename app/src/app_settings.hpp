#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

namespace volta {

// Einstellungen der Oberflaeche, lokal gespeichert in einstellungen.ini unter QStandardPaths::AppConfigLocation
// (keine Datenerhebung). Sie bleiben ueber Neustarts erhalten: Darstellung (Schriftgroesse, Stromfluss, Animationen,
// Startbildschirm), Freigabe, und die zuletzt gewaehlte Arbeitsumgebung (Felder je Reihe, Messgeraet, Leitungsfarbe,
// Schaltplan, Experimentfeld). Der Aufbau selbst gehoert nicht hierher.
class AppSettings : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(AppSettings)
    QML_UNCREATABLE("Wird von main.cpp bereitgestellt")
    Q_PROPERTY(bool showCurrentFlow READ showCurrentFlow WRITE setShowCurrentFlow NOTIFY changed)
    Q_PROPERTY(bool reduceMotion READ reduceMotion WRITE setReduceMotion NOTIFY changed)
    Q_PROPERTY(bool showSplash READ showSplash WRITE setShowSplash NOTIFY changed)
    Q_PROPERTY(bool soundOn READ soundOn WRITE setSoundOn NOTIFY changed) // Tonausgabe ueber die Soundkarte
    // Alle Bauteile und Stufen freigeben (spaeter hinter der Elternschranke, Phase 3)
    Q_PROPERTY(bool unlockAll READ unlockAll WRITE setUnlockAll NOTIFY changed)
    // Schriftgroesse in drei Stufen: 0 klein (80 %), 1 mittel (90 %), 2 gross (100 %, Vorgabe)
    Q_PROPERTY(int fontLevel READ fontLevel WRITE setFontLevel NOTIFY changed)
    Q_PROPERTY(bool splashAvailable READ splashAvailable CONSTANT) // nur Desktop-Ausgabe
    // Arbeitsumgebung
    Q_PROPERTY(int boardColumns READ boardColumns WRITE setBoardColumns NOTIFY changed) // 8, 10, 12
    Q_PROPERTY(bool meterVisible READ meterVisible WRITE setMeterVisible NOTIFY changed) // im freien Labor
    // Platz des Messgeraets: oben im Pult (Buchsen in der Pultleiste) oder neben der Grundplatte
    Q_PROPERTY(bool meterDocked READ meterDocked WRITE setMeterDocked NOTIFY changed)
    // Breite der Grundplatte automatisch (Lernpfad 8, freies Labor 12, Messgeraet daneben 10, im Pult 12)
    Q_PROPERTY(bool autoBoardColumns READ autoBoardColumns WRITE setAutoBoardColumns NOTIFY changed)
    // Kontaktfelder silbern (Standard) oder messingfarben, jeweils mit Metallglanz
    Q_PROPERTY(bool silverFields READ silverFields WRITE setSilverFields NOTIFY changed)
    // Forscherprofile fuer geteilte Geraete (Elternbereich)
    Q_PROPERTY(bool profilesEnabled READ profilesEnabled WRITE setProfilesEnabled NOTIFY changed)
    // Klassenmodus Ton (Elternbereich): "normal", "kopfhoerer" (nur mit Kopfhoerern) oder "aus"
    // Maskottchen Tick in Hilfen, Fehlersuche und Erfolg (Elternbereich); aus: dieselben Hinweise ohne Figur
    Q_PROPERTY(bool tickEnabled READ tickEnabled WRITE setTickEnabled NOTIFY changed)
    Q_PROPERTY(QString classroomSound READ classroomSound WRITE setClassroomSound NOTIFY changed)
    // Elternschranke: PIN (4-8 Ziffern), gespeichert nur als gesalzener SHA-256-Pruefwert; Start-PIN 1234
    Q_PROPERTY(bool parentPinIsDefault READ parentPinIsDefault NOTIFY changed)
    Q_PROPERTY(QString wireColor READ wireColor WRITE setWireColor NOTIFY changed)
    Q_PROPERTY(bool autoWireColor READ autoWireColor WRITE setAutoWireColor NOTIFY changed) // Plus rot, Minus schwarz
    Q_PROPERTY(bool schematicVisible READ schematicVisible WRITE setSchematicVisible NOTIFY changed)
    Q_PROPERTY(bool experimentPanelCollapsed READ experimentPanelCollapsed WRITE setExperimentPanelCollapsed NOTIFY changed)

public:
    // persist: laden und speichern; file: eigene Datei (Tests), sonst der Standardort
    explicit AppSettings(bool persist = true, QString file = {}, QObject* parent = nullptr);

    bool showCurrentFlow() const { return m_showCurrentFlow; }
    void setShowCurrentFlow(bool on) { set(m_showCurrentFlow, on, "anzeige/stromfluss"); }
    bool reduceMotion() const { return m_reduceMotion; }
    void setReduceMotion(bool on) { set(m_reduceMotion, on, "anzeige/animationen_reduziert"); }
    bool showSplash() const { return m_showSplash; }
    void setShowSplash(bool on) { set(m_showSplash, on, "anzeige/startbildschirm"); }
    static bool splashAvailable();
    bool soundOn() const { return m_soundOn; }
    void setSoundOn(bool on) { set(m_soundOn, on, "anzeige/ton"); }
    bool unlockAll() const { return m_unlockAll; }
    void setUnlockAll(bool on) { set(m_unlockAll, on, "lernpfad/alles_frei"); }
    int fontLevel() const { return m_fontLevel; }
    void setFontLevel(int level);
    int boardColumns() const { return m_boardColumns; }
    void setBoardColumns(int columns);
    bool meterVisible() const { return m_meterVisible; }
    void setMeterVisible(bool on) { set(m_meterVisible, on, "labor/messgeraet"); }
    bool meterDocked() const { return m_meterDocked; }
    void setMeterDocked(bool on) { set(m_meterDocked, on, "labor/messgeraet_im_pult"); }
    bool autoBoardColumns() const { return m_autoBoardColumns; }
    void setAutoBoardColumns(bool on) { set(m_autoBoardColumns, on, "labor/felder_automatisch"); }
    bool silverFields() const { return m_silverFields; }
    void setSilverFields(bool on) { set(m_silverFields, on, "anzeige/felder_silber"); }
    bool profilesEnabled() const { return m_profilesEnabled; }
    bool tickEnabled() const { return m_tickEnabled; }
    void setTickEnabled(bool on) { set(m_tickEnabled, on, "anzeige/tick"); }
    QString classroomSound() const { return m_classroomSound; }
    void setClassroomSound(const QString& mode);
    bool parentPinIsDefault() const { return m_pinHash.isEmpty(); }
    // PIN pruefen; nach 5 Fehlversuchen 30 s gesperrt (auch ueber einen Neustart). false waehrend der Sperre.
    Q_INVOKABLE bool checkParentPin(const QString& pin);
    // verbleibende Sperre in Sekunden (0: frei)
    Q_INVOKABLE int parentPinLockSeconds() const;
    // neue PIN setzen (nur Ziffern, 4-8 Stellen); false bei ungueltiger PIN
    Q_INVOKABLE bool setParentPin(const QString& pin);
    static constexpr const char* kDefaultPin = "1234";
    void setProfilesEnabled(bool on) { set(m_profilesEnabled, on, "profile/aktiv"); }
    QString wireColor() const { return m_wireColor; }
    bool autoWireColor() const { return m_autoWireColor; }
    void setAutoWireColor(bool on) { set(m_autoWireColor, on, "labor/leitungsfarbe_automatisch"); }
    void setWireColor(const QString& color);
    bool schematicVisible() const { return m_schematicVisible; }
    void setSchematicVisible(bool on) { set(m_schematicVisible, on, "labor/schaltplan"); }
    bool experimentPanelCollapsed() const { return m_panelCollapsed; }
    void setExperimentPanelCollapsed(bool on) { set(m_panelCollapsed, on, "labor/experimentfeld_eingeklappt"); }

signals:
    void changed();

private:
    template <typename T>
    void set(T& member, const T& value, const char* key) {
        if (member == value) return;
        member = value;
        save(QString::fromLatin1(key), QVariant::fromValue(value));
        emit changed();
    }
    void save(const QString& key, const QVariant& value);

    bool m_persist;
    QString m_file;
    bool m_showCurrentFlow = true;
    bool m_reduceMotion = false;
    bool m_showSplash = true;
    bool m_soundOn = true;
    bool m_unlockAll = false;
    int m_fontLevel = 2;
    int m_boardColumns = 12;
    bool m_meterVisible = false;
    bool m_meterDocked = false;
    bool m_autoBoardColumns = true;
    bool m_silverFields = true;
    bool m_profilesEnabled = false;
    bool m_tickEnabled = true;
    QString m_classroomSound = QStringLiteral("normal");
    QString m_pinHash; // leer: Start-PIN
    QString m_pinSalt;
    int m_pinFailures = 0;
    qint64 m_pinLockedUntil = 0; // ms seit Epoche
    static QString hashPin(const QString& salt, const QString& pin);
    void savePinState();
    QString m_wireColor = QStringLiteral("rot");
    bool m_autoWireColor = true;
    bool m_schematicVisible = false;
    bool m_panelCollapsed = false;
};

} // namespace volta

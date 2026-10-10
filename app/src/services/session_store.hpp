#pragma once

#include <QJsonObject>
#include <QString>

namespace volta::services {

// Automatisches Speichern des Laborstands (docs/ui-spec.md): der aktuelle Aufbau mit Pult, das laufende Experiment
// samt Ablauf und ob das Labor offen war. Damit geht nichts verloren, wenn Android die App im Hintergrund beendet.
// Gespeichert als JSON (atomar mit QSaveFile) im privaten App-Speicher; ohne Dateipfad nur im Speicher (Tests).
class SessionStore {
public:
    explicit SessionStore(QString filePath = {});

    // zuletzt gespeicherter Stand (leer, wenn keiner)
    QJsonObject load() const;
    bool save(const QJsonObject& state);
    bool clear();
    const QString& filePath() const { return m_path; }
    // Standardort: QStandardPaths::AppDataLocation/laborstand.json
    static QString defaultFile();

private:
    QString m_path;
    QJsonObject m_memory; // ohne Dateipfad
};

} // namespace volta::services

#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace volta::services {

// Fortschritt im Lernpfad (docs/privacy-safety.md): je Experiment nur ID, »erledigt« und Datum - keine Namen,
// keine Inhalte. Gespeichert als JSON (atomar mit QSaveFile) im privaten App-Speicher; ohne Dateipfad nur im
// Speicher (Tests). Profile folgen in Phase 3.
class ProgressStore {
public:
    explicit ProgressStore(QString filePath = {});

    bool isDone(const QString& experimentId) const;
    QStringList doneIds() const;
    // Erledigt markieren (bleibt erledigt); false, wenn das Speichern fehlschlug
    bool markDone(const QString& experimentId);
    bool reset();
    const QString& filePath() const { return m_path; }
    // Standardort: QStandardPaths::AppDataLocation/fortschritt.json
    static QString defaultFile();

private:
    void load();
    bool save() const;

    QString m_path;
    QJsonObject m_done; // id -> {"datum": "2026-10-05"}
};

} // namespace volta::services

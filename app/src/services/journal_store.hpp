#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace volta::services {

// Forscherheft (Konzept 5.6 und 5.8): je Experiment Vermutung, Beobachtung, Vergleich und eigene Notizen, als Text in
// der Sprache, in der das Kind gearbeitet hat. Nur im privaten App-Speicher (docs/privacy-safety.md), JSON atomar mit
// QSaveFile; ohne Dateipfad nur im Speicher (Tests). Zeichnungen und Fotos folgen (Phase 3).
class JournalStore {
public:
    explicit JournalStore(QString filePath = {});

    // Vermutung festhalten (ohne Bewertung); eine neue Vermutung beginnt den Eintrag neu
    bool setGuess(const QString& experimentId, const QString& guess);
    // Beobachtung und Vergleich ("passt", "anders", "offen")
    bool setObservation(const QString& experimentId, const QString& observation, const QString& comparison);
    bool addNote(const QString& experimentId, const QString& note);
    bool reset();

    // Eintraege, neueste zuerst: {id, datum, vermutung, beobachtung, vergleich, notizen[]}
    QList<QJsonObject> entries() const;
    QJsonObject entry(const QString& experimentId) const { return m_entries.value(experimentId).toObject(); }
    static QString defaultFile();

private:
    void load();
    bool save() const;
    QJsonObject touched(const QString& experimentId) const; // Eintrag mit aktuellem Datum

    QString m_path;
    QJsonObject m_entries; // id -> Eintrag
};

} // namespace volta::services

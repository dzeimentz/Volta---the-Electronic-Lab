#pragma once

#include <QList>
#include <QString>

namespace volta::services {

// Forscherprofile (Konzept 8.2, docs/privacy-safety.md): auf geteilten Geraeten waehlt jedes Kind ein Symbol und
// einen Spitznamen - ohne Klarnamen, ohne Passwort, nur lokal. Je Profil ein eigener Ordner fuer Fortschritt,
// Forscherheft und Laborstand. Liste in <Basis>/profile/profile.json (atomar mit QSaveFile); ohne Basisordner nur
// im Speicher (Tests).
struct Profile {
    QString id;     // "p1", "p2", ... (nie wiederverwendet)
    QString symbol; // Bildzeichen
    QString name;   // Spitzname, hoechstens kMaxName Zeichen
};

class ProfileStore {
public:
    static constexpr int kMaxProfiles = 30;
    static constexpr int kMaxName = 16;

    explicit ProfileStore(QString baseDir = {});

    const QList<Profile>& profiles() const { return m_profiles; }
    const Profile* find(const QString& id) const;
    // neues Profil; leere ID, wenn der Name leer ist oder schon kMaxProfiles bestehen
    QString add(const QString& symbol, const QString& name);
    // Profil samt seinem Ordner loeschen
    bool remove(const QString& id);
    bool removeAll();
    // Ordner der Daten eines Profils (leer ohne Basisordner)
    QString dataDir(const QString& id) const;

private:
    void load();
    bool save() const;

    QString m_base;
    QList<Profile> m_profiles;
    int m_next = 1;
};

} // namespace volta::services

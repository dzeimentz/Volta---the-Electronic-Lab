#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace volta::services {

// Mitnahme eines Forscherprofils auf ein anderes Geraet (Konzept 8.2), ohne Server:
//  - Fortschrittscode (QR): nur die erledigten Experimente, kein Name (docs/privacy-safety.md), als Link
//    volta1://fortschritt?d=1.1,1.2,2.4 - die Kamera-App des Systems oeffnet damit Volta (keine Kameraberechtigung).
//  - Profildatei (*.volta1profil): Symbol, Spitzname, Fortschritt, Forscherheft und letzter Aufbau als JSON.
namespace transfer {

inline constexpr const char* kScheme = "volta1";
inline constexpr const char* kFileSuffix = "volta1profil";

// Link mit den erledigten Experimenten (IDs ohne Bandpraefix, sortiert)
QString progressLink(const QString& band, const QStringList& doneIds);
// erledigte Experiment-IDs aus einem Link; nur bekannte IDs (knownIds), leer bei fremdem oder kaputtem Link
QStringList parseProgressLink(const QString& link, const QString& band, const QStringList& knownIds);

struct ProfileFile {
    QString symbol;
    QString name;
    QJsonObject progress; // Inhalt von fortschritt.json
    QJsonObject journal;  // forscherheft.json
    QJsonObject session;  // laborstand.json
};
// Profildatei schreiben bzw. lesen; target/source ist ein Dateipfad oder (Android) eine content://-Adresse
bool writeProfileFile(const QString& target, const ProfileFile& file);
bool readProfileFile(const QString& source, ProfileFile& out);
// Profildaten aus einem Datenordner lesen bzw. dorthin schreiben (fehlende Dateien bleiben leer)
ProfileFile loadFromDir(const QString& dir);
bool saveToDir(const QString& dir, const ProfileFile& file);

} // namespace transfer
} // namespace volta::services

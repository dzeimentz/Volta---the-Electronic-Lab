#include "journal_store.hpp"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>

#include <algorithm>

namespace volta::services {

JournalStore::JournalStore(QString filePath) : m_path(std::move(filePath)) { load(); }

QString JournalStore::defaultFile() {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/forscherheft.json");
}

void JournalStore::load() {
    if (m_path.isEmpty()) return;
    QFile f(m_path);
    if (!f.open(QIODevice::ReadOnly)) return;
    m_entries = QJsonDocument::fromJson(f.readAll()).object().value(QStringLiteral("eintraege")).toObject();
}

bool JournalStore::save() const {
    if (m_path.isEmpty()) return true;
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QSaveFile f(m_path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("eintraege"), m_entries);
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return f.commit();
}

QJsonObject JournalStore::touched(const QString& experimentId) const {
    QJsonObject e = m_entries.value(experimentId).toObject();
    e.insert(QStringLiteral("datum"), QDateTime::currentDateTime().toString(Qt::ISODate));
    return e;
}

bool JournalStore::setGuess(const QString& experimentId, const QString& guess) {
    if (experimentId.isEmpty()) return false;
    QJsonObject e = touched(experimentId);
    e.insert(QStringLiteral("vermutung"), guess);
    e.remove(QStringLiteral("beobachtung")); // neuer Durchgang: Beobachtung folgt neu
    e.remove(QStringLiteral("vergleich"));
    m_entries.insert(experimentId, e);
    return save();
}

bool JournalStore::setObservation(const QString& experimentId, const QString& observation, const QString& comparison) {
    if (experimentId.isEmpty()) return false;
    QJsonObject e = touched(experimentId);
    e.insert(QStringLiteral("beobachtung"), observation);
    e.insert(QStringLiteral("vergleich"), comparison);
    m_entries.insert(experimentId, e);
    return save();
}

bool JournalStore::addNote(const QString& experimentId, const QString& note) {
    const QString text = note.trimmed();
    if (experimentId.isEmpty() || text.isEmpty()) return false;
    QJsonObject e = touched(experimentId);
    QJsonArray notes = e.value(QStringLiteral("notizen")).toArray();
    notes.append(text.left(500)); // Kindernotizen sind kurz; Obergrenze schuetzt die Datei
    e.insert(QStringLiteral("notizen"), notes);
    m_entries.insert(experimentId, e);
    return save();
}

bool JournalStore::reset() {
    m_entries = QJsonObject{};
    return save();
}

QList<QJsonObject> JournalStore::entries() const {
    QList<QJsonObject> out;
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
        QJsonObject e = it.value().toObject();
        e.insert(QStringLiteral("id"), it.key());
        out.append(e);
    }
    std::sort(out.begin(), out.end(), [](const QJsonObject& a, const QJsonObject& b) {
        return a.value(QStringLiteral("datum")).toString() > b.value(QStringLiteral("datum")).toString();
    });
    return out;
}

} // namespace volta::services

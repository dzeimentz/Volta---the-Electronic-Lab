#include "journal_store.hpp"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
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
    if (const QString dir = photoDir(); !dir.isEmpty()) QDir(dir).removeRecursively(); // Fotos gehoeren dazu
    return save();
}

QString JournalStore::photoDir() const {
    return m_path.isEmpty() ? QString() : QFileInfo(m_path).absolutePath() + QStringLiteral("/fotos");
}

QString JournalStore::addPhoto(const QString& experimentId, const QString& source) {
    if (experimentId.isEmpty() || m_path.isEmpty()) return {};
    QJsonObject e = touched(experimentId);
    QJsonArray photos = e.value(QStringLiteral("fotos")).toArray();
    if (photos.size() >= kMaxPhotos) return {};
    QFile in(source); // content://-Adressen (Android) liest QFile direkt
    if (!in.open(QIODevice::ReadOnly) || in.size() > 40 * 1024 * 1024) return {};
    QImageReader reader(&in);
    reader.setAutoTransform(true); // Hochformat aus der Kamera richtig herum
    const QSize size = reader.size();
    constexpr int kMaxSide = 1600;
    if (size.isValid() && std::max(size.width(), size.height()) > kMaxSide)
        reader.setScaledSize(size.scaled(kMaxSide, kMaxSide, Qt::KeepAspectRatio));
    QImage image = reader.read();
    if (image.isNull()) return {};
    if (std::max(image.width(), image.height()) > kMaxSide)
        image = image.scaled(kMaxSide, kMaxSide, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    const QString dir = photoDir();
    QDir().mkpath(dir);
    QString name = experimentId;
    name.replace(QLatin1Char('.'), QLatin1Char('_'));
    name += QStringLiteral("_%1.jpg").arg(QDateTime::currentMSecsSinceEpoch());
    QSaveFile out(dir + QLatin1Char('/') + name);
    if (!out.open(QIODevice::WriteOnly) || !image.convertToFormat(QImage::Format_RGB32).save(&out, "JPG", 85) ||
        !out.commit())
        return {};
    photos.append(name);
    e.insert(QStringLiteral("fotos"), photos);
    m_entries.insert(experimentId, e);
    return save() ? name : QString();
}

bool JournalStore::removePhoto(const QString& experimentId, const QString& fileName) {
    QJsonObject e = m_entries.value(experimentId).toObject();
    QJsonArray photos = e.value(QStringLiteral("fotos")).toArray();
    for (qsizetype i = 0; i < photos.size(); ++i) {
        if (photos.at(i).toString() != fileName) continue;
        photos.removeAt(i);
        e.insert(QStringLiteral("fotos"), photos);
        m_entries.insert(experimentId, e);
        // nur Dateinamen ohne Pfad (Schutz vor »../«)
        if (!fileName.contains(QLatin1Char('/')) && !fileName.contains(QLatin1Char('\\')))
            QFile::remove(photoDir() + QLatin1Char('/') + fileName);
        return save();
    }
    return false;
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

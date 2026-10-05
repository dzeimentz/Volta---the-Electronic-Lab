#include "progress_store.hpp"

#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>

namespace volta::services {

ProgressStore::ProgressStore(QString filePath) : m_path(std::move(filePath)) { load(); }

QString ProgressStore::defaultFile() {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/fortschritt.json");
}

void ProgressStore::load() {
    if (m_path.isEmpty()) return;
    QFile f(m_path);
    if (!f.open(QIODevice::ReadOnly)) return; // noch kein Fortschritt
    const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    m_done = root.value(QStringLiteral("erledigt")).toObject();
}

bool ProgressStore::save() const {
    if (m_path.isEmpty()) return true;
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QSaveFile f(m_path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("erledigt"), m_done);
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return f.commit();
}

bool ProgressStore::isDone(const QString& experimentId) const { return m_done.contains(experimentId); }

QStringList ProgressStore::doneIds() const { return m_done.keys(); }

bool ProgressStore::markDone(const QString& experimentId) {
    if (experimentId.isEmpty() || m_done.contains(experimentId)) return true;
    m_done.insert(experimentId,
                  QJsonObject{{QStringLiteral("datum"), QDate::currentDate().toString(Qt::ISODate)}});
    return save();
}

bool ProgressStore::reset() {
    m_done = QJsonObject{};
    return save();
}

} // namespace volta::services

#include "session_store.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>

namespace volta::services {

SessionStore::SessionStore(QString filePath) : m_path(std::move(filePath)) {}

QString SessionStore::defaultFile() {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/laborstand.json");
}

QJsonObject SessionStore::load() const {
    if (m_path.isEmpty()) return m_memory;
    QFile f(m_path);
    if (!f.open(QIODevice::ReadOnly)) return {}; // noch nichts gespeichert
    const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    return root.value(QStringLiteral("version")).toInt() == 1 ? root : QJsonObject{};
}

bool SessionStore::save(const QJsonObject& state) {
    QJsonObject root = state;
    root.insert(QStringLiteral("version"), 1);
    if (m_path.isEmpty()) {
        m_memory = root;
        return true;
    }
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QSaveFile f(m_path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    return f.commit();
}

bool SessionStore::clear() {
    m_memory = {};
    return m_path.isEmpty() || !QFile::exists(m_path) || QFile::remove(m_path);
}

} // namespace volta::services

#include "profile_store.hpp"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

namespace volta::services {

namespace {
QString listFile(const QString& base) { return base + QStringLiteral("/profile/profile.json"); }
} // namespace

ProfileStore::ProfileStore(QString baseDir) : m_base(std::move(baseDir)) { load(); }

void ProfileStore::load() {
    if (m_base.isEmpty()) return;
    QFile f(listFile(m_base));
    if (!f.open(QIODevice::ReadOnly)) return;
    const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    m_next = std::max(1, root.value(QStringLiteral("naechste")).toInt(1));
    for (const QJsonValue& v : root.value(QStringLiteral("profile")).toArray()) {
        const QJsonObject o = v.toObject();
        const Profile p{o.value(QStringLiteral("id")).toString(), o.value(QStringLiteral("symbol")).toString(),
                        o.value(QStringLiteral("name")).toString()};
        if (!p.id.isEmpty() && !p.name.isEmpty()) m_profiles.push_back(p);
    }
}

bool ProfileStore::save() const {
    if (m_base.isEmpty()) return true;
    QDir().mkpath(m_base + QStringLiteral("/profile"));
    QSaveFile f(listFile(m_base));
    if (!f.open(QIODevice::WriteOnly)) return false;
    QJsonArray list;
    for (const auto& p : m_profiles)
        list.append(QJsonObject{{QStringLiteral("id"), p.id}, {QStringLiteral("symbol"), p.symbol},
                                {QStringLiteral("name"), p.name}});
    const QJsonObject root{{QStringLiteral("version"), 1}, {QStringLiteral("naechste"), m_next},
                           {QStringLiteral("profile"), list}};
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return f.commit();
}

const Profile* ProfileStore::find(const QString& id) const {
    for (const auto& p : m_profiles)
        if (p.id == id) return &p;
    return nullptr;
}

QString ProfileStore::add(const QString& symbol, const QString& name) {
    const QString n = name.simplified().left(kMaxName);
    if (n.isEmpty() || m_profiles.size() >= kMaxProfiles) return {};
    const Profile p{QStringLiteral("p%1").arg(m_next++), symbol, n};
    m_profiles.push_back(p);
    save();
    return p.id;
}

bool ProfileStore::remove(const QString& id) {
    for (int i = 0; i < m_profiles.size(); ++i) {
        if (m_profiles[i].id != id) continue;
        m_profiles.removeAt(i);
        if (const QString dir = dataDir(id); !dir.isEmpty()) QDir(dir).removeRecursively();
        return save();
    }
    return false;
}

bool ProfileStore::removeAll() {
    const auto ids = m_profiles;
    for (const auto& p : ids)
        if (const QString dir = dataDir(p.id); !dir.isEmpty()) QDir(dir).removeRecursively();
    m_profiles.clear();
    return save();
}

QString ProfileStore::dataDir(const QString& id) const {
    return m_base.isEmpty() || id.isEmpty() ? QString() : m_base + QStringLiteral("/profile/") + id;
}

} // namespace volta::services

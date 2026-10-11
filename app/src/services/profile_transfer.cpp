#include "profile_transfer.hpp"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QSaveFile>
#include <QUrl>
#include <QUrlQuery>

#include <algorithm>

namespace volta::services::transfer {

namespace {

constexpr const char* kFormat = "volta1-profil";
constexpr qint64 kMaxFileBytes = 4 * 1024 * 1024; // Schutz vor fremden, sehr grossen Dateien

QJsonObject readJson(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(f.readAll()).object();
}

bool writeJson(const QString& path, const QJsonObject& o) {
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    return f.commit();
}

} // namespace

QString progressLink(const QString& band, const QStringList& doneIds) {
    QStringList shortIds;
    const QString prefix = band + QLatin1Char('.');
    for (const QString& id : doneIds)
        if (id.startsWith(prefix)) shortIds << id.mid(prefix.size());
    std::sort(shortIds.begin(), shortIds.end());
    return QStringLiteral("%1://fortschritt?d=%2").arg(QLatin1String(kScheme), shortIds.join(QLatin1Char(',')));
}

QStringList parseProgressLink(const QString& link, const QString& band, const QStringList& knownIds) {
    const QUrl url(link);
    if (url.scheme() != QLatin1String(kScheme) || url.host() != QLatin1String("fortschritt")) return {};
    QStringList out;
    const QString list = QUrlQuery(url).queryItemValue(QStringLiteral("d"));
    for (const QString& s : list.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
        const QString id = band + QLatin1Char('.') + s.trimmed();
        if (knownIds.contains(id) && !out.contains(id)) out << id;
    }
    return out;
}

bool writeProfileFile(const QString& target, const ProfileFile& file) {
    const QJsonObject root{{QStringLiteral("format"), QLatin1String(kFormat)},
                           {QStringLiteral("version"), 1},
                           {QStringLiteral("symbol"), file.symbol},
                           {QStringLiteral("name"), file.name},
                           {QStringLiteral("fortschritt"), file.progress},
                           {QStringLiteral("forscherheft"), file.journal},
                           {QStringLiteral("laborstand"), file.session}};
    QFile f(target); // content://-Adressen (Android) schreibt QFile direkt
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    const QByteArray data = QJsonDocument(root).toJson(QJsonDocument::Indented);
    return f.write(data) == data.size();
}

bool readProfileFile(const QString& source, ProfileFile& out) {
    QFile f(source);
    if (!f.open(QIODevice::ReadOnly) || f.size() > kMaxFileBytes) return false;
    const QJsonObject root = QJsonDocument::fromJson(f.read(kMaxFileBytes)).object();
    if (root.value(QStringLiteral("format")).toString() != QLatin1String(kFormat) ||
        root.value(QStringLiteral("version")).toInt() != 1)
        return false;
    out.symbol = root.value(QStringLiteral("symbol")).toString().left(8);
    out.name = root.value(QStringLiteral("name")).toString().simplified().left(16);
    out.progress = root.value(QStringLiteral("fortschritt")).toObject();
    out.journal = root.value(QStringLiteral("forscherheft")).toObject();
    out.session = root.value(QStringLiteral("laborstand")).toObject();
    return true;
}

ProfileFile loadFromDir(const QString& dir) {
    ProfileFile f;
    f.progress = readJson(dir + QStringLiteral("/fortschritt.json"));
    f.journal = readJson(dir + QStringLiteral("/forscherheft.json"));
    f.session = readJson(dir + QStringLiteral("/laborstand.json"));
    return f;
}

bool saveToDir(const QString& dir, const ProfileFile& file) {
    if (dir.isEmpty() || !QDir().mkpath(dir)) return false;
    bool ok = true;
    auto put = [&](const char* name, const QJsonObject& o) {
        const QString path = dir + QLatin1Char('/') + QLatin1String(name);
        if (o.isEmpty()) QFile::remove(path);
        else ok = writeJson(path, o) && ok;
    };
    put("fortschritt.json", file.progress);
    put("forscherheft.json", file.journal);
    put("laborstand.json", file.session);
    return ok;
}

} // namespace volta::services::transfer

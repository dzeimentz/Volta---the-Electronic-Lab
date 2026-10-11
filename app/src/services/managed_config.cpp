#include "managed_config.hpp"

#include <QCoreApplication>
#include <QFileInfo>
#include <QSettings>
#include <QStringList>

#ifdef Q_OS_ANDROID
#include <QJniObject>
#include <QtCore/qcoreapplication_platform.h>
#endif

#include <algorithm>

namespace volta::services {

namespace {

const QStringList& keys() {
    static const QStringList k{QStringLiteral("sprache"),        QStringLiteral("sprache_gesperrt"),
                               QStringLiteral("stufen_frei"),    QStringLiteral("elternbereich"),
                               QStringLiteral("fotoimport"),     QStringLiteral("klassenmodus_ton"),
                               QStringLiteral("profile"),        QStringLiteral("praesentation")};
    return k;
}

QVariantMap fromSettings(QSettings& s) {
    QVariantMap out;
    for (const QString& k : keys())
        if (s.contains(k)) out.insert(k, s.value(k));
    return out;
}

bool toBool(const QVariant& v) {
    const QString t = v.toString().trimmed().toLower();
    return t == QLatin1String("1") || t == QLatin1String("true") || t == QLatin1String("ja") || t == QLatin1String("yes");
}

} // namespace

ManagedConfig::ManagedConfig(QObject* parent) : QObject(parent) { apply(readPlatform()); }

ManagedConfig::ManagedConfig(const QVariantMap& values, QObject* parent) : QObject(parent) { apply(values); }

void ManagedConfig::apply(const QVariantMap& v) {
    for (const QString& k : keys())
        if (v.contains(k)) m_managed = true;
    const QString lang = v.value(QStringLiteral("sprache")).toString().trimmed().toLower();
    if (lang == QLatin1String("de") || lang == QLatin1String("fr") || lang == QLatin1String("en") ||
        lang == QLatin1String("lb"))
        m_language = lang;
    if (v.contains(QStringLiteral("sprache_gesperrt")))
        m_languageLocked = toBool(v.value(QStringLiteral("sprache_gesperrt")));
    if (v.contains(QStringLiteral("stufen_frei"))) {
        bool ok = false;
        const int n = v.value(QStringLiteral("stufen_frei")).toInt(&ok);
        if (ok) m_unlockStage = std::clamp(n, 0, 99);
    }
    if (v.contains(QStringLiteral("elternbereich"))) m_parentArea = toBool(v.value(QStringLiteral("elternbereich")));
    if (v.contains(QStringLiteral("fotoimport"))) m_photoImport = toBool(v.value(QStringLiteral("fotoimport")));
    const QString sound = v.value(QStringLiteral("klassenmodus_ton")).toString().trimmed().toLower();
    if (sound == QLatin1String("aus") || sound == QLatin1String("kopfhoerer") || sound == QLatin1String("normal"))
        m_soundMode = sound;
    if (v.contains(QStringLiteral("profile"))) {
        bool ok = false;
        const int n = v.value(QStringLiteral("profile")).toInt(&ok);
        if (ok) m_profiles = std::clamp(n, 0, 30);
    }
    if (v.contains(QStringLiteral("praesentation"))) m_presentation = toBool(v.value(QStringLiteral("praesentation")));
}

QVariantMap ManagedConfig::readPlatform() {
    if (const QString ini = qEnvironmentVariable("VOLTA_MANAGED_INI"); !ini.isEmpty()) {
        QSettings s(ini, QSettings::IniFormat);
        return fromSettings(s);
    }
#if defined(Q_OS_ANDROID)
    // RestrictionsManager.getApplicationRestrictions(): Bundle mit den Werten aus app_restrictions.xml
    QVariantMap out;
    const QJniObject context = QNativeInterface::QAndroidApplication::context();
    if (!context.isValid()) return out;
    const QJniObject service = QJniObject::getStaticObjectField<jstring>("android/content/Context", "RESTRICTIONS_SERVICE");
    const QJniObject manager = context.callObjectMethod("getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;",
                                                        service.object<jstring>());
    if (!manager.isValid()) return out;
    const QJniObject bundle = manager.callObjectMethod("getApplicationRestrictions", "()Landroid/os/Bundle;");
    if (!bundle.isValid()) return out;
    for (const QString& k : keys()) {
        const QJniObject key = QJniObject::fromString(k);
        if (!bundle.callMethod<jboolean>("containsKey", "(Ljava/lang/String;)Z", key.object<jstring>())) continue;
        const QJniObject value = bundle.callObjectMethod("get", "(Ljava/lang/String;)Ljava/lang/Object;",
                                                         key.object<jstring>());
        if (value.isValid()) out.insert(k, value.callObjectMethod("toString", "()Ljava/lang/String;").toString());
    }
    return out;
#elif defined(Q_OS_WIN)
    QSettings reg(QStringLiteral("HKEY_LOCAL_MACHINE\\Software\\DC Software Engineering\\Volta1"),
                  QSettings::NativeFormat);
    QVariantMap out = fromSettings(reg);
    if (!out.isEmpty()) return out;
    const QString ini = QCoreApplication::applicationDirPath() + QStringLiteral("/volta.ini");
    if (!QFileInfo::exists(ini)) return {};
    QSettings s(ini, QSettings::IniFormat);
    return fromSettings(s);
#elif defined(Q_OS_LINUX)
    const QString ini = QStringLiteral("/etc/xdg/dc-software-engineering/volta1.ini");
    if (!QFileInfo::exists(ini)) return {};
    QSettings s(ini, QSettings::IniFormat);
    return fromSettings(s);
#else
    return {};
#endif
}

} // namespace volta::services

#include "zuse_settings.hpp"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QDir>
#include <QFileInfo>
#include <QLocale>
#include <QSettings>
#include <QVariantMap>

namespace zuse {

namespace {
struct Language {
    const char* code;
    const char* shortName;
    const char* name; // Eigenname
};
// wie Volta (docs/i18n.md); Luxemburgisch folgt bei Zuse spaeter (Entscheidung des Inhabers, 25.10.2026)
constexpr Language kLanguages[] = {{"de", "DE", "Deutsch"}, {"fr", "FR", "Français"}, {"en", "EN", "English"}};
} // namespace

ZuseSettings::ZuseSettings(const QString& dataDir, const QString& language, QObject* parent) : QObject(parent) {
    if (!dataDir.isEmpty()) m_file = QDir(dataDir).filePath(QStringLiteral("einstellungen.ini"));
    QString code = isAvailable(language) ? language : QStringLiteral("de");
    if (!m_file.isEmpty()) {
        const QSettings s(m_file, QSettings::IniFormat);
        code = s.value(QStringLiteral("anzeige/sprache"), code).toString();
        m_fontLevel = qBound(0, s.value(QStringLiteral("anzeige/schrift"), 2).toInt(), 2);
        m_schematicVisible = s.value(QStringLiteral("anzeige/schaltplan"), false).toBool();
        m_zettiVisible = s.value(QStringLiteral("anzeige/zetti"), true).toBool();
        m_unlockAll = s.value(QStringLiteral("eltern/alles_frei"), false).toBool();
        m_profilesEnabled = s.value(QStringLiteral("profile/aktiv"), false).toBool();
        m_demoVisible = s.value(QStringLiteral("eltern/vorfuehrung"), true).toBool();
        m_pinHash = s.value(QStringLiteral("eltern/pin_pruefwert")).toString();
        m_pinSalt = s.value(QStringLiteral("eltern/pin_salz")).toString();
        m_pinFailures = s.value(QStringLiteral("eltern/fehlversuche"), 0).toInt();
        m_pinLockedUntil = s.value(QStringLiteral("eltern/gesperrt_bis"), 0).toLongLong();
    }
    m_language = isAvailable(code) ? code : QStringLiteral("de");
    installTranslator();
}

ZuseSettings::~ZuseSettings() {
    if (m_installed) QCoreApplication::removeTranslator(&m_translator);
}

QVariantList ZuseSettings::languages() const {
    QVariantList out;
    for (const Language& l : kLanguages)
        out << QVariantMap{{QStringLiteral("code"), QString::fromLatin1(l.code)},
                           {QStringLiteral("short"), QString::fromLatin1(l.shortName)},
                           {QStringLiteral("name"), QString::fromUtf8(l.name)}};
    return out;
}

bool ZuseSettings::isAvailable(const QString& code) {
    for (const Language& l : kLanguages)
        if (code == QLatin1String(l.code)) return true;
    return false;
}

void ZuseSettings::installTranslator() {
    if (m_installed) {
        QCoreApplication::removeTranslator(&m_translator);
        m_installed = false;
    }
    // Deutsch ist Quellsprache
    if (m_language != QLatin1String("de") && m_translator.load(QStringLiteral(":/i18n/zuse_%1.qm").arg(m_language)))
        m_installed = QCoreApplication::installTranslator(&m_translator);
    QLocale::setDefault(QLocale(m_language));
}

bool ZuseSettings::setLanguage(const QString& code) {
    if (!isAvailable(code)) return false;
    if (code == m_language) return true;
    m_language = code;
    installTranslator();
    save();
    emit languageChanged(m_language);
    return true;
}

void ZuseSettings::setFontLevel(int level) {
    level = qBound(0, level, 2);
    if (level == m_fontLevel) return;
    m_fontLevel = level;
    save();
    emit fontLevelChanged();
}

void ZuseSettings::setSchematicVisible(bool on) {
    if (on == m_schematicVisible) return;
    m_schematicVisible = on;
    save();
    emit schematicVisibleChanged();
}

void ZuseSettings::setZettiVisible(bool on) {
    if (on == m_zettiVisible) return;
    m_zettiVisible = on;
    save();
    emit zettiVisibleChanged();
}

void ZuseSettings::setUnlockAll(bool on) {
    if (on == m_unlockAll) return;
    m_unlockAll = on;
    save();
    emit parentChanged();
}

void ZuseSettings::setProfilesEnabled(bool on) {
    if (on == m_profilesEnabled) return;
    m_profilesEnabled = on;
    save();
    emit parentChanged();
}

void ZuseSettings::setDemoVisible(bool on) {
    if (on == m_demoVisible) return;
    m_demoVisible = on;
    save();
    emit parentChanged();
}

QString ZuseSettings::hashPin(const QString& salt, const QString& pin) {
    return QString::fromLatin1(
        QCryptographicHash::hash((salt + QLatin1Char(':') + pin).toUtf8(), QCryptographicHash::Sha256).toHex());
}

void ZuseSettings::savePinState() const {
    if (m_file.isEmpty()) return;
    QSettings s(m_file, QSettings::IniFormat);
    s.setValue(QStringLiteral("eltern/pin_pruefwert"), m_pinHash);
    s.setValue(QStringLiteral("eltern/pin_salz"), m_pinSalt);
    s.setValue(QStringLiteral("eltern/fehlversuche"), m_pinFailures);
    s.setValue(QStringLiteral("eltern/gesperrt_bis"), m_pinLockedUntil);
}

int ZuseSettings::parentPinLockSeconds() const {
    const qint64 rest = m_pinLockedUntil - QDateTime::currentMSecsSinceEpoch();
    return rest > 0 ? static_cast<int>((rest + 999) / 1000) : 0;
}

bool ZuseSettings::checkParentPin(const QString& pin) {
    if (parentPinLockSeconds() > 0) return false;
    const bool ok = m_pinHash.isEmpty() ? pin == QLatin1String(kDefaultPin) : hashPin(m_pinSalt, pin) == m_pinHash;
    if (ok) {
        m_pinFailures = 0;
    } else if (++m_pinFailures >= 5) {
        m_pinFailures = 0;
        m_pinLockedUntil = QDateTime::currentMSecsSinceEpoch() + 30000;
    }
    savePinState();
    return ok;
}

bool ZuseSettings::setParentPin(const QString& pin) {
    static const QRegularExpression digits(QStringLiteral("^[0-9]{4,8}$"));
    if (!digits.match(pin).hasMatch()) return false;
    QByteArray salt(16, Qt::Uninitialized);
    QRandomGenerator::system()->fillRange(reinterpret_cast<quint32*>(salt.data()), salt.size() / 4);
    m_pinSalt = QString::fromLatin1(salt.toHex());
    m_pinHash = hashPin(m_pinSalt, pin);
    m_pinFailures = 0;
    savePinState();
    emit parentChanged();
    return true;
}

void ZuseSettings::save() const {
    if (m_file.isEmpty()) return;
    QDir().mkpath(QFileInfo(m_file).absolutePath());
    QSettings s(m_file, QSettings::IniFormat);
    s.setValue(QStringLiteral("anzeige/sprache"), m_language);
    s.setValue(QStringLiteral("anzeige/schrift"), m_fontLevel);
    s.setValue(QStringLiteral("anzeige/schaltplan"), m_schematicVisible);
    s.setValue(QStringLiteral("anzeige/zetti"), m_zettiVisible);
    s.setValue(QStringLiteral("eltern/alles_frei"), m_unlockAll);
    s.setValue(QStringLiteral("profile/aktiv"), m_profilesEnabled);
    s.setValue(QStringLiteral("eltern/vorfuehrung"), m_demoVisible);
}

} // namespace zuse

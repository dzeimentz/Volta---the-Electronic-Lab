#include "app_settings.hpp"

#include <QDir>

#include <algorithm>
#include <QSettings>
#include <QStandardPaths>

namespace volta {

namespace {
QString settingsFile() {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/einstellungen.ini");
}
} // namespace

AppSettings::AppSettings(bool persist, QObject* parent) : QObject(parent), m_persist(persist) {
    if (!m_persist) return;
    const QSettings s(settingsFile(), QSettings::IniFormat);
    m_showCurrentFlow = s.value(QStringLiteral("anzeige/stromfluss"), true).toBool();
    m_reduceMotion = s.value(QStringLiteral("anzeige/animationen_reduziert"), false).toBool();
    m_showSplash = s.value(QStringLiteral("anzeige/startbildschirm"), true).toBool();
    m_unlockAll = s.value(QStringLiteral("lernpfad/alles_frei"), false).toBool();
    m_fontLevel = std::clamp(s.value(QStringLiteral("anzeige/schriftgroesse"), 2).toInt(), 0, 2);
}

bool AppSettings::splashAvailable() {
#if (defined(Q_OS_WIN) || defined(Q_OS_LINUX) || defined(Q_OS_MACOS)) && !defined(Q_OS_ANDROID)
    return true;
#else
    return false;
#endif
}

void AppSettings::save(const QString& key, bool value) {
    if (!m_persist) return;
    QSettings s(settingsFile(), QSettings::IniFormat);
    s.setValue(key, value);
}

void AppSettings::setShowCurrentFlow(bool on) {
    if (on == m_showCurrentFlow) return;
    m_showCurrentFlow = on;
    save(QStringLiteral("anzeige/stromfluss"), on);
    emit changed();
}

void AppSettings::setReduceMotion(bool on) {
    if (on == m_reduceMotion) return;
    m_reduceMotion = on;
    save(QStringLiteral("anzeige/animationen_reduziert"), on);
    emit changed();
}

void AppSettings::setShowSplash(bool on) {
    if (on == m_showSplash) return;
    m_showSplash = on;
    save(QStringLiteral("anzeige/startbildschirm"), on);
    emit changed();
}

void AppSettings::setFontLevel(int level) {
    level = std::clamp(level, 0, 2);
    if (level == m_fontLevel) return;
    m_fontLevel = level;
    if (m_persist) {
        QSettings s(settingsFile(), QSettings::IniFormat);
        s.setValue(QStringLiteral("anzeige/schriftgroesse"), level);
    }
    emit changed();
}

void AppSettings::setUnlockAll(bool on) {
    if (on == m_unlockAll) return;
    m_unlockAll = on;
    save(QStringLiteral("lernpfad/alles_frei"), on);
    emit changed();
}

} // namespace volta

#include "app_settings.hpp"

#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>

#include <algorithm>

namespace volta {

namespace {
QString defaultFile() {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/einstellungen.ini");
}
const QStringList kWireColors{QStringLiteral("rot"), QStringLiteral("schwarz"), QStringLiteral("blau"),
                              QStringLiteral("gelb"), QStringLiteral("gruen"), QStringLiteral("weiss")};
} // namespace

AppSettings::AppSettings(bool persist, QString file, QObject* parent)
    : QObject(parent), m_persist(persist), m_file(std::move(file)) {
    if (!m_persist) return;
    if (m_file.isEmpty()) m_file = defaultFile();
    const QSettings s(m_file, QSettings::IniFormat);
    m_showCurrentFlow = s.value(QStringLiteral("anzeige/stromfluss"), true).toBool();
    m_reduceMotion = s.value(QStringLiteral("anzeige/animationen_reduziert"), false).toBool();
    m_showSplash = s.value(QStringLiteral("anzeige/startbildschirm"), true).toBool();
    m_soundOn = s.value(QStringLiteral("anzeige/ton"), true).toBool();
    m_unlockAll = s.value(QStringLiteral("lernpfad/alles_frei"), false).toBool();
    m_fontLevel = std::clamp(s.value(QStringLiteral("anzeige/schriftgroesse"), 2).toInt(), 0, 2);
    const int cols = s.value(QStringLiteral("labor/felder_je_reihe"), 12).toInt();
    m_boardColumns = (cols == 8 || cols == 10 || cols == 12) ? cols : 12;
    m_meterVisible = s.value(QStringLiteral("labor/messgeraet"), false).toBool();
    const QString color = s.value(QStringLiteral("labor/leitungsfarbe"), QStringLiteral("rot")).toString();
    m_wireColor = kWireColors.contains(color) ? color : QStringLiteral("rot");
    m_schematicVisible = s.value(QStringLiteral("labor/schaltplan"), false).toBool();
    m_panelCollapsed = s.value(QStringLiteral("labor/experimentfeld_eingeklappt"), false).toBool();
}

bool AppSettings::splashAvailable() {
#if (defined(Q_OS_WIN) || defined(Q_OS_LINUX)) && !defined(Q_OS_ANDROID)
    return true;
#else
    return false;
#endif
}

void AppSettings::save(const QString& key, const QVariant& value) {
    if (!m_persist) return;
    QSettings s(m_file, QSettings::IniFormat);
    s.setValue(key, value);
}

void AppSettings::setFontLevel(int level) { set(m_fontLevel, std::clamp(level, 0, 2), "anzeige/schriftgroesse"); }

void AppSettings::setBoardColumns(int columns) {
    if (columns != 8 && columns != 10 && columns != 12) return;
    set(m_boardColumns, columns, "labor/felder_je_reihe");
}

void AppSettings::setWireColor(const QString& color) {
    if (!kWireColors.contains(color)) return;
    set(m_wireColor, color, "labor/leitungsfarbe");
}

} // namespace volta

#include "app_settings.hpp"

#include <QDir>
#include <QCryptographicHash>
#include <QDateTime>
#include <QRandomGenerator>
#include <QRegularExpression>
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
    m_meterDocked = s.value(QStringLiteral("labor/messgeraet_im_pult"), false).toBool();
    m_autoBoardColumns = s.value(QStringLiteral("labor/felder_automatisch"), true).toBool();
    m_silverFields = s.value(QStringLiteral("anzeige/felder_silber"), true).toBool();
    m_profilesEnabled = s.value(QStringLiteral("profile/aktiv"), false).toBool();
    m_tickEnabled = s.value(QStringLiteral("anzeige/tick"), true).toBool();
    m_pinHash = s.value(QStringLiteral("eltern/pin_pruefwert")).toString();
    m_pinSalt = s.value(QStringLiteral("eltern/pin_salz")).toString();
    m_pinFailures = s.value(QStringLiteral("eltern/fehlversuche"), 0).toInt();
    m_pinLockedUntil = s.value(QStringLiteral("eltern/gesperrt_bis"), 0).toLongLong();
    const QString sound = s.value(QStringLiteral("anzeige/ton_klasse"), QStringLiteral("normal")).toString();
    m_classroomSound = (sound == QLatin1String("aus") || sound == QLatin1String("kopfhoerer")) ? sound : QStringLiteral("normal");
    const QString color = s.value(QStringLiteral("labor/leitungsfarbe"), QStringLiteral("rot")).toString();
    m_wireColor = kWireColors.contains(color) ? color : QStringLiteral("rot");
    m_autoWireColor = s.value(QStringLiteral("labor/leitungsfarbe_automatisch"), true).toBool();
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

void AppSettings::setClassroomSound(const QString& mode) {
    const QString m = (mode == QLatin1String("aus") || mode == QLatin1String("kopfhoerer")) ? mode : QStringLiteral("normal");
    if (m == m_classroomSound) return;
    m_classroomSound = m;
    if (m_persist) QSettings(m_file, QSettings::IniFormat).setValue(QStringLiteral("anzeige/ton_klasse"), m);
    emit changed();
}

QString AppSettings::hashPin(const QString& salt, const QString& pin) {
    return QString::fromLatin1(
        QCryptographicHash::hash((salt + QLatin1Char(':') + pin).toUtf8(), QCryptographicHash::Sha256).toHex());
}

void AppSettings::savePinState() {
    if (!m_persist) return;
    QSettings s(m_file, QSettings::IniFormat);
    s.setValue(QStringLiteral("eltern/pin_pruefwert"), m_pinHash);
    s.setValue(QStringLiteral("eltern/pin_salz"), m_pinSalt);
    s.setValue(QStringLiteral("eltern/fehlversuche"), m_pinFailures);
    s.setValue(QStringLiteral("eltern/gesperrt_bis"), m_pinLockedUntil);
}

int AppSettings::parentPinLockSeconds() const {
    const qint64 rest = m_pinLockedUntil - QDateTime::currentMSecsSinceEpoch();
    return rest > 0 ? static_cast<int>((rest + 999) / 1000) : 0;
}

bool AppSettings::checkParentPin(const QString& pin) {
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

bool AppSettings::setParentPin(const QString& pin) {
    static const QRegularExpression digits(QStringLiteral("^[0-9]{4,8}$"));
    if (!digits.match(pin).hasMatch()) return false;
    QByteArray salt(16, Qt::Uninitialized);
    QRandomGenerator::system()->fillRange(reinterpret_cast<quint32*>(salt.data()), salt.size() / 4);
    m_pinSalt = QString::fromLatin1(salt.toHex());
    m_pinHash = hashPin(m_pinSalt, pin);
    m_pinFailures = 0;
    savePinState();
    emit changed();
    return true;
}

} // namespace volta

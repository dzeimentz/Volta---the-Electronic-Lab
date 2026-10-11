#include "language_service.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QLocale>
#include <QQmlEngine>
#include <QSettings>
#include <QStandardPaths>
#include <QVariantMap>

namespace volta {

namespace {
struct Language {
    const char* code;
    const char* shortName; // Anzeige
    const char* name;      // Eigenname
    bool available;
};
// Reihenfolge wie docs/i18n.md; Luxemburgisch folgt mit eigenen Aufnahmen (Phase 6)
constexpr Language kLanguages[] = {{"de", "DE", "Deutsch", true},
                                   {"fr", "FR", "Français", true},
                                   {"en", "EN", "English", true},
                                   {"lb", "LU", "Lëtzebuergesch", false}};
} // namespace

LanguageService::LanguageService(QQmlEngine* engine, bool persist, QObject* parent)
    : QObject(parent), m_engine(engine), m_persist(persist) {
    if (m_persist) {
        const QSettings s(settingsFile(), QSettings::IniFormat);
        const QString saved = s.value(QStringLiteral("anzeige/sprache"), QStringLiteral("de")).toString();
        if (saved != m_code) setLanguage(saved);
    }
}

bool LanguageService::hasSavedChoice() const {
    return m_persist && QSettings(settingsFile(), QSettings::IniFormat).contains(QStringLiteral("anzeige/sprache"));
}

LanguageService::~LanguageService() {
    if (m_installed) QCoreApplication::removeTranslator(&m_translator);
}

QVariantList LanguageService::languages() const {
    QVariantList out;
    for (const Language& l : kLanguages)
        out << QVariantMap{{QStringLiteral("code"), QString::fromLatin1(l.code)},
                           {QStringLiteral("short"), QString::fromLatin1(l.shortName)},
                           {QStringLiteral("name"), QString::fromUtf8(l.name)},
                           {QStringLiteral("available"), l.available}};
    return out;
}

bool LanguageService::isAvailable(const QString& code) {
    for (const Language& l : kLanguages)
        if (code == QLatin1String(l.code)) return l.available;
    return false;
}

bool LanguageService::setLanguage(const QString& code) {
    if (!isAvailable(code)) return false;
    if (code == m_code) return true;
    if (m_installed) {
        QCoreApplication::removeTranslator(&m_translator);
        m_installed = false;
    }
    // Deutsch ist Quellsprache (keine Uebersetzung noetig)
    if (code != QLatin1String("de") && m_translator.load(QStringLiteral(":/i18n/volta_%1.qm").arg(code)))
        m_installed = QCoreApplication::installTranslator(&m_translator);
    QLocale::setDefault(QLocale(code));
    m_code = code;
    if (m_persist) {
        QSettings s(settingsFile(), QSettings::IniFormat);
        s.setValue(QStringLiteral("anzeige/sprache"), m_code);
    }
    if (m_engine) m_engine->retranslate();
    emit languageChanged(m_code);
    return true;
}

QString LanguageService::settingsFile() const {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/einstellungen.ini");
}

} // namespace volta

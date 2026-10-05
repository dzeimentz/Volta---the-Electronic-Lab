#pragma once

#include <QObject>
#include <QString>
#include <QTranslator>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;

namespace volta {

// Anzeigesprache der Programmoberflaeche (docs/i18n.md): Qt-Uebersetzung (:/i18n/volta_<code>.qm), Zahlenformat
// (QLocale) und QQmlEngine::retranslate(). Die Inhalte (Experimente) laedt der LabController ueber das Signal
// languageChanged nach. Angezeigt wird das Kuerzel (DE, FR, EN, LU); intern gilt der ISO-Code (lb fuer LU).
// Wahl wird lokal gespeichert (QStandardPaths, keine Datenerhebung).
class LanguageService : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(LanguageService)
    QML_UNCREATABLE("Wird von main.cpp bereitgestellt")
    Q_PROPERTY(QString language READ language NOTIFY languageChanged)
    Q_PROPERTY(QVariantList languages READ languages CONSTANT)

public:
    // persist: Wahl laden und speichern (Tests: false)
    explicit LanguageService(QQmlEngine* engine, bool persist = true, QObject* parent = nullptr);
    ~LanguageService() override;

    QString language() const { return m_code; }
    // {code, short, name, available} je Sprache; LU (lb) folgt spaeter und ist noch nicht waehlbar
    QVariantList languages() const;
    static bool isAvailable(const QString& code);

    Q_INVOKABLE bool setLanguage(const QString& code);

signals:
    void languageChanged(const QString& code);

private:
    QString settingsFile() const;

    QQmlEngine* m_engine;
    bool m_persist;
    QTranslator m_translator;
    bool m_installed = false;
    QString m_code = QStringLiteral("de");
};

} // namespace volta

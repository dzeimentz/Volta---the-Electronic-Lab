#pragma once

// Einstellungen von Zuse 1 wie bei Volta: Anzeigesprache (DE, FR, EN; LU folgt spaeter) und Schriftgroesse
// (drei Stufen, groesste ist die Vorgabe). Gespeichert in <Ablage>/einstellungen.ini, nichts verlaesst das Geraet.
#include <QObject>
#include <QString>
#include <QTranslator>
#include <QVariantList>

namespace zuse {

class ZuseSettings : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString language READ language NOTIFY languageChanged)
    Q_PROPERTY(QVariantList languages READ languages CONSTANT)
    Q_PROPERTY(int fontLevel READ fontLevel WRITE setFontLevel NOTIFY fontLevelChanged)
    Q_PROPERTY(qreal fontScale READ fontScale NOTIFY fontLevelChanged)

public:
    // dataDir leer: nichts laden und speichern (Tests); language: Vorgabe, falls noch nichts gewaehlt wurde
    ZuseSettings(const QString& dataDir, const QString& language, QObject* parent = nullptr);
    ~ZuseSettings() override;

    QString language() const { return m_language; }
    QVariantList languages() const; // {code, short, name}
    static bool isAvailable(const QString& code);
    Q_INVOKABLE bool setLanguage(const QString& code);

    int fontLevel() const { return m_fontLevel; }
    void setFontLevel(int level);
    qreal fontScale() const { return 0.8 + 0.1 * m_fontLevel; } // 0,8 / 0,9 / 1,0

signals:
    void languageChanged(const QString& code);
    void fontLevelChanged();

private:
    void save() const;
    void installTranslator();

    QString m_file;
    QString m_language = QStringLiteral("de");
    int m_fontLevel = 2;
    QTranslator m_translator;
    bool m_installed = false;
};

} // namespace zuse

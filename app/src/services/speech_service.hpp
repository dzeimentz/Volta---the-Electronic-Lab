#pragma once

#include <QObject>
#include <QString>

class QTextToSpeech;

namespace volta::services {

// Vorlesen ueber die Systemstimme (Konzept 5.6, Barrierefreiheit; Qt TextToSpeech). Gesprochen wird in der
// eingestellten Sprache, sofern das System dafuer eine Stimme hat (DE/FR/EN). Fuer Luxemburgisch reichen die
// Systemstimmen nicht: Dort folgen eigene Tonaufnahmen im Inhaltspaket (docs/i18n.md); bis dahin ist available
// false. Kein Netz: nur die lokale Sprachausgabe des Systems (Windows: SAPI).
class SpeechService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY changed)   // Stimme fuer die Sprache vorhanden
    Q_PROPERTY(bool speaking READ speaking NOTIFY changed)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY changed)

public:
    explicit SpeechService(QObject* parent = nullptr);

    bool available() const { return m_available; }
    bool speaking() const { return m_speaking; }
    QString language() const { return m_language; }
    void setLanguage(const QString& code);

    Q_INVOKABLE void say(const QString& text);
    Q_INVOKABLE void stop();

signals:
    void changed();

private:
    void chooseVoice();

    QTextToSpeech* m_tts = nullptr;
    QString m_language = QStringLiteral("de");
    bool m_available = false;
    bool m_speaking = false;
};

} // namespace volta::services

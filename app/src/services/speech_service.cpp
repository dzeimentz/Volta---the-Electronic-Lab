#include "speech_service.hpp"

#include <QLocale>
#include <QTextToSpeech>

namespace volta::services {

SpeechService::SpeechService(QObject* parent) : QObject(parent) {
#ifdef Q_OS_WIN
    m_tts = new QTextToSpeech(QStringLiteral("sapi"), this); // lokale Windows-Sprachausgabe
#else
    m_tts = new QTextToSpeech(this);
#endif
    connect(m_tts, &QTextToSpeech::stateChanged, this, [this](QTextToSpeech::State s) {
        const bool now = s == QTextToSpeech::Speaking || s == QTextToSpeech::Synthesizing;
        if (now != m_speaking) {
            m_speaking = now;
            emit changed();
        }
        if (s == QTextToSpeech::Ready && !m_available) chooseVoice(); // Engine ist erst jetzt bereit
    });
    chooseVoice();
}

void SpeechService::setLanguage(const QString& code) {
    if (code == m_language) return;
    m_language = code;
    stop();
    chooseVoice();
    emit changed();
}

void SpeechService::chooseVoice() {
    const bool before = m_available;
    m_available = false;
    if (m_tts->state() == QTextToSpeech::Error || m_language == QLatin1String("lb")) {
        if (before != m_available) emit changed();
        return;
    }
    const QLocale wanted(m_language);
    for (const QLocale& l : m_tts->availableLocales()) {
        if (l.language() != wanted.language()) continue;
        m_tts->setLocale(l);
        m_tts->setRate(-0.1); // etwas langsamer: Kinder lesen mit
        m_available = true;
        break;
    }
    if (before != m_available) emit changed();
}

void SpeechService::say(const QString& text) {
    if (!m_available || text.trimmed().isEmpty()) return;
    m_tts->stop();
    m_tts->say(text);
}

void SpeechService::stop() {
    if (m_tts->state() == QTextToSpeech::Speaking || m_tts->state() == QTextToSpeech::Synthesizing) m_tts->stop();
}

} // namespace volta::services

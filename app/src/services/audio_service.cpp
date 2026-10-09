#include "audio_service.hpp"

#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSink>
#include <QMediaDevices>

#include <algorithm>
#include <cstring>

namespace volta::services {

// Liefert dem QAudioSink die Abtastwerte des Klangerzeugers (Ziehbetrieb, unendlicher Strom)
class AudioService::Source : public QIODevice {
public:
    Source(std::mutex& mutex, model::ToneSynth& synth) : m_mutex(mutex), m_synth(synth) {}
    bool isSequential() const override { return true; }
    qint64 bytesAvailable() const override { return 4096 + QIODevice::bytesAvailable(); }

protected:
    qint64 readData(char* data, qint64 maxSize) override {
        const auto frames = static_cast<std::size_t>(maxSize / 2);
        if (frames == 0) return 0;
        std::lock_guard<std::mutex> lock(m_mutex);
        m_synth.render(reinterpret_cast<std::int16_t*>(data), frames);
        return static_cast<qint64>(frames * 2);
    }
    qint64 writeData(const char*, qint64) override { return -1; }

private:
    std::mutex& m_mutex;
    model::ToneSynth& m_synth;
};

AudioService::AudioService(QObject* parent) : QObject(parent) {
    const QAudioDevice device = QMediaDevices::defaultAudioOutput();
    m_available = !device.isNull();
    QAudioFormat format;
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);
    format.setSampleRate(48000);
    if (m_available && !device.isFormatSupported(format))
        format.setSampleRate(std::max(8000, device.preferredFormat().sampleRate()));
    m_synth = std::make_unique<model::ToneSynth>(format.sampleRate());
    if (!m_available) return;
    m_source = std::make_unique<Source>(m_mutex, *m_synth);
    m_source->open(QIODevice::ReadOnly);
    m_sink = new QAudioSink(device, format, this);
    m_sink->setBufferSize(format.bytesForDuration(60000)); // ca. 60 ms Puffer: kurze Verzoegerung
    update();
}

AudioService::~AudioService() {
    if (m_sink) m_sink->stop();
}

void AudioService::setEnabled(bool on) {
    if (m_enabled == on) return;
    m_enabled = on;
    update();
    emit changed();
}

void AudioService::setActive(bool on) {
    if (m_active == on) return;
    m_active = on;
    update();
    emit changed();
}

void AudioService::update() {
    if (!m_sink) return;
    const bool run = m_enabled && m_active && !m_suppressed;
    if (run && m_sink->state() == QAudio::StoppedState) {
        m_sink->start(m_source.get());
    } else if (!run && m_sink->state() != QAudio::StoppedState) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_synth->setVoices({});
        }
        m_sink->stop();
    }
}

void AudioService::setVoices(const std::vector<model::SynthVoice>& voices) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_synth->setVoices(m_enabled && m_active && !m_suppressed ? voices : std::vector<model::SynthVoice>{});
}

void AudioService::click(double strength) {
    if (!m_enabled || !m_active || m_suppressed) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_synth->click(strength);
}

} // namespace volta::services

#include "audio_stream.hpp"

#include <algorithm>
#include <cmath>

namespace volta::model {

AudioStream::AudioStream(int sampleRate, std::size_t capacity)
    : m_rate(std::max(sampleRate, 8000)), m_buf(std::max<std::size_t>(capacity, 1024)) {}

std::size_t AudioStream::buffered() const {
    return m_write.load(std::memory_order_acquire) - m_read.load(std::memory_order_acquire);
}

void AudioStream::push(float currentA) {
    const std::size_t w = m_write.load(std::memory_order_relaxed);
    if (w - m_read.load(std::memory_order_acquire) >= m_buf.size()) return; // voll
    m_buf[w % m_buf.size()] = std::isfinite(currentA) ? currentA : 0.0f;
    m_write.store(w + 1, std::memory_order_release);
}

bool AudioStream::pop(float& v) {
    const std::size_t r = m_read.load(std::memory_order_relaxed);
    if (r == m_write.load(std::memory_order_acquire)) return false;
    v = m_buf[r % m_buf.size()];
    m_read.store(r + 1, std::memory_order_release);
    return true;
}

void AudioStream::drain() {
    m_read.store(m_write.load(std::memory_order_acquire), std::memory_order_release);
    m_playing = false;
    m_fade = 0.0;
    m_last = 0.0;
}

void AudioStream::mixInto(std::int16_t* out, std::size_t frames, double volume) {
    const auto prebuffer = static_cast<std::size_t>(kPrebufferS * m_rate);
    const auto maxLag = static_cast<std::size_t>(kMaxLagS * m_rate);
    // zu weit hinten: die aeltesten Werte verwerfen, bis wieder der Vorlauf im Puffer ist
    if (buffered() > maxLag) {
        float dummy = 0.0f;
        while (buffered() > prebuffer && pop(dummy)) {}
    }
    if (!m_playing) {
        if (buffered() < prebuffer) return;
        m_playing = true;
        m_starved = 0;
    }
    const double dcR = 1.0 - 2.0 * 3.14159265358979 * 20.0 / m_rate; // Hochpass ca. 20 Hz
    const double release = std::exp(-1.0 / (0.3 * m_rate));          // Begrenzer erholt sich in ca. 0,3 s
    const double fadeStep = 1.0 / (0.02 * m_rate);                     // 20 ms einblenden
    const double scale = kHeadroom / kFullScaleA;
    const double vol = std::clamp(volume, 0.0, 1.0);
    for (std::size_t k = 0; k < frames; ++k) {
        float v = 0.0f;
        if (pop(v)) {
            m_last = v;
            m_starved = 0;
        } else {
            // Leerlauf: ausklingen lassen, nach 20 ms wieder auf den Vorlauf warten
            if (m_starved == 0) m_underruns.fetch_add(1, std::memory_order_relaxed);
            m_last *= 0.995;
            v = static_cast<float>(m_last);
            if (++m_starved > static_cast<std::size_t>(0.02 * m_rate)) {
                m_playing = false;
                m_fade = 0.0;
                return;
            }
        }
        // Gleichanteil heraus
        const double x = v * scale;
        const double y = x - m_dcIn + dcR * m_dcOut;
        m_dcIn = x;
        m_dcOut = y;
        // Begrenzer: Spitze sofort, Erholung langsam; dann weiche Saettigung
        m_env = std::max(std::fabs(y), m_env * release);
        const double gain = m_env > kHeadroom ? kHeadroom / m_env : 1.0;
        double s = y * gain;
        s = kHeadroom * std::tanh(s / kHeadroom);
        m_fade = std::min(1.0, m_fade + fadeStep);
        const double mixed = out[k] + s * m_fade * vol * 32767.0;
        out[k] = static_cast<std::int16_t>(std::clamp(mixed, -32767.0, 32767.0));
    }
}

} // namespace volta::model

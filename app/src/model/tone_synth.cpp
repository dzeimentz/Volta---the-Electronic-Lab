#include "tone_synth.hpp"

#include <algorithm>
#include <cmath>

namespace volta::model {

namespace {
constexpr double kTwoPi = 6.283185307179586;
constexpr double kGlideS = 0.008;
constexpr double kClickS = 0.015;
} // namespace

ToneSynth::ToneSynth(int sampleRate)
    : m_rate(sampleRate > 0 ? sampleRate : 48000),
      m_glide(1.0 - std::exp(-1.0 / (kGlideS * m_rate))),
      m_clickDecay(std::exp(-1.0 / (kClickS * m_rate / 4.0))) {}

void ToneSynth::setVoices(const std::vector<SynthVoice>& voices) {
    for (std::size_t i = 0; i < kMaxVoices; ++i) {
        Slot& s = m_slots[i];
        if (i < voices.size() && std::isfinite(voices[i].frequencyHz) && voices[i].frequencyHz >= 20.0 &&
            std::isfinite(voices[i].level) && voices[i].level > 0.0) {
            const double f = std::min(voices[i].frequencyHz, 0.2 * m_rate); // 5. Oberton unter Nyquist
            if (s.amp < 1e-4) s.freq = f; // neuer Ton: sofort auf der richtigen Hoehe einsetzen
            s.targetFreq = f;
            s.targetAmp = std::clamp(voices[i].level, 0.0, 1.0);
        } else {
            s.targetAmp = 0.0;
        }
    }
}

void ToneSynth::setVolume(double volume) { m_volume = std::isfinite(volume) ? std::clamp(volume, 0.0, 1.0) : 0.0; }

void ToneSynth::click(double strength) {
    if (std::isfinite(strength)) m_click = std::max(m_click, std::clamp(strength, 0.0, 1.0));
}

void ToneSynth::render(std::int16_t* out, std::size_t frames) {
    const double gain = kHeadroom * m_volume / static_cast<double>(kMaxVoices) * 2.0;
    for (std::size_t n = 0; n < frames; ++n) {
        double sum = 0.0;
        for (Slot& s : m_slots) {
            s.amp += (s.targetAmp - s.amp) * m_glide;
            s.freq += (s.targetFreq - s.freq) * m_glide;
            if (s.amp < 1e-5 && s.targetAmp == 0.0) {
                s.amp = 0.0;
                continue;
            }
            s.phase += kTwoPi * s.freq / m_rate;
            if (s.phase >= kTwoPi) s.phase -= kTwoPi;
            // weiches Rechteck: Grundton mit 3. und 5. Oberton (Fourier-Reihe, Amplitude ca. 1)
            const double w = std::sin(s.phase) + std::sin(3.0 * s.phase) / 3.0 + std::sin(5.0 * s.phase) / 5.0;
            sum += s.amp * w * 0.85;
        }
        if (m_click > 1e-4) {
            m_noise = m_noise * 1664525u + 1013904223u; // deterministisches Rauschen
            const double r = static_cast<double>(m_noise >> 8) / static_cast<double>(1u << 24) * 2.0 - 1.0;
            sum += m_click * (0.6 + 0.4 * r) * 1.5;
            m_click *= m_clickDecay;
        } else {
            m_click = 0.0;
        }
        const double y = std::tanh(sum * gain / kHeadroom) * kHeadroom; // weich begrenzt auf kHeadroom
        out[n] = static_cast<std::int16_t>(std::lround(y * 32767.0));
    }
}

} // namespace volta::model

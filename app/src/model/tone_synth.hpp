#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace volta::model {

// Eine Tonquelle fuer die Ausgabe: Frequenz in Hz und Staerke 0..1 (aus der Simulation: Lautsprecher, Summer).
struct SynthVoice {
    double frequencyHz = 0.0;
    double level = 0.0;
};

// Klangerzeuger fuer die Tonausgabe ueber die Soundkarte (Konzept 5.4, Klanganzeige: »Der Ton selbst wird zusaetzlich
// ueber den Lautsprecher ausgegeben«). Ohne Qt, damit er testbar bleibt; der AudioService ruft render() aus dem
// Audio-Thread.
// - Bis zu kMaxVoices Stimmen; jede ist ein weiches Rechteck (Grundton mit 3. und 5. Oberton), wie Summer und
//   geschalteter Lautsprecher klingen, aber ohne harte Kanten (kein Aliasing bis ca. 4 kHz bei 48 kHz).
// - Lautstaerke und Tonhoehe gleiten (ca. 8 ms), damit Wechsel nicht knacken; die Phase laeuft weiter.
// - click(): kurzes Knacken (abklingender Impuls, ca. 15 ms) fuer Impulse unter der Hoergrenze.
// - Hoechste Summenaussteuerung ca. 30 % (Gehoerschutz fuer Kinder), weich begrenzt.
class ToneSynth {
public:
    static constexpr std::size_t kMaxVoices = 4;
    static constexpr double kHeadroom = 0.3;

    explicit ToneSynth(int sampleRate = 48000);

    int sampleRate() const { return m_rate; }
    void setVoices(const std::vector<SynthVoice>& voices); // ueberzaehlige Stimmen werden ignoriert
    void setVolume(double volume);                         // 0..1, Gesamtlautstaerke
    void click(double strength);                           // 0..1
    void render(std::int16_t* out, std::size_t frames);

private:
    struct Slot {
        double phase = 0.0;
        double freq = 0.0;       // aktuelle (gleitende) Frequenz
        double targetFreq = 0.0;
        double amp = 0.0;        // aktuelle (gleitende) Staerke
        double targetAmp = 0.0;
    };
    int m_rate;
    double m_volume = 1.0;
    double m_glide;              // Gleitfaktor je Abtastwert
    std::array<Slot, kMaxVoices> m_slots{};
    double m_click = 0.0;        // Huellkurve des Knackens
    double m_clickDecay;
    std::uint32_t m_noise = 12345u;
};

} // namespace volta::model

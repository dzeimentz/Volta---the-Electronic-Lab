#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace volta::model {

// Schrittweite der hoerbaren Simulation (Leistungstest 14.10.2026, docs/volta2-planung.md)
inline constexpr double kAudioStepS = 20e-6;
// Schrittweite mit Speicherdrossel (Schaltwandler ab Volta 3, Konzept Volta 3 Kap. 4)
inline constexpr double kSwitchStepS = 5e-6;
// Thyristor, Triac, Diac (ab Volta 3): Schrittweite mit Ereignisortung fuer Zuenden und Loeschen
inline constexpr double kThyristorStepS = 1e-4;

// Hoerbare Simulation (ab Volta 2, Konzept 5.3): der Lautsprecherstrom aus der Simulation als Tonstrom.
// Ein Erzeuger (Simulations-Thread) legt Abtastwerte in der Abtastrate der Soundkarte ab (die Bruecke rechnet sie
// aus den 20-us-Schritten um), ein Verbraucher (Audio-Thread) mischt sie in die Ausgabe. Ohne Sperren (ein Erzeuger,
// ein Verbraucher, atomare Zeiger); ohne Qt, damit er testbar bleibt.
// Verbraucherseite:
//  - Vorlauf: erst ab kPrebufferS im Puffer wird ausgegeben (rund 50 ms Verzoegerung, faengt Ruckler ab).
//  - Leerlauf (die Simulation kommt nicht nach): der letzte Wert klingt aus, danach wieder Vorlauf.
//  - Laeuft der Puffer ueber kMaxLagS voll (Uhren von Rechner und Soundkarte laufen leicht verschieden), werden die
//    aeltesten Werte verworfen.
//  - Gleichanteil wird herausgefiltert (Hochpass ca. 20 Hz): Gleichstrom ist am Lautsprecher stumm.
//  - Begrenzer (Kinderohren): Spitzen ueber kHeadroom werden sofort heruntergeregelt und erholen sich langsam
//    (ca. 0,3 s), danach weiche Saettigung; nach dem Einsetzen blendet der Ton in 20 ms ein (kein Knacken).
class AudioStream {
public:
    static constexpr double kPrebufferS = 0.05;
    static constexpr double kMaxLagS = 0.2;
    static constexpr double kHeadroom = 0.3;   // wie ToneSynth: hoechste Aussteuerung
    static constexpr double kFullScaleA = 0.1; // Lautsprecherstrom fuer volle Aussteuerung (wie core::Speaker)

    explicit AudioStream(int sampleRate = 48000, std::size_t capacity = 1u << 15);

    int sampleRate() const { return m_rate; }

    // Erzeuger: Lautsprecherstrom in A; ist der Puffer voll, geht der Wert verloren
    void push(float currentA);
    // Verbraucher: Ton addieren (gesaettigt auf 16 Bit); volume 0..1
    void mixInto(std::int16_t* out, std::size_t frames, double volume = 1.0);
    // Verbraucher: Puffer leeren und neu vorlaufen (Ton aus)
    void drain();

    std::size_t buffered() const;
    bool playing() const { return m_playing; }        // nur im Verbraucher-Thread verlaesslich
    std::uint64_t underruns() const { return m_underruns.load(std::memory_order_relaxed); }

private:
    bool pop(float& v);

    int m_rate;
    std::vector<float> m_buf;
    std::atomic<std::size_t> m_write{0};
    std::atomic<std::size_t> m_read{0};
    std::atomic<std::uint64_t> m_underruns{0};
    // Verbraucherzustand
    bool m_playing = false;
    double m_last = 0.0;     // letzter Wert (fuer das Ausklingen bei Leerlauf)
    std::size_t m_starved = 0;
    double m_dcIn = 0.0, m_dcOut = 0.0;
    double m_env = 0.0;      // Huellkurve des Begrenzers
    double m_fade = 0.0;     // Einblenden 0..1
};

} // namespace volta::model

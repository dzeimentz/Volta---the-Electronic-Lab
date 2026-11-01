#pragma once

#include <array>
#include <cstddef>
#include <utility>
#include <vector>

namespace volta::model {

// Oszilloskop ab Volta 2 (Konzept 5.2): zwei Kanaele an den Tastkoepfen K1/K2 (Bezug 0V). Die Simulationsbruecke
// tastet beide Kanaele in jedem Schritt in einen Ringpuffer ab; hier entsteht daraus das Bild: Zeitbasis mit
// Trigger (Flanke, Pegel, Kanal; ohne Ausloesung frei laufend), Rollbetrieb (der Zeitschreiber von Volta 1, die
// Kurve laeuft von rechts nach links), XY-Betrieb (K1 waagrecht, K2 senkrecht) und die Kennwerte je Kanal.
// Ohne Qt, damit testbar.

inline constexpr std::size_t kScopeChannels = 2;
inline constexpr int kScopeDivX = 10; // Teilungen waagrecht
inline constexpr int kScopeDivY = 8;  // Teilungen senkrecht (Nulllinie in der Mitte, verschiebbar)
inline constexpr int kScopePointsPerScreen = 1000; // Abtastpunkte je Bildbreite

struct ScopeSample {
    double t = 0.0;
    std::array<double, kScopeChannels> v{};
};

// Zeitbasis je Teilung (1-2-5); Normalbetrieb 100 µs ... 1 s, Rollbetrieb 100 ms ... 1 min
const std::vector<double>& scopeTimebases();
const std::vector<double>& scopeVoltsPerDiv(); // 10 mV ... 5 V je Teilung
inline constexpr int kScopeFirstRollTimebase = 9;   // 100 ms
inline constexpr int kScopeLastNormalTimebase = 12; // 1 s

struct ScopeChannelSettings {
    int voltsIndex = 7;   // 2 V je Teilung
    int position = 0;     // Lage der Nulllinie in Teilungen ueber der Mitte (-4 ... 4)
};

struct ScopeSettings {
    int timebaseIndex = 3;     // 1 ms je Teilung
    bool roll = false;         // Rollbetrieb (ohne Trigger)
    bool xy = false;           // XY-Betrieb
    int triggerChannel = 0;
    bool triggerRising = true;
    double triggerLevelV = 0.0;
    std::array<ScopeChannelSettings, kScopeChannels> channels{};

    double timebaseS() const;
    double screenS() const { return timebaseS() * kScopeDivX; }
    // Abtastabstand fuer den Ringpuffer und passende Schrittweite der Simulation (feiner bei kurzer Zeitbasis)
    double sampleIntervalS() const { return screenS() / kScopePointsPerScreen; }
    double simStepS() const;
};

// Kennwerte eines Kanals im Bildausschnitt
struct ScopeMeasure {
    bool any = false;
    double min = 0.0, max = 0.0, mean = 0.0;
    double frequencyHz = 0.0; // 0 = keine Periode erkennbar
};

struct ScopeFrame {
    bool triggered = false; // Normalbetrieb: Trigger hat ausgeloest (sonst frei laufend)
    double startS = 0.0;    // Zeit am linken Rand
    // Zeitbetrieb: je Kanal Punkte (x in Teilungen 0 ... 10, Spannung in V); XY: (V an K1, V an K2)
    std::array<std::vector<std::pair<double, double>>, kScopeChannels> traces;
    std::vector<std::pair<double, double>> xy;
    std::array<ScopeMeasure, kScopeChannels> measure;
};

// Bild aus den Abtastwerten (zeitlich geordnet); has[c] = Kanal angeschlossen
ScopeFrame scopeFrame(const std::vector<ScopeSample>& samples, const ScopeSettings& settings,
                      const std::array<bool, kScopeChannels>& has);

// Kennwerte aus einem Abschnitt: Spitze-Spitze, Mittel, Frequenz aus steigenden Durchgaengen durch die Mitte
// zwischen Minimum und Maximum (Hysterese 10 % der Spannweite)
ScopeMeasure scopeMeasure(const std::vector<ScopeSample>& samples, std::size_t channel, double fromS, double toS);

} // namespace volta::model

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

// Logikanalysator (ab Volta 3, Stufe 7): acht Kanaele an den Logik-Tastkoepfen LA1-LA8 (Bezug 0V). Die Bruecke
// tastet die Spannungen ab; H, L oder ? (verbotener Bereich) entscheidet die Anzeige mit den Schwellen 70 % und
// 30 % der Batteriespannung (CMOS). Zwei Zeitfenster: fein 20 ms (Prellen) und grob 5 s (Bedienen).
inline constexpr std::size_t kLogicChannels = 8;
inline constexpr std::size_t kLogicPoints = 1000; // Abtastpunkte je Fenster
inline constexpr double kLogicFineWindowS = 20e-3;
inline constexpr double kLogicCoarseWindowS = 5.0;
inline constexpr double kLogicHighFraction = 0.7;
inline constexpr double kLogicLowFraction = 0.3;
struct LogicSample {
    double t = 0.0;
    std::array<float, kLogicChannels> v{};
};

// Kennlinienschreiber (ab Volta 2, Konzept 5.2): hoechste Pruefspannung, nur positiv oder beide Richtungen, Strom-
// begrenzung (daraus der Vorwiderstand). Ein Durchlauf hin und zurueck dauert kTracerPeriodS; die Bruecke tastet
// ihn mit kTracerPoints Punkten ab (v[0] = Spannung zwischen KS+ und KS-, v[1] = Strom aus KS+ durch das Bauteil).
inline constexpr double kTracerPeriodS = 2.0;
inline constexpr std::size_t kTracerPoints = 1000;
inline const std::array<double, 4>& tracerVolts() {
    static const std::array<double, 4> v{2.0, 5.0, 10.0, 20.0};
    return v;
}
inline const std::array<double, 4>& tracerLimitsA() {
    static const std::array<double, 4> a{0.005, 0.02, 0.05, 0.2};
    return a;
}
struct TracerSettings {
    int voltsIndex = 2;   // 10 V
    bool bipolar = true;  // -U ... +U (Sperrrichtung sichtbar), sonst 0 ... +U
    int limitIndex = 1;   // 20 mA

    double maxV() const { return tracerVolts()[static_cast<std::size_t>(voltsIndex)]; }
    double minV() const { return bipolar ? -maxV() : 0.0; }
    double limitA() const { return tracerLimitsA()[static_cast<std::size_t>(limitIndex)]; }
    double seriesOhm() const { return maxV() / limitA(); } // Kurzschluss bei hoechster Spannung = Begrenzung
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

// Auto (Konzept 5.2): faengt das Signal ein; m sind die Kennwerte ueber den ganzen Puffer.
// Kanal: Nulllinie in der Mitte, bei nur positiver (negativer) Spannung 3 Teilungen darunter (darueber), und die
// empfindlichste Stellung, bei der Minimum und Maximum mit etwas Rand auf den Schirm passen
void scopeAutoChannel(ScopeSettings& s, std::size_t channel, const ScopeMeasure& m);
// Zeit: kuerzeste Zeitbasis im Normalbetrieb, bei der mindestens zwei Perioden auf dem Schirm stehen (2 bis 5)
int scopeAutoTimebase(double frequencyHz);
// Trigger: dieser Kanal, steigende Flanke, Pegel in der Mitte zwischen Minimum und Maximum
void scopeAutoTrigger(ScopeSettings& s, std::size_t channel, const ScopeMeasure& m);

} // namespace volta::model

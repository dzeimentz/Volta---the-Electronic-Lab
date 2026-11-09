#pragma once

#include <vector>

#include "volta/core/complex_mna.hpp"
#include "volta/core/simulator.hpp"

namespace volta::core {

// Auswertung eines Frequenzgangs (Volta 3, Frequenzgang-Schreiber, Konzept Kap. 5.2): aus der komplexen Uebertragung
// der Wechselstromanalyse (Simulator::acSweep) Verstaerkung in dB, Phase, Art der Kurve, Resonanz- und
// Grenzfrequenzen (-3 dB unter dem Hoechstwert) samt Bandbreite und Guete. Allokiert; nicht fuer den Echtzeitpfad.
struct FrequencyResponse {
    enum class Kind { None, Flat, LowPass, HighPass, BandPass, BandStop };
    std::vector<double> db;       // 20 log10 |H|, unten bei kFloorDb begrenzt
    std::vector<double> phaseDeg; // Phase in Grad, stetig fortgesetzt
    Kind kind = Kind::None;
    double maxDb = 0.0;
    double resonanceHz = 0.0; // Bandpass/Resonanzueberhoehung: Gipfel; Bandsperre: tiefste Stelle; 0 = keine
    double cutLowHz = 0.0;    // untere Grenzfrequenz (0 = keine)
    double cutHighHz = 0.0;   // obere Grenzfrequenz (0 = keine)
    double bandwidthHz = 0.0; // Bandpass/Bandsperre: Abstand der beiden Grenzfrequenzen
    double quality = 0.0;     // Guete = Resonanzfrequenz / Bandbreite
};

inline constexpr double kFloorDb = -200.0;

// Logarithmisch gestufte Frequenzen von fMin bis fMax (beide enthalten), perDecade Punkte je Dekade
std::vector<double> logFrequencies(double fMin, double fMax, int perDecade);

// freqsHz aufsteigend; h gleich lang
FrequencyResponse analyzeResponse(const std::vector<double>& freqsHz, const std::vector<Complex>& h);

// Kompletter Durchlauf fuer den Frequenzgang-Schreiber: grob perDecade Punkte je Dekade von fMin bis fMax, danach
// fein (je 200 Punkte) um Resonanz und Grenzfrequenzen, damit auch schmale Kreise hoher Guete genau markiert
// werden. freqsHz und h enthalten danach alle Punkte aufsteigend. Aendert den Zustand der Simulation nicht.
Status sweepResponse(Simulator& sim, std::size_t excitation, NodeId outPlus, NodeId outMinus, double fMin, double fMax,
                     int perDecade, std::vector<double>& freqsHz, std::vector<Complex>& h, FrequencyResponse& result);

} // namespace volta::core

#pragma once

#include <cstddef>

namespace volta::core {

// Knotenindex; 0 ist Masse (Pultfeld 0V).
using NodeId = std::size_t;
inline constexpr NodeId kGround = 0;

// Hoechstzahl der Anschluesse eines Bauteilmodells (Relais: Spule + Kontakt).
inline constexpr std::size_t kMaxTerminals = 4;

// Ergebnis von Kernoperationen. Der Kern wirft keine Ausnahmen; Fehler werden als Status gemeldet.
enum class Status {
    Ok,
    NotConverged,   // Newton-Verfahren ohne Konvergenz, letzter gueltiger Zustand bleibt erhalten
    Singular,       // Gleichungssystem nicht loesbar
    InvalidCircuit, // Netzliste fehlerhaft (z. B. Anschluss an nicht vorhandenen Knoten)
    OutOfMemory,    // Speicheranforderung beim Topologiewechsel fehlgeschlagen
    NoCircuit,      // noch keine Schaltung gesetzt
    InvalidStep     // Schrittweite nicht endlich oder <= 0; Zustand unveraendert
};

// Numerische Konstanten (docs/simulation-core.md).
inline constexpr double kGmin = 1e-9;          // Ableitleitwert je Knoten gegen Masse (1 GOhm)
inline constexpr double kWireOhm = 0.01;       // Leitungswiderstand je Leitung
// Kleinster zulaessiger Widerstandswert. Begrenzt den groessten Leitwert auf 1 kS, damit GMIN (1e-9 S)
// gegenueber der Pivotschwelle der LU-Zerlegung (1e-14 relativ) erhalten bleibt.
inline constexpr double kMinOhm = 1e-3;
inline constexpr double kMaxConductance = 1.0 / kMinOhm; // zentral in MnaSystem::stampConductance erzwungen
inline constexpr int kMaxNewtonIter = 50;      // Abbruch des Newton-Verfahrens
inline constexpr double kVoltTolAbs = 1e-6;    // Konvergenz: |dV| (V)
inline constexpr double kCurrentTolAbs = 1e-9; // Konvergenz: |dI| (A)
inline constexpr double kTolRel = 1e-6;        // zusaetzliche relative Toleranz bezogen auf |x|
inline constexpr int kMaxStepHalvings = 4;     // Zeitschritt bis 1/16 halbieren
inline constexpr double kDefaultStepS = 1e-3;  // feste Schrittweite Volta 1
inline constexpr double kAmbientK = 293.15;    // Umgebungstemperatur 20 Grad C

} // namespace volta::core

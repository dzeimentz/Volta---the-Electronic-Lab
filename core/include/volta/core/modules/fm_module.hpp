#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "volta/core/circuit.hpp"
#include "volta/core/component_model.hpp"
#include "volta/core/components/radio.hpp"
#include "volta/core/modules/blink_module.hpp"

namespace volta::core {

// Anschluesse des UKW-Moduls (Katalog: plus, minus, ant, abst, laut, aus; ab Volta 3).
struct FmModulePins {
    NodeId plus = kGround;
    NodeId minus = kGround;
    NodeId ant = kGround;    // Antenne (Pultfeld ANT der Senderanlage)
    NodeId tune = kGround;   // Abstimmspannung gegen minus: 0 ... Versorgung = 87,5 ... 108 MHz
    NodeId volume = kGround; // Lautstaerke: Spannung gegen minus im Verhaeltnis zur Versorgung
    NodeId out = kGround;    // Endstufe nach minus: Lautsprecher zwischen plus und aus
};

struct FmModuleParams {
    double supplyOhm = 1800.0;    // Ruhestrom des Empfaengers (5 mA bei 9 V)
    double inputOhm = 100000.0;   // Eingaenge abst und laut gegen minus
    double antennaOhm = 75.0;     // Antenneneingang (UKW-Antennen sind niederohmig)
    double outputOhm = 100.0;     // Endstufe voll leitend, einschliesslich Schutzwiderstand
    double vSupplyMinV = 3.0;     // darunter schweigt das Modul
    double minMHz = 87.5, maxMHz = 108.0;
    // Fangbereich: bis fullMHz Verstimmung voller Empfang, ab edgeMHz nur Rauschen. Echte Empfaenger rasten bei etwa
    // 0,1 MHz ein; der breitere Bereich ist eine Vereinfachung, damit man mit dem Pultregler (20,5 MHz auf dem ganzen
    // Weg) einen Sender sicher trifft. Die Sender liegen 7 MHz auseinander.
    double fullMHz = 0.25;
    double edgeMHz = 0.6;
    double noise = 0.3;           // Rauschen zwischen den Sendern (Anteil der Aussteuerung)
    bool antenna = false;         // ant haengt an der Antenne der Senderanlage (von der Netzliste gesetzt)
    unsigned stationsOn = 0b111u; // Senderfenster: Bit i = UKW-Sender i sendet (aus: dort nur Rauschen)
};

// UKW-Sender der Senderanlage (eigene Programme wie bei Mittelwelle; im Senderfenster schaltbar).
struct FmStation {
    double mhz;
    StationProgram program;
    double toneHz;
};
inline constexpr std::array<FmStation, 3> kFmStations{{{89.3, StationProgram::TimeSignal, 1000.0},
                                                       {96.8, StationProgram::Melody, 0.0},
                                                       {104.5, StationProgram::Morse, 600.0}}};

// UKW-Modul als Verhaltensmodell (Konzept Volta 3, 5.4): Ein UKW-Traeger schwingt rund 100 Millionen Mal je
// Sekunde; seine Frequenzmodulation ist im Zeitbereich nicht rechenbar. Elektrisch ist das Modul ein Ersatzschaltbild
// aus Ruhestrom (plus -> minus), Eingaengen (abst, laut, ant -> minus) und Endstufe (aus -> minus). Die Endstufe
// folgt dem demodulierten Ton Schritt fuer Schritt: Leitwert = laut * (0,5 + 0,45 * a(t)) / outputOhm mit
// a(t) = q * Programm(t) + (1 - q) * Rauschen(t); q = Empfang 1 ... 0 je nach Verstimmung gegen den naechsten
// Sender. Ohne Antenne nur Rauschen. Im »Blick hinein« und im Elternbereich offengelegt.
//
// Messwerte: voltV = Versorgung, currentA = Strom in den Ausgang, level = Empfang q, displayValue = eingestellte
// Frequenz in MHz, displayRange = Lautstaerke 0 ... 1 (Anteil der Versorgung an laut), displayFlags = Sender + 1
// (0 = kein Sender).
// Anschluesse: 0 plus, 1 minus, 2 ant, 3 abst, 4 laut, 5 aus.
class FmModule final : public ComponentModel {
public:
    FmModule(std::string id, const FmModulePins& pins, FmModuleParams params = {});

    const FmModuleParams& params() const noexcept { return m_params; }
    double tunedMHz() const noexcept { return m_tunedMHz; }
    double reception() const noexcept { return m_quality; }
    double volume() const noexcept { return m_volume; }
    int station() const noexcept { return m_station; } // naechster Sender (Index in kFmStations), -1 ohne Empfang
    bool powered() const noexcept { return m_powered; }
    // Senderfenster (Bedienhandlung): Bit i = UKW-Sender i sendet
    void setStationsOn(unsigned mask) noexcept { m_params.stationsOn = mask & 0b111u; }
    // Empfang q bei der Frequenz f (MHz) und naechster Sender
    double receptionAt(double mhz, int& station) const noexcept;

    void beginSolve(const StampContext& ctx) noexcept override;
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    void updateState(const SolutionView& x, double dtS) noexcept override;
    ComponentReading read(const SolutionView& x) const noexcept override;
    ComponentState saveState() const noexcept override;
    void restoreState(const ComponentState& state) noexcept override;

private:
    double audio() const noexcept;

    FmModuleParams m_params;
    double m_timeS = 0.0;
    double m_tunedMHz = 87.5;
    double m_quality = 0.0;
    double m_volume = 0.0;
    int m_station = -1;
    bool m_powered = false;
    double m_noise = 0.0;
    std::uint32_t m_seed = 12345u;
    double m_g = kGmin; // Leitwert der Endstufe im laufenden Loesungsvorgang
};

SubcircuitRange addFmModule(Circuit& circuit, const std::string& prefix, const FmModulePins& pins, bool antenna,
                            unsigned stationsOn = 0b111u);

} // namespace volta::core

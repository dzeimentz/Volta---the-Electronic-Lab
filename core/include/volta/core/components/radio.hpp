#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include "volta/core/component_model.hpp"
#include "volta/core/components/diode.hpp"

namespace volta::core {

class Simulator;

// Radio (Volta 3, Konzept Kap. 4 und 5.3, docs/volta3-machbarkeit.md): Der Traeger der Sender (Mittelwelle, um 1 MHz)
// ist viel schneller, als der Zeitbereich mit 20 us rechnen kann. Deshalb:
// - Abstimmen echt ueber die Wechselstromanalyse: tuneRadio() bestimmt je Sender und Detektordiode die
//   Leerlaufamplitude E0 des Traegers an der Diode und den Innenwiderstand R des Kreises (Thevenin bei der
//   Traegerfrequenz) aus dem tatsaechlich gesteckten Aufbau (Antenne, Koppelkondensator, Spule, Drehkondensator).
// - Huellkurve im Zeitbereich: die Detektordiode rechnet mit dem ueber eine Traegerperiode gemittelten Diodenstrom
//   (gemittelter Huellkurvendetektor). Die Belastung des Kreises durch die Diode ist enthalten: die
//   Traegeramplitude E an der Diode ist eine eigene Unbekannte (innerer Knoten) mit E = E0 - R * I1, I1 =
//   Grundschwingung des Diodenstroms (Beschreibungsfunktion). Oberschwingungen schliesst der Kreis kurz.
//   Die Traegheit des Kreises (Bandbreite, gedaempfte Seitenbaender der Modulation) wirkt als Kapazitaet am inneren
//   Knoten: Huellkurvengleichung des Kreises Y(w0) * E + C_h * dE/dt mit C_h = Im(dY/dw) aus der Wechselstromanalyse
//   (Parallelkreis: C_h = 2 C).

// Programm eines Senders: Modulation s(t) im Bereich -1 ... 1 (eigene Inhalte, keine fremden Aufnahmen).
enum class StationProgram {
    Tone,       // Dauerton
    Morse,      // Morsezeichen "VOLTA" mit Ton, dann Pause
    TimeSignal, // Zeitzeichen: kurzer Piep jede Sekunde, langer zur vollen Minute (hier alle 10 s)
    Melody,     // Volta-Erkennungsmelodie (eigene Komposition, 08.11.2026)
    Carol,      // erste Zeile von "Minuit, chretiens" (Adolphe Adam 1847, gemeinfrei) - das Lied, das Reginald
                // Fessenden am 24.12.1906 als erste Musik im Rundfunk gespielt haben soll (nur spaeter berichtet)
};

// Modulationssignal des Programms zur Zeit t (s), Tonhoehe toneHz (Morse, Zeitzeichen; die Melodie hat eigene Toene).
double stationSignal(StationProgram program, double toneHz, double timeS) noexcept;
// Tonhoehe des Programms zur Zeit timeS in Hz, 0 in den Pausen (Ersatzton, wenn die Simulation langsamer als
// Echtzeit rechnet)
double stationPitchHz(StationProgram program, double toneHz, double timeS) noexcept;

struct RadioStation {
    double carrierHz = 1e6; // Traegerfrequenz
    double emfV = 1.0;      // Traegeramplitude an der Antenne (Leerlauf)
    double depth = 0.5;     // Modulationsgrad (AM)
    StationProgram program = StationProgram::Tone;
    double toneHz = 1000.0;
    bool on = true;
};

// Antenneneinspeisung der Senderanlage (Pultfeld ANT gegen 0V): im Zeitbereich nur ihr Innenwiderstand (die Sender
// liefern keine Gleich- oder Tonfrequenzspannung); in der Wechselstromanalyse die Anregung mit 1 V hinter dem
// Innenwiderstand. Anschluss 0 = ANT, Anschluss 1 = Bezug.
class AntennaFeed final : public ComponentModel {
public:
    AntennaFeed(std::string id, NodeId ant, NodeId ref, double sourceOhm = 1e3);
    double sourceOhm() const noexcept { return m_ohm; }
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    bool stampAc(ComplexMna& sys, const AcContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;

private:
    double m_ohm;
};

// Detektordiode (Germanium-Spitzendiode bzw. Schottky): ohne Traeger eine gewoehnliche Diode (Shockley mit
// Bahnwiderstand, Mittelwert ueber die Phase entfaellt), mit Traeger der gemittelte Huellkurvendetektor.
// Anschluss 0 = Anode, Anschluss 1 = Kathode; ein innerer Knoten traegt die Traegeramplitude E an der Diode.
class DetectorDiode final : public ComponentModel {
public:
    static constexpr std::size_t kMaxStations = 8;

    DetectorDiode(std::string id, NodeId anode, NodeId cathode, DiodeParams params = DiodeParams::germanium());

    // Traeger an der Diode (von tuneRadio gesetzt): je Sender Leerlaufamplitude e0V (V) ohne Modulation; r =
    // Innenwiderstand, cEnvF = Huellkurvenkapazitaet des Kreises beim staerksten Sender. Mehrere Sender: Summe der Leistungen (Wurzel der
    // Quadratsumme der Huellkurven), d. h. ein schwacher Nachbarsender stoert nur quadratisch wie beim
    // Huellkurvendetektor. Leere Liste: kein Traeger.
    struct Drive {
        double e0V = 0.0;
        double depth = 0.0;
        StationProgram program = StationProgram::Tone;
        double toneHz = 1000.0;
    };
    void setDrive(const Drive* drives, std::size_t count, double sourceOhm, double cEnvF = 0.0) noexcept;
    void clearDrive() noexcept { setDrive(nullptr, 0, 0.0); }
    std::size_t driveCount() const noexcept { return m_driveCount; }
    const Drive& drive(std::size_t i) const noexcept { return m_drives[i]; }
    double driveSourceOhm() const noexcept { return m_sourceOhm; }
    double driveEnvelopeF() const noexcept { return m_cEnvF; }
    // Traegeramplitude an der Diode im letzten angenommenen Zustand (V)
    double carrierAmplitudeV() const noexcept { return m_lastE; }
    const DiodeParams& params() const noexcept { return m_params; }

    std::size_t internalNodeCount() const noexcept override { return 1; }
    bool isNonlinear() const noexcept override { return true; }
    void beginSolve(const StampContext& ctx) noexcept override;
    bool linearize(const SolutionView& x) noexcept override;
    void resetLinearization(const SolutionView& x) noexcept override;
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    // Wechselstromanalyse: offen (die Diode ist der Empfaenger, nicht Teil des Kreises); als Anregung ein Strom von
    // 1 A von der Kathode in die Anode (Innenwiderstand des Kreises an der Diode).
    bool stampAc(ComplexMna& sys, const AcContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    void updateState(const SolutionView& x, double dtS) noexcept override;
    void thermal(const SolutionView& x, double dtS) noexcept override;
    void checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept override;
    ComponentReading read(const SolutionView& x) const noexcept override;
    ComponentState saveState() const noexcept override;
    void restoreState(const ComponentState& state) noexcept override;

private:
    static constexpr double kVmin = -40.0, kDv = 1e-3;
    static constexpr std::size_t kTableN = 50001; // -40 ... +10 V
    static constexpr int kPhases = 32;

    double vak(const SolutionView& x) const noexcept { return x.voltage(terminal(0)) - x.voltage(terminal(1)); }
    double amp(const SolutionView& x) const noexcept { return x.voltage(internalNode(0)); }
    void g(double v, double& i, double& di) const noexcept;
    void evaluate(double e, double v) noexcept;

    DiodeParams m_params;
    std::vector<double> m_table; // Strom durch Bahnwiderstand und Uebergang bei der Gesamtspannung v
    std::array<double, kPhases> m_cos{};
    std::array<Drive, kMaxStations> m_drives{};
    std::size_t m_driveCount = 0;
    double m_sourceOhm = 1.0;
    double m_cEnvF = 0.0;
    double m_ePrev = 0.0; // Traegeramplitude des letzten angenommenen Zustands (Huellkurvenkapazitaet)
    // im Loesungsvorgang festgeschrieben (beginSolve): Leerlaufamplitude mit Modulation und Innenwiderstand
    double m_e0Now = 0.0;
    double m_rNow = 1.0;
    // Linearisierung um (m_eLin, m_vLin): I0 Gleichstrom Anode -> Kathode, I1 Grundschwingung
    double m_eLin = 0.0, m_vLin = 0.0;
    double m_i0 = 0.0, m_i0dv = kJunctionGmin, m_i0de = 0.0;
    double m_i1 = 0.0, m_i1dv = 0.0, m_i1de = 0.0;
    double m_lastE = 0.0;
    double m_lastCurrentA = 0.0;
    double m_lastDtS = 0.0;
    double m_overloadS = 0.0;
};

// Stimmt alle Detektordioden der Schaltung auf die Sender ab (Wechselstromanalyse am angenommenen Zustand, von der
// Antenneneinspeisung aus). Aufrufen nach setCircuit() und nach jeder Bedienhandlung, die den Kreis verstimmt
// (Drehkondensator), sowie beim Ein-/Ausschalten von Sendern. Nicht im Echtzeitpfad (Speicheranforderung).
// Ohne Antenneneinspeisung oder ohne eingeschalteten Sender erhalten alle Detektordioden keinen Traeger.
// strongest (optional): staerkster Traeger an einer Detektordiode (e0V = 0: kein Empfang)
Status tuneRadio(Simulator& sim, const std::vector<RadioStation>& stations,
                 DetectorDiode::Drive* strongest = nullptr);

} // namespace volta::core

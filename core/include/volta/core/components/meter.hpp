#pragma once

#include <array>

#include "volta/core/component_model.hpp"

namespace volta::core {

enum class MeterMode { Volt, Ampere, Ohm };

// Anzeigezustand des Messgeraets in ComponentReading::displayFlags.
inline constexpr int kDisplayOverrange = 1;      // »OL«: Messbereich ueberschritten bzw. Kreis offen (Ohm)
inline constexpr int kDisplayForeignVoltage = 2; // »Spannung abschalten!« im Ohm-Bereich
inline constexpr int kDisplayFuseBlown = 4;      // Sicherung des A-Eingangs ausgeloest

struct MeterParams {
    double digitalInputOhm = 10e6;                     // Eingangswiderstand digital (V)
    double analogOhmPerVolt = 20e3;                    // Kennwiderstand analog (V): 20 kOhm/V * Messbereich
    std::array<double, 4> analogRangesV{1.0, 3.0, 10.0, 30.0}; // selbsttaetige Bereichswahl (Volta 1)
    double shuntOhm = 0.1;                             // A-Eingang
    double fuseA = 0.5;                                // Sicherung A-Eingang
    double fuseTimeS = 0.01;                           // loest aus, wenn der Strom so lange ueber fuseA liegt
    double ohmSourceV = 3.0;                           // Messquelle im Ohm-Bereich (Leerlaufspannung)
    double ohmSourceOhm = 3000.0;                      // ... mit Innenwiderstand: Messstrom hoechstens 1 mA
    double ohmMaxOhm = 40e6;                           // groesster Ohm-Bereich; darueber »OL«
};

// Dreifach-Messgeraet des Bedienpults (docs/simulation-core.md, Messschnittstelle). Anschluesse 0 = COM,
// 1 = V/Ohm, 2 = A (Inhaltspakete: MESS:COM, MESS:VOHM, MESS:A). Das Geraet ist ein gewoehnlicher Teil des
// Gleichungssystems und belastet die Schaltung wie ein reales Geraet:
//  - V/Ohm-COM: im V- und A-Bereich Eingangswiderstand (digital 10 MOhm, analog 20 kOhm/V * Bereich, Bereich
//    selbsttaetig aus dem letzten Messwert); im Ohm-Bereich Messquelle 3 V ueber 3 kOhm, Anzeige U/I.
//  - A-COM: immer Shunt 0,1 Ohm mit Sicherung (danach offen, SimEventType::FuseBlown).
// Messart und Anzeigeart sind Bedienhandlungen. Anzeigewert und Zustand in ComponentReading::displayValue/-Flags.
class Meter final : public ComponentModel {
public:
    Meter(std::string id, NodeId com, NodeId vOhm, NodeId amp, MeterParams params = {});

    MeterMode mode() const noexcept { return m_mode; }
    void setMode(MeterMode mode) noexcept { m_mode = mode; }
    bool isAnalog() const noexcept { return m_analog; }
    void setAnalog(bool analog) noexcept { m_analog = analog; }
    bool isFuseBlown() const noexcept { return m_fuseBlown; }
    double analogRangeV() const noexcept { return m_rangeV; }
    // Eingangswiderstand im V-Bereich fuer den laufenden Loesungsvorgang
    double inputOhm() const noexcept { return m_inputOhm; }

    void beginSolve(const StampContext& ctx) noexcept override;
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    void thermal(const SolutionView& x, double dtS) noexcept override;
    void checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept override;
    ComponentReading read(const SolutionView& x) const noexcept override;
    ComponentState saveState() const noexcept override;
    void restoreState(const ComponentState& state) noexcept override;

private:
    NodeId com() const noexcept { return terminal(0); }
    NodeId vo() const noexcept { return terminal(1); }
    NodeId amp() const noexcept { return terminal(2); }
    double shuntConductance() const noexcept { return m_fuseBlown ? kGmin : 1.0 / m_params.shuntOhm; }
    bool ohmActive() const noexcept { return m_mode == MeterMode::Ohm; }

    MeterParams m_params;
    MeterMode m_mode = MeterMode::Volt;
    bool m_analog = false;
    bool m_fuseBlown = false;
    double m_inputOhm = 10e6;
    double m_rangeV = 30.0;
    double m_lastVoltV = 0.0; // letzter Messwert V/Ohm-COM (Bereichswahl)
    double m_lastAmpA = 0.0;  // letzter Strom im A-Eingang (Sicherung)
    double m_fuseS = 0.0;
    double m_lastDtS = 0.0;
};

} // namespace volta::core

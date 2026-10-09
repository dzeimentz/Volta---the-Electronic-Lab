#pragma once

#include "volta/core/component_model.hpp"

namespace volta::core {

// Fotowiderstand (Katalogtyp `fotowiderstand`, CdS-Typ wie GL5528): Leitwert aus Dunkelwiderstand und
// Beleuchtungsanteil, R_hell(E) = r10Ohm * (E / 10 lx)^-gamma (Datenblaetter: 8-20 kOhm bei 10 lx, gamma 0,6-0,8,
// Dunkelwiderstand >= 1 MOhm). Die Beleuchtungsstaerke ist eine Bedienhandlung (Raumlicht, Taschenlampe); das
// Bauteil folgt ihr traege (Anstieg tau ca. 30 ms, Abfall ca. 100 ms, wie reale CdS-Zellen).
struct PhotoresistorParams {
    double r10Ohm = 10000.0;    // bei 10 lx
    double gamma = 0.7;
    double darkOhm = 1.0e6;     // im Dunkeln
    double riseTauS = 0.03;     // heller werden
    double fallTauS = 0.1;      // dunkler werden
    double pmaxW = 0.1;
    ThermalParams thermal{400.0, 5.0 / 400.0, 473.0}; // klein: Zerstoerung dauerhaft ab ca. 0,4 W
};

class Photoresistor final : public ComponentModel {
public:
    Photoresistor(std::string id, NodeId a, NodeId b, PhotoresistorParams params = {}, double lux = 300.0);

    void setIlluminance(double lux) noexcept;   // Bedienhandlung
    double illuminance() const noexcept { return m_targetLux; }
    double effectiveLux() const noexcept { return m_lux; }
    double resistanceOhm() const noexcept { return 1.0 / m_g; }

    void beginSolve(const StampContext& ctx) noexcept override;
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    void updateState(const SolutionView& x, double dtS) noexcept override;
    void thermal(const SolutionView& x, double dtS) noexcept override;
    void checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept override;
    ComponentReading read(const SolutionView& x) const noexcept override;
    ComponentState saveState() const noexcept override;
    void restoreState(const ComponentState& state) noexcept override;

private:
    double conductanceFor(double lux) const noexcept;
    PhotoresistorParams m_params;
    double m_targetLux = 300.0;
    double m_lux = 300.0; // traege nachgefuehrt
    double m_g = 1e-4;    // Kennwert des laufenden Loesungsvorgangs
    double m_lastPowerW = 0.0;
};

// Heissleiter (NTC, Katalogtyp `heissleiter`): R(T) = r25Ohm * exp(B * (1/T - 1/298,15 K)), Datenblattwerte
// 10 kOhm, B = 3950 K. Seine Temperatur folgt der Umgebung am Bauteil (Raumtemperatur bzw. vom Foehn erwaermte Luft,
// Bedienhandlung) mit einer Zeitkonstante von einigen Sekunden; Eigenerwaermung durch den Strom kommt hinzu
// (Waermewiderstand ca. 150 K/W in Luft).
struct ThermistorParams {
    double r25Ohm = 10000.0;
    double betaK = 3950.0;
    double pmaxW = 0.25;
    ThermalParams thermal{150.0, 4.0 / 150.0, 423.0}; // tau 4 s; ueber 150 Grad C zerstoert
};

class Thermistor final : public ComponentModel {
public:
    Thermistor(std::string id, NodeId a, NodeId b, ThermistorParams params = {}, double ambientK = kAmbientK);

    void setAmbient(double kelvin) noexcept; // Bedienhandlung: Luft am Bauteil
    double ambientK() const noexcept { return m_ambientK; }
    double resistanceAt(double kelvin) const noexcept;

    void beginSolve(const StampContext& ctx) noexcept override;
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    void updateState(const SolutionView& x, double dtS) noexcept override;
    void thermal(const SolutionView& x, double dtS) noexcept override;
    void checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept override;
    ComponentReading read(const SolutionView& x) const noexcept override;
    void restoreState(const ComponentState& state) noexcept override;

private:
    ThermistorParams m_params;
    double m_ambientK = kAmbientK;
    double m_g = 1e-4;
    double m_lastPowerW = 0.0;
};

} // namespace volta::core

#pragma once

#include "volta/core/component_model.hpp"

namespace volta::core {

// Kennwerte einer Diode oder Leuchtdiode. Der Saettigungsstrom wird aus der Flussspannung ufV beim Bezugsstrom
// ifRefA abgeleitet, damit die Katalogangabe `uf_V` unmittelbar das Modell bestimmt.
// Alle Werte sind Ausgangswerte und werden in Phase 0 an realen Bauteilen kalibriert.
struct DiodeParams {
    double ufV = 0.70;    // Flussspannung beim Bezugsstrom
    double ifRefA = 0.01; // Bezugsstrom
    double n = 1.75;      // Emissionskoeffizient
    double rsOhm = 0.57;  // Bahnwiderstand
    double imaxA = 0.2;   // Dauergrenzstrom; darueber gilt das Bauteil als ueberlastet
    ThermalParams thermal{350.0, 5.0 / 350.0, 473.0}; // Vorgabe Silizium; eigene LED-Kennwerte: kLedThermal setzen
    // Starke Stromueberlast (I > overloadFactor * imaxA) zerstoert nach overloadFailS, auch wenn die Temperatur
    // die Grenze nicht erreicht (vereinfachte Alterung des Halbleiters). So geht eine LED an 100 Ohm (ca. 67 mA)
    // nach wenigen Sekunden kaputt, waehrend sie bei 50 mA nur als ueberlastet gilt.
    double overloadFactor = 2.5;
    double overloadFailS = 3.0;

    // Thermik: Rth nach Datenblatt (DO-35 bzw. 5-mm-LED ca. 350 K/W), Zeitkonstante didaktisch gewaehlt.
    // LED: tau = 12,6 s, Grenze 125 Grad C. Ohne Vorwiderstand an 9 V (ca. 1 A, 7,7 W) wird sie nach ca. 0,5 s
    // zerstoert; ein Stromstoss von 50 ms erwaermt sie nur um ca. 10 K.
    static constexpr ThermalParams kSiliconThermal{350.0, 5.0 / 350.0, 473.0};
    static constexpr ThermalParams kGermaniumThermal{500.0, 3.0 / 500.0, 363.0};
    static constexpr ThermalParams kLedThermal{350.0, 12.6 / 350.0, 398.0};

    // Siliziumdiode, angelehnt an 1N4148 (SPICE-Modell Is = 2,52 nA, N = 1,752, Rs = 0,568 Ohm -> 0,70 V bei 10 mA)
    static DiodeParams silicon() noexcept { return {0.70, 0.01, 1.75, 0.57, 0.2, kSiliconThermal, 2.5, 3.0}; }
    // Germaniumdiode (Spitzendiode, typisch 0,3-0,4 V bei 10 mA)
    static DiodeParams germanium() noexcept { return {0.35, 0.01, 1.5, 2.0, 0.05, kGermaniumThermal, 2.5, 3.0}; }
    // 5-mm-Standard-LEDs, Flussspannung bei 20 mA nach typischen Datenblattwerten (AlGaInP bzw. InGaN)
    static DiodeParams ledRed() noexcept { return {1.9, 0.02, 2.0, 5.0, 0.02, kLedThermal, 2.5, 3.0}; }
    static DiodeParams ledYellow() noexcept { return {2.0, 0.02, 2.0, 5.0, 0.02, kLedThermal, 2.5, 3.0}; }
    static DiodeParams ledGreen() noexcept { return {2.1, 0.02, 2.0, 5.0, 0.02, kLedThermal, 2.5, 3.0}; }
    static DiodeParams ledBlue() noexcept { return {3.1, 0.02, 3.0, 10.0, 0.02, kLedThermal, 2.5, 3.0}; }
};

// Shockley-Diode mit Bahnwiderstand (Katalogtypen `diode_si`, `diode_ge`, `led_*`).
// Anschluss 0 = Anode, Anschluss 1 = Kathode. Der Bahnwiderstand liegt zwischen Anode und einem inneren Knoten.
// Newton-Linearisierung mit Spannungsbegrenzung nach SPICE (pnjlim). Ueberlastet oberhalb imaxA, zerstoert
// (danach offen) bei Uebertemperatur oder anhaltender starker Stromueberlast.
class Diode : public ComponentModel {
public:
    Diode(std::string id, NodeId anode, NodeId cathode, DiodeParams params = DiodeParams::silicon());

    const DiodeParams& params() const noexcept { return m_params; }
    double saturationCurrentA() const noexcept { return m_isA; }
    double thermalVoltageV() const noexcept { return m_nVt / m_params.n; }
    double criticalVoltageV() const noexcept { return m_vcrit; }

    // Strom des pn-Uebergangs nach Shockley, oberhalb der Leitwertgrenze linear fortgesetzt (Tests, Erklaerungen).
    double junctionCurrentA(double vdV) const noexcept;

    std::size_t internalNodeCount() const noexcept override { return m_params.rsOhm > 0.0 ? 1 : 0; }
    bool isNonlinear() const noexcept override { return true; }
    bool linearize(const SolutionView& iterate) noexcept override;
    void resetLinearization(const SolutionView& accepted) noexcept override;
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    void thermal(const SolutionView& x, double dtS) noexcept override;
    void checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept override;
    // level = Strom / Bezugsstrom (LED: 1 bei 20 mA)
    ComponentReading read(const SolutionView& x) const noexcept override;
    ComponentState saveState() const noexcept override;
    void restoreState(const ComponentState& state) noexcept override;

private:
    NodeId junctionAnode() const noexcept { return m_params.rsOhm > 0.0 ? internalNode(0) : terminal(0); }
    double junctionVoltage(const SolutionView& x) const noexcept {
        return x.voltage(junctionAnode()) - x.voltage(terminal(1));
    }
    void evaluate(double vdV) noexcept;

    DiodeParams m_params;
    double m_isA = 1e-14; // Saettigungsstrom
    double m_nVt = 0.0;   // n * Temperaturspannung
    double m_vcrit = 0.0; // kritische Spannung fuer die Begrenzung
    double m_xMax = 80.0; // darueber Kennlinie linear (Steigung = Leitwertgrenze)
    // Linearisierung i = m_g * vd + m_ieq um m_vdLin
    double m_vdLin = 0.0;
    double m_g = kJunctionGmin;
    double m_ieq = 0.0;
    double m_lastCurrentA = 0.0;
    double m_lastDtS = 0.0;
    double m_overloadS = 0.0; // bisherige Dauer starker Stromueberlast
};

} // namespace volta::core

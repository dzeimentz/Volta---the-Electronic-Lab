#pragma once

#include <array>

#include "volta/core/component_model.hpp"

namespace volta::core {

// Kennwerte eines Festspannungsreglers 5 V (ab Volta 2), angelehnt an Datenblattwerte der Bauart 78L05:
// Ausgang 5 V +- 4 %, Lastregelung hoechstens 60 mV (1 bis 100 mA), Mindest-Spannungsabfall 1,7 V, Ruhestrom ca. 3 mA,
// Kurzschlussstrom ca. 140 mA, Gehaeuse TO-92 mit ca. 200 K/W, Uebertemperaturschutz ab ca. 125 Grad C.
struct RegulatorParams {
    double refV = 4.98;        // Sollwert (bei 1 mA Last ca. 5,03 V, bei 100 mA ca. 4,97 V)
    double errorV = 0.02;      // Regelsteilheit: Laststrom = Ilim * (1 + tanh((Uref - Uaus) / errorV)) / 2
    double dropoutV = 1.6;     // Durchlass: Faktor (1 + tanh((Uein - Uaus - dropoutV) / headroomV)) / 2
    double headroomV = 0.1;    //   -> bei 100 mA ca. 1,7 V Mindestabfall
    double limitA = 0.14;      // Strombegrenzung (Kurzschluss)
    double ratedA = 0.1;       // Dauerstrom laut Datenblatt; darueber ueberlastet
    double quiescentA = 0.003; // Ruhestrom Ein -> Masse
    double dividerOhm = 5000.0; // innerer Spannungsteiler am Ausgang (Grundlast, wie beim echten Regler)
    double shutdownK = 398.0;   // Uebertemperaturschutz beginnt (125 Grad C) ...
    double shutdownSpanK = 25.0; // ... und hat den Strom 25 K darueber ganz zurueckgenommen
    ThermalParams thermal{200.0, 2.0 / 200.0, 0.0}; // keine Zerstoerung: der Schutz regelt ab (tau 2 s, didaktisch)
};

// Spannungsregler (Katalogtyp `spannungsregler_5v`, Traegerplatine): Anschluesse 0 = Ein, 1 = Masse, 2 = Aus.
// Verhaltensmodell: Laengsstrom Ein -> Aus I = Ilim * c(Uref - Uaus) * f(Uein - Uaus - Udrop) * s(T) mit glatten
// tanh-Stufen c und f (Newton-tauglich), Ruhestrom Ein -> Masse, innerer Teiler Aus -> Masse. Spannungen gegen den
// Masseanschluss. s(T) nimmt den Strom bei Uebertemperatur zurueck (in beginSolve aus der Temperatur festgeschrieben).
class VoltageRegulator final : public ComponentModel {
public:
    VoltageRegulator(std::string id, NodeId in, NodeId ground, NodeId out, RegulatorParams params = {});

    const RegulatorParams& params() const noexcept { return m_params; }
    double thermalScale() const noexcept { return m_scale; }

    bool isNonlinear() const noexcept override { return true; }
    void beginSolve(const StampContext& ctx) noexcept override;
    bool linearize(const SolutionView& iterate) noexcept override;
    void resetLinearization(const SolutionView& accepted) noexcept override;
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    void thermal(const SolutionView& x, double dtS) noexcept override;
    void checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept override;
    // level: Ausgangsstrom / Dauerstrom; displayValue: Ausgangsspannung gegen Masse
    ComponentReading read(const SolutionView& x) const noexcept override;

private:
    void evaluate(double vin, double vgnd, double vout) noexcept;
    double passCurrent(const SolutionView& x) const noexcept; // Laengsstrom der laufenden Linearisierung
    double quiescent(const SolutionView& x) const noexcept;

    RegulatorParams m_params;
    double m_scale = 1.0; // Uebertemperaturschutz 0..1
    // Linearisierung Laengsstrom: I = m_i0 + sum g_k (v_k - v_k0), k = Ein, Masse, Aus
    std::array<double, 3> m_v0{};
    std::array<double, 3> m_g{};
    double m_i0 = 0.0;
    // Ruhestrom Iq = quiescentA * tanh(Uein-Masse / 1 V), linearisiert
    double m_q0 = 0.0, m_qg = 0.0, m_qv0 = 0.0;
    double m_lastOutA = 0.0;
};

} // namespace volta::core

#pragma once

#include "volta/core/component_model.hpp"

namespace volta::core {

struct LampParams {
    double uNomV = 6.0;       // Nennspannung (Katalog `u_nenn_V`)
    double iNomA = 0.05;      // Nennstrom (Katalog `i_nenn_mA`)
    double coldRatio = 0.1;   // R_kalt / R_heiss (Wolfram: ca. 1/10 bis 1/15)
    double alpha = 1.2;       // R ~ T^alpha (Wolfram, Naeherung)
    double filamentTauS = 0.05; // thermische Zeitkonstante des Glühfadens bei Nenntemperatur (20-100 ms)
    // Alterung: oberhalb der Gleichgewichtstemperatur bei wearVoltageFactor * uNom brennt der Faden nach wearFailS
    // durch; oberhalb der Temperatur bei burstVoltageFactor * uNom sofort.
    double wearVoltageFactor = 1.25;
    double wearFailS = 5.0;
    double burstVoltageFactor = 1.6;
};

// Glühlämpchen (Katalogtyp `gluehlampe`): temperaturabhaengiger Widerstand R(T) = R_kalt * (T/T0)^alpha.
// Der Faden gibt Waerme durch Strahlung ab: Cth * dT/dt = P - k * (T^4 - T0^4), k aus Nennleistung und
// Nenntemperatur, Cth aus der Fadenzeitkonstante bei Nenntemperatur. Daraus folgt die reale Kennlinie
// I ~ U^0,54 und der Einschaltstromstoss (kalt ca. 10-facher Strom). R folgt der Temperatur bei beginSolve().
// level = Leistung / Nennleistung (die sichtbare Helligkeit steigt staerker, etwa mit U^3,4; Sache der Anzeige).
class Lamp final : public ComponentModel {
public:
    Lamp(std::string id, NodeId a, NodeId b, LampParams params = {});

    const LampParams& params() const noexcept { return m_params; }
    double resistanceOhm() const noexcept;
    double hotResistanceOhm() const noexcept { return m_params.uNomV / m_params.iNomA; }
    double nominalTempK() const noexcept { return m_nominalK; }
    // Gleichgewichtstemperatur bei Gleichspannung u am Lämpchen (fuer Tests und Grenzen; nicht im Schritt).
    double steadyTempK(double uV) const noexcept;

    void beginSolve(const StampContext& ctx) noexcept override;
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    void thermal(const SolutionView& x, double dtS) noexcept override;
    void checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept override;
    ComponentReading read(const SolutionView& x) const noexcept override;
    ComponentState saveState() const noexcept override;
    void restoreState(const ComponentState& state) noexcept override;

private:
    double resistanceAt(double tempK) const noexcept;
    double conductance() const noexcept { return m_destroyed ? kGmin : m_g; }

    LampParams m_params;
    double m_coldOhm = 12.0;
    double m_nominalK = 2000.0;
    double m_radiationWPerK4 = 0.0; // k im Strahlungsgesetz
    double m_cthJPerK = 0.0;
    double m_burstK = 0.0;
    double m_wearK = 0.0;
    double m_wearS = 0.0;
    double m_lastDtS = 0.0;
    double m_g = 1.0 / 12.0; // Leitwert des laufenden Loesungsvorgangs (aus der Temperatur bei beginSolve)
};

} // namespace volta::core

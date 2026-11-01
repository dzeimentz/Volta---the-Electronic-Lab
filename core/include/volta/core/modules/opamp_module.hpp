#pragma once

#include <array>
#include <string>

#include "volta/core/circuit.hpp"
#include "volta/core/component_model.hpp"
#include "volta/core/modules/blink_module.hpp"

namespace volta::core {

// Anschluesse des OP-Moduls (Katalog: plus, minus, nichtinv, inv, aus).
struct OpAmpModulePins {
    NodeId plus = kGround;  // Versorgung +U
    NodeId minus = kGround; // Versorgung -U (oder 0V bei einfacher Versorgung)
    NodeId inP = kGround;   // nichtinvertierender Eingang (IN+)
    NodeId inN = kGround;   // invertierender Eingang (IN-)
    NodeId out = kGround;
};

// Kennwerte des Makromodells (Konzept Volta 2, Kap. 6): angelehnt an einen Universal-OP der Bauart 741/TL071.
struct OpAmpParams {
    double openLoopGain = 2.0e5;   // Leerlaufverstaerkung A0
    double gbwHz = 1.0e6;          // Verstaerkungs-Bandbreite-Produkt
    double slewVPerS = 0.5e6;      // Anstiegsrate 0,5 V/us
    double headroomV = 1.5;        // Aussteuergrenze: so weit bleibt der Ausgang unter +U bzw. ueber -U
    double outputOhm = 75.0;       // Ausgangswiderstand
    double currentLimitA = 0.025;  // Ausgangsstrombegrenzung (Kurzschlussfest)
    double inputOhm = 10.0e6;      // Differenz-Eingangswiderstand
    double quiescentA = 0.0015;    // Ruhestrom +U -> -U
    double minSupplyV = 4.0;       // darunter arbeitet der OP nicht (Ausgang hochohmig)
};

// OP-Modul als Makromodell (ein Bauteil). Innere Groesse y (Ausgang ohne Begrenzung) mit einem Pol:
// tau dy/dt = A0 (U+ - U-) - y, tau = A0 / (2 pi GBW), im Zeitschritt implizit (Rueckwaerts-Euler), dazu die
// Anstiegsrate als weiche Begrenzung der Aenderung je Schritt (S = SR * dt). Die Aussteuergrenzen begrenzen weich
// (tanh) auf Mitte +- (U_Versorgung / 2 - headroomV), die Endstufe liefert I = Ilim * tanh((U_Ziel - U_aus) /
// (Ilim * Rout)) aus +U bzw. -U. Die Versorgungsspannungen werden je Loesungsvorgang aus dem angenommenen Zustand
// festgeschrieben (aendern sich langsam). Ohne Versorgung (unter minSupplyV) liefert der Ausgang keinen Strom.
// Messwerte: voltV = Versorgung, level = 1 wenn der Ausgang ueber der Mitte liegt, displayValue = Ausgangsspannung
// gegen -U. Anschluesse: 0 plus, 1 minus, 2 inP, 3 inN, 4 out.
class OpAmpModule final : public ComponentModel {
public:
    OpAmpModule(std::string id, const OpAmpModulePins& pins, OpAmpParams params = {});

    const OpAmpParams& params() const noexcept { return m_params; }
    double internalV() const noexcept { return m_y; }

    bool isNonlinear() const noexcept override { return true; }
    void beginSolve(const StampContext& ctx) noexcept override;
    bool linearize(const SolutionView& iterate) noexcept override;
    void resetLinearization(const SolutionView& accepted) noexcept override;
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    void updateState(const SolutionView& x, double dtS) noexcept override;
    ComponentReading read(const SolutionView& x) const noexcept override;
    ComponentState saveState() const noexcept override;
    void restoreState(const ComponentState& state) noexcept override;

private:
    double targetFor(double vd, double* dTarget) const noexcept; // Zielspannung des Ausgangs und Ableitung nach vd
    void evaluate(double vd, double vout) noexcept;

    OpAmpParams m_params;
    double m_y = 0.0;      // innerer Zustand (angenommen)
    // im Loesungsvorgang festgeschrieben
    double m_dtS = 0.0;
    double m_mid = 0.0, m_half = 1.0, m_on = 0.0, m_iq = 0.0;
    // Linearisierung des Ausgangsstroms Iout(vd, vout) und der Versorgungsanteile
    double m_vd0 = 0.0, m_vo0 = 0.0;
    double m_iout = 0.0, m_dIdVd = 0.0, m_dIdVo = 0.0;
    double m_ip = 0.0, m_dIpdI = 0.5; // Anteil aus +U: Ip = (Iout + sqrt(Iout^2 + eps^2)) / 2
    double m_lastVd = 0.0;
    // Anstoss an die Aussteuergrenze, solange Gegenkopplung nicht nachgewiesen ist (siehe linearize)
    bool m_kickPending = false, m_kicked = false;
    int m_verified = 0;
};

SubcircuitRange addOpAmpModule(Circuit& circuit, const std::string& prefix, const OpAmpModulePins& pins);

} // namespace volta::core

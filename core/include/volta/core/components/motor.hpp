#pragma once

#include "volta/core/component_model.hpp"

namespace volta::core {

// Kleiner Gleichstrommotor mit Propeller, angelehnt an Solarmotoren (Typ RF-300): 3 V Nennspannung,
// Leerlauf ca. 2200 U/min, Ankerwiderstand 8 Ohm (Katalog `ra_ohm`). Ausgangswerte, Kalibrierung Phase 0.
struct MotorParams {
    double raOhm = 8.0;          // Ankerwiderstand
    double kVs = 0.013;          // Motorkonstante (V*s/rad = N*m/A); 3 V / 230 rad/s
    double inertiaKgm2 = 2.1e-6; // Anker + Propeller; mechanische Zeitkonstante J*Ra/k^2 ca. 0,1 s
    double viscousNms = 1e-7;    // geschwindigkeitsproportionale Reibung/Luftwiderstand
    double frictionNm = 2.6e-4;  // Haft-/Gleitreibung: Anlaufstrom ca. 20 mA
    double uNomV = 3.0;
    double overloadA = 0.4;      // darueber ueberlastet (ca. Blockierstrom bei Nennspannung)
    ThermalParams thermal{30.0, 20.0 / 30.0, 403.0}; // Wicklung; tau 20 s, Grenze 130 Grad C
};

// Gleichstrommotor (Katalogtyp `motor`): Anker mit Ra und Gegenspannung k*omega, Mechanik J*domega/dt =
// k*I - b*omega - Reibung. Implizit (Rueckwaerts-Euler) als Spannungszweig gekoppelt:
//   V(a) - V(b) - (Ra + k^2*dt/(J + b*dt)) * I = k * (J*omega_alt - dt*Reibung) / (J + b*dt)
// Haftreibung: steht der Motor und reicht das Moment k*|I| nicht, bleibt er stehen (nur Ra). Drehrichtung folgt
// der Polung. Blockieren (setBlocked) ist eine Bedienhandlung; ein blockierter Motor erwaermt sich.
// level = omega / Leerlaufdrehzahl bei Nennspannung (mit Reibung; mit Vorzeichen). Im Schritt, in dem die
// Reibung den Motor anhaelt, rechnet der Stempel einmalig mit der ueberschiessenden Drehzahl (vernachlaessigbar).
class Motor final : public ComponentModel {
public:
    Motor(std::string id, NodeId a, NodeId b, MotorParams params = {});

    const MotorParams& params() const noexcept { return m_params; }
    double omegaRadS() const noexcept { return m_omega; }
    // Leerlaufdrehzahl bei Nennspannung: (U - Ra*Tf/k) / (k + Ra*b/k)
    double noLoadOmegaRadS() const noexcept;
    double rpm() const noexcept;
    bool isBlocked() const noexcept { return m_blocked; }
    void setBlocked(bool blocked) noexcept;

    std::size_t branchCount() const noexcept override { return 1; }
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    void updateState(const SolutionView& x, double dtS) noexcept override;
    void thermal(const SolutionView& x, double dtS) noexcept override;
    void checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept override;
    ComponentReading read(const SolutionView& x) const noexcept override;
    ComponentState saveState() const noexcept override;
    void restoreState(const ComponentState& state) noexcept override;

private:
    // Richtung der Reibung im naechsten Schritt: 0 = Stillstand (Haftreibung haelt), sonst +-1.
    int motionSign() const noexcept;

    MotorParams m_params;
    double m_omega = 0.0;
    double m_lastCurrentA = 0.0;
    bool m_blocked = false;
};

} // namespace volta::core

#pragma once

#include "volta/core/component_model.hpp"

namespace volta::core {

// MOSFET mit Anreicherung (ab Volta 3, Stufe 5: Klasse D), N- oder P-Kanal, Logikpegel. Quadratisches Modell
// (Shichman-Hodges) mit weichem Einsatz an der Schwellspannung, Kanallaengenmodulation, Body-Diode und konstanter
// Gate-Kapazitaet:
//   Uov = s * ln(1 + e^((Ugs - Uth)/s))     (weicher Einsatz, s = smoothV; weit unter Uth praktisch 0)
//   Id  = K * (Uov*Uds - Uds^2/2) * (1 + lambda*Uds)   fuer Uds < Uov (Widerstandsbereich, Rds ~ 1/(K*Uov))
//   Id  = K/2 * Uov^2 * (1 + lambda*Uds)               sonst (Saettigung)
// Bei Uds < 0 leitet der Kanal in Gegenrichtung mit derselben Kennlinie, gesteuert weiter von Ugs (das Substrat liegt
// an der Source; ein gesperrter MOSFET bleibt gesperrt). Die Body-Diode liegt von Source nach Drain (N-Kanal) und
// leitet, wenn der Drain unter die Source gezogen wird. Kennwerte angelehnt an kleine
// Logikpegel-MOSFETs (Uth etwa 2 V, Rds(on) etwa 0,2 Ohm bei 7 V Gate-Spannung, 2 A Dauerstrom, Ptot 1 W ohne
// Kuehlkoerper). P-Kanal: dieselben Gleichungen mit umgekehrten Spannungen und Stroemen (Vorzeichen p = -1); die
// Ableitungen nach den Knotenspannungen bleiben dabei unveraendert.
// Vereinfachungen: keine Gate-Drain-Kapazitaet (Miller), keine Temperaturabhaengigkeit der Kennlinie.
struct MosfetParams {
    bool pChannel = false;
    double vthV = 2.0;          // Schwellspannung (Betrag)
    double kAPerV2 = 1.0;       // Steilheitsfaktor K
    double lambdaPerV = 0.01;   // Kanallaengenmodulation
    double smoothV = 0.05;      // Breite des weichen Einsatzes
    double cgsF = 300e-12;      // Gate-Source-Kapazitaet
    double bodyIsA = 1e-13;     // Saettigungsstrom der Body-Diode (etwa 0,8 V bei 1 A)
    double iMaxA = 2.0;         // Drain-Dauerstrom
    ThermalParams thermal{125.0, 5.0 / 125.0, 423.0}; // Ptot 1 W, Grenze 150 Grad C, Zeitkonstante didaktisch 5 s
    double overloadFactor = 2.5; // starke Ueberlast zerstoert nach overloadFailS
    double overloadFailS = 1.0;

    static MosfetParams nChannel() noexcept { return {}; }
    static MosfetParams pChannelDefault() noexcept {
        MosfetParams p;
        p.pChannel = true;
        p.kAPerV2 = 0.6; // Loecher sind langsamer: bei gleicher Groesse etwas hoeherer Rds(on)
        return p;
    }
};

// Anschluesse: 0 = Gate, 1 = Drain, 2 = Source (Katalog g, d, s). Zerstoert: alle Anschluesse offen.
class Mosfet final : public ComponentModel {
public:
    Mosfet(std::string id, NodeId gate, NodeId drain, NodeId source, MosfetParams params = {});

    const MosfetParams& params() const noexcept { return m_params; }
    bool isPChannel() const noexcept { return m_sign < 0.0; }

    bool isNonlinear() const noexcept override { return true; }
    void beginSolve(const StampContext& ctx) noexcept override;
    bool linearize(const SolutionView& iterate) noexcept override;
    void resetLinearization(const SolutionView& accepted) noexcept override;
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    void updateState(const SolutionView& x, double dtS) noexcept override;
    void thermal(const SolutionView& x, double dtS) noexcept override;
    void checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept override;
    ComponentReading read(const SolutionView& x) const noexcept override;
    ComponentState saveState() const noexcept override;
    void restoreState(const ComponentState& state) noexcept override;

    // Kanalstrom in den Drain (N-Kanal-Sicht, ohne Body-Diode) bei ugs, uds; fuer Tests und Dokumentation
    double channelCurrent(double ugs, double uds) const noexcept;

private:
    NodeId g() const noexcept { return terminal(0); }
    NodeId d() const noexcept { return terminal(1); }
    NodeId s() const noexcept { return terminal(2); }
    double vgs(const SolutionView& x) const noexcept { return m_sign * (x.voltage(g()) - x.voltage(s())); }
    double vds(const SolutionView& x) const noexcept { return m_sign * (x.voltage(d()) - x.voltage(s())); }
    // Kanal fuer Uds >= 0: Strom und Ableitungen nach Ugs (gm) und Uds (gds)
    void channel(double ugs, double uds, double& i, double& gm, double& gds) const noexcept;
    void evaluate(double ugs, double uds) noexcept;
    // Linearisierter Strom in Drain bzw. Gate (N-Kanal-Sicht)
    double drainLinear(double ugs, double uds) const noexcept;
    double gateLinear(double ugs) const noexcept;

    MosfetParams m_params;
    double m_sign = 1.0;
    double m_vt = 0.0;
    double m_vcrit = 0.0;
    double m_xMax = 80.0;
    // Linearisierungspunkt
    double m_vgs0 = 0.0;
    double m_vds0 = 0.0;
    double m_id0 = 0.0;
    double m_gm = 0.0;
    double m_gds = 0.0;
    // Gate-Kapazitaet (Rueckwaerts-Euler)
    double m_invDt = 0.0;
    double m_qOld = 0.0;
    double m_qNext = 0.0;
    double m_lastIA = 0.0;
    double m_lastDtS = 0.0;
    double m_overloadS = 0.0;
};

} // namespace volta::core

#pragma once

#include "volta/core/component_model.hpp"

namespace volta::core {

// Kennwerte eines NPN-Kleinsignaltransistors, angelehnt an BC547B (SPICE-Modell IS = 1,8e-14 A; BR ca. 7).
struct BjtParams {
    double isA = 1.8e-14; // Transportsaettigungsstrom
    double betaF = 200.0; // Stromverstaerkung vorwaerts (Katalog `beta`)
    double betaR = 7.0;   // Stromverstaerkung rueckwaerts
    double icMaxA = 0.1;  // Kollektor-Dauerstrom
    // Ladungsspeicher: Sperrschichtkapazitaeten (BC547: Cibo ca. 9 pF, Cobo ca. 4,5 pF, konstant angenommen) und
    // Diffusionsladung tau*I (Transitzeit vorwaerts ca. 0,5 ns, rueckwaerts 10 ns). Fuer die Zeitskala von Volta
    // ohne Einfluss, geben aber Umschaltvorgaengen (Kippstufen) ihre physikalische Zeitkonstante.
    double cbeF = 10e-12;
    double cbcF = 4e-12;
    double tauFS = 0.5e-9;
    double tauRS = 10e-9;
    // Ptot 0,5 W, Rth ca. 250 K/W; Zeitkonstante didaktisch 5 s, Grenze 150 Grad C
    ThermalParams thermal{250.0, 5.0 / 250.0, 423.0};
    // Starke Stromueberlast (Ic > overloadFactor * icMaxA) zerstoert nach overloadFailS
    double overloadFactor = 2.5;
    double overloadFailS = 3.0;
    // PNP (ab Volta 2): alle Spannungen und Stroeme mit umgekehrtem Vorzeichen
    bool pnp = false;

    // PNP-Komplementaertyp zum BC547B, angelehnt an BC557B (gleiche Stromverstaerkungsgruppe B, Ic 0,1 A, Ptot
    // 0,5 W); Kennwerte gespiegelt wie BC547B, damit Gegentaktstufen symmetrisch arbeiten.
    // Leistungstransistoren (ab Volta 3, Stufe 5: Endstufen am 8-Ohm-Lautsprecher), angelehnt an BD139/BD140:
    // Ic 1,5 A, Ptot 1,25 W ohne Kuehlkoerper (Rth ca. 100 K/W), Stromverstaerkung ca. 120, Ube ca. 0,7 V bei 0,1 A.
    // Ohne Bahnwiderstaende: Ube und Uce(sat) bei 0,5 A etwas zu klein (Datenblatt ca. 0,8 V bzw. 0,5 V).
    static BjtParams bd139() noexcept {
        BjtParams p;
        p.isA = 1.5e-13;
        p.betaF = 120.0;
        p.betaR = 5.0;
        p.icMaxA = 1.5;
        p.cbeF = 100e-12;
        p.cbcF = 30e-12;
        p.tauFS = 3e-9;
        p.tauRS = 50e-9;
        p.thermal = ThermalParams{100.0, 5.0 / 100.0, 423.0};
        return p;
    }
    static BjtParams bd140() noexcept {
        BjtParams p = bd139();
        p.pnp = true;
        return p;
    }

    static BjtParams bc557() noexcept {
        BjtParams p;
        p.pnp = true;
        return p;
    }
};

// NPN- bzw. PNP-Transistor auf 2-x-2-Platine (Katalogtypen `transistor_npn`, ab Volta 2 `transistor_pnp`; zweiter
// E-Fuss als Leitung in der Netzliste), vereinfachtes Ebers-Moll-Modell
// (Transportform, ohne Bahnwiderstaende und Early-Effekt, mit Ladungsspeicher Q = C*V + tau*I je Uebergang):
//   Ic = Is*(e^(Vbe/Vt) - e^(Vbc/Vt)) - Is/BR*(e^(Vbc/Vt) - 1)
//   Ib = Is/BF*(e^(Vbe/Vt) - 1) + Is/BR*(e^(Vbc/Vt) - 1)
// Anschluesse: 0 = Basis, 1 = Kollektor, 2 = Emitter (wie im Katalog b, c, e). Beide Uebergaenge mit pnjlim
// begrenzt, kJunctionGmin parallel zu jedem Uebergang, Ladungsspeicher als Begleitmodell (Rueckwaerts-Euler,
// im Arbeitspunkt ohne Wirkung). Zerstoert: alle Anschluesse offen.
// PNP: dieselben Gleichungen mit Veb, Vcb und umgekehrten Anschlussstroemen (Vorzeichen s = -1). Die Ableitungen
// nach den Knotenspannungen sind dabei unveraendert (s * s = 1); nur Strom und Konstante wechseln das Vorzeichen.
class Npn final : public ComponentModel {
public:
    Npn(std::string id, NodeId base, NodeId collector, NodeId emitter, BjtParams params = {});

    bool isPnp() const noexcept { return m_sign < 0.0; }

    const BjtParams& params() const noexcept { return m_params; }

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

private:
    NodeId b() const noexcept { return terminal(0); }
    NodeId c() const noexcept { return terminal(1); }
    NodeId e() const noexcept { return terminal(2); }
    // Uebergangsspannungen in Durchlassrichtung gezaehlt (PNP: Veb, Vcb)
    double vbe(const SolutionView& x) const noexcept { return m_sign * (x.voltage(b()) - x.voltage(e())); }
    double vbc(const SolutionView& x) const noexcept { return m_sign * (x.voltage(b()) - x.voltage(c())); }
    void evaluate(double vbe, double vbc) noexcept;
    double chargeBe(double vbe) const noexcept;
    double chargeBc(double vbc) const noexcept;
    // Linearisierter Strom in den Anschluss t (0 = B, 1 = C, 2 = E) bei (vbe, vbc)
    double linearCurrent(std::size_t t, double vbe, double vbc) const noexcept;

    BjtParams m_params;
    double m_sign = 1.0; // +1 NPN, -1 PNP
    double m_vt = 0.0;
    double m_vcrit = 0.0;
    double m_xMax = 80.0; // darueber Kennlinien linear (Steigung = Leitwertgrenze)
    // Linearisierung: I_t = m_i0[t] + m_dbe[t]*(vbe - m_vbe0) + m_dbc[t]*(vbc - m_vbc0)
    double m_vbe0 = 0.0;
    double m_vbc0 = 0.0;
    double m_i0[3] = {0.0, 0.0, 0.0};
    double m_dbe[3] = {0.0, 0.0, 0.0};
    double m_dbc[3] = {0.0, 0.0, 0.0};
    double m_lastIcA = 0.0;
    double m_lastDtS = 0.0;
    double m_overloadS = 0.0;
    // Ladungsspeicher: 1/dt des laufenden Loesungsvorgangs; Ladungen am Ende des letzten angenommenen Schritts.
    // updateState() schreibt m_q*Next, beginSolve() uebernimmt sie nach m_q*Old (Stempel und Messwerte bleiben
    // bis zum naechsten Loesungsvorgang konsistent).
    double m_invDt = 0.0;
    double m_qbeOld = 0.0;
    double m_qbcOld = 0.0;
    double m_qbeNext = 0.0;
    double m_qbcNext = 0.0;
};

// Bipolartransistor allgemein (NPN oder PNP nach BjtParams::pnp)
using Bjt = Npn;

} // namespace volta::core

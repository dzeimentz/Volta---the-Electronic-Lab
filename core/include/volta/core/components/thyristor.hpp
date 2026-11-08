#pragma once

#include "volta/core/component_model.hpp"

namespace volta::core {

// Thyristor, Triac und Diac (ab Volta 3, Stufe 4) als Zustandsmodell: Der Hauptpfad ist gesperrt (Leckleitwert) oder
// gezuendet (Schleusenspannung onV mit Bahnwiderstand onOhm). Der Zustand wechselt nur nach angenommenen
// Zeitschritten; die Ereignisortung teilt den Schritt am Kipppunkt, damit Zuend- und Loeschzeitpunkt unabhaengig von
// der Schrittweite sind.
//  - Zuenden (Thyristor, Triac): Strom im Gate-Uebergang (echter Si-pn-Uebergang Gate -> Kathode bzw. beim Triac
//    in beide Richtungen zu MT1) erreicht igtA, und der Hauptpfad kaeme gezuendet auf mindestens holdA.
//  - Kippen (Diac): Betrag der Spannung erreicht breakoverV.
//  - Loeschen: Hauptstrom faellt unter holdA (oder kehrt sich um).
// Vereinfachungen: keine Kippspannung in Durchlassrichtung (Thyristor/Triac sperren bis weit ueber 9 V), keine
// Freiwerdezeit, kein du/dt-Zuenden; der Triac zuendet in allen vier Quadranten gleich. Kennwerte angelehnt an
// kleine Thyristoren und Triacs im TO-92-Gehaeuse (Datenblattwerte: IGT 0,2 mA bzw. 3 mA, IH 3 mA bzw. 5 mA,
// UT etwa 1 V, IT 0,8 A) und an einen bidirektionalen Kippschalter fuer Kleinspannung (Kippspannung 6 V, Haltestrom
// 1,5 mA); Diacs fuer Netzdimmer kippen bei etwa 30 V und waeren an 9 V~ nutzlos.
enum class ThyristorKind { Scr, Triac, Diac };

struct ThyristorParams {
    ThyristorKind kind = ThyristorKind::Scr;
    double onV = 0.8;           // Schleusenspannung gezuendet
    double onOhm = 0.5;         // Bahnwiderstand gezuendet
    double offSiemens = 1e-8;   // Leckleitwert gesperrt
    double holdA = 3e-3;        // Haltestrom
    double igtA = 0.2e-3;       // Zuendstrom im Gate-Uebergang (nicht Diac)
    double gateIsA = 1e-14;     // Saettigungsstrom des Gate-Uebergangs (0,2 mA bei etwa 0,61 V)
    double gateShuntOhm = 1000; // innerer Widerstand Gate -> Kathode bzw. MT1
    double breakoverV = 0.0;    // Kippspannung (nur Diac)
    double iMaxA = 0.8;         // Dauerstrom
    ThermalParams thermal{100.0, 5.0 / 100.0, 398.0};
    double overloadFactor = 2.5; // starke Ueberlast zerstoert nach overloadFailS
    double overloadFailS = 1.0;

    static ThyristorParams scr() noexcept { return {}; }
    static ThyristorParams triac() noexcept {
        ThyristorParams p;
        p.kind = ThyristorKind::Triac;
        p.holdA = 5e-3;
        p.igtA = 3e-3;
        p.iMaxA = 1.0;
        return p;
    }
    static ThyristorParams diac() noexcept {
        ThyristorParams p;
        p.kind = ThyristorKind::Diac;
        p.onV = 1.5;
        p.onOhm = 5.0;
        p.holdA = 1.5e-3;
        p.breakoverV = 6.0;
        p.iMaxA = 0.2;
        p.thermal = ThermalParams{300.0, 2.0 / 300.0, 398.0};
        return p;
    }
};

// Anschluesse: Thyristor 0 = Anode, 1 = Kathode, 2 = Gate; Triac 0 = MT2, 1 = MT1, 2 = Gate; Diac 0 = a, 1 = b.
class Thyristor final : public ComponentModel {
public:
    Thyristor(std::string id, NodeId a, NodeId b, NodeId gate, ThyristorParams params = {});
    Thyristor(std::string id, NodeId a, NodeId b, ThyristorParams params); // Diac

    const ThyristorParams& params() const noexcept { return m_params; }
    bool isOn() const noexcept { return m_on; }

    bool isNonlinear() const noexcept override { return hasGate(); }
    bool linearize(const SolutionView& iterate) noexcept override;
    void resetLinearization(const SolutionView& accepted) noexcept override;
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    double eventFraction(const SolutionView& accepted, const SolutionView& next, double dtS) const noexcept override;
    void updateState(const SolutionView& x, double dtS) noexcept override;
    void thermal(const SolutionView& x, double dtS) noexcept override;
    void checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept override;
    ComponentReading read(const SolutionView& x) const noexcept override;
    ComponentState saveState() const noexcept override;
    void restoreState(const ComponentState& state) noexcept override;

private:
    bool hasGate() const noexcept { return m_params.kind != ThyristorKind::Diac; }
    double vMain(const SolutionView& x) const noexcept { return x.voltage(terminal(0)) - x.voltage(terminal(1)); }
    double vGate(const SolutionView& x) const noexcept { return x.voltage(terminal(2)) - x.voltage(terminal(1)); }
    // Hauptstrom (in Anschluss 0) beim Zustand on/Polaritaet s
    double mainCurrent(double v, bool on, double s) const noexcept;
    // Kippfunktionen (> 0: Wechsel faellig): Zuenden in Polaritaet s bzw. Loeschen
    double fireMargin(const SolutionView& x, double s) const noexcept;
    double holdMargin(const SolutionView& x) const noexcept;
    // Gate-Uebergang bei vg: Strom und Ableitung (Triac: beide Richtungen)
    void gateJunction(double vg, double& i, double& g) const noexcept;
    void evaluateGate(double vg) noexcept;

    ThyristorParams m_params;
    bool m_on = false;
    double m_sign = 1.0; // Stromrichtung im gezuendeten Zustand (Triac, Diac)
    double m_vt = 0.0;
    double m_vcrit = 0.0;
    double m_xMax = 80.0;
    double m_vTrig = 0.0;  // Gate-Spannung, bei der der Uebergang igtA fuehrt
    double m_vLatch = 0.0; // Hauptspannung, bei der gezuendet holdA fliesst
    // Linearisierung des Gate-Uebergangs: i = m_ig0 + m_gg * (vg - m_vg0)
    double m_vg0 = 0.0;
    double m_ig0 = 0.0;
    double m_gg = 0.0;
    double m_lastIA = 0.0;
    double m_lastDtS = 0.0;
    double m_overloadS = 0.0;
};

} // namespace volta::core

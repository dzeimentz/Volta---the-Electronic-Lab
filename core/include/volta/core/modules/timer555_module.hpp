#pragma once

#include <string>

#include "volta/core/circuit.hpp"
#include "volta/core/component_model.hpp"
#include "volta/core/modules/blink_module.hpp"

namespace volta::core {

// Anschluesse des Timers 555 (Katalog: minus, tr, aus, reset, cv, thr, dis, plus = Pins 1 bis 8).
struct Timer555Pins {
    NodeId gnd = kGround;       // 1 GND
    NodeId trigger = kGround;   // 2 TR: unter 1/3 U setzt das Flipflop
    NodeId out = kGround;       // 3 OUT: Gegentakt-Ausgang
    NodeId reset = kGround;     // 4 R: unter 0,7 V Ausgang aus (Vorrang)
    NodeId control = kGround;   // 5 CV: Abgriff des Teilers (2/3 U)
    NodeId threshold = kGround; // 6 THR: ueber CV setzt das Flipflop zurueck
    NodeId discharge = kGround; // 7 DIS: entlaedt nach GND, solange der Ausgang aus ist
    NodeId vcc = kGround;       // 8 +U
};

// Kennwerte angelehnt an das Datenblatt des NE555 (bipolar, 4,5-16 V).
struct Timer555Params {
    double dividerOhm = 5000.0;   // Teiler 3 x 5 kOhm
    double quiescentOhm = 1500.0; // Ruhestrom ca. 6 mA bei 9 V
    double highDropV = 1.7;       // Ausgang hoch: ca. 1,7 V unter +U
    double highOhm = 15.0;
    double lowV = 0.1;            // Ausgang niedrig: ca. 0,1 V
    double lowOhm = 10.0;
    double dischargeOhm = 10.0;   // Entladetransistor leitend
    double resetV = 0.7;          // Reset-Schwelle gegen GND
    double minSupplyV = 4.5;      // darunter arbeitet der Baustein nicht (Ausgang aus, DIS offen)
    double ratedA = 0.2;          // Ausgangsstrom; darueber ueberlastet
};

// Timer 555 als Verhaltensmodell (Konzept Volta 2, Kap. 6): Teiler 3 x 5 kOhm (innerer Knoten zwischen dem
// zweiten und dritten Widerstand), zwei Komparatoren, Flipflop, Entladetransistor und Gegentakt-Ausgang.
// Das Flipflop wird je Loesungsvorgang festgeschrieben (das Bauteil ist damit linear) und nach jedem angenommenen
// Schritt aus den Komparatoren fortgeschrieben. Damit die Kippzeitpunkte nicht auf das Schrittraster fallen,
// meldet eventFraction() den Bruchteil des Schritts, an dem ein Komparator kippt; der Simulator teilt den Schritt
// dort (Ereignisortung). Logik wie im Datenblatt: Reset hat Vorrang, dann setzt TR < CV/2, dann setzt THR > CV
// zurueck. Messwerte: voltV = Versorgung, level = Flipflop (1 = Ausgang hoch), displayValue = Ausgangsspannung
// gegen GND. Anschluesse 0 gnd, 1 tr, 2 out, 3 reset, 4 cv, 5 thr, 6 dis, 7 vcc.
class Timer555 final : public ComponentModel {
public:
    Timer555(std::string id, const Timer555Pins& pins, Timer555Params params = {});

    const Timer555Params& params() const noexcept { return m_params; }
    bool outputHigh() const noexcept { return m_q; }
    bool powered() const noexcept { return m_powered; }

    std::size_t internalNodeCount() const noexcept override { return 1; }
    void beginSolve(const StampContext& ctx) noexcept override;
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    double eventFraction(const SolutionView& accepted, const SolutionView& next, double dtS) const noexcept override;
    void updateState(const SolutionView& x, double dtS) noexcept override;
    void checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept override;
    ComponentReading read(const SolutionView& x) const noexcept override;
    ComponentState saveState() const noexcept override;
    void restoreState(const ComponentState& state) noexcept override;

private:
    // Neuer Zustand des Flipflops aus den Spannungen (gegen GND) bei altem Zustand q
    bool logic(const SolutionView& x, bool q) const noexcept;
    double outputCurrent(const SolutionView& x) const noexcept; // aus dem Ausgang heraus

    Timer555Params m_params;
    bool m_q = false;       // Flipflop (angenommen)
    bool m_powered = false; // im Loesungsvorgang festgeschrieben
    double m_lastOutA = 0.0;
};

SubcircuitRange addTimer555(Circuit& circuit, const std::string& prefix, const Timer555Pins& pins);

} // namespace volta::core

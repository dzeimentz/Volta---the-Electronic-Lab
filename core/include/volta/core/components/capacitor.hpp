#pragma once

#include "volta/core/component_model.hpp"

namespace volta::core {

struct CapacitorParams {
    double capacitanceF = 100e-6;
    double leakOhm = 1e9;     // Isolationswiderstand (Folie/Keramik >= 1 GOhm)
    bool polarized = false;   // Elko: Anschluss 0 = Plus
    double umaxV = 50.0;      // Nennspannung
    double reverseMaxV = 1.0; // Elko: hoechstens zulaessige Sperrspannung
    double failAfterS = 0.2;  // Dauer einer Grenzwertverletzung bis zum Defekt (Erwaermung vereinfacht)

    static CapacitorParams film(double capacitanceF) noexcept { return {capacitanceF, 1e9, false, 63.0, 0.0, 0.2}; }
    // Elektrolytkondensator 16 V; Leckstrom typisch < 0,01*C*U bzw. wenige uA -> 10 MOhm als Ausgangswert
    static CapacitorParams electrolytic(double capacitanceF, double umaxV = 16.0) noexcept {
        return {capacitanceF, 1e7, true, umaxV, 1.0, 0.2};
    }
};

// Kondensator und Elko (Katalogtypen `kondensator`, `elko`). Anschluss 0 = a bzw. Plus, Anschluss 1 = b bzw. Minus.
//
// Zeitschritt (Rueckwaerts-Euler) als Spannungszweig: V(a) - V(b) - (dt/C) * I = U_alt, mit U_alt aus dem eigenen
// Zustand. Im Arbeitspunkt (dt <= 0) wirkt der Kondensator als Spannungsquelle seiner Ladung (Serienwiderstand
// 10 us / C, mindestens kMinOhm), sodass Leitungswechsel und Undo die Ladung erhalten und parallele Kondensatoren
// ladungsgewichtet zusammenfinden. Die Quellenrampe skaliert diese Quelle nicht. Parallel liegt der Leckwiderstand.
// Elko: Defekt (danach offen), wenn Sperr- oder Ueberspannung laenger als failAfterS anliegt; Folienkondensator:
// Defekt bei Ueberspannung.
class Capacitor final : public ComponentModel {
public:
    Capacitor(std::string id, NodeId a, NodeId b, CapacitorParams params = {});

    const CapacitorParams& params() const noexcept { return m_params; }
    double voltageV() const noexcept { return m_voltV; }
    // Setzt die Ladespannung (z. B. Vorgabe eines Experiments). Kein Teil des Simulationsschritts.
    void setVoltage(double voltV) noexcept;

    std::size_t branchCount() const noexcept override { return 1; }
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    void updateState(const SolutionView& x, double dtS) noexcept override;
    void checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept override;
    ComponentState saveState() const noexcept override;
    void restoreState(const ComponentState& state) noexcept override;

private:
    CapacitorParams m_params;
    double m_voltV = 0.0;     // Spannung am Ende des letzten angenommenen Schritts
    double m_violationS = 0.0; // bisherige Dauer einer Grenzwertverletzung
    double m_lastDtS = 0.0;
};

} // namespace volta::core

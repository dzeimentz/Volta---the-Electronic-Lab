#pragma once

#include "volta/core/component_model.hpp"

namespace volta::core {

// Kennwerte eines Uebertragers mit zwei Wicklungen auf einem Kern (ab Volta 3, Stufe 3: Sperrschwinger).
// Ausgangswerte: Ringkern mit zwei gleichen Wicklungen (bifilar gewickelt), je 10 mH und 1,5 Ohm, Kopplung 0,98
// (Streuinduktivitaet 2 %), Nennstrom 0,3 A je Wicklung. Rp je Wicklung wie bei der Spule (Kernverluste,
// Wicklungskapazitaet): begrenzt die Abschaltspitze auf I * Rp.
struct TransformerParams {
    double inductance1H = 10e-3;
    double inductance2H = 10e-3;
    double coupling = 0.98; // k, Gegeninduktivitaet M = k * sqrt(L1 * L2)
    double winding1Ohm = 1.5;
    double winding2Ohm = 1.5;
    double parallelOhm = 2e5;
    double imaxA = 0.3;
    // Erwaermung beider Wicklungen (I*I*R), Rth ca. 80 K/W, Zeitkonstante didaktisch 10 s, Grenze 130 Grad C
    ThermalParams thermal{80.0, 10.0 / 80.0, 403.0};
};

// Uebertrager (Katalogtyp `uebertrager`): zwei gekoppelte Spulen. Anschluesse 0/1 = Wicklung 1 (Anfang a1,
// Ende e1), 2/3 = Wicklung 2 (Anfang a2, Ende e2); die Anfaenge sind im Schaltzeichen mit Punkt markiert
// (gleichsinnig gewickelt). Zeitschritt als Begleitmodell ohne Zweigstroeme (Norton-Form des Zweitors):
//   u = Z i + h,  Z = R + a0/dt * L,  h = -L/dt * (a1 i(n) - a2 i(n-1)),  L = [[L1, M], [M, L2]]
//   i = Y (u - h), Y = Z^-1
// Die Stroeme i1, i2 sind die Wicklungsstroeme ohne Rp. Rueckwaerts-Euler oder BDF2 wie Spule und Kondensator.
// Im Arbeitspunkt wirken nur die Wicklungswiderstaende. Zerstoert (Wicklungen offen) bei Uebertemperatur.
class Transformer final : public ComponentModel {
public:
    Transformer(std::string id, NodeId a1, NodeId e1, NodeId a2, NodeId e2, TransformerParams params = {});

    const TransformerParams& params() const noexcept { return m_params; }
    double current1A() const noexcept { return m_i[0]; }
    double current2A() const noexcept { return m_i[1]; }
    double mutualH() const noexcept;

    void beginSolve(const StampContext& ctx) noexcept override;
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    bool stampAc(ComplexMna& sys, const AcContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    void updateState(const SolutionView& x, double dtS) noexcept override;
    void thermal(const SolutionView& x, double dtS) noexcept override;
    void checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept override;
    ComponentReading read(const SolutionView& x) const noexcept override;
    ComponentState saveState() const noexcept override;
    void restoreState(const ComponentState& state) noexcept override;

private:
    double windingVolt(const SolutionView& x, int w) const noexcept;
    double windingCurrent(const SolutionView& x, int w) const noexcept; // ohne Rp

    TransformerParams m_params;
    double m_i[2] = {0.0, 0.0};
    double m_prev[2] = {0.0, 0.0};
    double m_lastDtS = 0.0;
    bool m_havePrev = false;
    // in beginSolve festgeschrieben: i = Y u + j
    double m_y[2][2] = {{0.0, 0.0}, {0.0, 0.0}};
    double m_j[2] = {0.0, 0.0};
};

} // namespace volta::core

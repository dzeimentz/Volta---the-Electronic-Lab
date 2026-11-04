#pragma once

#include "volta/core/component_model.hpp"

namespace volta::core {

// Kennwerte einer Spule (ab Volta 2, Katalog 100 mH / 1 H). Ausgangswerte nach typischen Festinduktivitaeten bzw.
// Drosseln (Datenblattbereiche, Kalibrierung offen): 100 mH mit ca. 30 Ohm und 0,1 A, 1 H mit ca. 100 Ohm und
// 0,05 A. Der Parallelwiderstand steht fuer Wicklungskapazitaet und Kernverluste: Er begrenzt die Spannungsspitze
// beim Abschalten auf I * Rp (bei 50 mA etwa 5 kV bzw. so viel, wie die Schaltung abnimmt), statt sie ueber GMIN
// ins Unendliche laufen zu lassen.
struct InductorParams {
    double inductanceH = 0.1;
    double windingOhm = 30.0;
    double parallelOhm = 1e5;
    double imaxA = 0.1;
    // Erwaermung der Wicklung (I*I*R), Rth ca. 100 K/W, Zeitkonstante didaktisch 10 s, Grenze 130 Grad C (Lack)
    ThermalParams thermal{100.0, 10.0 / 100.0, 403.0};

    static InductorParams mH100() noexcept { return {0.1, 30.0, 1e5, 0.1, {100.0, 0.1, 403.0}}; }
    static InductorParams h1() noexcept { return {1.0, 100.0, 1e5, 0.05, {100.0, 0.1, 403.0}}; }
};

// Spule (Katalogtyp `spule`): Induktivitaet L in Reihe mit dem Wicklungswiderstand R, parallel dazu Rp.
// Zeitschritt (Rueckwaerts-Euler) als Spannungszweig: V(a) - V(b) - (R + L/dt) * I = -(L/dt) * I_alt, mit I_alt aus
// dem eigenen Zustand. Im Arbeitspunkt wirkt sie als Wicklungswiderstand (Gleichstrom); der Zustand folgt dem
// Arbeitspunkt. Zerstoert (danach offen) bei Uebertemperatur der Wicklung.
class Inductor final : public ComponentModel {
public:
    Inductor(std::string id, NodeId a, NodeId b, InductorParams params = {});

    const InductorParams& params() const noexcept { return m_params; }
    double currentA() const noexcept { return m_currentA; } // Strom durch L am Ende des letzten Schritts

    std::size_t branchCount() const noexcept override { return 1; }
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    bool stampAc(ComplexMna& sys, const AcContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    void updateState(const SolutionView& x, double dtS) noexcept override;
    void thermal(const SolutionView& x, double dtS) noexcept override;
    void checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept override;
    ComponentState saveState() const noexcept override;
    void restoreState(const ComponentState& state) noexcept override;

private:
    InductorParams m_params;
    double m_currentA = 0.0;
};

} // namespace volta::core

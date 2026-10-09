#pragma once

#include "volta/core/component_model.hpp"

namespace volta::core {

// Vereinfachung fuer Volta 1: Die Spulen (Elektromagnet, Relais) werden ohne Induktivitaet gerechnet. Mit L/R im
// Bereich weniger Millisekunden ist der Stromanstieg bei der festen Schrittweite von 1 ms praktisch nicht sichtbar;
// die Abschaltspannung einer Spule (und die Freilaufdiode) behandelt erst Volta 2. Im Elternbereich offengelegt.

// Elektromagnet (Katalogtyp `elektromagnet`): Spule auf Eisenkern, ca. 100 Ohm. Feldstaerke ~ Strom; level = Strom /
// Nennstrom (1 bei 9 V), mit Vorzeichen fuer die Polung. Erwaermt sich bei Dauerbetrieb (1 W Belastbarkeit).
struct ElectromagnetParams {
    double ohm = 100.0;
    double nominalA = 0.09; // 9 V / 100 Ohm
    double pmaxW = 1.0;
    ThermalParams thermal{60.0, 20.0 / 60.0, 423.0}; // tau 20 s; ueber 150 Grad C zerstoert (dauerhaft ab ca. 2 W)
};

class Electromagnet final : public ComponentModel {
public:
    Electromagnet(std::string id, NodeId a, NodeId b, ElectromagnetParams params = {});

    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    void thermal(const SolutionView& x, double dtS) noexcept override;
    void checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept override;
    ComponentReading read(const SolutionView& x) const noexcept override;

private:
    double g() const noexcept { return m_destroyed ? kGmin : 1.0 / m_params.ohm; }
    ElectromagnetParams m_params;
    double m_lastPowerW = 0.0;
};

// Relais (Katalogtyp `relais`, Kleinrelais 9 V auf 2-x-2-Platine): Spule zwischen a und b (180 Ohm, 50 mA bei 9 V),
// Schliesser zwischen com und no. Zieht an, wenn der Spulenstrom mindestens pullA betraegt (ca. 60 % des
// Nennstroms) und das fuer pullDelayS; faellt ab unter dropA (Hysterese) nach dropDelayS. Kontakt 0,05 Ohm,
// hoechstens 1 A. level = 1 angezogen, 0 abgefallen; currentA = Spulenstrom.
// Anschluesse: 0 a, 1 b (Spule), 2 com, 3 no.
struct RelayParams {
    double coilOhm = 180.0;
    double pullA = 0.03;
    double dropA = 0.012;
    double pullDelayS = 0.008;
    double dropDelayS = 0.004;
    double contactOhm = 0.05;
    double contactMaxA = 1.0;
    double coilPmaxW = 1.0;
};

class Relay final : public ComponentModel {
public:
    Relay(std::string id, NodeId a, NodeId b, NodeId common, NodeId normallyOpen, RelayParams params = {});

    bool isClosed() const noexcept { return m_closed; }

    void beginSolve(const StampContext& ctx) noexcept override;
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    void updateState(const SolutionView& x, double dtS) noexcept override;
    void checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept override;
    ComponentReading read(const SolutionView& x) const noexcept override;
    ComponentState saveState() const noexcept override;
    void restoreState(const ComponentState& state) noexcept override;

private:
    double coilG() const noexcept { return m_destroyed ? kGmin : 1.0 / m_params.coilOhm; }
    double contactG() const noexcept { return m_closed && !m_destroyed ? 1.0 / m_params.contactOhm : kGmin; }
    RelayParams m_params;
    bool m_closed = false;  // Kennwert des laufenden Loesungsvorgangs
    bool m_pending = false; // aus dem angenommenen Zustand
    double m_timerS = 0.0;  // Dauer, seit der die Schaltschwelle ueberschritten ist
    double m_coilPowerW = 0.0;
    double m_contactA = 0.0;
};

} // namespace volta::core

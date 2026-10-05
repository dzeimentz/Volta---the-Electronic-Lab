#pragma once

#include "volta/core/component_model.hpp"

namespace volta::core {

struct PotentiometerParams {
    double ohm = 10000.0; // Gesamtwiderstand (Pult-Drehregler: 10 kOhm linear, Ausgangswert)
    double pmaxW = 0.25;  // Belastbarkeit der Widerstandsbahn
    double minOhm = 1.0;  // Restwiderstand je Teilbahn an den Anschlaegen (Schleiferkontakt)
};

// Potentiometer (Pult-Drehregler P1/P2, Katalogtyp `potentiometer`): Anschluesse 0 = a, 1 = Schleifer s, 2 = b.
// Zwei Teilwiderstaende a-s = position * R und s-b = (1 - position) * R, jeweils mindestens minOhm.
// Die Stellung (0..1, stufenlos) ist eine Bedienhandlung und wird in beginSolve() fuer den Loesungsvorgang
// festgeschrieben. Ueberlastet, wenn eine Teilbahn mehr als ihren Anteil an pmaxW umsetzt.
class Potentiometer final : public ComponentModel {
public:
    Potentiometer(std::string id, NodeId a, NodeId wiper, NodeId b, PotentiometerParams params = {},
                  double position = 0.5);

    const PotentiometerParams& params() const noexcept { return m_params; }
    double position() const noexcept { return m_position; }
    void setPosition(double position) noexcept;
    // Teilwiderstaende fuer den laufenden Loesungsvorgang
    double ohmAS() const noexcept { return 1.0 / m_gAS; }
    double ohmSB() const noexcept { return 1.0 / m_gSB; }

    void beginSolve(const StampContext& ctx) noexcept override;
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    void thermal(const SolutionView& x, double dtS) noexcept override;
    void checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept override;
    ComponentReading read(const SolutionView& x) const noexcept override;

private:
    void applyPosition() noexcept;

    PotentiometerParams m_params;
    double m_position = 0.5;
    double m_gAS = 1.0 / 5000.0;
    double m_gSB = 1.0 / 5000.0;
    double m_lastPowerAS = 0.0;
    double m_lastPowerSB = 0.0;
};

} // namespace volta::core

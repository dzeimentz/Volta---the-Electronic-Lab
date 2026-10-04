#pragma once

#include "volta/core/component_model.hpp"

namespace volta::core {

struct ResistorParams {
    double ohm = 1000.0;
    double pmaxW = 0.25; // Belastbarkeit Kohleschichtwiderstand 1/4 W (Datenblatt ueblicher Bauform 0207)
    // Rth ca. 340 K/W (0,25 W bei 70 Grad C -> 155 Grad C); Zeitkonstante didaktisch 10 s;
    // Zerstoerung bei ca. 230 Grad C, d. h. dauerhaft ab ca. 2,5 * pmaxW.
    ThermalParams thermal{340.0, 10.0 / 340.0, 505.0};
};

// Linearer Widerstand (Katalogtyp `widerstand`). Ueberlastet oberhalb pmaxW, brennt bei Uebertemperatur durch
// (danach offen).
class Resistor final : public ComponentModel {
public:
    Resistor(std::string id, NodeId a, NodeId b, ResistorParams params = {});

    const ResistorParams& params() const noexcept { return m_params; }
    void setOhm(double ohm) noexcept;

    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    void thermal(const SolutionView& x, double dtS) noexcept override;
    void checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept override;

private:
    double powerW(const SolutionView& x) const noexcept;

    ResistorParams m_params;
    double m_lastPowerW = 0.0;
};

} // namespace volta::core

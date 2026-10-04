#pragma once

#include "volta/core/component_model.hpp"

namespace volta::core {

struct ResistorParams {
    double ohm = 1000.0;
    double pmaxW = 0.25; // Belastbarkeit Kohleschichtwiderstand 1/4 W (Datenblatt ueblicher Bauform 0207)
};

// Linearer Widerstand (Katalogtyp `widerstand`). Thermik und Zerstoerung folgen mit dem Thermikmodell.
class Resistor final : public ComponentModel {
public:
    Resistor(std::string id, NodeId a, NodeId b, ResistorParams params = {});

    const ResistorParams& params() const noexcept { return m_params; }
    void setOhm(double ohm) noexcept;

    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;

private:
    ResistorParams m_params;
};

} // namespace volta::core

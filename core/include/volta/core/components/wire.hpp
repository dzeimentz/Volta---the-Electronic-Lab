#pragma once

#include "volta/core/component_model.hpp"

namespace volta::core {

// Gezogene Leitung zwischen zwei Anschlusspunkten: kWireOhm (nur numerisch, in der Darstellung vernachlaessigt).
// Leitungen werden nicht zerstoert; bei grossem Strom (Kurzschluss) gelten sie als ueberlastet und gluehen in der
// Darstellung. Der Strom je Leitung speist die Stromflussdarstellung.
class Wire final : public ComponentModel {
public:
    static constexpr double kGlowA = 1.0; // ab hier glueht die Leitung (Kurzschluss)

    Wire(std::string id, NodeId a, NodeId b);

    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    void thermal(const SolutionView& x, double dtS) noexcept override;
    void checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept override;

private:
    double m_lastCurrentA = 0.0;
};

} // namespace volta::core

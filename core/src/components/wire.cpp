#include "volta/core/components/wire.hpp"

#include <cmath>
#include <utility>

namespace volta::core {

Wire::Wire(std::string id, NodeId a, NodeId b) : ComponentModel(std::move(id), {a, b}) {}

void Wire::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    sys.stampConductance(terminal(0), terminal(1), 1.0 / kWireOhm);
}

double Wire::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    const double i = (x.voltage(terminal(0)) - x.voltage(terminal(1))) / kWireOhm;
    return t == 0 ? i : -i;
}

void Wire::thermal(const SolutionView& x, double /*dtS*/) noexcept { m_lastCurrentA = terminalCurrent(x, 0); }

void Wire::checkLimits(EventQueue& /*events*/, std::size_t /*index*/, double /*timeS*/) noexcept {
    m_overloaded = std::fabs(m_lastCurrentA) > kGlowA;
}

} // namespace volta::core

#include "volta/core/components/resistor.hpp"

#include <cmath>
#include <utility>

namespace volta::core {

namespace {
// Unsinnige Werte abfangen: nicht endlich -> praktisch offen, zu klein -> kMinOhm.
double sanitizeOhm(double ohm) noexcept {
    if (!std::isfinite(ohm)) return 1e12;
    return ohm < kMinOhm ? kMinOhm : ohm;
}
} // namespace

Resistor::Resistor(std::string id, NodeId a, NodeId b, ResistorParams params)
    : ComponentModel(std::move(id), {a, b}), m_params(params) {
    m_params.ohm = sanitizeOhm(m_params.ohm);
}

void Resistor::setOhm(double ohm) noexcept { m_params.ohm = sanitizeOhm(ohm); }

void Resistor::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    sys.stampConductance(terminal(0), terminal(1), 1.0 / m_params.ohm);
}

double Resistor::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    const double i = (x.voltage(terminal(0)) - x.voltage(terminal(1))) / m_params.ohm;
    return t == 0 ? i : -i;
}

} // namespace volta::core

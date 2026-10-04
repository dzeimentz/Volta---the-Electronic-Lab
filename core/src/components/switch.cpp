#include "volta/core/components/switch.hpp"

#include <utility>

namespace volta::core {

Switch::Switch(std::string id, NodeId a, NodeId b, bool closed)
    : ComponentModel(std::move(id), {a, b}), m_closed(closed) {}

void Switch::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    sys.stampConductance(terminal(0), terminal(1), conductance());
}

double Switch::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    const double i = (x.voltage(terminal(0)) - x.voltage(terminal(1))) * conductance();
    return t == 0 ? i : -i;
}

ComponentReading Switch::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    r.level = m_closed ? 1.0 : 0.0;
    return r;
}

ChangeoverSwitch::ChangeoverSwitch(std::string id, NodeId common, NodeId a, NodeId b, bool toB)
    : ComponentModel(std::move(id), {common, a, b}), m_toB(toB) {}

void ChangeoverSwitch::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    sys.stampConductance(terminal(0), terminal(1), gA());
    sys.stampConductance(terminal(0), terminal(2), gB());
}

double ChangeoverSwitch::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    const double vc = x.voltage(terminal(0));
    const double ia = (vc - x.voltage(terminal(1))) * gA(); // von der Mitte nach A
    const double ib = (vc - x.voltage(terminal(2))) * gB();
    if (t == 0) return ia + ib;
    return t == 1 ? -ia : -ib;
}

ComponentReading ChangeoverSwitch::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    r.level = m_toB ? 1.0 : 0.0;
    return r;
}

} // namespace volta::core

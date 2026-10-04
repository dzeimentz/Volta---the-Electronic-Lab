#include "volta/core/components/battery.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace volta::core {

Battery::Battery(std::string id, NodeId plus, NodeId minus, BatteryParams params)
    : ComponentModel(std::move(id), {plus, minus}), m_params(params) {
    setCells(m_params.cells);
    if (!(m_params.cellInnerOhm >= kMinOhm)) m_params.cellInnerOhm = kMinOhm;
    if (!std::isfinite(m_params.cellVolt) || m_params.cellVolt < 0.0) m_params.cellVolt = BatteryParams{}.cellVolt;
}

void Battery::setCells(int cells) noexcept { m_params.cells = std::clamp(cells, kMinCells, kMaxCells); }

void Battery::stamp(MnaSystem& sys, const StampContext& ctx) const noexcept {
    sys.stampVoltageBranch(terminal(0), terminal(1), firstBranch(), emfV() * ctx.sourceScale, innerOhm());
}

double Battery::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    // Zweigstrom fliesst am Pluspol hinein (beim Entladen negativ).
    const double i = x.branchCurrent(firstBranch());
    return t == 0 ? i : -i;
}

void Battery::thermal(const SolutionView& x, double dtS) noexcept {
    const double i = x.branchCurrent(firstBranch());
    integrateThermal(i * i * innerOhm(), dtS, m_params.thermal);
}

void Battery::checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept {
    if (!m_hot && m_tempK > m_params.hotK) {
        m_hot = true;
        events.push({SimEventType::BatteryHot, index, timeS});
    } else if (m_hot && m_tempK < m_params.coolK) {
        m_hot = false;
    }
    m_overloaded = m_hot;
}

ComponentState Battery::saveState() const noexcept {
    ComponentState s = ComponentModel::saveState();
    s.internal[0] = m_hot ? 1.0 : 0.0;
    return s;
}

void Battery::restoreState(const ComponentState& state) noexcept {
    ComponentModel::restoreState(state);
    m_hot = state.internal[0] != 0.0;
    m_overloaded = m_hot;
}

} // namespace volta::core

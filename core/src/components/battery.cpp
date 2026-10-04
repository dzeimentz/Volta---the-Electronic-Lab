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

} // namespace volta::core

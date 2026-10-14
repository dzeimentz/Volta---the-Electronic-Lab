#include "volta/logic/logic_sim.hpp"

#include <utility>

namespace volta::logic {

void LogicSim::load(const Netlist& netlist) {
    m_net = netlist;
    m_value.assign(m_net.netCount, Level::X);
    m_fanout.assign(m_net.netCount, {});
    for (std::size_t g = 0; g < m_net.gates.size(); ++g) {
        const Gate& gate = m_net.gates[g];
        m_fanout[gate.a].push_back(g);
        if (gate.b != gate.a) m_fanout[gate.b].push_back(g);
    }
    m_inputNet.assign(m_net.netCount, 0);
    for (NetId n : m_net.inputs) m_inputNet[n] = 1;
    m_isPending.assign(m_net.gates.size(), 1); // zu Beginn rechnet jedes Gatter einmal
    m_pending.resize(m_net.gates.size());
    for (std::size_t g = 0; g < m_pending.size(); ++g) m_pending[g] = g;
    m_nextPending.clear();
    m_nextPending.reserve(m_net.gates.size());
    m_changes.clear();
    m_changes.reserve(m_net.gates.size());
    m_lastRounds = 0;
}

void LogicSim::setInput(std::size_t index, Level level) noexcept {
    if (index >= m_net.inputs.size()) return;
    const NetId n = m_net.inputs[index];
    if (m_value[n] == level) return;
    m_value[n] = level;
    for (std::size_t g : m_fanout[n])
        if (!m_isPending[g]) {
            m_isPending[g] = 1;
            m_pending.push_back(g);
        }
}

Level LogicSim::input(std::size_t index) const noexcept {
    return index < m_net.inputs.size() ? m_value[m_net.inputs[index]] : Level::X;
}

Level LogicSim::output(std::size_t index) const noexcept {
    return index < m_net.outputs.size() ? m_value[m_net.outputs[index]] : Level::X;
}

bool LogicSim::round() noexcept {
    if (m_pending.empty()) return false;
    // alle Gatter der Runde lesen den alten Zustand, dann wechseln die Ausgaenge gleichzeitig
    m_changes.clear();
    for (std::size_t g : m_pending) {
        m_isPending[g] = 0;
        const Gate& gate = m_net.gates[g];
        if (m_inputNet[gate.y]) continue; // Eingang wird von aussen bestimmt
        const Level next = evaluate(gate.kind, m_value[gate.a], m_value[gate.b]);
        if (next != m_value[gate.y]) m_changes.emplace_back(gate.y, next);
    }
    m_nextPending.clear();
    for (const auto& change : m_changes) {
        m_value[change.first] = change.second;
        for (std::size_t g : m_fanout[change.first])
            if (!m_isPending[g]) {
                m_isPending[g] = 1;
                m_nextPending.push_back(g);
            }
    }
    std::swap(m_pending, m_nextPending);
    return !m_changes.empty();
}

SettleStatus LogicSim::settle(int maxRounds) noexcept {
    m_lastRounds = 0;
    while (!m_pending.empty()) {
        if (m_lastRounds >= maxRounds) return SettleStatus::Oscillating;
        round();
        ++m_lastRounds;
    }
    return SettleStatus::Stable;
}

namespace {
std::vector<Level> rowInputs(std::size_t row, std::size_t count) {
    std::vector<Level> in(count);
    for (std::size_t i = 0; i < count; ++i)
        in[i] = ((row >> (count - 1 - i)) & 1u) ? Level::One : Level::Zero;
    return in;
}
} // namespace

CheckResult checkTruthTable(const Netlist& netlist, const std::vector<std::vector<Level>>& expected) {
    CheckResult result;
    const std::size_t n = netlist.inputs.size();
    const std::size_t rows = std::size_t{1} << n;
    if (n > 12 || expected.size() != rows) return result;
    LogicSim sim;
    sim.load(netlist);
    result.passed = true;
    for (std::size_t r = 0; r < rows; ++r) {
        TruthRow row;
        row.inputs = rowInputs(r, n);
        for (std::size_t i = 0; i < n; ++i) sim.setInput(i, row.inputs[i]);
        if (sim.settle() != SettleStatus::Stable) result.oscillating = true;
        row.expected = expected[r];
        for (std::size_t o = 0; o < netlist.outputs.size(); ++o) row.actual.push_back(sim.output(o));
        row.ok = row.actual == row.expected;
        result.passed = result.passed && row.ok;
        result.rows.push_back(std::move(row));
    }
    result.passed = result.passed && !result.oscillating;
    return result;
}

std::vector<std::vector<Level>> truthTable(const Netlist& netlist) {
    std::vector<std::vector<Level>> table;
    const std::size_t n = netlist.inputs.size();
    if (n > 12) return table;
    LogicSim sim;
    sim.load(netlist);
    for (std::size_t r = 0; r < (std::size_t{1} << n); ++r) {
        const std::vector<Level> in = rowInputs(r, n);
        for (std::size_t i = 0; i < n; ++i) sim.setInput(i, in[i]);
        sim.settle();
        std::vector<Level> out;
        for (std::size_t o = 0; o < netlist.outputs.size(); ++o) out.push_back(sim.output(o));
        table.push_back(std::move(out));
    }
    return table;
}

} // namespace volta::logic

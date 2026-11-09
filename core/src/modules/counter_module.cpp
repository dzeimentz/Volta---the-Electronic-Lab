#include "volta/core/modules/counter_module.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace volta::core {

namespace {
constexpr double kEventMarginS = 1e-7;

double crossing(double fa, double fn) noexcept {
    if (!(fa <= 0.0 && fn > 0.0)) return 2.0;
    return fa / (fa - fn);
}

LogicModuleParams sane(LogicModuleParams p) noexcept {
    if (!(p.outOhm > 0.0)) p.outOhm = 200.0;
    if (!(p.upperFraction > p.lowerFraction)) {
        p.upperFraction = 0.6;
        p.lowerFraction = 0.4;
    }
    return p;
}

// Segmente a b c d e f g = Bit 0 ... 6
constexpr int kPatterns[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

int popcount7(int m) noexcept {
    int n = 0;
    for (int k = 0; k < 7; ++k) n += (m >> k) & 1;
    return n;
}
} // namespace

int sevenSegmentPattern(int digit) noexcept { return (digit >= 0 && digit <= 9) ? kPatterns[digit] : 0; }

// --- gemeinsamer Teil ---------------------------------------------------------------------------------------------

LogicModelBase::LogicModelBase(std::string id, std::initializer_list<NodeId> terminals, std::size_t firstInput,
                               std::size_t inputCount, LogicModuleParams params)
    : ComponentModel(std::move(id), terminals), m_params(sane(params)), m_firstInput(firstInput),
      m_inputCount(std::min<std::size_t>(inputCount, 4)) {}

double LogicModelBase::rel(const SolutionView& x, std::size_t t) const noexcept {
    return x.voltage(terminal(t)) - x.voltage(terminal(1));
}

void LogicModelBase::beginSolve(const StampContext& ctx) noexcept {
    m_powered = !m_destroyed && rel(ctx.accepted, 0) >= m_params.minSupplyV;
}

void LogicModelBase::stampOutput(MnaSystem& sys, std::size_t t, bool high) const noexcept {
    if (!m_powered) return;
    sys.stampConductance(terminal(t), high ? terminal(0) : terminal(1), 1.0 / m_params.outOhm);
}

double LogicModelBase::outputCurrent(const SolutionView& x, std::size_t t, bool high) const noexcept {
    if (!m_powered) return 0.0;
    return (x.voltage(terminal(high ? 0 : 1)) - x.voltage(terminal(t))) / m_params.outOhm;
}

double LogicModelBase::eventFraction(const SolutionView& a, const SolutionView& n, double dtS) const noexcept {
    if (!m_powered || !(dtS > 0.0)) return 1.0;
    double theta = 2.0;
    for (std::size_t k = 0; k < m_inputCount; ++k) {
        const std::size_t t = m_firstInput + k;
        if (m_in[k]) {
            theta = std::min(theta, crossing(m_params.lowerFraction * rel(a, 0) - rel(a, t),
                                             m_params.lowerFraction * rel(n, 0) - rel(n, t)));
        } else {
            theta = std::min(theta, crossing(rel(a, t) - m_params.upperFraction * rel(a, 0),
                                             rel(n, t) - m_params.upperFraction * rel(n, 0)));
        }
    }
    if (theta > 1.0) return 1.0;
    return std::min(1.0, theta + kEventMarginS / dtS);
}

void LogicModelBase::updateState(const SolutionView& x, double /*dtS*/) noexcept {
    const double u = rel(x, 0);
    if (m_destroyed || !m_powered || u < m_params.minSupplyV) { // ohne Versorgung zurueck in den Einschaltzustand
        m_in = {false, false, false, false};
        onPowerLost();
        return;
    }
    const std::array<bool, 4> prev = m_in;
    for (std::size_t k = 0; k < m_inputCount; ++k) {
        const double v = rel(x, m_firstInput + k);
        if (m_in[k] && v < m_params.lowerFraction * u) m_in[k] = false;
        else if (!m_in[k] && v > m_params.upperFraction * u) m_in[k] = true;
    }
    onInputs(prev);
}

// --- Binaerzaehler ------------------------------------------------------------------------------------------------

BinaryCounter::BinaryCounter(std::string id, NodeId plus, NodeId minus, NodeId clock, NodeId reset,
                             std::array<NodeId, 4> q, LogicModuleParams params)
    : LogicModelBase(std::move(id), {plus, minus, clock, reset, q[0], q[1], q[2], q[3]}, 2, 2, params) {}

void BinaryCounter::onInputs(const std::array<bool, 4>& prev) noexcept {
    if (m_in[1]) m_count = 0;
    else if (m_in[0] && !prev[0]) m_count = (m_count + 1) & 0xF;
}

void BinaryCounter::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    for (std::size_t k = 0; k < 4; ++k) stampOutput(sys, 4 + k, bit(k));
}

double BinaryCounter::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    double ip = 0.0, im = 0.0;
    for (std::size_t k = 0; k < 4; ++k) {
        const double i = outputCurrent(x, 4 + k, bit(k));
        if (t == 4 + k) return -i;
        (bit(k) ? ip : im) += i;
    }
    if (t == 0) return ip;
    if (t == 1) return im;
    return 0.0;
}

ComponentReading BinaryCounter::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    r.voltV = rel(x, 0);
    r.level = bit(0) ? 1.0 : 0.0;
    r.displayValue = m_count;
    return r;
}

ComponentState BinaryCounter::saveState() const noexcept {
    ComponentState s = ComponentModel::saveState();
    s.internal[0] = m_in[0] ? 1.0 : 0.0;
    s.internal[1] = m_in[1] ? 1.0 : 0.0;
    s.internal[2] = m_count;
    return s;
}

void BinaryCounter::restoreState(const ComponentState& state) noexcept {
    ComponentModel::restoreState(state);
    m_in = {state.internal[0] > 0.5, state.internal[1] > 0.5, false, false};
    m_count = std::clamp(static_cast<int>(std::lround(state.internal[2])), 0, 15);
}

// --- Dekadenzaehler -----------------------------------------------------------------------------------------------

DecadeCounter::DecadeCounter(std::string id, NodeId plus, NodeId minus, NodeId clock, NodeId reset,
                             std::array<NodeId, 4> q, LogicModuleParams params)
    : LogicModelBase(std::move(id), {plus, minus, clock, reset, q[0], q[1], q[2], q[3]}, 2, 2, params) {}

void DecadeCounter::onInputs(const std::array<bool, 4>& prev) noexcept {
    if (m_in[1]) m_count = 0;
    else if (m_in[0] && !prev[0]) m_count = (m_count + 1) % 10;
}

void DecadeCounter::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    for (int k = 0; k < 4; ++k) stampOutput(sys, 4 + static_cast<std::size_t>(k), m_count == k);
}

double DecadeCounter::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    double ip = 0.0, im = 0.0;
    for (int k = 0; k < 4; ++k) {
        const bool h = m_count == k;
        const double i = outputCurrent(x, 4 + static_cast<std::size_t>(k), h);
        if (t == 4 + static_cast<std::size_t>(k)) return -i;
        (h ? ip : im) += i;
    }
    if (t == 0) return ip;
    if (t == 1) return im;
    return 0.0;
}

ComponentReading DecadeCounter::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    r.voltV = rel(x, 0);
    r.level = m_count == 0 ? 1.0 : 0.0;
    r.displayValue = m_count;
    return r;
}

ComponentState DecadeCounter::saveState() const noexcept {
    ComponentState s = ComponentModel::saveState();
    s.internal[0] = m_in[0] ? 1.0 : 0.0;
    s.internal[1] = m_in[1] ? 1.0 : 0.0;
    s.internal[2] = m_count;
    return s;
}

void DecadeCounter::restoreState(const ComponentState& state) noexcept {
    ComponentModel::restoreState(state);
    m_in = {state.internal[0] > 0.5, state.internal[1] > 0.5, false, false};
    m_count = std::clamp(static_cast<int>(std::lround(state.internal[2])), 0, 9);
}

DecadeOutput::DecadeOutput(std::string id, NodeId plus, NodeId minus, NodeId out, const DecadeCounter& counter,
                           int index)
    : ComponentModel(std::move(id), {plus, minus, out}), m_counter(&counter), m_index(index) {}

void DecadeOutput::beginSolve(const StampContext& ctx) noexcept {
    const double u = ctx.accepted.voltage(terminal(0)) - ctx.accepted.voltage(terminal(1));
    m_powered = !m_destroyed && !m_counter->isDestroyed() && u >= m_counter->params().minSupplyV;
    m_high = m_counter->count() == m_index;
}

void DecadeOutput::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    if (!m_powered) return;
    sys.stampConductance(terminal(2), m_high ? terminal(0) : terminal(1), 1.0 / m_counter->params().outOhm);
}

double DecadeOutput::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    if (!m_powered) return 0.0;
    const double i = (x.voltage(terminal(m_high ? 0 : 1)) - x.voltage(terminal(2))) / m_counter->params().outOhm;
    if (t == 2) return -i;
    if (t == 0) return m_high ? i : 0.0;
    if (t == 1) return m_high ? 0.0 : i;
    return 0.0;
}

ComponentReading DecadeOutput::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    r.voltV = x.voltage(terminal(0)) - x.voltage(terminal(1));
    r.level = m_high ? 1.0 : 0.0;
    return r;
}

// --- Anzeige-Modul ------------------------------------------------------------------------------------------------

SevenSegmentModule::SevenSegmentModule(std::string id, NodeId plus, NodeId minus, std::array<NodeId, 4> bcd,
                                       DisplayModuleParams display, LogicModuleParams params)
    : LogicModelBase(std::move(id), {plus, minus, bcd[0], bcd[1], bcd[2], bcd[3]}, 2, 4, params),
      m_display(display) {
    if (!(m_display.segmentOhm > 0.0)) m_display.segmentOhm = 680.0;
    decode();
}

void SevenSegmentModule::decode() noexcept {
    const int v = (m_in[0] ? 1 : 0) + (m_in[1] ? 2 : 0) + (m_in[2] ? 4 : 0) + (m_in[3] ? 8 : 0);
    m_digit = v <= 9 ? v : -1; // wie 4511: Pseudotetraden dunkel
    m_segments = sevenSegmentPattern(m_digit);
}

void SevenSegmentModule::onInputs(const std::array<bool, 4>& /*prev*/) noexcept { decode(); }

void SevenSegmentModule::beginSolve(const StampContext& ctx) noexcept {
    LogicModelBase::beginSolve(ctx);
    const double u = rel(ctx.accepted, 0);
    m_segmentA = m_powered ? std::max(0.0, u - m_display.segmentV) / m_display.segmentOhm : 0.0;
}

void SevenSegmentModule::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    const double i = m_segmentA * popcount7(m_segments);
    if (i > 0.0) sys.stampCurrentSource(terminal(0), terminal(1), i);
}

double SevenSegmentModule::terminalCurrent(const SolutionView& /*x*/, std::size_t t) const noexcept {
    const double i = m_segmentA * popcount7(m_segments);
    if (t == 0) return i;
    if (t == 1) return -i;
    return 0.0;
}

ComponentReading SevenSegmentModule::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    const bool lit = m_powered && m_segmentA > 0.0;
    r.voltV = rel(x, 0);
    r.currentA = m_segmentA * popcount7(m_segments);
    r.displayValue = lit ? m_digit : -1;
    r.displayFlags = lit ? m_segments : 0;
    r.level = lit ? popcount7(m_segments) / 7.0 : 0.0;
    return r;
}

ComponentState SevenSegmentModule::saveState() const noexcept {
    ComponentState s = ComponentModel::saveState();
    s.internal[0] = (m_in[0] ? 1 : 0) + (m_in[1] ? 2 : 0) + (m_in[2] ? 4 : 0) + (m_in[3] ? 8 : 0);
    return s;
}

void SevenSegmentModule::restoreState(const ComponentState& state) noexcept {
    ComponentModel::restoreState(state);
    const int v = std::clamp(static_cast<int>(std::lround(state.internal[0])), 0, 15);
    m_in = {(v & 1) != 0, (v & 2) != 0, (v & 4) != 0, (v & 8) != 0};
    decode();
}

// --- Module -------------------------------------------------------------------------------------------------------

SubcircuitRange addBinaryCounterModule(Circuit& circuit, const std::string& prefix, const CounterModulePins& p) {
    SubcircuitRange range;
    range.first = circuit.componentCount();
    circuit.add<BinaryCounter>(prefix + ".Z", p.plus, p.minus, p.clock, p.reset,
                               std::array<NodeId, 4>{p.q[0], p.q[1], p.q[2], p.q[3]});
    range.count = circuit.componentCount() - range.first;
    return range;
}

SubcircuitRange addDecadeCounterModule(Circuit& circuit, const std::string& prefix, const CounterModulePins& p) {
    SubcircuitRange range;
    range.first = circuit.componentCount();
    auto& counter = circuit.add<DecadeCounter>(prefix + ".Z", p.plus, p.minus, p.clock, p.reset,
                                               std::array<NodeId, 4>{p.q[0], p.q[1], p.q[2], p.q[3]});
    for (int k = 4; k < 10; ++k)
        circuit.add<DecadeOutput>(prefix + ".Q" + std::to_string(k), p.plus, p.minus,
                                  p.q[static_cast<std::size_t>(k)], counter, k);
    range.count = circuit.componentCount() - range.first;
    return range;
}

SubcircuitRange addDisplayModule(Circuit& circuit, const std::string& prefix, NodeId plus, NodeId minus,
                                 const std::array<NodeId, 4>& bcd) {
    SubcircuitRange range;
    range.first = circuit.componentCount();
    circuit.add<SevenSegmentModule>(prefix + ".A", plus, minus, bcd);
    range.count = circuit.componentCount() - range.first;
    return range;
}

} // namespace volta::core

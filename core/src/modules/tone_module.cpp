#include "volta/core/modules/tone_module.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace volta::core {

ToneModule::ToneModule(std::string id, const ToneModulePins& pins, ToneModuleParams params)
    : ComponentModel(std::move(id), {pins.plus, pins.minus, pins.in, pins.out}), m_params(params) {
    auto fix = [](double& v, double fallback) {
        if (!std::isfinite(v) || v <= kMinOhm) v = fallback;
    };
    fix(m_params.supplyOhm, 4700.0);
    fix(m_params.inputOhm, 100000.0);
    fix(m_params.outputOhm, 100.0);
    if (!std::isfinite(m_params.duty) || m_params.duty <= 0.0 || m_params.duty > 1.0) m_params.duty = 0.5;
    if (!std::isfinite(m_params.noteS) || m_params.noteS <= 0.0) m_params.noteS = 0.2;
    if (!(m_params.vOffV < m_params.vOnV)) m_params.vOffV = m_params.vOnV;
}

double ToneModule::outputConductance() const noexcept {
    return m_conducting ? m_params.duty / m_params.outputOhm : kGmin;
}

void ToneModule::beginSolve(const StampContext& /*ctx*/) noexcept {
    m_active = m_pending;
    m_conducting = m_active && kMelodyHz[static_cast<std::size_t>(m_note)] > 0.0;
}

void ToneModule::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    sys.stampConductance(terminal(0), terminal(1), 1.0 / m_params.supplyOhm);
    sys.stampConductance(terminal(2), terminal(1), 1.0 / m_params.inputOhm);
    sys.stampConductance(terminal(3), terminal(1), outputConductance());
}

double ToneModule::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    const double vm = x.voltage(terminal(1));
    const double iSupply = (x.voltage(terminal(0)) - vm) / m_params.supplyOhm;
    const double iIn = (x.voltage(terminal(2)) - vm) / m_params.inputOhm;
    const double iOut = (x.voltage(terminal(3)) - vm) * outputConductance();
    switch (t) {
    case 0: return iSupply;
    case 1: return -(iSupply + iIn + iOut);
    case 2: return iIn;
    default: return iOut;
    }
}

void ToneModule::updateState(const SolutionView& x, double dtS) noexcept {
    const double vm = x.voltage(terminal(1));
    const double supply = x.voltage(terminal(0)) - vm;
    const double in = x.voltage(terminal(2)) - vm;
    const bool powered = supply >= m_params.vSupplyMinV;
    const bool trigger = m_pending ? in >= m_params.vOffV : in >= m_params.vOnV; // Hysterese
    const bool next = powered && trigger;
    if (next && !m_pending) { // Melodie beginnt von vorn
        m_playS = 0.0;
        m_note = 0;
    }
    m_pending = next;
    if (!next || dtS <= 0.0) return;
    m_playS += dtS;
    const double cycleS = m_params.noteS * static_cast<double>(kNotes);
    if (m_playS >= cycleS) m_playS = std::fmod(m_playS, cycleS);
    m_note = std::clamp(static_cast<int>(m_playS / m_params.noteS), 0, static_cast<int>(kNotes) - 1);
}

ComponentReading ToneModule::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    r.currentA = r.terminalCurrentA[3]; // Laststrom am Ausgang
    const double supply = r.voltV;
    r.level = m_active ? std::clamp(supply / 9.0, 0.0, 1.0) : 0.0;
    r.frequencyHz = frequencyHz();
    r.displayValue = static_cast<double>(noteIndex());
    return r;
}

SubcircuitRange addToneModule(Circuit& circuit, const std::string& prefix, const ToneModulePins& pins) {
    SubcircuitRange range;
    range.first = circuit.componentCount();
    circuit.add<ToneModule>(prefix, pins);
    range.count = circuit.componentCount() - range.first;
    return range;
}

} // namespace volta::core

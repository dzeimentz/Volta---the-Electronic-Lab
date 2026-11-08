#include "volta/core/modules/fm_module.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace volta::core {

FmModule::FmModule(std::string id, const FmModulePins& pins, FmModuleParams params)
    : ComponentModel(std::move(id), {pins.plus, pins.minus, pins.ant, pins.tune, pins.volume, pins.out}),
      m_params(params) {
    auto fix = [](double& v, double fallback) {
        if (!std::isfinite(v) || v <= kMinOhm) v = fallback;
    };
    fix(m_params.supplyOhm, 1800.0);
    fix(m_params.inputOhm, 100000.0);
    fix(m_params.antennaOhm, 75.0);
    fix(m_params.outputOhm, 100.0);
    if (!(m_params.maxMHz > m_params.minMHz)) {
        m_params.minMHz = 87.5;
        m_params.maxMHz = 108.0;
    }
    if (!(m_params.edgeMHz > m_params.fullMHz) || !(m_params.fullMHz >= 0.0)) {
        m_params.fullMHz = 0.25;
        m_params.edgeMHz = 0.6;
    }
    m_params.noise = std::clamp(std::isfinite(m_params.noise) ? m_params.noise : 0.3, 0.0, 1.0);
    m_tunedMHz = m_params.minMHz;
}

double FmModule::receptionAt(double mhz, int& station) const noexcept {
    station = -1;
    if (!m_params.antenna) return 0.0;
    double best = 1e9;
    for (std::size_t i = 0; i < kFmStations.size(); ++i) {
        const double d = std::fabs(mhz - kFmStations[i].mhz);
        if (d < best) {
            best = d;
            station = static_cast<int>(i);
        }
    }
    const double q = std::clamp((m_params.edgeMHz - best) / (m_params.edgeMHz - m_params.fullMHz), 0.0, 1.0);
    if (q <= 0.0) station = -1;
    return q;
}

double FmModule::audio() const noexcept {
    double s = 0.0;
    if (m_station >= 0) {
        const FmStation& st = kFmStations[static_cast<std::size_t>(m_station)];
        s = stationSignal(st.program, st.toneHz, m_timeS);
    }
    return std::clamp(m_quality * s + (1.0 - m_quality) * m_params.noise * m_noise, -1.0, 1.0);
}

void FmModule::beginSolve(const StampContext& /*ctx*/) noexcept {
    m_g = m_powered ? std::max(m_volume * (0.5 + 0.45 * audio()) / m_params.outputOhm, kGmin) : kGmin;
}

void FmModule::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    sys.stampConductance(terminal(0), terminal(1), 1.0 / m_params.supplyOhm);
    sys.stampConductance(terminal(2), terminal(1), 1.0 / m_params.antennaOhm);
    sys.stampConductance(terminal(3), terminal(1), 1.0 / m_params.inputOhm);
    sys.stampConductance(terminal(4), terminal(1), 1.0 / m_params.inputOhm);
    sys.stampConductance(terminal(5), terminal(1), m_g);
}

double FmModule::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    const double vm = x.voltage(terminal(1));
    const double i0 = (x.voltage(terminal(0)) - vm) / m_params.supplyOhm;
    const double i2 = (x.voltage(terminal(2)) - vm) / m_params.antennaOhm;
    const double i3 = (x.voltage(terminal(3)) - vm) / m_params.inputOhm;
    const double i4 = (x.voltage(terminal(4)) - vm) / m_params.inputOhm;
    const double i5 = (x.voltage(terminal(5)) - vm) * m_g;
    switch (t) {
    case 0: return i0;
    case 1: return -(i0 + i2 + i3 + i4 + i5);
    case 2: return i2;
    case 3: return i3;
    case 4: return i4;
    default: return i5;
    }
}

void FmModule::updateState(const SolutionView& x, double dtS) noexcept {
    const double vm = x.voltage(terminal(1));
    const double supply = x.voltage(terminal(0)) - vm;
    m_powered = supply >= m_params.vSupplyMinV;
    const double ref = std::max(supply, 1.0);
    const double tune = std::clamp((x.voltage(terminal(3)) - vm) / ref, 0.0, 1.0);
    m_tunedMHz = m_params.minMHz + tune * (m_params.maxMHz - m_params.minMHz);
    m_volume = std::clamp((x.voltage(terminal(4)) - vm) / ref, 0.0, 1.0);
    m_quality = receptionAt(m_tunedMHz, m_station);
    if (dtS > 0.0) {
        m_timeS += dtS;
        if (m_timeS > 3600.0) m_timeS = std::fmod(m_timeS, 3600.0);
        // Rauschen: gleichverteilt -1 ... 1 aus einem einfachen Zufallsgenerator (reproduzierbar)
        m_seed = m_seed * 1664525u + 1013904223u;
        m_noise = static_cast<double>(m_seed >> 8) / static_cast<double>(1u << 24) * 2.0 - 1.0;
    }
}

ComponentReading FmModule::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    r.currentA = r.terminalCurrentA[5];
    r.level = m_powered ? m_quality : 0.0;
    r.displayValue = m_tunedMHz;
    r.displayRange = m_volume;
    r.displayFlags = m_powered ? m_station + 1 : 0;
    return r;
}

ComponentState FmModule::saveState() const noexcept {
    ComponentState s = ComponentModel::saveState();
    s.internal[0] = m_timeS;
    return s;
}

void FmModule::restoreState(const ComponentState& state) noexcept {
    ComponentModel::restoreState(state);
    m_timeS = std::isfinite(state.internal[0]) && state.internal[0] >= 0.0 ? state.internal[0] : 0.0;
}

SubcircuitRange addFmModule(Circuit& circuit, const std::string& prefix, const FmModulePins& pins, bool antenna) {
    SubcircuitRange range;
    range.first = circuit.componentCount();
    FmModuleParams p;
    p.antenna = antenna;
    circuit.add<FmModule>(prefix, pins, p);
    range.count = circuit.componentCount() - range.first;
    return range;
}

} // namespace volta::core

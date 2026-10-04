#include "volta/core/components/meter.hpp"

#include <cmath>
#include <utility>

namespace volta::core {

namespace {
constexpr double kOpenCurrentA = 1e-9; // darunter gilt der Ohm-Messkreis als offen
} // namespace

Meter::Meter(std::string id, NodeId com, NodeId vOhm, NodeId amp, MeterParams params)
    : ComponentModel(std::move(id), {com, vOhm, amp}), m_params(params) {
    const MeterParams def;
    auto fix = [](double& v, double fallback, double minimum) {
        if (!std::isfinite(v) || v < minimum) v = fallback;
    };
    fix(m_params.digitalInputOhm, def.digitalInputOhm, 1.0);
    fix(m_params.analogOhmPerVolt, def.analogOhmPerVolt, 1.0);
    fix(m_params.shuntOhm, def.shuntOhm, kMinOhm);
    fix(m_params.fuseA, def.fuseA, 1e-3);
    fix(m_params.fuseTimeS, def.fuseTimeS, 0.0);
    fix(m_params.ohmSourceV, def.ohmSourceV, 0.01);
    fix(m_params.ohmSourceOhm, def.ohmSourceOhm, 1.0);
    fix(m_params.ohmMaxOhm, def.ohmMaxOhm, 1.0);
    for (double& r : m_params.analogRangesV) fix(r, 30.0, 0.01);
    m_rangeV = m_params.analogRangesV.back();
    m_inputOhm = m_params.digitalInputOhm;
}

void Meter::beginSolve(const StampContext& /*ctx*/) noexcept {
    if (!m_analog) {
        m_inputOhm = m_params.digitalInputOhm;
        return;
    }
    // Selbsttaetige Bereichswahl: kleinster Bereich, in dem der letzte Messwert unter 95 % des Endwerts liegt
    const double v = std::fabs(m_lastVoltV);
    m_rangeV = m_params.analogRangesV.back();
    for (double r : m_params.analogRangesV) {
        if (v <= 0.95 * r) {
            m_rangeV = r;
            break;
        }
    }
    m_inputOhm = m_params.analogOhmPerVolt * m_rangeV;
}

void Meter::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    sys.stampConductance(amp(), com(), shuntConductance());
    if (ohmActive()) {
        const double g = 1.0 / m_params.ohmSourceOhm;
        sys.stampConductance(vo(), com(), g);
        sys.stampCurrentSource(com(), vo(), m_params.ohmSourceV * g); // Messquelle treibt Strom aus V/Ohm heraus
    } else {
        sys.stampConductance(vo(), com(), 1.0 / m_inputOhm);
    }
}

double Meter::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    const double vCom = x.voltage(com());
    const double iAmp = (x.voltage(amp()) - vCom) * shuntConductance(); // in den A-Eingang hinein
    double iVo = 0.0;                                                     // in den V/Ohm-Eingang hinein
    if (ohmActive()) {
        const double g = 1.0 / m_params.ohmSourceOhm;
        iVo = (x.voltage(vo()) - vCom) * g - m_params.ohmSourceV * g;
    } else {
        iVo = (x.voltage(vo()) - vCom) / m_inputOhm;
    }
    if (t == 1) return iVo;
    if (t == 2) return iAmp;
    return -(iVo + iAmp);
}

void Meter::thermal(const SolutionView& x, double dtS) noexcept {
    m_lastDtS = dtS;
    m_lastVoltV = x.voltage(vo()) - x.voltage(com());
    m_lastAmpA = terminalCurrent(x, 2);
}

void Meter::checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept {
    if (m_fuseBlown) return;
    const bool over = std::fabs(m_lastAmpA) > m_params.fuseA;
    m_fuseS = over ? m_fuseS + m_lastDtS : 0.0;
    m_overloaded = over;
    if (over && m_fuseS >= m_params.fuseTimeS - 1e-12) {
        m_fuseBlown = true;
        m_overloaded = false;
        events.push({SimEventType::FuseBlown, index, timeS});
    }
}

ComponentReading Meter::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    const double v = x.voltage(vo()) - x.voltage(com());
    int flags = m_fuseBlown ? kDisplayFuseBlown : 0;
    double value = 0.0;
    switch (m_mode) {
    case MeterMode::Volt:
        value = v;
        if (m_analog && std::fabs(v) > m_params.analogRangesV.back()) flags |= kDisplayOverrange;
        break;
    case MeterMode::Ampere:
        value = terminalCurrent(x, 2);
        break;
    case MeterMode::Ohm: {
        const double iOut = -terminalCurrent(x, 1); // Messstrom aus V/Ohm in die Schaltung
        if (v > m_params.ohmSourceV * 1.001 || v < -1e-3 || iOut < -kOpenCurrentA) {
            flags |= kDisplayForeignVoltage; // fremde Spannung treibt gegen die Messquelle
        } else if (iOut <= kOpenCurrentA || v / iOut > m_params.ohmMaxOhm) {
            flags |= kDisplayOverrange; // offener Kreis oder ueber dem groessten Bereich
        } else {
            value = v / iOut;
        }
        break;
    }
    }
    r.displayValue = value;
    r.displayFlags = flags;
    r.level = m_analog ? value / (m_mode == MeterMode::Volt ? m_rangeV : 1.0) : 0.0; // Zeigerausschlag (V)
    return r;
}

ComponentState Meter::saveState() const noexcept {
    ComponentState s = ComponentModel::saveState();
    s.internal[0] = m_fuseBlown ? 1.0 : 0.0;
    s.internal[1] = m_fuseS;
    return s;
}

void Meter::restoreState(const ComponentState& state) noexcept {
    ComponentModel::restoreState(state);
    m_fuseBlown = state.internal[0] != 0.0;
    m_fuseS = std::isfinite(state.internal[1]) ? state.internal[1] : 0.0;
}

} // namespace volta::core

#include "volta/core/components/buzzer.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace volta::core {

DiodeParams Buzzer::electricalParams(const BuzzerParams& p) noexcept {
    DiodeParams d = DiodeParams::silicon(); // Verpolschutz-Diode des Tongebers
    d.rsOhm = std::isfinite(p.resistanceOhm) && p.resistanceOhm > kMinOhm ? p.resistanceOhm : 330.0;
    d.ufV = 0.70 + d.ifRefA * d.rsOhm; // 0,70 V am Uebergang beim Bezugsstrom plus Abfall an der Ersatzlast
    d.imaxA = 0.05;
    return d;
}

Buzzer::Buzzer(std::string id, NodeId plus, NodeId minus, BuzzerParams params)
    : Diode(std::move(id), plus, minus, electricalParams(params)), m_buzzer(params) {
    if (!std::isfinite(m_buzzer.frequencyHz) || m_buzzer.frequencyHz < 0.0) m_buzzer.frequencyHz = 2400.0;
    if (!std::isfinite(m_buzzer.vMinV) || m_buzzer.vMinV < 0.0) m_buzzer.vMinV = 1.5;
    if (!std::isfinite(m_buzzer.vFullV) || m_buzzer.vFullV <= m_buzzer.vMinV) m_buzzer.vFullV = m_buzzer.vMinV + 1.0;
}

ComponentReading Buzzer::read(const SolutionView& x) const noexcept {
    ComponentReading r = Diode::read(x);
    const double loud = (r.voltV - m_buzzer.vMinV) / (m_buzzer.vFullV - m_buzzer.vMinV);
    r.level = r.destroyed ? 0.0 : std::clamp(loud, 0.0, 1.0);
    r.frequencyHz = r.level > 0.0 ? m_buzzer.frequencyHz : 0.0;
    return r;
}

} // namespace volta::core

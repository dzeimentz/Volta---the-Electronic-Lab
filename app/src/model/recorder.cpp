#include "recorder.hpp"

#include <algorithm>
#include <cmath>

namespace volta::model {

void Recorder::setWindow(double seconds) {
    if (std::isfinite(seconds) && seconds > 0.0) m_windowS = std::clamp(seconds, 0.5, 120.0);
}

void Recorder::setChannel(std::size_t ch, const std::string& source) {
    if (ch < kChannels) m_channels[ch].source = source;
}

bool Recorder::active() const {
    return std::any_of(m_channels.begin(), m_channels.end(), [](const Channel& c) { return !c.source.empty(); });
}

void Recorder::clear() {
    m_samples.clear();
}

void Recorder::push(double simTimeS, const std::array<double, kChannels>& v, const std::array<bool, kChannels>& has) {
    if (!std::isfinite(simTimeS)) return;
    if (m_lastRaw >= 0.0 && simTimeS < m_lastRaw) m_offsetS = m_lastT - simTimeS; // neue Netzliste: weiterzaehlen
    if (m_lastRaw >= 0.0 && simTimeS == m_lastRaw) return;                         // kein Fortschritt
    m_lastRaw = simTimeS;
    m_lastT = simTimeS + m_offsetS;
    if (m_paused || !active()) return;
    Sample s;
    s.t = m_lastT;
    for (std::size_t c = 0; c < kChannels; ++c) {
        s.has[c] = has[c] && std::isfinite(v[c]) && !m_channels[c].source.empty();
        s.v[c] = s.has[c] ? v[c] : 0.0;
    }
    m_samples.push_back(s);
    // nur das groesste Fenster (plus Reserve) aufheben
    const double keep = std::max(m_windowS, 30.0) * 1.1;
    while (!m_samples.empty() && m_samples.front().t < m_lastT - keep) m_samples.pop_front();
}

Recorder::Series Recorder::series(std::size_t ch, std::size_t maxPoints) const {
    Series out;
    if (ch >= kChannels || m_samples.empty() || maxPoints == 0) return out;
    const double end = m_samples.back().t;
    const double start = end - m_windowS;
    // Punkte im Fenster sammeln, gleichmaessig ausduennen (jeden k-ten), letzter Punkt immer dabei
    std::vector<const Sample*> in;
    for (const auto& s : m_samples)
        if (s.t >= start && s.has[ch]) in.push_back(&s);
    if (in.empty()) return out;
    const std::size_t step = std::max<std::size_t>(1, (in.size() + maxPoints - 1) / maxPoints);
    for (std::size_t i = 0; i < in.size(); i += step) out.points.emplace_back(in[i]->t - start, in[i]->v[ch]);
    if ((in.size() - 1) % step != 0) out.points.emplace_back(in.back()->t - start, in.back()->v[ch]);
    out.min = out.max = in.front()->v[ch];
    for (const Sample* s : in) {
        out.min = std::min(out.min, s->v[ch]);
        out.max = std::max(out.max, s->v[ch]);
    }
    out.any = true;
    return out;
}

} // namespace volta::model

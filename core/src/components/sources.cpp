#include "volta/core/components/sources.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace volta::core {

namespace {
constexpr double kPi = 3.14159265358979323846;

double wrapPhase(double p) noexcept {
    p -= std::floor(p);
    return std::isfinite(p) ? p : 0.0;
}

double finiteOr(double v, double fallback) noexcept { return std::isfinite(v) ? v : fallback; }
} // namespace

// ---------------------------------------------------------------- Wechselquelle

AcSource::AcSource(std::string id, NodeId a, NodeId b, AcSourceParams params)
    : ComponentModel(std::move(id), {a, b}), m_params(params) {
    const AcSourceParams def;
    setRms(m_params.rmsV);
    if (!(m_params.frequencyHz > 0.0) || !std::isfinite(m_params.frequencyHz)) m_params.frequencyHz = def.frequencyHz;
    if (!(m_params.innerOhm >= kMinOhm) || !std::isfinite(m_params.innerOhm)) m_params.innerOhm = def.innerOhm;
    if (!(m_params.ratedRmsA > 0.0) || !std::isfinite(m_params.ratedRmsA)) m_params.ratedRmsA = def.ratedRmsA;
    if (!(m_params.rmsWindowS > 0.0) || !std::isfinite(m_params.rmsWindowS)) m_params.rmsWindowS = def.rmsWindowS;
}

void AcSource::setRms(double rmsV) noexcept { m_params.rmsV = std::isfinite(rmsV) ? std::clamp(rmsV, 0.0, 24.0) : 9.0; }

double AcSource::peakV() const noexcept { return std::sqrt(2.0) * m_params.rmsV; }

double AcSource::emfAt(double phase) const noexcept { return peakV() * std::sin(2.0 * kPi * phase); }

double AcSource::rmsCurrentA() const noexcept { return std::sqrt(std::max(m_meanSqA2, 0.0)); }

void AcSource::stamp(MnaSystem& sys, const StampContext& ctx) const noexcept {
    // Zeitschritt: Wert am Ende des Schritts; Arbeitspunkt: Wert zur aktuellen Phase
    const double p = ctx.isDc() ? m_phase : m_phase + m_params.frequencyHz * ctx.dtS;
    const double emf = m_enabled ? emfAt(p) * ctx.sourceScale : 0.0;
    sys.stampVoltageBranch(terminal(0), terminal(1), firstBranch(), emf, m_params.innerOhm);
}

double AcSource::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    const double i = x.branchCurrent(firstBranch());
    return t == 0 ? i : -i;
}

void AcSource::updateState(const SolutionView& x, double dtS) noexcept {
    if (dtS <= 0.0) return;
    m_phase = wrapPhase(m_phase + m_params.frequencyHz * dtS);
    const double i = x.branchCurrent(firstBranch());
    if (!std::isfinite(i)) return;
    const double k = 1.0 - std::exp(-dtS / m_params.rmsWindowS);
    m_meanSqA2 += k * (i * i - m_meanSqA2);
}

void AcSource::checkLimits(EventQueue& /*events*/, std::size_t /*index*/, double /*timeS*/) noexcept {
    m_overloaded = rmsCurrentA() > m_params.ratedRmsA;
}

ComponentReading AcSource::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    r.frequencyHz = m_params.frequencyHz;
    r.level = rmsCurrentA() / m_params.ratedRmsA; // Auslastung
    return r;
}

ComponentState AcSource::saveState() const noexcept {
    ComponentState s = ComponentModel::saveState();
    s.internal[0] = m_meanSqA2;
    return s;
}

void AcSource::restoreState(const ComponentState& state) noexcept {
    ComponentModel::restoreState(state);
    m_meanSqA2 = std::max(finiteOr(state.internal[0], 0.0), 0.0);
    m_overloaded = rmsCurrentA() > m_params.ratedRmsA;
}

// ---------------------------------------------------------------- Funktionsgenerator

FunctionGenerator::FunctionGenerator(std::string id, NodeId out, NodeId ground, GeneratorParams params)
    : ComponentModel(std::move(id), {out, ground}), m_params(params) {
    setFrequency(m_params.frequencyHz);
    setAmplitude(m_params.amplitudeV);
    setOffset(m_params.offsetV);
    if (!(m_params.outputOhm >= kMinOhm) || !std::isfinite(m_params.outputOhm)) m_params.outputOhm = 50.0;
}

void FunctionGenerator::setFrequency(double hz) noexcept {
    m_params.frequencyHz =
        std::clamp(finiteOr(hz, 100.0), GeneratorParams::kMinFrequencyHz, GeneratorParams::kMaxFrequencyHz);
}

void FunctionGenerator::setAmplitude(double peakV) noexcept {
    m_params.amplitudeV = std::clamp(finiteOr(peakV, 0.0), 0.0, GeneratorParams::kMaxAmplitudeV);
}

void FunctionGenerator::setOffset(double volt) noexcept {
    m_params.offsetV = std::clamp(finiteOr(volt, 0.0), -GeneratorParams::kMaxOffsetV, GeneratorParams::kMaxOffsetV);
}

double FunctionGenerator::emfAt(double phase) const noexcept {
    const double p = wrapPhase(phase);
    double shape = 0.0;
    switch (m_params.waveform) {
    case Waveform::Sine: shape = std::sin(2.0 * kPi * p); break;
    case Waveform::Triangle: shape = p < 0.25 ? 4.0 * p : p < 0.75 ? 2.0 - 4.0 * p : 4.0 * p - 4.0; break;
    case Waveform::Square: shape = p < 0.5 ? 1.0 : -1.0; break;
    }
    return m_params.offsetV + m_params.amplitudeV * shape;
}

void FunctionGenerator::stamp(MnaSystem& sys, const StampContext& ctx) const noexcept {
    const double p = ctx.isDc() ? m_phase : m_phase + m_params.frequencyHz * ctx.dtS;
    sys.stampVoltageBranch(terminal(0), terminal(1), firstBranch(), emfAt(p) * ctx.sourceScale, m_params.outputOhm);
}

double FunctionGenerator::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    const double i = x.branchCurrent(firstBranch());
    return t == 0 ? i : -i;
}

void FunctionGenerator::updateState(const SolutionView& /*x*/, double dtS) noexcept {
    if (dtS > 0.0) m_phase = wrapPhase(m_phase + m_params.frequencyHz * dtS);
}

ComponentReading FunctionGenerator::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    r.frequencyHz = m_params.frequencyHz;
    return r;
}

// ---------------------------------------------------------------- Kennlinienschreiber

CurveTracer::CurveTracer(std::string id, NodeId plus, NodeId minus, TracerParams params)
    : ComponentModel(std::move(id), {plus, minus}), m_params(params) {
    setRange(m_params.minV, m_params.maxV);
    setSeriesOhm(m_params.seriesOhm);
    if (!(m_params.periodS > 0.0) || !std::isfinite(m_params.periodS)) m_params.periodS = 2.0;
}

void CurveTracer::setRange(double minV, double maxV) noexcept {
    const double lo = std::clamp(finiteOr(minV, 0.0), -TracerParams::kMaxV, TracerParams::kMaxV);
    const double hi = std::clamp(finiteOr(maxV, 0.0), -TracerParams::kMaxV, TracerParams::kMaxV);
    m_params.minV = std::min(lo, hi);
    m_params.maxV = std::max(lo, hi);
}

void CurveTracer::setSeriesOhm(double ohm) noexcept {
    m_params.seriesOhm = std::clamp(finiteOr(ohm, 500.0), TracerParams::kMinSeriesOhm, TracerParams::kMaxSeriesOhm);
}

double CurveTracer::emfAt(double phase) const noexcept {
    const double p = wrapPhase(phase);
    const double up = p < 0.5 ? 2.0 * p : 2.0 - 2.0 * p; // 0 -> 1 -> 0
    return m_params.minV + (m_params.maxV - m_params.minV) * up;
}

void CurveTracer::stamp(MnaSystem& sys, const StampContext& ctx) const noexcept {
    const double p = ctx.isDc() ? m_phase : m_phase + ctx.dtS / m_params.periodS;
    sys.stampVoltageBranch(terminal(0), terminal(1), firstBranch(), emfAt(p) * ctx.sourceScale, m_params.seriesOhm);
}

double CurveTracer::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    const double i = x.branchCurrent(firstBranch());
    return t == 0 ? i : -i;
}

void CurveTracer::updateState(const SolutionView& /*x*/, double dtS) noexcept {
    if (dtS > 0.0) m_phase = wrapPhase(m_phase + dtS / m_params.periodS);
}

} // namespace volta::core

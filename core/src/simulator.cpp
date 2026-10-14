#include "volta/core/simulator.hpp"

#include <algorithm>
#include <cmath>
#include <new>
#include <utility>

namespace volta::core {

namespace {
constexpr int kSourceRampSteps = 10; // Quellenrampe in 10-%-Stufen
constexpr int kMaxReversals = 4;          // so viele Richtungsumkehrungen gelten als Pendeln
constexpr double kReversalShrink = 0.5;   // Umkehr zaehlt nur, wenn die Korrektur nicht auf die Haelfte schrumpft
} // namespace

Simulator::Simulator() {
    m_events.reserve(kEventCapacity);
    m_probe.nodeVolt.assign(1, 0.0);
}

Status Simulator::setCircuit(Circuit circuit) {
    if (!circuit.isValid()) return Status::InvalidCircuit;

    std::size_t branches = 0;
    NodeId nextInternal = circuit.nodeCount(); // innere Knoten folgen auf die Knoten der Netzliste
    for (std::size_t i = 0; i < circuit.componentCount(); ++i) {
        ComponentModel& c = circuit.component(i);
        c.assignFirstBranch(branches);
        branches += c.branchCount();
        c.assignFirstInternalNode(nextInternal);
        nextInternal += c.internalNodeCount();
    }

    try {
        m_sys.resize(nextInternal, branches);
        const std::size_t n = m_sys.size();
        m_lu.resize(n);
        m_luMatrix.assign(n * n, 0.0);
        m_luValid = false;
        m_accepted.assign(n, 0.0);
        m_iter.assign(n, 0.0);
        m_next.assign(n, 0.0);
        m_backup.assign(n, 0.0);
        m_delta.assign(n, 0.0);
        m_nonlinear.clear();
        for (std::size_t i = 0; i < circuit.componentCount(); ++i)
            if (circuit.component(i).isNonlinear()) m_nonlinear.push_back(i);
        m_probe.nodeVolt.assign(circuit.nodeCount(), 0.0);
        m_probe.readings.assign(circuit.componentCount(), ComponentReading{});
    } catch (const std::bad_alloc&) {
        m_ready = false;
        return Status::OutOfMemory;
    }

    m_circuit = std::move(circuit);
    m_probe.valid = false;
    m_ready = true;
    return solveOperatingPoint();
}

bool Simulator::withinTolerance() const noexcept {
    const std::size_t nv = m_sys.nodeUnknowns();
    for (std::size_t i = 0; i < m_next.size(); ++i) {
        const double absTol = i < nv ? kVoltTolAbs : kCurrentTolAbs;
        const double tol = absTol + kTolRel * std::max(std::fabs(m_next[i]), std::fabs(m_iter[i]));
        if (std::fabs(m_next[i] - m_iter[i]) > tol) return false;
    }
    return true;
}

Status Simulator::newton(const StampContext& ctx, bool warmStart, bool allowEarlyAbort) noexcept {
    m_hitIterationLimit = false;
    const std::size_t count = m_circuit.componentCount();
    for (std::size_t i = 0; i < count; ++i) {
        ComponentModel& c = m_circuit.component(i);
        c.applyPendingState();
        c.beginSolve(ctx);
    }
    if (!warmStart) { // sonst: letzte Naeherung und Linearisierung des gescheiterten Versuchs weiterverwenden
        std::copy(m_accepted.begin(), m_accepted.end(), m_iter.begin());
        for (std::size_t idx : m_nonlinear) m_circuit.component(idx).resetLinearization(ctx.accepted);
    }

    const int maxIter = ctx.isDc() ? kMaxNewtonIter : kMaxNewtonIterTransient;
    int reversals = 0;       // Richtungsumkehrungen ohne Schrumpfen seit der letzten Begrenzung
    double prevNorm2 = 0.0;  // Quadrat der vorigen Korrektur
    double firstNorm2 = -1.0; // Quadrat der ersten Korrektur (Fortschritt fuer den Warmstart)
    std::fill(m_delta.begin(), m_delta.end(), 0.0);
    for (int k = 0; k < maxIter; ++k) {
        if (!ctx.isDc() && m_stepIterations >= kStepIterationBudget) {
            m_budgetExceeded = true;
            return Status::NotConverged;
        }
        ++m_stepIterations;
        m_lastIterations = k + 1;
        bool limited = false;
        const SolutionView iterView = view(m_iter);
        for (std::size_t idx : m_nonlinear) limited = m_circuit.component(idx).linearize(iterView) || limited;

        m_sys.clear();
        m_sys.stampGmin(m_circuit.nodeCount(), kGmin);
        for (std::size_t i = 0; i < count; ++i) m_circuit.component(i).stamp(m_sys, ctx);

        // Lineare Teile und eingeschwungene Modelle stempeln oft dieselbe Matrix: dann nur vorwaerts/rueckwaerts
        // einsetzen (O(n^2) statt O(n^3)). Vergleich exakt, das Ergebnis ist bitgleich zur neuen Zerlegung.
        if (!m_luValid || m_sys.matrix() != m_luMatrix) {
            m_luValid = false;
            if (!m_lu.factor(m_sys.matrix())) return Status::Singular;
            std::copy(m_sys.matrix().begin(), m_sys.matrix().end(), m_luMatrix.begin());
            m_luValid = true;
        }
        if (!m_lu.solve(m_sys.rhs(), m_next)) return Status::Singular;

        if (m_nonlinear.empty()) { // lineares System: eine Loesung ist exakt
            std::swap(m_iter, m_next);
            return Status::Ok;
        }
        const bool converged = !limited && withinTolerance();
        // Pendeln erkennen (nur im Zeitschritt, dort hilft Halbieren): kehrt die Korrektur der Knotenpotentiale
        // wiederholt ihre Richtung um (Skalarprodukt aufeinanderfolgender Korrekturen negativ), ohne zu schrumpfen
        // und ohne dass ein Modell begrenzt, haengt das Verfahren am Kipppunkt einer Rueckkopplung: frueh
        // abbrechen, damit sofort halbiert wird. Monotone oder gedaempft pendelnde Konvergenz bleibt unberuehrt.
        double dot = 0.0;
        double norm2 = 0.0;
        for (std::size_t i = 0; i < m_sys.nodeUnknowns(); ++i) {
            const double delta = m_next[i] - m_iter[i];
            dot += delta * m_delta[i];
            norm2 += delta * delta;
            m_delta[i] = delta;
        }
        std::swap(m_iter, m_next);
        if (converged) return Status::Ok;
        if (firstNorm2 < 0.0) firstNorm2 = norm2;
        const bool reversal = k > 0 && dot < 0.0 && norm2 > kReversalShrink * kReversalShrink * prevNorm2;
        if (limited || ctx.isDc()) {
            reversals = 0;
        } else if (reversal && ++reversals >= kMaxReversals && allowEarlyAbort) {
            return Status::NotConverged;
        }
        prevNorm2 = norm2;
    }
    // Iterationsgrenze erreicht: Warmstart lohnt nur, wenn das Verfahren vorankam (letzte Korrektur kleiner als die
    // erste); sonst startet der halbe Schritt wieder beim angenommenen Zustand.
    m_hitIterationLimit = prevNorm2 < firstNorm2;
    return Status::NotConverged;
}

Status Simulator::solveOperatingPoint() noexcept {
    if (!m_ready) return Status::NoCircuit;

    StampContext ctx;
    ctx.dtS = 0.0;
    ctx.timeS = m_timeS;
    ctx.accepted = view(m_accepted);

    Status s = newton(ctx);
    if (s == Status::NotConverged) {
        // Quellenrampe: Quellen schrittweise von 10 % auf 100 %, jede Stufe startet von der vorigen Loesung.
        std::copy(m_accepted.begin(), m_accepted.end(), m_backup.begin());
        std::fill(m_accepted.begin(), m_accepted.end(), 0.0);
        for (int r = 1; r <= kSourceRampSteps; ++r) {
            ctx.sourceScale = static_cast<double>(r) / kSourceRampSteps;
            s = newton(ctx);
            if (s != Status::Ok) break;
            std::copy(m_iter.begin(), m_iter.end(), m_accepted.begin());
        }
        if (s != Status::Ok) std::copy(m_backup.begin(), m_backup.end(), m_accepted.begin());
    }
    if (s != Status::Ok) {
        for (std::size_t idx : m_nonlinear) m_circuit.component(idx).resetLinearization(view(m_accepted));
        m_events.push({SimEventType::ConvergenceTrouble, kNoComponent, m_timeS});
        return s;
    }
    std::copy(m_iter.begin(), m_iter.end(), m_accepted.begin());
    // Innere Zustaende an den Arbeitspunkt angleichen (z. B. Transistor-Ladungen), ohne Zeitfortschritt
    const SolutionView x = view(m_accepted);
    for (std::size_t i = 0; i < m_circuit.componentCount(); ++i) m_circuit.component(i).updateState(x, 0.0);
    publish();
    return Status::Ok;
}

Status Simulator::step(double dtS) noexcept {
    if (!m_ready) return Status::NoCircuit;
    if (!(dtS > 0.0) || !std::isfinite(dtS)) return Status::InvalidStep;
    m_stepIterations = 0;
    m_stepMaxDepth = 0;
    m_budgetExceeded = false;
    return advance(dtS, 0, false);
}

Status Simulator::advance(double dtS, int depth, bool warmStart) noexcept {
    m_stepMaxDepth = std::max(m_stepMaxDepth, depth);
    StampContext ctx;
    ctx.dtS = dtS;
    ctx.timeS = m_timeS + dtS;
    ctx.accepted = view(m_accepted);

    // Auf der tiefsten Ebene kann nicht mehr halbiert werden: dort die Iterationen ausschoepfen.
    const Status s = newton(ctx, warmStart, depth < kMaxStepHalvings);
    if (s == Status::Ok) {
        accept(dtS);
        return Status::Ok;
    }
    // Halbieren hilft nur bei Nichtkonvergenz, nicht bei einem singulaeren Gleichungssystem oder erschoepftem Budget.
    if (s == Status::Singular || depth >= kMaxStepHalvings || m_budgetExceeded) {
        for (std::size_t idx : m_nonlinear) m_circuit.component(idx).resetLinearization(view(m_accepted));
        m_events.push({SimEventType::ConvergenceTrouble, kNoComponent, m_timeS});
        return s;
    }
    // Scheiterte der Versuch nur an der Iterationsgrenze (Newton kam voran, z. B. Diodenkette nach dem Schliessen
    // eines Schalters), setzt der halbe Schritt bei der letzten Naeherung fort, statt den Fortschritt zu verwerfen.
    const Status first = advance(0.5 * dtS, depth + 1, m_hitIterationLimit);
    if (first != Status::Ok) return first;
    return advance(0.5 * dtS, depth + 1, false);
}

void Simulator::accept(double dtS) noexcept {
    std::copy(m_iter.begin(), m_iter.end(), m_accepted.begin());
    m_timeS += dtS;
    const SolutionView x = view(m_accepted);
    for (std::size_t i = 0; i < m_circuit.componentCount(); ++i) {
        ComponentModel& c = m_circuit.component(i);
        c.updateState(x, dtS);
        c.thermal(x, dtS);
        c.checkLimits(m_events, i, m_timeS);
    }
    publish();
}

void Simulator::publish() noexcept {
    auto finiteOr0 = [](double v) { return std::isfinite(v) ? v : 0.0; };
    const SolutionView x = view(m_accepted);
    m_probe.timeS = m_timeS;
    m_probe.nodeVolt[0] = 0.0;
    for (NodeId n = 1; n < m_probe.nodeVolt.size(); ++n) m_probe.nodeVolt[n] = finiteOr0(x.voltage(n));
    for (std::size_t i = 0; i < m_probe.readings.size(); ++i) {
        ComponentReading r = m_circuit.component(i).read(x);
        r.voltV = finiteOr0(r.voltV);
        r.currentA = finiteOr0(r.currentA);
        r.powerW = finiteOr0(r.powerW);
        r.level = finiteOr0(r.level);
        r.frequencyHz = finiteOr0(r.frequencyHz);
        r.displayValue = finiteOr0(r.displayValue);
        for (double& ti : r.terminalCurrentA) ti = finiteOr0(ti);
        r.tempK = std::isfinite(r.tempK) ? r.tempK : kAmbientK;
        m_probe.readings[i] = r;
    }
    m_probe.valid = true;
}

} // namespace volta::core

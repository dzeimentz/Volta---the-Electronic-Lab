// Wechselstromanalyse (Volta 3, Konzept Kap. 4): Linearisierung am letzten angenommenen Zustand, komplexe Loesung
// je Frequenz. Reelle Kleinsignal-Leitwerte kommen aus den Gleichstrom-Stempeln der Bauteile (rechte Seite entfaellt:
// Gleichquellen wirken als Kurzschluss bzw. Innenwiderstand), speichernde Bauteile und die Anregung stempeln ueber
// ComponentModel::stampAc selbst.
#include <cmath>

#include "volta/core/simulator.hpp"

namespace volta::core {

Status Simulator::acSweep(std::size_t excitation, const std::vector<double>& freqsHz, NodeId outPlus,
                          NodeId outMinus, std::vector<Complex>& result) {
    result.assign(freqsHz.size(), Complex{});
    if (!m_ready) return Status::NoCircuit;
    const std::size_t count = m_circuit.componentCount();
    if (excitation >= count || outPlus >= m_circuit.nodeCount() || outMinus >= m_circuit.nodeCount())
        return Status::InvalidCircuit;

    const std::size_t nodeCount = m_circuit.nodeCount();
    MnaSystem real;
    ComplexMna ac;
    const std::size_t branches = m_sys.branchCount();
    const std::size_t internalNodes = m_sys.nodeUnknowns() + 1; // inkl. innerer Knoten
    real.resize(internalNodes, branches);
    ac.resize(internalNodes, branches);

    // Arbeitspunkt-Kontext am angenommenen Zustand; nichtlineare Modelle dort linearisieren
    StampContext ctx;
    ctx.dtS = 0.0;
    ctx.timeS = m_timeS;
    ctx.accepted = view(m_accepted);
    for (std::size_t i = 0; i < count; ++i) m_circuit.component(i).beginSolve(ctx);
    for (std::size_t idx : m_nonlinear) {
        ComponentModel& c = m_circuit.component(idx);
        c.resetLinearization(ctx.accepted);
        c.linearize(ctx.accepted);
    }

    // Welche Bauteile stempeln selbst? Probe mit w = 1 (nur die Rueckgabe zaehlt).
    std::vector<char> own(count, 0);
    {
        ComplexMna probe;
        probe.resize(internalNodes, branches);
        for (std::size_t i = 0; i < count; ++i) {
            AcContext pc;
            pc.omegaRadS = 1.0;
            pc.excite = (i == excitation);
            own[i] = m_circuit.component(i).stampAc(probe, pc) ? 1 : 0;
        }
    }
    if (!own[excitation]) return Status::InvalidCircuit; // Anregung muss eine wechselstromfaehige Quelle sein

    real.clear();
    real.stampGmin(nodeCount, kGmin);
    for (std::size_t i = 0; i < count; ++i)
        if (!own[i]) m_circuit.component(i).stamp(real, ctx);

    std::vector<Complex> b;
    for (std::size_t f = 0; f < freqsHz.size(); ++f) {
        const double hz = freqsHz[f];
        if (!(hz >= 0.0) || !std::isfinite(hz)) return Status::InvalidStep;
        ac.clear();
        ac.addReal(real.matrix());
        AcContext ac0;
        ac0.omegaRadS = 2.0 * 3.14159265358979323846 * hz;
        for (std::size_t i = 0; i < count; ++i) {
            if (!own[i]) continue;
            ac0.excite = (i == excitation);
            m_circuit.component(i).stampAc(ac, ac0);
        }
        b = ac.rhs();
        if (!solveComplexDense(ac.matrix(), b, ac.size())) return Status::Singular;
        const Complex vp = outPlus == kGround ? Complex{} : b[outPlus - 1];
        const Complex vm = outMinus == kGround ? Complex{} : b[outMinus - 1];
        result[f] = vp - vm;
    }
    return Status::Ok;
}

} // namespace volta::core

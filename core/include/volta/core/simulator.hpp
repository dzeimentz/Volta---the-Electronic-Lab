#pragma once

#include <cstddef>
#include <vector>

#include "volta/core/circuit.hpp"
#include "volta/core/dense_lu.hpp"
#include "volta/core/mna_system.hpp"
#include "volta/core/probe.hpp"
#include "volta/core/sim_event.hpp"
#include "volta/core/types.hpp"

namespace volta::core {

// Simulator: besitzt die Schaltung exklusiv, rechnet Arbeitspunkt und Zeitschritte.
//
// - setCircuit(): Topologiewechsel. Einzige Stelle mit Speicheranforderung; danach Gleichstrom-Arbeitspunkt.
// - step(): ein Zeitschritt (Rueckwaerts-Euler). Allokationsfrei, wirft nie. Bei Nichtkonvergenz wird der
//   Schritt bis auf 1/16 halbiert; scheitert auch das, bleibt der letzte gueltige Zustand erhalten und
//   SimEventType::ConvergenceTrouble wird gemeldet.
// - probe(): Messwerte des letzten angenommenen Zustands, nie NaN/Inf.
//
// - Je step() stehen hoechstens kStepIterationBudget Newton-Iterationen zur Verfuegung (Echtzeit); danach wird
//   wie bei Nichtkonvergenz abgebrochen.
// - Gelingt bei der Halbierung ein Teilschritt und scheitert ein spaeterer, bleiben die angenommenen Teilschritte
//   erhalten (sie sind gueltige Zustaende); die Zeit rueckt nur um diese vor und step() meldet NotConverged.
// - setCircuit() setzt weder die Simulationszeit noch die Ereignis-Queue zurueck: die Zeit laeuft ueber
//   Topologiewechsel hinweg weiter. Schlaegt setCircuit() mit OutOfMemory fehl, ist der Simulator ohne Schaltung.
//
// Nicht threadsicher: die Kopplung an die Oberflaeche (Kopien, lockfreie Queues) uebernimmt die App.
class Simulator {
public:
    static constexpr std::size_t kEventCapacity = 256;

    Simulator();

    Status setCircuit(Circuit circuit);

    // Gleichstrom-Arbeitspunkt, bei Bedarf mit Quellenrampe (Source Stepping). Allokationsfrei.
    Status solveOperatingPoint() noexcept;

    Status step(double dtS = kDefaultStepS) noexcept;

    bool hasCircuit() const noexcept { return m_ready; }
    double timeS() const noexcept { return m_timeS; }
    const Probe& probe() const noexcept { return m_probe; }
    EventQueue& events() noexcept { return m_events; }

    // Zugriff auf die Bauteile, z. B. fuer Bedienhandlungen und Zustandsschnappschuesse (Undo).
    // Strukturelle Aenderungen sind nur ueber setCircuit() erlaubt.
    Circuit& circuit() noexcept { return m_circuit; }
    const Circuit& circuit() const noexcept { return m_circuit; }

    // Newton-Iterationen des letzten Loesungsvorgangs (Diagnose, Leistungsmessung).
    int lastNewtonIterations() const noexcept { return m_lastIterations; }
    // Summe der Newton-Iterationen und groesste Halbierungstiefe des letzten step() (Diagnose, Leistungsmessung).
    int lastStepIterations() const noexcept { return m_stepIterations; }
    int lastStepMaxDepth() const noexcept { return m_stepMaxDepth; }

private:
    // warmStart: von der letzten Naeherung statt vom angenommenen Zustand ausgehen.
    // allowEarlyAbort: bei Pendeln frueh abbrechen (nur sinnvoll, wenn danach noch halbiert werden kann).
    Status newton(const StampContext& ctx, bool warmStart = false, bool allowEarlyAbort = true) noexcept;
    Status advance(double dtS, int depth, bool warmStart) noexcept;
    void accept(double dtS) noexcept;
    void publish() noexcept;
    SolutionView view(const std::vector<double>& x) const noexcept { return {x.data(), m_sys.nodeUnknowns()}; }
    bool withinTolerance() const noexcept;

    Circuit m_circuit;
    MnaSystem m_sys;
    DenseLu m_lu;
    std::vector<double> m_luMatrix; // zuletzt zerlegte Matrix (Zerlegung wiederverwenden, solange gleich)
    bool m_luValid = false;
    std::vector<double> m_accepted; // letzter angenommener Zustand
    std::vector<double> m_iter;     // aktuelle Newton-Naeherung
    std::vector<double> m_next;     // naechste Newton-Naeherung
    std::vector<double> m_backup;   // Sicherung waehrend der Quellenrampe
    std::vector<double> m_delta;    // letzte Newton-Korrektur (Erkennung von Pendeln)
    std::vector<std::size_t> m_nonlinear;
    Probe m_probe;
    EventQueue m_events;
    double m_timeS = 0.0;
    int m_lastIterations = 0;
    int m_stepIterations = 0;
    int m_stepMaxDepth = 0;
    bool m_budgetExceeded = false;
    bool m_hitIterationLimit = false; // letzter Newton-Lauf endete an der Iterationsgrenze mit Fortschritt
    bool m_ready = false;
};

} // namespace volta::core

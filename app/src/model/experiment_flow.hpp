#pragma once

namespace volta::model {

// Forschungskreislauf eines gefuehrten Experiments (Konzept 5.6, docs/ui-spec.md):
// Frage mit Vermutung -> Aufbau und Versuch mit gestuften Hilfen -> Beobachtung (Vergleich mit der Vermutung)
// -> Erklaerung mit Weiterforschen. Ohne Beobachtungsangebot geht es vom Erfolg direkt zur Erklaerung.
// Ohne Oberflaeche testbar.
class ExperimentFlow {
public:
    enum class Phase { None, Question, Build, Observe, Explain };
    // Vergleich der Beobachtung mit der Vermutung (wertet nicht, ermuntert)
    enum class Comparison { None, Matches, Differs, Open };

    // Neues Experiment mit hintCount Hilfen (die letzte ist die Loesung) und observationCount Beobachtungen
    void start(int hintCount, int observationCount = 0) {
        *this = ExperimentFlow{};
        m_phase = Phase::Question;
        m_hintCount = hintCount;
        m_observationCount = observationCount;
    }
    void stop() { *this = ExperimentFlow{}; }

    // Vermutung waehlen (Index); weiter zum Aufbau
    bool chooseGuess(int index) {
        if (m_phase != Phase::Question || index < 0) return false;
        m_guess = index;
        m_phase = Phase::Build;
        return true;
    }
    // Naechste Hilfe aufdecken; false, wenn alle sichtbar sind
    bool nextHint() {
        if (m_phase != Phase::Build || m_hintsShown >= m_hintCount) return false;
        ++m_hintsShown;
        return true;
    }
    // Erfolgsbedingung erreicht (nur waehrend des Aufbaus wirksam)
    bool reportSuccess() {
        if (m_phase != Phase::Build) return false;
        m_phase = m_observationCount > 0 ? Phase::Observe : Phase::Explain;
        return true;
    }
    // Beobachtung waehlen; vergleicht mit der Vermutung: matches = Vermutung passt, open = Vermutung war offen
    // (z. B. »Ich weiss es nicht«)
    bool chooseObservation(int index, bool matches, bool open) {
        if (m_phase != Phase::Observe || index < 0 || index >= m_observationCount) return false;
        m_observation = index;
        m_comparison = open ? Comparison::Open : (matches ? Comparison::Matches : Comparison::Differs);
        m_phase = Phase::Explain;
        return true;
    }

    // Wiederherstellen (automatisches Speichern) nach start(): Werte werden gegen die Anzahl der Hilfen und
    // Beobachtungen geprueft; false und unveraendert, wenn sie nicht passen
    bool restore(Phase phase, int guess, int hintsShown, int observation, Comparison comparison) {
        if (m_phase == Phase::None || phase == Phase::None) return false;
        if (phase != Phase::Question && guess < 0) return false;
        if (hintsShown < 0 || hintsShown > m_hintCount) return false;
        if (phase == Phase::Observe && m_observationCount == 0) return false;
        if (observation >= m_observationCount) return false;
        m_phase = phase;
        m_guess = guess;
        m_hintsShown = hintsShown;
        m_observation = phase == Phase::Explain ? observation : -1;
        m_comparison = phase == Phase::Explain ? comparison : Comparison::None;
        return true;
    }

    Phase phase() const { return m_phase; }
    int guess() const { return m_guess; }
    int observation() const { return m_observation; }
    Comparison comparison() const { return m_comparison; }
    int hintsShown() const { return m_hintsShown; }
    int hintCount() const { return m_hintCount; }
    bool solutionShown() const { return m_hintCount > 0 && m_hintsShown == m_hintCount; }

private:
    Phase m_phase = Phase::None;
    int m_hintCount = 0;
    int m_hintsShown = 0;
    int m_observationCount = 0;
    int m_guess = -1;
    int m_observation = -1;
    Comparison m_comparison = Comparison::None;
};

} // namespace volta::model

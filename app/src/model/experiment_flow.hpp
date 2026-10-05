#pragma once

namespace volta::model {

// Einfacher Ablauf eines gefuehrten Experiments (Phase 1, docs/roadmap.md): Frage mit Vermutung -> Aufbau mit
// gestuften Hilfen -> Erfolg mit Erklaerung. Der volle Forschungskreislauf (Beobachten, Vergleich mit der Vermutung,
// Forscherheft) folgt in Phase 2. Ohne Oberflaeche testbar.
class ExperimentFlow {
public:
    enum class Phase { None, Question, Build, Success };

    // Neues Experiment mit hintCount Hilfen (die letzte ist die Loesung)
    void start(int hintCount) {
        m_phase = Phase::Question;
        m_hintCount = hintCount;
        m_hintsShown = 0;
        m_guess = -1;
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
        m_phase = Phase::Success;
        return true;
    }

    Phase phase() const { return m_phase; }
    int guess() const { return m_guess; }
    int hintsShown() const { return m_hintsShown; }
    int hintCount() const { return m_hintCount; }
    bool solutionShown() const { return m_hintCount > 0 && m_hintsShown == m_hintCount; }

private:
    Phase m_phase = Phase::None;
    int m_hintCount = 0;
    int m_hintsShown = 0;
    int m_guess = -1;
};

} // namespace volta::model

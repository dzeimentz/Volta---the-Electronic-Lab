#pragma once

#include <memory>
#include <string>
#include <vector>

#include "netlist_builder.hpp"
#include "volta/core/simulator.hpp"

namespace volta::model {

// Erfolgsbedingung eines Experiments (docs/content-format.md, Erfolgsbedingungen). Wird ueber den simulierten
// Zustand geprueft, nie ueber den Vergleich mit der Vorlage: jeder funktionierende Aufbau gilt.
struct SuccessCondition {
    enum class Kind { All, Any, Not, Quantity, State };
    // Display: Anzeige des Messgeraets (bauteil "MESS") in V, A bzw. Ohm je nach Messart, Betrag
    enum class Quantity { CurrentMilliA, VoltageV, PowerMilliW, Brightness, Rpm, FrequencyHz, TemperatureC, Display };
    enum class State { Intact, Overloaded, Destroyed };

    Kind kind = Kind::All;
    std::vector<SuccessCondition> children; // All, Any; Not: genau ein Kind
    std::string part;                       // Bauteil-ID (Vorlage) fuer Quantity/State
    std::string field;                      // Anschlusspunkt fuer Quantity::VoltageV am Feld
    Quantity quantity = Quantity::CurrentMilliA;
    State state = State::Intact;
    bool hasMin = false;
    bool hasMax = false;
    double min = 0.0;
    double max = 0.0;
    double holdS = 0.0; // dauer_s: so lange muss die Bedingung ununterbrochen gelten
};

// Wertet eine Erfolgsbedingung Schritt fuer Schritt aus. Haltedauern (dauer_s) werden je Blatt verfolgt.
class SuccessEvaluator {
public:
    explicit SuccessEvaluator(SuccessCondition condition);
    ~SuccessEvaluator();
    SuccessEvaluator(SuccessEvaluator&&) noexcept;
    SuccessEvaluator& operator=(SuccessEvaluator&&) noexcept;

    // Nach jedem Simulationsschritt aufrufen. Liefert true, solange die Bedingung insgesamt erfuellt ist.
    bool update(const core::Simulator& sim, const Netlist& net);
    // Einmal erfuellt (Experiment geschafft) - bleibt true.
    bool reached() const { return m_reached; }
    // Fehler in der Bedingung (unbekanntes Bauteil oder Feld)
    const std::vector<std::string>& errors() const { return m_errors; }
    void reset();

private:
    struct Node;
    bool evaluate(Node& node, const core::Simulator& sim, const Netlist& net);
    static void build(Node& node, const SuccessCondition& c);

    std::unique_ptr<Node> m_root;
    bool m_reached = false;
    std::vector<std::string> m_errors;
};

} // namespace volta::model

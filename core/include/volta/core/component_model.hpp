#pragma once

#include <array>
#include <cstddef>
#include <initializer_list>
#include <string>

#include "volta/core/mna_system.hpp"
#include "volta/core/sim_event.hpp"
#include "volta/core/types.hpp"

namespace volta::core {

// Lesender Zugriff auf einen Loesungsvektor der MNA (Potentiale und Zweigstroeme).
struct SolutionView {
    const double* x = nullptr;
    std::size_t nodeUnknowns = 0;

    double voltage(NodeId node) const noexcept { return node == kGround ? 0.0 : x[node - 1]; }
    double branchCurrent(std::size_t branch) const noexcept { return x[nodeUnknowns + branch]; }
};

// Kontext eines Stempelvorgangs.
struct StampContext {
    double dtS = 0.0;         // Zeitschritt; <= 0 bedeutet Gleichstrom-Arbeitspunkt
    double timeS = 0.0;       // Zeitpunkt am Ende des Schritts
    double sourceScale = 1.0; // Quellenrampe (Source Stepping) beim Arbeitspunkt, sonst 1
    SolutionView accepted;    // letzter angenommener Zustand (nur als Startwert, z. B. fuer Spannungsbegrenzung)

    bool isDc() const noexcept { return dtS <= 0.0; }
};

// Messwerte eines Bauteils fuer die Messschnittstelle (Probe).
struct ComponentReading {
    double voltV = 0.0;    // Spannung zwischen Anschluss 0 und 1
    double currentA = 0.0; // Strom, der am Anschluss 0 in das Bauteil hineinfliesst
    double powerW = 0.0;   // aufgenommene Leistung (negativ bei Quellen, die Leistung abgeben)
    std::array<double, kMaxTerminals> terminalCurrentA{}; // Strom je Anschluss, in das Bauteil hinein
    double tempK = kAmbientK;
    bool destroyed = false;
};

// Zustand eines Bauteils fuer Rueckgaengig/Wiederholen (ADR-0005): intakt/zerstoert, Temperatur und
// modellspezifische innere Groessen (z. B. Kondensatorspannung, Drehzahl).
struct ComponentState {
    bool destroyed = false;
    double tempK = kAmbientK;
    std::array<double, 4> internal{};
};

// Schnittstelle fuer Bauteilmodelle (docs/simulation-core.md).
// Ablauf je Zeitschritt: Newton-Schleife aus linearize() und stamp(); nach Annahme des Schritts
// updateState(), thermal() und checkLimits(). Alle Methoden ausser Konstruktor sind allokationsfrei und noexcept.
//
// Vertraege:
// - Begleitmodelle (Kondensator, Spule, Motor) beziehen ihren Vorzustand ausschliesslich aus dem eigenen,
//   in updateState() fortgeschriebenen Zustand, nie aus StampContext::accepted. Nur so stellt restoreState()
//   (Undo) den Zustand vollstaendig her.
// - terminalCurrent() nichtlinearer Modelle liefert den Strom der zuletzt gestempelten Linearisierung, ausgewertet
//   an x. Damit gilt die Kirchhoffsche Knotenregel exakt im Rahmen der Rechengenauigkeit.
// - Leitwerte ueber kMaxConductance werden in MnaSystem begrenzt; ein Modell begrenzt selbst ebenso, damit
//   terminalCurrent() zum Stempel passt.
// - branchCount() wird nur beim Topologiewechsel gelesen. Wechselt ein Modell die Zahl seiner Zweigstroeme
//   (z. B. Messgeraet V <-> A), ist das ein Topologiewechsel.
// - Bedienparameter (Zellenzahl, Schalterstellung, Reglerstellung) gehoeren nicht zu ComponentState; sie sind
//   Bedienhandlungen und werden von der App gesetzt.
class ComponentModel {
public:
    ComponentModel(std::string id, std::initializer_list<NodeId> terminals);
    virtual ~ComponentModel() = default;

    ComponentModel(const ComponentModel&) = delete;
    ComponentModel& operator=(const ComponentModel&) = delete;

    const std::string& id() const noexcept { return m_id; }
    std::size_t terminalCount() const noexcept { return m_terminalCount; }
    NodeId terminal(std::size_t i) const noexcept { return m_terminals[i]; }

    // Anzahl der Zweigstroeme, die das Modell als Unbekannte benoetigt (Quelle, Spule, Strommessung).
    virtual std::size_t branchCount() const noexcept { return 0; }
    // Vom Simulator beim Topologiewechsel gesetzt: Index des ersten eigenen Zweigstroms.
    void assignFirstBranch(std::size_t branch) noexcept { m_firstBranch = branch; }

    // Anzahl innerer Knoten (z. B. zwischen Bahnwiderstand und pn-Uebergang). Innere Knoten sind nach aussen
    // unsichtbar (nicht in der Probe) und erhalten kein GMIN; das Modell muss sie selbst leitend anbinden.
    virtual std::size_t internalNodeCount() const noexcept { return 0; }
    // Vom Simulator beim Topologiewechsel gesetzt: NodeId des ersten inneren Knotens.
    void assignFirstInternalNode(NodeId node) noexcept { m_firstInternalNode = node; }

    virtual bool isNonlinear() const noexcept { return false; }

    // Nichtlineare Modelle: Linearisierung um die aktuelle Newton-Naeherung, inkl. Spannungsbegrenzung.
    // Rueckgabe true, wenn begrenzt wurde (die Iteration gilt dann noch nicht als konvergiert).
    virtual bool linearize(const SolutionView& /*iterate*/) noexcept { return false; }
    // Nichtlineare Modelle: Linearisierungspunkt auf den letzten angenommenen Zustand zuruecksetzen.
    virtual void resetLinearization(const SolutionView& /*accepted*/) noexcept {}

    // Beitrag in Matrix und rechte Seite.
    virtual void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept = 0;

    // Strom, der am Anschluss t in das Bauteil hineinfliesst (Kirchhoff, Stromflussdarstellung).
    virtual double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept = 0;

    // Nach angenommenem Zeitschritt: innere Zustaende (Ladung, Drehzahl) fortschreiben.
    virtual void updateState(const SolutionView& /*x*/, double /*dtS*/) noexcept {}
    // Thermisches Modell erster Ordnung fortschreiben (ab Thermikmodell, Phase 0 Schritt 3).
    virtual void thermal(const SolutionView& /*x*/, double /*dtS*/) noexcept {}
    // Grenzwerte pruefen und Ereignisse melden.
    virtual void checkLimits(EventQueue& /*events*/, std::size_t /*index*/, double /*timeS*/) noexcept {}

    virtual ComponentReading read(const SolutionView& x) const noexcept;

    virtual ComponentState saveState() const noexcept;
    virtual void restoreState(const ComponentState& state) noexcept;

    bool isDestroyed() const noexcept { return m_destroyed; }
    double tempK() const noexcept { return m_tempK; }

protected:
    std::size_t firstBranch() const noexcept { return m_firstBranch; }
    NodeId internalNode(std::size_t k) const noexcept { return m_firstInternalNode + k; }

    bool m_destroyed = false;
    double m_tempK = kAmbientK;

private:
    std::string m_id;
    std::array<NodeId, kMaxTerminals> m_terminals{};
    std::size_t m_terminalCount = 0;
    std::size_t m_firstBranch = 0;
    NodeId m_firstInternalNode = kGround;
};

} // namespace volta::core

#pragma once

#include <QString>
#include <QUndoStack>

#include <cstdint>
#include <deque>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "board.hpp"
#include "board_edit.hpp"
#include "catalog.hpp"
#include "commands/board_command.hpp"
#include "netlist_builder.hpp"
#include "sim/simulation_bridge.hpp"

namespace volta::model {

// Eine Experimentsitzung ohne Oberflaeche: Aufbau, Netzliste, Simulation (SimulationBridge), Verlauf
// (Rueckgaengig/Wiederholen) und Bedienhandlungen. Die QML-Anbindung (LabController) ist eine duenne Schicht darueber;
// Tests treiben die Sitzung synchron (bridge().runFor).
//
// Jede Aufbauaenderung ist ein Befehl (BoardCommand) mit Zustandsschnappschuss aller Bauteile (ADR-0005).
// Bedienhandlungen (Hauptschalter, Taster, Regler, Zellenwahl) sind keine Befehle.
class LabSession : public BoardDocument {
public:
    explicit LabSession(Catalog catalog);
    ~LabSession() override;

    // Neuer Aufbau (z. B. Vorlage eines Experiments); der Verlauf wird geleert.
    void loadBoard(const Board& board);

    const Board& board() const { return m_board; }
    const Catalog& catalog() const { return m_catalog; }
    const Netlist& netlist() const { return m_net; }
    sim::SimulationBridge& bridge() { return m_bridge; }
    QUndoStack& undoStack() { return m_undo; }
    const std::string& lastError() const { return m_error; }

    // ---- Simulation
    bool poll(); // neuesten Schnappschuss holen; true, wenn neu
    const sim::Snapshot& snapshot() const { return m_snap; }
    bool snapshotCurrent() const { return m_snap.valid && m_snap.topology == m_topology; }
    double voltage(const std::string& anchor) const;
    const core::ComponentReading* reading(const std::string& partId) const;
    // Spannungsanzeige 0-4 Balken mit Hysterese (docs/domain-model.md)
    int voltageBars(const std::string& anchor);

    // ---- Aufbauaenderungen (Befehle). false und lastError() bei ungueltiger Aenderung.
    bool addPart(const std::string& type, const std::string& startField, Orientation o, Span span,
                 std::string* newId = nullptr, double value = 0.0);
    bool movePart(const std::string& id, const std::string& startField, Orientation o, Span span);
    bool removePart(const std::string& id);
    bool flipPart(const std::string& id); // umdrehen (Polung tauschen)
    bool setPartValue(const std::string& id, double value);
    bool repairPart(const std::string& id);
    bool addWire(const std::string& from, const std::string& to, const std::string& color,
                 std::string* newId = nullptr);
    bool moveWireEnd(const std::string& id, bool fromEnd, const std::string& anchor);
    bool removeWire(const std::string& id);
    bool setWireColor(const std::string& id, const std::string& color);
    bool toggleVoltageDisplay(const std::string& field);

    // ---- Bedienhandlungen (kein Verlauf)
    void setMainSwitch(bool on);
    void setCells(int cells);
    void setPartClosed(const std::string& id, bool closed); // Schalter, Taster, Reedkontakt, Wechselschalter
    void setDeskSwitch(bool closed);
    void setPot(int which, double position); // Pult-Drehregler 1/2
    void setMotorBlocked(const std::string& id, bool blocked);

    // Wird nach jeder Aenderung des Aufbaus aufgerufen (Oberflaeche aktualisieren)
    std::function<void()> onBoardChanged;

    // BoardDocument
    void applyBoard(const Board& board, const std::vector<sim::StateOverride>& states) override;

private:
    void rebuild(const std::vector<sim::StateOverride>& states);
    std::vector<sim::StateOverride> captureStates() const;
    bool push(const QString& text, Board after, int mergeId = BoardCommand::NoMerge, const QString& key = {},
              std::vector<sim::StateOverride> statesAfter = {});
    bool fail(std::string message);

    Catalog m_catalog;
    Board m_board;
    Netlist m_net;
    sim::SimulationBridge m_bridge;
    sim::Snapshot m_snap;
    std::uint64_t m_topology = 0;
    QUndoStack m_undo;
    std::string m_error;
    std::unordered_map<std::string, int> m_bars;
    // Bauteilkennungen der letzten Topologie-Versionen: Zustaende aus einem Schnappschuss einer aelteren Version
    // (Befehle ohne Simulationsfortschritt dazwischen) bleiben ueber die Kennung zuordenbar.
    std::deque<std::pair<std::uint64_t, std::vector<std::string>>> m_idHistory;
    // Zustandsvorgaben der juengsten Topologien, die noch in keinem Schnappschuss sichtbar sind (Topologie, Vorgaben)
    std::vector<std::pair<std::uint64_t, std::vector<sim::StateOverride>>> m_pendingStates;
};

} // namespace volta::model

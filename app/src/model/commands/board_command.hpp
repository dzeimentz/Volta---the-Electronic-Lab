#pragma once

#include <QString>
#include <QUndoCommand>

#include <vector>

#include "model/board.hpp"
#include "sim/simulation_bridge.hpp"

namespace volta::model {

// Empfaenger von Aufbauaenderungen: setzt einen Aufbau und optional Bauteilzustaende (Undo: Zustand zum Zeitpunkt
// des Befehls). Der Pultzustand (Bedienhandlungen) gehoert nicht zum Verlauf und bleibt unveraendert.
class BoardDocument {
public:
    virtual ~BoardDocument() = default;
    virtual void applyBoard(const Board& board, const std::vector<sim::StateOverride>& states) = 0;
};

// Eine Aufbauaenderung im Verlauf (ADR-0005). Gespeichert werden Aufbau vorher/nachher und die Zustaende aller
// Bauteile vor der Ausfuehrung. Undo stellt Aufbau und Zustand her (die zerstoerte LED ist wieder heil, die
// Kondensatorladung wie damals); Redo stellt den Aufbau her und uebernimmt die aktuellen Zustaende, ausser der
// Befehl legt eigene fest (Reparieren).
class BoardCommand : public QUndoCommand {
public:
    // Zusammenfassen aufeinanderfolgender Befehle derselben Art und desselben Ziels (z. B. Verschieben beim Ziehen)
    enum MergeId { NoMerge = -1, MergeMovePart = 1, MergeMoveWireEnd = 2, MergePartValue = 3 };

    BoardCommand(BoardDocument& doc, const QString& text, Board before, Board after,
                 std::vector<sim::StateOverride> statesBefore, std::vector<sim::StateOverride> statesAfter = {},
                 int mergeId = NoMerge, QString mergeKey = {});

    void undo() override;
    void redo() override;
    int id() const override { return m_mergeId; }
    bool mergeWith(const QUndoCommand* other) override;

    const Board& before() const { return m_before; }
    const Board& after() const { return m_after; }

private:
    BoardDocument& m_doc;
    Board m_before;
    Board m_after;
    std::vector<sim::StateOverride> m_statesBefore;
    std::vector<sim::StateOverride> m_statesAfter;
    int m_mergeId;
    QString m_mergeKey;
};

} // namespace volta::model

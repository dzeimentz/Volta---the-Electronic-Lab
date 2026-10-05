#include "board_command.hpp"

#include <utility>

namespace volta::model {

BoardCommand::BoardCommand(BoardDocument& doc, const QString& text, Board before, Board after,
                           std::vector<sim::StateOverride> statesBefore, std::vector<sim::StateOverride> statesAfter,
                           int mergeId, QString mergeKey)
    : QUndoCommand(text), m_doc(doc), m_before(std::move(before)), m_after(std::move(after)),
      m_statesBefore(std::move(statesBefore)), m_statesAfter(std::move(statesAfter)), m_mergeId(mergeId),
      m_mergeKey(std::move(mergeKey)) {}

void BoardCommand::undo() { m_doc.applyBoard(m_before, m_statesBefore); }

void BoardCommand::redo() { m_doc.applyBoard(m_after, m_statesAfter); }

bool BoardCommand::mergeWith(const QUndoCommand* other) {
    const auto* o = dynamic_cast<const BoardCommand*>(other);
    if (!o || o->m_mergeId != m_mergeId || m_mergeId == NoMerge || o->m_mergeKey != m_mergeKey) return false;
    m_after = o->m_after; // Ausgangszustand (before, statesBefore) bleibt der des ersten Befehls
    return true;
}

} // namespace volta::model

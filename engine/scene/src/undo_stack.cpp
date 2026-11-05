#include "qivforge/scene/undo_stack.h"

namespace qivforge::scene {

void UndoStack::PushAndExecute(std::unique_ptr<Command> cmd) {
    if (!cmd) return;

    cmd->Execute();
    m_undoStack.push_back(std::move(cmd));

    // Clear redo history when a new command is executed
    m_redoStack.clear();

    // Enforce 200-command maximum history cap per Doc 02 §8
    while (m_undoStack.size() > m_maxCapacity) {
        m_undoStack.pop_front();
    }
}

bool UndoStack::Undo() {
    if (m_undoStack.empty()) return false;

    auto cmd = std::move(m_undoStack.back());
    m_undoStack.pop_back();

    cmd->Undo();
    m_redoStack.push_back(std::move(cmd));
    return true;
}

bool UndoStack::Redo() {
    if (m_redoStack.empty()) return false;

    auto cmd = std::move(m_redoStack.back());
    m_redoStack.pop_back();

    cmd->Execute();
    m_undoStack.push_back(std::move(cmd));
    return true;
}

// CreateActorCommand Implementation
void CreateActorCommand::Execute() {
    Actor* actor = m_scene.CreateActor(m_name, m_parentId);
    if (actor) {
        m_createdActorId = actor->GetId();
    }
}

void CreateActorCommand::Undo() {
    if (m_createdActorId != 0) {
        m_scene.DestroyActor(m_createdActorId);
    }
}

// SetTransformCommand Implementation
void SetTransformCommand::Execute() {
    Actor* actor = m_scene.FindActor(m_actorId);
    if (actor) {
        actor->GetTransform().SetPosition(m_newPos.x, m_newPos.y, m_newPos.z);
    }
}

void SetTransformCommand::Undo() {
    Actor* actor = m_scene.FindActor(m_actorId);
    if (actor) {
        actor->GetTransform().SetPosition(m_oldPos.x, m_oldPos.y, m_oldPos.z);
    }
}

} // namespace qivforge::scene

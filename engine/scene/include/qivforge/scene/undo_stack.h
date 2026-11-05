#pragma once

#include "qivforge/scene/scene.h"
#include <memory>
#include <deque>
#include <string>

namespace qivforge::scene {

class Command {
public:
    virtual ~Command() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
    [[nodiscard]] virtual std::string GetName() const = 0;
};

class UndoStack {
public:
    static constexpr size_t MAX_CAPACITY = 200; // Doc 02 §8

    explicit UndoStack(size_t maxCapacity = MAX_CAPACITY) noexcept : m_maxCapacity(maxCapacity) {}

    void PushAndExecute(std::unique_ptr<Command> cmd);
    bool Undo();
    bool Redo();

    [[nodiscard]] bool CanUndo() const noexcept { return !m_undoStack.empty(); }
    [[nodiscard]] bool CanRedo() const noexcept { return !m_redoStack.empty(); }
    [[nodiscard]] size_t UndoCount() const noexcept { return m_undoStack.size(); }
    [[nodiscard]] size_t RedoCount() const noexcept { return m_redoStack.size(); }

    void Clear() noexcept {
        m_undoStack.clear();
        m_redoStack.clear();
    }

private:
    size_t m_maxCapacity;
    std::deque<std::unique_ptr<Command>> m_undoStack;
    std::deque<std::unique_ptr<Command>> m_redoStack;
};

// Concrete Commands
class CreateActorCommand : public Command {
public:
    CreateActorCommand(Scene& scene, std::string name, uint32_t parentId = 0)
        : m_scene(scene), m_name(std::move(name)), m_parentId(parentId), m_createdActorId(0) {}

    void Execute() override;
    void Undo() override;
    [[nodiscard]] std::string GetName() const override { return "Create Actor"; }
    [[nodiscard]] uint32_t GetCreatedActorId() const noexcept { return m_createdActorId; }

private:
    Scene& m_scene;
    std::string m_name;
    uint32_t m_parentId;
    uint32_t m_createdActorId;
};

class SetTransformCommand : public Command {
public:
    SetTransformCommand(Scene& scene, uint32_t actorId, const Vec3& oldPos, const Vec3& newPos)
        : m_scene(scene), m_actorId(actorId), m_oldPos(oldPos), m_newPos(newPos) {}

    void Execute() override;
    void Undo() override;
    [[nodiscard]] std::string GetName() const override { return "Set Transform"; }

private:
    Scene& m_scene;
    uint32_t m_actorId;
    Vec3 m_oldPos;
    Vec3 m_newPos;
};

} // namespace qivforge::scene

#pragma once

#include "qivforge/core/error_codes.h"
#include "qivforge/core/event_ring_buffer.h"
#include "qivforge/core/command_queue.h"
#include <string>
#include <memory>
#include <atomic>

namespace qivforge::render {
    class RenderScene;
}

namespace qivforge::scene {
    class Scene;
    class UndoStack;
}

namespace qivforge::physics {
    class PhysicsWorld;
    class VehicleController;
    class CharacterController;
    class RigidBody;
}

namespace qivforge::core {

enum class EngineState : uint8_t {
    EDITING = 0,
    PLAYING = 1,
    PAUSED = 2
};

struct EngineConfig {
    std::string appName = "QivForge";
    int32_t targetFps = 60;
    bool enableValidationLayers = false;
    bool useVulkan = true;
};

struct EngineStats {
    float fps = 0.0f;
    float frameMs = 0.0f;
    uint32_t drawCalls = 0;
    uint32_t triangleCount = 0;
    size_t memoryBytes = 0;
    size_t activePhysicsBodies = 0;
    size_t sleepingPhysicsBodies = 0;
};

class Engine {
public:
    static Engine& Instance();

    ErrorCode Initialize(const EngineConfig& config);
    ErrorCode Shutdown();

    ErrorCode OnSurfaceCreated(void* nativeWindow);
    ErrorCode OnSurfaceChanged(int32_t width, int32_t height);
    ErrorCode OnSurfaceDestroyed();

    ErrorCode Tick(float deltaTime);

    [[nodiscard]] bool IsInitialized() const noexcept { return m_initialized.load(); }
    [[nodiscard]] bool HasSurface() const noexcept { return m_hasSurface.load(); }
    [[nodiscard]] EngineStats GetStats() const noexcept;

    EventRingBuffer& GetEventRingBuffer() noexcept { return m_ringBuffer; }
    CommandQueue& GetCommandQueue() noexcept { return m_commandQueue; }

    scene::Scene& GetScene() noexcept { return *m_scene; }
    scene::UndoStack& GetUndoStack() noexcept { return *m_undoStack; }
    physics::PhysicsWorld& GetPhysicsWorld() noexcept { return *m_physicsWorld; }
    physics::VehicleController* GetActiveVehicle() noexcept { return m_activeVehicle.get(); }
    physics::CharacterController* GetActiveCharacter() noexcept { return m_activeCharacter.get(); }

    uint32_t CreateActor(const std::string& name, uint32_t parentId = 0);
    bool DestroyActor(uint32_t actorId);
    bool Undo();
    bool Redo();

    // Instant Play Mode & Scene Snapshot (Rule R5 & Doc 02 §16)
    ErrorCode StartPlay();
    ErrorCode PausePlay();
    ErrorCode StopPlay();

    [[nodiscard]] EngineState GetEngineState() const noexcept { return m_state; }
    [[nodiscard]] bool IsPlaying() const noexcept { return m_state == EngineState::PLAYING; }
    [[nodiscard]] bool IsPaused() const noexcept { return m_state == EngineState::PAUSED; }

    // Vehicle control input bridge
    void SetVehicleInput(float throttle, float steer, float brake, bool handbrake = false) noexcept;

    // Non-copyable, non-movable singleton
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;

private:
    Engine();
    ~Engine();

    void SyncPhysicsToScene(float alpha) noexcept;

    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_hasSurface{false};
    EngineConfig m_config;
    EngineStats m_stats;
    int32_t m_width{0};
    int32_t m_height{0};

    EngineState m_state = EngineState::EDITING;
    std::unique_ptr<scene::Scene> m_editSceneSnapshot;

    EventRingBuffer m_ringBuffer;
    CommandQueue m_commandQueue;
    std::unique_ptr<render::RenderScene> m_renderScene;
    std::unique_ptr<scene::Scene> m_scene;
    std::unique_ptr<scene::UndoStack> m_undoStack;
    std::unique_ptr<physics::PhysicsWorld> m_physicsWorld;
    std::unique_ptr<physics::VehicleController> m_activeVehicle;
    std::unique_ptr<physics::CharacterController> m_activeCharacter;
};

} // namespace qivforge::core

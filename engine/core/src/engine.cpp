#include "qivforge/core/engine.h"
#include "qivforge/render/render_device.h"
#include "qivforge/render/render_scene.h"
#include "qivforge/scene/scene.h"
#include "qivforge/scene/undo_stack.h"
#include "qivforge/physics/physics_world.h"
#include "qivforge/physics/vehicle_controller.h"
#include "qivforge/physics/character_controller.h"
#include <chrono>
#include <cstring>

namespace qivforge::core {

Engine::Engine()
    : m_renderScene(std::make_unique<render::RenderScene>()),
      m_scene(std::make_unique<scene::Scene>()),
      m_undoStack(std::make_unique<scene::UndoStack>()),
      m_physicsWorld(std::make_unique<physics::PhysicsWorld>()) {
}

Engine::~Engine() = default;

Engine& Engine::Instance() {
    static Engine s_instance;
    return s_instance;
}

ErrorCode Engine::Initialize(const EngineConfig& config) {
    if (m_initialized.load()) {
        return ErrorCode::ERR_CORE_ALREADY_INITIALIZED;
    }

    m_config = config;
    m_stats = EngineStats{};

    render::RenderDeviceConfig renderConfig;
    renderConfig.backend = config.useVulkan ? render::RenderBackend::VULKAN_1_1 : render::RenderBackend::OPENGL_ES_3_1;
    renderConfig.enableValidation = config.enableValidationLayers;

    ErrorCode renderInitRes = render::RenderDevice::Instance().Initialize(renderConfig);
    if (renderInitRes != ErrorCode::OK && renderInitRes != ErrorCode::ERR_CORE_ALREADY_INITIALIZED) {
        return renderInitRes;
    }

    m_initialized.store(true);
    return ErrorCode::OK;
}

ErrorCode Engine::Shutdown() {
    if (!m_initialized.load()) {
        return ErrorCode::ERR_CORE_NOT_INITIALIZED;
    }

    if (m_state != EngineState::EDITING) {
        StopPlay();
    }

    if (m_hasSurface.load()) {
        OnSurfaceDestroyed();
    }

    render::RenderDevice::Instance().Shutdown();
    m_ringBuffer.Clear();
    m_undoStack->Clear();
    m_scene->Clear();
    m_physicsWorld->Clear();

    m_initialized.store(false);
    return ErrorCode::OK;
}

ErrorCode Engine::OnSurfaceCreated(void* nativeWindow) {
    if (!m_initialized.load()) {
        return ErrorCode::ERR_CORE_NOT_INITIALIZED;
    }
    if (nativeWindow == nullptr) {
        return ErrorCode::ERR_CORE_INVALID_ARGUMENT;
    }

    ErrorCode res = render::RenderDevice::Instance().CreateSwapChain(nativeWindow);
    if (res != ErrorCode::OK) {
        return res;
    }

    m_hasSurface.store(true);

    EngineEvent ev;
    ev.type = EventType::SURFACE_STATE_CHANGED;
    ev.payload.surface.hasSurface = true;
    ev.payload.surface.width = m_width;
    ev.payload.surface.height = m_height;
    m_ringBuffer.Push(ev);

    return ErrorCode::OK;
}

ErrorCode Engine::OnSurfaceChanged(int32_t width, int32_t height) {
    if (!m_initialized.load()) {
        return ErrorCode::ERR_CORE_NOT_INITIALIZED;
    }
    if (width <= 0 || height <= 0) {
        return ErrorCode::ERR_CORE_INVALID_ARGUMENT;
    }

    m_width = width;
    m_height = height;

    ErrorCode res = render::RenderDevice::Instance().Resize(width, height);
    if (res != ErrorCode::OK) {
        return res;
    }

    EngineEvent ev;
    ev.type = EventType::SURFACE_STATE_CHANGED;
    ev.payload.surface.hasSurface = true;
    ev.payload.surface.width = width;
    ev.payload.surface.height = height;
    m_ringBuffer.Push(ev);

    return ErrorCode::OK;
}

ErrorCode Engine::OnSurfaceDestroyed() {
    render::RenderDevice::Instance().DestroySwapChain();
    m_hasSurface.store(false);
    m_width = 0;
    m_height = 0;

    EngineEvent ev;
    ev.type = EventType::SURFACE_STATE_CHANGED;
    ev.payload.surface.hasSurface = false;
    ev.payload.surface.width = 0;
    ev.payload.surface.height = 0;
    m_ringBuffer.Push(ev);

    return ErrorCode::OK;
}

ErrorCode Engine::Tick(float deltaTime) {
    if (!m_initialized.load()) {
        return ErrorCode::ERR_CORE_NOT_INITIALIZED;
    }

    // 1. Drain command queue from Kotlin bridge (Doc 02 §6)
    auto commands = m_commandQueue.Drain();
    for (const auto& cmd : commands) {
        if (cmd.type == CommandType::CREATE_ACTOR) {
            CreateActor(cmd.paramStr, cmd.targetId);
        } else if (cmd.type == CommandType::DESTROY_ACTOR) {
            DestroyActor(cmd.targetId);
        } else if (cmd.type == CommandType::PLAY) {
            StartPlay();
        } else if (cmd.type == CommandType::PAUSE) {
            PausePlay();
        } else if (cmd.type == CommandType::STOP) {
            StopPlay();
        }
    }

    // 2. Physics simulation if playing
    if (m_state == EngineState::PLAYING) {
        if (m_activeVehicle) {
            m_activeVehicle->Update(deltaTime);
        }
        m_physicsWorld->Update(deltaTime);
        SyncPhysicsToScene(m_physicsWorld->GetInterpolationAlpha());

        m_stats.activePhysicsBodies = m_physicsWorld->GetActiveBodyCount();
        m_stats.sleepingPhysicsBodies = m_physicsWorld->GetSleepingBodyCount();
    }

    // 3. Update render scene (animate cube if editing, or synced actors)
    if (m_renderScene) {
        m_renderScene->Update(deltaTime);
    }

    // 4. Render frame if surface is active
    if (render::RenderDevice::Instance().BeginFrame()) {
        if (m_renderScene) {
            m_renderScene->Render();
            m_stats.drawCalls = m_renderScene->GetDrawCallCount();
            m_stats.triangleCount = m_renderScene->GetTriangleCount();
        }
        render::RenderDevice::Instance().EndFrame();
    }

    // 5. Update stats
    if (deltaTime > 0.0f) {
        m_stats.fps = 1.0f / deltaTime;
        m_stats.frameMs = deltaTime * 1000.0f;
    }

    // 6. Emit StatsUpdated event to ring buffer (Doc 02 §6)
    EngineEvent statsEvent;
    statsEvent.type = EventType::STATS_UPDATED;
    statsEvent.payload.stats.fps = m_stats.fps;
    statsEvent.payload.stats.frameMs = m_stats.frameMs;
    statsEvent.payload.stats.drawCalls = m_stats.drawCalls;
    statsEvent.payload.stats.triangleCount = m_stats.triangleCount;
    m_ringBuffer.Push(statsEvent);

    return ErrorCode::OK;
}

uint32_t Engine::CreateActor(const std::string& name, uint32_t parentId) {
    auto cmd = std::make_unique<scene::CreateActorCommand>(*m_scene, name, parentId);
    scene::CreateActorCommand* cmdPtr = cmd.get();
    m_undoStack->PushAndExecute(std::move(cmd));

    uint32_t actorId = cmdPtr->GetCreatedActorId();

    EngineEvent ev;
    ev.type = EventType::HIERARCHY_CHANGED;
    ev.payload.actor.actorId = actorId;
    std::strncpy(ev.payload.actor.name, name.c_str(), sizeof(ev.payload.actor.name) - 1);
    ev.payload.actor.name[sizeof(ev.payload.actor.name) - 1] = '\0';
    m_ringBuffer.Push(ev);

    return actorId;
}

bool Engine::DestroyActor(uint32_t actorId) {
    bool res = m_scene->DestroyActor(actorId);
    if (res) {
        EngineEvent ev;
        ev.type = EventType::HIERARCHY_CHANGED;
        ev.payload.actor.actorId = 0; // 0 signals reload
        m_ringBuffer.Push(ev);
    }
    return res;
}

bool Engine::Undo() {
    bool res = m_undoStack->Undo();
    if (res) {
        EngineEvent ev;
        ev.type = EventType::HIERARCHY_CHANGED;
        ev.payload.actor.actorId = 0;
        m_ringBuffer.Push(ev);
    }
    return res;
}

bool Engine::Redo() {
    bool res = m_undoStack->Redo();
    if (res) {
        EngineEvent ev;
        ev.type = EventType::HIERARCHY_CHANGED;
        ev.payload.actor.actorId = 0;
        m_ringBuffer.Push(ev);
    }
    return res;
}

// Instant Play Mode & Scene Snapshot (Rule R5 & Doc 02 §16)
ErrorCode Engine::StartPlay() {
    if (m_state == EngineState::PLAYING) {
        return ErrorCode::OK;
    }

    if (m_state == EngineState::PAUSED) {
        m_state = EngineState::PLAYING;
        EngineEvent ev;
        ev.type = EventType::PLAY_STATE_CHANGED;
        ev.payload.rawData[0] = static_cast<uint32_t>(m_state);
        m_ringBuffer.Push(ev);
        return ErrorCode::OK;
    }

    // 1. Take deep snapshot clone of scene before simulation (<5ms)
    m_editSceneSnapshot = m_scene->Clone();

    // 2. Initialize physics world bodies from scene actors
    m_physicsWorld->Clear();

    for (uint32_t rootId : m_scene->GetRootActorIds()) {
        scene::Actor* actor = m_scene->FindActor(rootId);
        if (!actor) continue;

        const auto& pos = actor->GetTransform().GetPosition();
        const auto& rot = actor->GetTransform().GetRotation();

        // Check if tractor/vehicle
        if (actor->GetName() == "Tractor" || actor->GetName().find("Vehicle") != std::string::npos) {
            physics::RigidBody* chassis = m_physicsWorld->CreateBody(actor->GetId(), physics::BodyType::DYNAMIC);
            chassis->SetPosition(physics::Vec3{pos.x, pos.y, pos.z});
            chassis->SetRotation(physics::Vec3{rot.x, rot.y, rot.z});
            m_activeVehicle = std::make_unique<physics::VehicleController>(chassis, physics::VehiclePreset::TRACTOR);
        } else {
            // Dynamic prop
            physics::RigidBody* body = m_physicsWorld->CreateBody(actor->GetId(), physics::BodyType::DYNAMIC);
            body->SetPosition(physics::Vec3{pos.x, pos.y, pos.z});
            body->SetRotation(physics::Vec3{rot.x, rot.y, rot.z});
            body->SetColliderSize(physics::Vec3{0.5f, 0.5f, 0.5f});
        }
    }

    m_state = EngineState::PLAYING;

    EngineEvent ev;
    ev.type = EventType::PLAY_STATE_CHANGED;
    ev.payload.rawData[0] = static_cast<uint32_t>(m_state);
    m_ringBuffer.Push(ev);

    return ErrorCode::OK;
}

ErrorCode Engine::PausePlay() {
    if (m_state != EngineState::PLAYING) {
        return ErrorCode::OK;
    }

    m_state = EngineState::PAUSED;

    EngineEvent ev;
    ev.type = EventType::PLAY_STATE_CHANGED;
    ev.payload.rawData[0] = static_cast<uint32_t>(m_state);
    m_ringBuffer.Push(ev);

    return ErrorCode::OK;
}

ErrorCode Engine::StopPlay() {
    if (m_state == EngineState::EDITING) {
        return ErrorCode::OK;
    }

    // Restore initial scene snapshot cleanly (<5ms) per Rule R5
    if (m_editSceneSnapshot) {
        m_scene = std::move(m_editSceneSnapshot);
        m_editSceneSnapshot.reset();
    }

    m_physicsWorld->Clear();
    m_activeVehicle.reset();
    m_activeCharacter.reset();

    m_state = EngineState::EDITING;

    EngineEvent playEv;
    playEv.type = EventType::PLAY_STATE_CHANGED;
    playEv.payload.rawData[0] = static_cast<uint32_t>(m_state);
    m_ringBuffer.Push(playEv);

    EngineEvent hierEv;
    hierEv.type = EventType::HIERARCHY_CHANGED;
    hierEv.payload.actor.actorId = 0; // Trigger reload
    m_ringBuffer.Push(hierEv);

    return ErrorCode::OK;
}

void Engine::SetVehicleInput(float throttle, float steer, float brake, bool handbrake) noexcept {
    if (m_activeVehicle) {
        m_activeVehicle->SetThrottle(throttle);
        m_activeVehicle->SetSteer(steer);
        m_activeVehicle->SetBrake(brake);
        m_activeVehicle->SetHandbrake(handbrake);
    }
}

void Engine::SyncPhysicsToScene(float alpha) noexcept {
    for (uint32_t rootId : m_scene->GetRootActorIds()) {
        scene::Actor* actor = m_scene->FindActor(rootId);
        if (!actor) continue;

        if (const physics::RigidBody* body = m_physicsWorld->GetBody(actor->GetId())) {
            physics::Vec3 pos = body->GetInterpolatedPosition(alpha);
            physics::Vec3 rot = body->GetRotation();
            actor->GetTransform().SetPosition(pos.x, pos.y, pos.z);
            actor->GetTransform().SetRotation(rot.x, rot.y, rot.z);
        }
    }
}

EngineStats Engine::GetStats() const noexcept {
    return m_stats;
}

} // namespace qivforge::core

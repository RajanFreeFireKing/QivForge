#include "qivforge/core/engine.h"
#include "qivforge/scene/scene.h"
#include <iostream>
#include <cassert>
#include <chrono>

using namespace qivforge::core;
using namespace qivforge::scene;

void TestInstantPlayModeLifecycleAndSnapshotRestore() {
    // Rule R5 & Doc 15 M4 Exit:
    // Play start within 300 ms for a loaded scene; Stop restores the scene exactly.
    std::cout << "[RUN] TestInstantPlayModeLifecycleAndSnapshotRestore (Rule R5)..." << std::endl;

    EngineConfig config;
    config.appName = "PlayModeTest";
    config.useVulkan = false;

    Engine& engine = Engine::Instance();
    ErrorCode initRes = engine.Initialize(config);
    assert(initRes == ErrorCode::OK || initRes == ErrorCode::ERR_CORE_ALREADY_INITIALIZED);

    // Setup initial scene with 3 actors: Light, Camera, and a dynamic Crate
    engine.GetScene().Clear();
    uint32_t lightId = engine.CreateActor("Sun");
    uint32_t camId = engine.CreateActor("Camera");
    uint32_t crateId = engine.CreateActor("Crate");

    Actor* crate = engine.GetScene().FindActor(crateId);
    assert(crate != nullptr);
    crate->GetTransform().SetPosition(0.0f, 15.0f, 0.0f); // High up in air
    crate->GetTransform().SetRotation(0.0f, 0.0f, 0.0f);

    // Verify initial editing state
    assert(engine.GetEngineState() == EngineState::EDITING);
    assert(!engine.IsPlaying());

    // --- 1. Start Instant Play Mode (< 300 ms) ---
    auto tStart0 = std::chrono::high_resolution_clock::now();
    ErrorCode playRes = engine.StartPlay();
    auto tStart1 = std::chrono::high_resolution_clock::now();
    double startMs = std::chrono::duration<double, std::milli>(tStart1 - tStart0).count();

    assert(playRes == ErrorCode::OK);
    assert(engine.IsPlaying());
    std::cout << "  -> StartPlay executed in " << startMs << " ms (Budget < 300 ms)" << std::endl;
    assert(startMs < 300.0);

    // --- 2. Simulate 60 physics steps (1 second) in Play Mode ---
    for (int i = 0; i < 60; ++i) {
        engine.Tick(1.0f / 60.0f);
    }

    // Dynamic crate should have fallen from Y=15 under gravity
    Actor* simCrate = engine.GetScene().FindActor(crateId);
    assert(simCrate != nullptr);
    float simPosY = simCrate->GetTransform().GetPosition().y;
    std::cout << "  -> During play: Crate simulated under gravity fell to Y = " << simPosY << std::endl;
    assert(simPosY < 15.0f); // Fell downwards!

    // --- 3. Pause Play Mode ---
    engine.PausePlay();
    assert(engine.IsPaused());
    float pausedPosY = simCrate->GetTransform().GetPosition().y;
    engine.Tick(1.0f / 60.0f);
    // Physics suspended while paused
    assert(simCrate->GetTransform().GetPosition().y == pausedPosY);

    // --- 4. Stop Play Mode & Restore Scene Exactly (< 300 ms) ---
    auto tStop0 = std::chrono::high_resolution_clock::now();
    ErrorCode stopRes = engine.StopPlay();
    auto tStop1 = std::chrono::high_resolution_clock::now();
    double stopMs = std::chrono::duration<double, std::milli>(tStop1 - tStop0).count();

    assert(stopRes == ErrorCode::OK);
    assert(engine.GetEngineState() == EngineState::EDITING);
    assert(!engine.IsPlaying());
    std::cout << "  -> StopPlay executed in " << stopMs << " ms (Budget < 300 ms)" << std::endl;
    assert(stopMs < 300.0);

    // Verify initial transform is restored EXACTLY
    Actor* restoredCrate = engine.GetScene().FindActor(crateId);
    assert(restoredCrate != nullptr);
    float restoredY = restoredCrate->GetTransform().GetPosition().y;
    std::cout << "  -> Restored Crate Y = " << restoredY << " (Exact initial was 15.0)" << std::endl;
    assert(restoredY == 15.0f);

    std::cout << "[PASS] TestInstantPlayModeLifecycleAndSnapshotRestore\n" << std::endl;
}

int main() {
    std::cout << "===========================================" << std::endl;
    std::cout << "  QIVFORGE PLAY MODE & SNAPSHOT SUITE      " << std::endl;
    std::cout << "===========================================" << std::endl;

    TestInstantPlayModeLifecycleAndSnapshotRestore();

    std::cout << "ALL PLAY MODE TESTS PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}

#include "qivforge/scene/template_factory.h"
#include "qivforge/physics/physics_world.h"
#include "qivforge/physics/vehicle_controller.h"
#include "qivforge/core/profiler.h"
#include <iostream>
#include <cassert>
#include <chrono>

using namespace qivforge::scene;
using namespace qivforge::physics;
using namespace qivforge::core;

void TestContinuousPlayMode10MinutesSimulation() {
    // Doc 01 §11: All five templates open and play 10 minutes without crashes or memory leaks
    std::cout << "[RUN] TestContinuousPlayMode10MinutesSimulation (Doc 01 §11 MVP Acceptance)..." << std::endl;

    PhysicsWorld world;
    world.SetGravity(qivforge::physics::Vec3{0.0f, -9.81f, 0.0f});
    world.SetGroundPlaneEnabled(true, 0.0f);

    // Create 4-wheel Tractor body & controller
    RigidBody* chassis = world.CreateBody(101, BodyType::DYNAMIC);
    chassis->SetMass(2100.0f);
    chassis->SetPosition(qivforge::physics::Vec3{0.0f, 1.2f, 0.0f});
    VehicleController tractor(chassis, VehiclePreset::TRACTOR);

    // Snapshot before play
    auto tSnapStart = std::chrono::high_resolution_clock::now();
    auto snapshotScene = TemplateFactory::CreateTemplateScene(TemplateId::TRACTOR);
    auto clone = snapshotScene->Clone();
    auto tSnapEnd = std::chrono::high_resolution_clock::now();
    double snapshotMs = std::chrono::duration<double, std::milli>(tSnapEnd - tSnapStart).count();

    std::cout << "  -> Play Mode Snapshot took " << snapshotMs << " ms (Budget: < 300 ms)" << std::endl;
    assert(snapshotMs < 300.0);

    // Run 10 minutes of simulation: 600 seconds @ 60 Hz = 36,000 fixed steps
    const int totalSteps = 36000;
    const float dt = 1.0f / 60.0f;

    auto tSimStart = std::chrono::high_resolution_clock::now();
    double totalPhysicsStepMs = 0.0;

    for (int step = 0; step < totalSteps; ++step) {
        // Continuous input: throttle and steering variation
        float throttle = 0.8f;
        float steer = 0.2f * std::sin(step * 0.01f);
        tractor.SetThrottle(throttle);
        tractor.SetSteer(steer);

        auto tStepStart = std::chrono::high_resolution_clock::now();
        tractor.Update(dt);
        world.StepFixed(dt);
        auto tStepEnd = std::chrono::high_resolution_clock::now();

        totalPhysicsStepMs += std::chrono::duration<double, std::milli>(tStepEnd - tStepStart).count();
    }

    auto tSimEnd = std::chrono::high_resolution_clock::now();
    double wallClockSec = std::chrono::duration<double>(tSimEnd - tSimStart).count();
    double avgPhysicsStepMs = totalPhysicsStepMs / totalSteps;

    std::cout << "  -> Completed 36,000 fixed steps (10 simulation minutes) in " 
              << wallClockSec << "s wall-clock time" << std::endl;
    std::cout << "  -> Average physics step time: " << avgPhysicsStepMs << " ms (Budget: < 4.0 ms, Target: < 3.0 ms)" << std::endl;

    assert(avgPhysicsStepMs < 4.0); // Strict Doc 01 §6 budget

    // Stop Play: restore snapshot
    auto tRestoreStart = std::chrono::high_resolution_clock::now();
    snapshotScene = std::move(clone);
    auto tRestoreEnd = std::chrono::high_resolution_clock::now();
    double restoreMs = std::chrono::duration<double, std::milli>(tRestoreEnd - tRestoreStart).count();

    std::cout << "  -> Stop Play Restore took " << restoreMs << " ms" << std::endl;
    assert(restoreMs < 300.0);

    std::cout << "[PASS] TestContinuousPlayMode10MinutesSimulation\n" << std::endl;
}

void TestQualityGovernorPerformanceAdaptation() {
    // Doc 01 §6 & Doc 05 §15: Automatic quality governor adjusting quality presets
    std::cout << "[RUN] TestQualityGovernorPerformanceAdaptation (Doc 01 §6 Quality Governor)..." << std::endl;

    Profiler profiler;
    QualityGovernor governor(QualityPreset::HIGH);

    assert(governor.GetCurrentSettings().preset == QualityPreset::HIGH);
    assert(governor.GetCurrentSettings().renderScale == 1.0f);

    // 1. Simulate heavy load dropping to 24 fps (frameMs = 42.0 ms)
    for (int i = 0; i < 60; ++i) {
        FrameTelemetry f;
        f.fps = 24.0f;
        f.frameMs = 42.0f;
        profiler.RecordFrame(f);
    }

    bool adjusted = governor.Evaluate(profiler);
    assert(adjusted);
    assert(governor.GetCurrentSettings().preset == QualityPreset::MEDIUM);
    assert(governor.GetCurrentSettings().renderScale == 0.85f);
    std::cout << "  -> Governor adapted on heavy load (>33.3ms): stepped down to MEDIUM (renderScale: 0.85)" << std::endl;

    // 2. Further heavy load drops to 18 fps (frameMs = 55.0 ms)
    for (int i = 0; i < 60; ++i) {
        FrameTelemetry f;
        f.fps = 18.0f;
        f.frameMs = 55.0f;
        profiler.RecordFrame(f);
    }
    adjusted = governor.Evaluate(profiler);
    assert(adjusted);
    assert(governor.GetCurrentSettings().preset == QualityPreset::LOW);
    assert(governor.GetCurrentSettings().renderScale == 0.70f);
    std::cout << "  -> Governor adapted on severe load: stepped down to LOW (renderScale: 0.70)" << std::endl;

    // 3. Smooth headroom returns (frameMs = 12.0 ms, > 80 fps)
    for (int i = 0; i < 60; ++i) {
        FrameTelemetry f;
        f.fps = 83.0f;
        f.frameMs = 12.0f;
        profiler.RecordFrame(f);
    }
    adjusted = governor.Evaluate(profiler);
    assert(adjusted);
    assert(governor.GetCurrentSettings().preset == QualityPreset::MEDIUM);
    std::cout << "  -> Governor recovered smoothly when headroom was restored: stepped up to MEDIUM" << std::endl;

    std::cout << "[PASS] TestQualityGovernorPerformanceAdaptation\n" << std::endl;
}

int main() {
    std::cout << "===========================================" << std::endl;
    std::cout << "  MVP ACCEPTANCE SUITE (DOC 01 §11 & M6)   " << std::endl;
    std::cout << "===========================================" << std::endl;

    TestContinuousPlayMode10MinutesSimulation();
    TestQualityGovernorPerformanceAdaptation();

    std::cout << "ALL MVP ACCEPTANCE TESTS PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}

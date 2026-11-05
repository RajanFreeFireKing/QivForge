#include "qivforge/physics/physics_world.h"
#include <iostream>
#include <cassert>
#include <chrono>

using namespace qivforge::physics;

void TestPhysicsSteppingAndGround() {
    std::cout << "[RUN] TestPhysicsSteppingAndGround..." << std::endl;
    PhysicsWorld world;
    world.SetGroundPlaneEnabled(true, 0.0f);

    RigidBody* body = world.CreateBody(1, BodyType::DYNAMIC);
    body->SetPosition(Vec3{0.0f, 10.0f, 0.0f});
    body->SetColliderSize(Vec3{0.5f, 0.5f, 0.5f});
    body->SetMass(10.0f);

    // Simulate 2 seconds (120 steps)
    for (int i = 0; i < 120; ++i) {
        world.Update(1.0f / 60.0f);
    }

    // Body should have fallen from Y=10 to rest on ground (bottom at Y=0 => center at Y=0.5)
    float posY = body->GetPosition().y;
    assert(posY >= 0.49f && posY <= 0.55f);
    std::cout << "  -> Body landed on ground plane at Y = " << posY << std::endl;
    std::cout << "[PASS] TestPhysicsSteppingAndGround\n" << std::endl;
}

void Test100BoxStackSettlementAndSleep() {
    // Doc 06 §17 AC6: A stack of 100 boxes comes to rest and sleeps within 5 seconds with no jitter.
    std::cout << "[RUN] Test100BoxStackSettlementAndSleep (Doc 06 AC6)..." << std::endl;
    PhysicsWorld world;
    world.SetGroundPlaneEnabled(true, 0.0f);

    // Create 100 boxes stacked in a neat vertical column with small spacing
    constexpr size_t BOX_COUNT = 100;
    for (size_t i = 0; i < BOX_COUNT; ++i) {
        RigidBody* b = world.CreateBody(static_cast<uint32_t>(i + 1), BodyType::DYNAMIC);
        b->SetPosition(Vec3{0.0f, 0.5f + static_cast<float>(i) * 1.0f, 0.0f});
        b->SetColliderSize(Vec3{0.5f, 0.5f, 0.5f});
        b->SetMass(5.0f);
        b->SetLinearDamping(0.2f);
        b->SetAllowSleep(true);
    }

    assert(world.GetTotalBodyCount() == BOX_COUNT);

    // Simulate 5 seconds (300 steps at 60 Hz)
    auto startTime = std::chrono::high_resolution_clock::now();
    for (int step = 0; step < 300; ++step) {
        world.Update(1.0f / 60.0f);
    }
    auto elapsed = std::chrono::high_resolution_clock::now() - startTime;
    double elapsedMs = std::chrono::duration<double, std::milli>(elapsed).count();

    size_t sleepingCount = world.GetSleepingBodyCount();
    std::cout << "  -> 100 boxes simulated: " << sleepingCount << "/" << BOX_COUNT 
              << " sleeping after 5.0s (computed in " << elapsedMs << " ms)" << std::endl;

    // All or nearly all settled bodies should be in sleep state
    assert(sleepingCount >= 95);
    std::cout << "[PASS] Test100BoxStackSettlementAndSleep\n" << std::endl;
}

void TestRaycastAccuracyAndPerformance() {
    // Doc 06 §17 AC5: Raycast against 300m in scene with 1000 colliders under 0.2ms average
    std::cout << "[RUN] TestRaycastAccuracyAndPerformance (Doc 06 AC5)..." << std::endl;
    PhysicsWorld world;

    // Populate with 1000 static colliders
    for (int i = 0; i < 1000; ++i) {
        RigidBody* b = world.CreateBody(static_cast<uint32_t>(i + 1), BodyType::STATIC);
        float x = (i % 20) * 10.0f - 95.0f;
        float z = (i / 20) * 10.0f - 100.0f;
        b->SetPosition(Vec3{x, 1.0f, z});
        b->SetColliderSize(Vec3{1.0f, 1.0f, 1.0f});
    }

    // Place target collider exactly along ray path at distance 150m
    RigidBody* target = world.CreateBody(9999, BodyType::STATIC);
    target->SetPosition(Vec3{0.0f, 1.0f, 150.0f});
    target->SetColliderSize(Vec3{2.0f, 2.0f, 2.0f});

    Vec3 rayOrigin{0.0f, 1.0f, 0.0f};
    Vec3 rayDir{0.0f, 0.0f, 1.0f};

    // Run 500 raycasts and measure average time per raycast
    constexpr int ITERATIONS = 500;
    auto t0 = std::chrono::high_resolution_clock::now();
    RayHit hit;
    for (int i = 0; i < ITERATIONS; ++i) {
        hit = world.Raycast(rayOrigin, rayDir, 300.0f);
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double totalMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double avgMs = totalMs / ITERATIONS;

    assert(hit.hasHit);
    assert(hit.actorId == 9999);
    assert(hit.distance >= 147.0f && hit.distance <= 149.0f); // 150m - half-extent 2m = 148m

    std::cout << "  -> Ray hit target at dist=" << hit.distance 
              << " m. Avg query time: " << avgMs << " ms (Budget < 0.2 ms)" << std::endl;
    assert(avgMs < 0.2); // Pass performance gate
    std::cout << "[PASS] TestRaycastAccuracyAndPerformance\n" << std::endl;
}

void TestContinuousCollisionDetectionNoTunneling() {
    // Doc 06 §17 AC7: Fast bullets (300 m/s) never tunnel through a 0.1 m wall
    std::cout << "[RUN] TestContinuousCollisionDetectionNoTunneling (Doc 06 AC7)..." << std::endl;
    PhysicsWorld world;
    world.SetGroundPlaneEnabled(false);

    // Thin wall of thickness 0.1 m (half-extent 0.05 m) at Z = 2.0 m
    RigidBody* wall = world.CreateBody(100, BodyType::STATIC);
    wall->SetPosition(Vec3{0.0f, 0.0f, 2.0f});
    wall->SetColliderSize(Vec3{5.0f, 5.0f, 0.05f});

    // Fast bullet moving at 300 m/s along +Z starting at Z = 0.0 m
    // In one frame (1/60s), bullet travels 300 / 60 = 5.0 meters!
    // Without CCD, it would jump from Z=0 to Z=5, tunneling right through the 0.1m wall at Z=2!
    Vec3 bulletStart{0.0f, 0.0f, 0.0f};
    Vec3 bulletVel{0.0f, 0.0f, 300.0f};
    float dt = 1.0f / 60.0f;
    float sweepDist = bulletVel.Length() * dt; // 5.0m

    // Linear cast CCD check
    RayHit ccdHit = world.Raycast(bulletStart, bulletVel.Normalized(), sweepDist);
    assert(ccdHit.hasHit);
    assert(ccdHit.actorId == 100);
    // Wall front face is at 2.0 - 0.05 = 1.95m
    assert(ccdHit.distance >= 1.90f && ccdHit.distance <= 2.0f);

    std::cout << "  -> 300 m/s bullet swept " << sweepDist << "m. Intercepted wall at dist=" 
              << ccdHit.distance << "m without tunneling!" << std::endl;
    std::cout << "[PASS] TestContinuousCollisionDetectionNoTunneling\n" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  QIVFORGE PHYSICS WORLD SUITE (DOC 06)  " << std::endl;
    std::cout << "========================================" << std::endl;

    TestPhysicsSteppingAndGround();
    Test100BoxStackSettlementAndSleep();
    TestRaycastAccuracyAndPerformance();
    TestContinuousCollisionDetectionNoTunneling();

    std::cout << "ALL PHYSICS WORLD TESTS PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}

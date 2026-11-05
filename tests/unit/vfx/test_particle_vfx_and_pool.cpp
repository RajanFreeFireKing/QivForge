#include "qivforge/vfx/particle_emitter.h"
#include "qivforge/vfx/vfx_manager.h"
#include <iostream>
#include <cassert>
#include <chrono>

using namespace qivforge::vfx;

void TestMuzzleFlashAndImpactPerformance() {
    std::cout << "[RUN] TestMuzzleFlashAndImpactPerformance (Doc 10 AC1)..." << std::endl;

    VFXManager vfxMgr;

    // Test MuzzleFlash
    auto* mf = vfxMgr.Spawn("MuzzleFlash", 0, 0, 0);
    assert(mf != nullptr);

    auto start = std::chrono::high_resolution_clock::now();
    for (int frame = 0; frame < 200; ++frame) {
        mf->Update(0.016f);
    }
    auto end = std::chrono::high_resolution_clock::now();
    double totalMs = std::chrono::duration<double, std::milli>(end - start).count();
    double avgMs = totalMs / 200.0;

    std::cout << "  MuzzleFlash avg update: " << avgMs << " ms" << std::endl;
    // Doc 10 AC1: under 0.15 ms CPU
    assert(avgMs < 0.15);

    // Test TractorExhaustSmoke
    auto* es = vfxMgr.Spawn("TractorExhaustSmoke", 0, 0, 0);
    assert(es != nullptr);

    start = std::chrono::high_resolution_clock::now();
    for (int frame = 0; frame < 200; ++frame) {
        es->Update(0.016f);
    }
    end = std::chrono::high_resolution_clock::now();
    totalMs = std::chrono::duration<double, std::milli>(end - start).count();
    avgMs = totalMs / 200.0;

    std::cout << "  TractorExhaustSmoke avg update: " << avgMs << " ms" << std::endl;
    assert(avgMs < 0.15);

    std::cout << "[PASS] TestMuzzleFlashAndImpactPerformance" << std::endl;
}

void Test2000ParticlesPerformance() {
    std::cout << "[RUN] Test2000ParticlesPerformance (Doc 10 AC2)..." << std::endl;

    EmitterConfig cfg;
    cfg.name = "StressEmitter";
    cfg.shape = EmitterShape::SPHERE;
    cfg.maxParticles = 2000.0f;
    cfg.emissionRate = 0.0f; // Manually burst to fill all 2,000 slots
    cfg.lifetime = 10.0f;
    cfg.speed = 10.0f;

    ParticleEmitter emitter(cfg);
    emitter.Burst(2000);
    assert(emitter.GetActiveParticleCount() == 2000);

    // Benchmark 2,000 active particles
    auto start = std::chrono::high_resolution_clock::now();
    for (int frame = 0; frame < 500; ++frame) {
        emitter.Update(0.016f);
    }
    auto end = std::chrono::high_resolution_clock::now();
    double totalMs = std::chrono::duration<double, std::milli>(end - start).count();
    double avgMs = totalMs / 500.0;

    std::cout << "  2,000 particles updated in avg " << avgMs << " ms" << std::endl;
    // Frame budget for 60 fps is 16.6 ms; particle CPU simulation should take < 1.0 ms
    assert(avgMs < 1.0);

    std::cout << "[PASS] Test2000ParticlesPerformance" << std::endl;
}

void TestPoolMemoryReuse() {
    std::cout << "[RUN] TestPoolMemoryReuse (Doc 10 AC3)..." << std::endl;

    VFXManager vfxMgr;

    // Spawn 20 muzzle flashes to warm up the pool
    for (int i = 0; i < 20; ++i) {
        vfxMgr.Spawn("MuzzleFlash", static_cast<float>(i), 0, 0);
    }

    // Step simulation so initial bursts finish
    for (int frame = 0; frame < 20; ++frame) {
        vfxMgr.Update(0.016f);
    }

    size_t capacityAfterWarmup = vfxMgr.GetPoolCapacity("MuzzleFlash");
    assert(capacityAfterWarmup > 0 && capacityAfterWarmup <= 20);

    // Now spawn the same effect 1,000 times
    for (int i = 0; i < 1000; ++i) {
        vfxMgr.Spawn("MuzzleFlash", 0, 0, 0);
        vfxMgr.Update(0.05f); // Fast step to finish bursts
    }

    size_t capacityAfter1000 = vfxMgr.GetPoolCapacity("MuzzleFlash");

    std::cout << "  Pool capacity after 20: " << capacityAfterWarmup
              << ", after 1,000 spawns: " << capacityAfter1000 << std::endl;

    // Doc 10 AC3: Spawning 1,000 times causes no memory growth after first 20 spawns
    assert(capacityAfter1000 == capacityAfterWarmup);

    std::cout << "[PASS] TestPoolMemoryReuse" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    PARTICLE VFX & OBJECT POOL (M9)     " << std::endl;
    std::cout << "========================================" << std::endl;

    TestMuzzleFlashAndImpactPerformance();
    Test2000ParticlesPerformance();
    TestPoolMemoryReuse();

    std::cout << "ALL PARTICLE VFX TESTS PASSED!" << std::endl;
    return 0;
}

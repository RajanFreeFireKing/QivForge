#include "qivforge/core/engine.h"
#include "qivforge/core/error_codes.h"
#include <cassert>
#include <iostream>
#include <cmath>

using namespace qivforge::core;

void TestErrorCodes() {
    assert(ErrorCode::OK == ErrorCode{0});
    assert(std::string(GetErrorMessage(ErrorCode::OK)) == "Success");
    assert(std::string(GetErrorMessage(ErrorCode::ERR_CORE_NOT_INITIALIZED)) == "Engine core not initialized");
    assert(std::string(GetErrorMessage(ErrorCode::ERR_RENDER_VULKAN_INIT_FAILED)) == "Vulkan 1.1 initialization failed");
    std::cout << "[PASS] TestErrorCodes" << std::endl;
}

void TestEngineLifecycle() {
    auto& engine = Engine::Instance();
    assert(!engine.IsInitialized());
    assert(!engine.HasSurface());

    // Uninitialized calls should fail cleanly
    assert(engine.Tick(0.016f) == ErrorCode::ERR_CORE_NOT_INITIALIZED);
    assert(engine.Shutdown() == ErrorCode::ERR_CORE_NOT_INITIALIZED);

    // Initialize
    EngineConfig cfg;
    cfg.appName = "QivForgeTest";
    cfg.targetFps = 60;
    cfg.useVulkan = true;

    assert(engine.Initialize(cfg) == ErrorCode::OK);
    assert(engine.IsInitialized());

    // Duplicate initialize should reject
    assert(engine.Initialize(cfg) == ErrorCode::ERR_CORE_ALREADY_INITIALIZED);

    // Surface lifecycle
    int dummyWindow = 42;
    assert(engine.OnSurfaceCreated(nullptr) == ErrorCode::ERR_CORE_INVALID_ARGUMENT);
    assert(engine.OnSurfaceCreated(&dummyWindow) == ErrorCode::OK);
    assert(engine.HasSurface());

    assert(engine.OnSurfaceChanged(0, 1080) == ErrorCode::ERR_CORE_INVALID_ARGUMENT);
    assert(engine.OnSurfaceChanged(1920, 1080) == ErrorCode::OK);

    // Tick test
    assert(engine.Tick(0.016666f) == ErrorCode::OK);
    EngineStats stats = engine.GetStats();
    assert(std::abs(stats.fps - 60.0f) < 1.0f);
    assert(std::abs(stats.frameMs - 16.666f) < 0.1f);

    assert(engine.OnSurfaceDestroyed() == ErrorCode::OK);
    assert(!engine.HasSurface());

    // Shutdown
    assert(engine.Shutdown() == ErrorCode::OK);
    assert(!engine.IsInitialized());

    std::cout << "[PASS] TestEngineLifecycle" << std::endl;
}

int main() {
    std::cout << "Running QivForge Core Sanity Tests..." << std::endl;
    TestErrorCodes();
    TestEngineLifecycle();
    std::cout << "All QivForge Core Sanity Tests Passed Successfully!" << std::endl;
    return 0;
}

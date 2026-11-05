#include "qivforge/core/engine.h"
#include "qivforge/render/render_device.h"
#include "qivforge/render/render_scene.h"
#include "qivforge/render/mesh_primitives.h"
#include <cassert>
#include <iostream>

using namespace qivforge::core;
using namespace qivforge::render;

void TestCubeGeometry() {
    MeshData cube = MeshPrimitives::CreateCube(0.5f);
    assert(cube.vertices.size() == 24);
    assert(cube.indices.size() == 36);

    // 36 indices / 3 = 12 triangles
    assert((cube.indices.size() / 3) == 12);

    // Verify all indices are within vertex range
    for (uint16_t idx : cube.indices) {
        assert(idx < cube.vertices.size());
    }

    std::cout << "[PASS] TestCubeGeometry (24 vertices, 36 indices, 12 triangles)" << std::endl;
}

void Test50CycleSurfaceLifecycleStress() {
    // Doc 02 §16 AC3: "Rotating the phone or backgrounding the app does not crash or leak the swap chain"
    auto& device = RenderDevice::Instance();
    RenderDeviceConfig cfg;
    cfg.backend = RenderBackend::VULKAN_1_1;
    assert(device.Initialize(cfg) == ErrorCode::OK);

    int dummySurface = 1001;

    for (int cycle = 1; cycle <= 50; ++cycle) {
        assert(device.CreateSwapChain(&dummySurface) == ErrorCode::OK);
        assert(device.HasSwapChain());

        assert(device.Resize(1080, 2400) == ErrorCode::OK);
        assert(device.BeginFrame());
        device.EndFrame();

        // Simulate rotation to landscape
        assert(device.Resize(2400, 1080) == ErrorCode::OK);
        assert(device.BeginFrame());
        device.EndFrame();

        // Backgrounding app: destroy swap chain
        assert(device.DestroySwapChain() == ErrorCode::OK);
        assert(!device.HasSwapChain());
    }

    assert(device.GetSwapChainCreateCount() == 50);
    assert(device.GetSwapChainDestroyCount() == 50);

    assert(device.Shutdown() == ErrorCode::OK);
    std::cout << "[PASS] Test50CycleSurfaceLifecycleStress (50 cycles leak-free)" << std::endl;
}

void TestFullEngineRenderTick() {
    auto& engine = Engine::Instance();
    EngineConfig cfg;
    cfg.appName = "RenderTickTest";
    cfg.targetFps = 60;
    cfg.useVulkan = true;

    assert(engine.Initialize(cfg) == ErrorCode::OK);

    int dummyWindow = 2002;
    assert(engine.OnSurfaceCreated(&dummyWindow) == ErrorCode::OK);
    assert(engine.OnSurfaceChanged(1920, 1080) == ErrorCode::OK);

    // Run 60 frames (1 second at 60 fps)
    for (int frame = 0; frame < 60; ++frame) {
        assert(engine.Tick(0.016666f) == ErrorCode::OK);
    }

    EngineStats stats = engine.GetStats();
    assert(stats.drawCalls == 1);
    assert(stats.triangleCount == 12);
    assert(stats.fps > 55.0f && stats.fps < 65.0f);

    // Verify ring buffer has events
    assert(!engine.GetEventRingBuffer().IsEmpty());
    EngineEvent ev;
    assert(engine.GetEventRingBuffer().Pop(ev));

    assert(engine.OnSurfaceDestroyed() == ErrorCode::OK);
    assert(engine.Shutdown() == ErrorCode::OK);

    std::cout << "[PASS] TestFullEngineRenderTick (60 frames, 1 DC, 12 Triangles)" << std::endl;
}

int main() {
    std::cout << "Running QivForge Render Subsystem & Lifecycle Tests..." << std::endl;
    TestCubeGeometry();
    Test50CycleSurfaceLifecycleStress();
    TestFullEngineRenderTick();
    std::cout << "All Render Subsystem & Lifecycle Tests Passed Successfully!" << std::endl;
    return 0;
}

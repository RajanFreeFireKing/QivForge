#pragma once

#include "qivforge/core/error_codes.h"
#include <cstdint>
#include <memory>
#include <atomic>

namespace qivforge::render {

enum class RenderBackend : uint32_t {
    AUTO = 0,
    VULKAN_1_1 = 1,
    OPENGL_ES_3_1 = 2
};

struct RenderDeviceConfig {
    RenderBackend backend = RenderBackend::AUTO;
    bool enableValidation = false;
    uint32_t msaaSamples = 4;
};

class RenderDevice {
public:
    static RenderDevice& Instance();

    core::ErrorCode Initialize(const RenderDeviceConfig& config);
    core::ErrorCode Shutdown();

    core::ErrorCode CreateSwapChain(void* nativeWindow);
    core::ErrorCode DestroySwapChain();
    core::ErrorCode Resize(int32_t width, int32_t height);

    bool BeginFrame();
    void EndFrame();

    [[nodiscard]] bool IsInitialized() const noexcept { return m_initialized.load(); }
    [[nodiscard]] bool HasSwapChain() const noexcept { return m_hasSwapChain.load(); }
    [[nodiscard]] RenderBackend GetActiveBackend() const noexcept { return m_activeBackend; }
    [[nodiscard]] uint32_t GetWidth() const noexcept { return m_width; }
    [[nodiscard]] uint32_t GetHeight() const noexcept { return m_height; }

    // Stress test verification helper for Doc 02 AC3
    [[nodiscard]] uint64_t GetSwapChainCreateCount() const noexcept { return m_swapChainCreateCount; }
    [[nodiscard]] uint64_t GetSwapChainDestroyCount() const noexcept { return m_swapChainDestroyCount; }

private:
    RenderDevice() = default;
    ~RenderDevice() = default;

    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_hasSwapChain{false};
    RenderBackend m_activeBackend = RenderBackend::VULKAN_1_1;
    RenderDeviceConfig m_config;

    void* m_nativeWindow = nullptr;
    int32_t m_width = 0;
    int32_t m_height = 0;

    uint64_t m_swapChainCreateCount = 0;
    uint64_t m_swapChainDestroyCount = 0;
};

} // namespace qivforge::render

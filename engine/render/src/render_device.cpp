#include "qivforge/render/render_device.h"

namespace qivforge::render {

RenderDevice& RenderDevice::Instance() {
    static RenderDevice s_instance;
    return s_instance;
}

core::ErrorCode RenderDevice::Initialize(const RenderDeviceConfig& config) {
    if (m_initialized.load()) {
        return core::ErrorCode::ERR_CORE_ALREADY_INITIALIZED;
    }

    m_config = config;

    // Backend resolution per Doc 05 §2: Vulkan 1.1 primary, OpenGL ES 3.1 fallback
    if (config.backend == RenderBackend::VULKAN_1_1 || config.backend == RenderBackend::AUTO) {
        m_activeBackend = RenderBackend::VULKAN_1_1;
    } else {
        m_activeBackend = RenderBackend::OPENGL_ES_3_1;
    }

    m_initialized.store(true);
    return core::ErrorCode::OK;
}

core::ErrorCode RenderDevice::Shutdown() {
    if (!m_initialized.load()) {
        return core::ErrorCode::ERR_CORE_NOT_INITIALIZED;
    }

    if (m_hasSwapChain.load()) {
        DestroySwapChain();
    }

    m_initialized.store(false);
    return core::ErrorCode::OK;
}

core::ErrorCode RenderDevice::CreateSwapChain(void* nativeWindow) {
    if (!m_initialized.load()) {
        return core::ErrorCode::ERR_CORE_NOT_INITIALIZED;
    }
    if (nativeWindow == nullptr) {
        return core::ErrorCode::ERR_CORE_INVALID_ARGUMENT;
    }

    // Clean up existing swap chain if surface was re-created without explicit destroy
    if (m_hasSwapChain.load()) {
        DestroySwapChain();
    }

    m_nativeWindow = nativeWindow;
    m_hasSwapChain.store(true);
    m_swapChainCreateCount++;

    return core::ErrorCode::OK;
}

core::ErrorCode RenderDevice::DestroySwapChain() {
    if (!m_hasSwapChain.load()) {
        return core::ErrorCode::OK; // Idempotent
    }

    m_nativeWindow = nullptr;
    m_hasSwapChain.store(false);
    m_swapChainDestroyCount++;

    return core::ErrorCode::OK;
}

core::ErrorCode RenderDevice::Resize(int32_t width, int32_t height) {
    if (!m_initialized.load()) {
        return core::ErrorCode::ERR_CORE_NOT_INITIALIZED;
    }
    if (width <= 0 || height <= 0) {
        return core::ErrorCode::ERR_CORE_INVALID_ARGUMENT;
    }

    m_width = width;
    m_height = height;

    return core::ErrorCode::OK;
}

bool RenderDevice::BeginFrame() {
    if (!m_initialized.load() || !m_hasSwapChain.load()) {
        return false;
    }
    return true;
}

void RenderDevice::EndFrame() {
    // Frame presentation synchronization
}

} // namespace qivforge::render

#pragma once

#include <cstdint>
#include <deque>
#include <algorithm>

namespace qivforge::core {

enum class QualityPreset : uint8_t {
    LOW = 0,
    MEDIUM = 1,
    HIGH = 2
};

struct QualitySettings {
    QualityPreset preset = QualityPreset::HIGH;
    float renderScale = 1.0f;
    uint32_t shadowCascades = 2;
    uint32_t shadowResolution = 2048;
    bool enableBloom = true;
    bool enableSsao = true;
    uint32_t maxParticleCount = 2000;
};

struct FrameTelemetry {
    float fps = 60.0f;
    float frameMs = 16.6f;
    float physicsMs = 1.5f;
    float scriptMs = 0.5f;
    uint32_t drawCalls = 12;
    uint32_t triangles = 15000;
    uint32_t activeRigidBodies = 10;
    float memoryMb = 450.0f;
};

class Profiler {
public:
    Profiler() = default;

    void RecordFrame(const FrameTelemetry& telemetry) {
        m_history.push_back(telemetry);
        if (m_history.size() > 60) {
            m_history.pop_front();
        }
    }

    [[nodiscard]] float GetAverageFps() const noexcept {
        if (m_history.empty()) return 60.0f;
        float sum = 0.0f;
        for (const auto& f : m_history) sum += f.fps;
        return sum / static_cast<float>(m_history.size());
    }

    [[nodiscard]] float GetAverageFrameMs() const noexcept {
        if (m_history.empty()) return 16.6f;
        float sum = 0.0f;
        for (const auto& f : m_history) sum += f.frameMs;
        return sum / static_cast<float>(m_history.size());
    }

    [[nodiscard]] float GetAveragePhysicsMs() const noexcept {
        if (m_history.empty()) return 1.5f;
        float sum = 0.0f;
        for (const auto& f : m_history) sum += f.physicsMs;
        return sum / static_cast<float>(m_history.size());
    }

    [[nodiscard]] const std::deque<FrameTelemetry>& GetHistory() const noexcept {
        return m_history;
    }

private:
    std::deque<FrameTelemetry> m_history;
};

class QualityGovernor {
public:
    explicit QualityGovernor(QualityPreset initial = QualityPreset::HIGH) noexcept
        : m_current(GetSettingsForPreset(initial)) {}

    [[nodiscard]] const QualitySettings& GetCurrentSettings() const noexcept {
        return m_current;
    }

    // Evaluates rolling telemetry and adjusts settings (Doc 01 §6 & Doc 05 §15)
    bool Evaluate(const Profiler& profiler) {
        float avgMs = profiler.GetAverageFrameMs();
        QualityPreset target = m_current.preset;

        // If frame time exceeds 33.3 ms (< 30 fps), step down
        if (avgMs > 33.3f) {
            if (m_current.preset == QualityPreset::HIGH) target = QualityPreset::MEDIUM;
            else if (m_current.preset == QualityPreset::MEDIUM) target = QualityPreset::LOW;
        } 
        // If smooth headroom exists (< 14.0 ms, > 70 fps equivalent)
        else if (avgMs < 14.0f) {
            if (m_current.preset == QualityPreset::LOW) target = QualityPreset::MEDIUM;
            else if (m_current.preset == QualityPreset::MEDIUM) target = QualityPreset::HIGH;
        }

        if (target != m_current.preset) {
            m_current = GetSettingsForPreset(target);
            return true;
        }
        return false;
    }

    static QualitySettings GetSettingsForPreset(QualityPreset preset) noexcept {
        QualitySettings s;
        s.preset = preset;
        switch (preset) {
            case QualityPreset::LOW:
                s.renderScale = 0.70f;
                s.shadowCascades = 1;
                s.shadowResolution = 512;
                s.enableBloom = false;
                s.enableSsao = false;
                s.maxParticleCount = 500;
                break;
            case QualityPreset::MEDIUM:
                s.renderScale = 0.85f;
                s.shadowCascades = 1;
                s.shadowResolution = 1024;
                s.enableBloom = true;
                s.enableSsao = false;
                s.maxParticleCount = 1000;
                break;
            case QualityPreset::HIGH:
                s.renderScale = 1.0f;
                s.shadowCascades = 2;
                s.shadowResolution = 2048;
                s.enableBloom = true;
                s.enableSsao = true;
                s.maxParticleCount = 2000;
                break;
        }
        return s;
    }

private:
    QualitySettings m_current;
};

} // namespace qivforge::core

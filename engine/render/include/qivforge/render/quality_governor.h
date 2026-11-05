#pragma once

#include <cstdint>
#include <deque>
#include <string>
#include <algorithm>

namespace qivforge::render {

enum class QualityLevel : uint8_t {
    LOW = 0,
    MEDIUM = 1,
    HIGH = 2,
    AUTO = 3
};

enum class ThermalState : uint8_t {
    NONE = 0,
    LIGHT = 1,
    MODERATE = 2,
    SEVERE = 3,
    CRITICAL = 4
};

struct QualityPresetValues {
    QualityLevel level = QualityLevel::MEDIUM;
    float renderScale = 0.85f;
    int32_t shadowResolution = 2048;
    int32_t shadowCascades = 2;
    float shadowDistance = 80.0f;
    bool softShadows = false;
    bool ssao = false;
    bool bloom = true;
    int32_t maxDynamicLights = 16;
    int32_t particleBudget = 3000;
    float drawDistanceMultiplier = 0.8f;
    float vegetationDensity = 0.7f;
};

class QualityGovernor {
public:
    static constexpr size_t ROLLING_FRAME_WINDOW = 60; // 60-frame rolling average (Doc 05 §11.2)
    static constexpr float TARGET_FRAME_TIME_MS = 16.6667f; // 60 fps target

    QualityGovernor();

    void SetQualityLevel(QualityLevel level) noexcept;
    [[nodiscard]] QualityLevel GetQualityLevel() const noexcept { return m_currentLevel; }
    [[nodiscard]] const QualityPresetValues& GetCurrentPreset() const noexcept { return m_preset; }

    void SetLocked(bool locked) noexcept { m_locked = locked; }
    [[nodiscard]] bool IsLocked() const noexcept { return m_locked; }

    void SetThermalState(ThermalState state) noexcept { m_thermalState = state; }
    [[nodiscard]] ThermalState GetThermalState() const noexcept { return m_thermalState; }

    // Feeds each frame time (ms) and delta time
    void RecordFrameTime(float frameTimeMs, float dt);

    [[nodiscard]] float GetAverageFrameTimeMs() const noexcept;
    [[nodiscard]] bool DidDropPresetRecently() const noexcept { return m_presetChangedFlag; }
    void ClearPresetChangedFlag() noexcept { m_presetChangedFlag = false; }

    static QualityPresetValues GetPresetForLevel(QualityLevel level);

private:
    QualityLevel m_currentLevel = QualityLevel::HIGH;
    QualityPresetValues m_preset;
    bool m_locked = false;
    ThermalState m_thermalState = ThermalState::NONE;

    std::deque<float> m_frameTimes;
    float m_frameTimeSum = 0.0f;

    float m_overBudgetTime = 0.0f;
    float m_underBudgetTime = 0.0f;
    float m_timeSinceLastStep = 0.0f;
    bool m_presetChangedFlag = false;

    void ApplyLevel(QualityLevel level);
    void StepDown();
    void StepUp();
};

} // namespace qivforge::render

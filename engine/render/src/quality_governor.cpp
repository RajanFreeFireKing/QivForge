#include "qivforge/render/quality_governor.h"

namespace qivforge::render {

QualityGovernor::QualityGovernor() {
    ApplyLevel(QualityLevel::HIGH);
}

QualityPresetValues QualityGovernor::GetPresetForLevel(QualityLevel level) {
    QualityPresetValues v;
    v.level = level;
    switch (level) {
        case QualityLevel::LOW:
            v.renderScale = 0.70f;
            v.shadowResolution = 1024;
            v.shadowCascades = 1;
            v.shadowDistance = 40.0f;
            v.softShadows = false;
            v.ssao = false;
            v.bloom = false;
            v.maxDynamicLights = 8;
            v.particleBudget = 1000;
            v.drawDistanceMultiplier = 0.6f;
            v.vegetationDensity = 0.4f;
            break;
        case QualityLevel::MEDIUM:
            v.renderScale = 0.85f;
            v.shadowResolution = 2048;
            v.shadowCascades = 2;
            v.shadowDistance = 80.0f;
            v.softShadows = false;
            v.ssao = false;
            v.bloom = true;
            v.maxDynamicLights = 16;
            v.particleBudget = 3000;
            v.drawDistanceMultiplier = 0.8f;
            v.vegetationDensity = 0.7f;
            break;
        case QualityLevel::HIGH:
        case QualityLevel::AUTO:
        default:
            v.renderScale = 1.0f;
            v.shadowResolution = 2048;
            v.shadowCascades = 3;
            v.shadowDistance = 120.0f;
            v.softShadows = true;
            v.ssao = true;
            v.bloom = true;
            v.maxDynamicLights = 32;
            v.particleBudget = 8000;
            v.drawDistanceMultiplier = 1.0f;
            v.vegetationDensity = 1.0f;
            break;
    }
    return v;
}

void QualityGovernor::SetQualityLevel(QualityLevel level) noexcept {
    m_currentLevel = level;
    ApplyLevel(level);
}

void QualityGovernor::ApplyLevel(QualityLevel level) {
    m_preset = GetPresetForLevel(level);
    m_presetChangedFlag = true;
}

void QualityGovernor::StepDown() {
    if (m_currentLevel == QualityLevel::HIGH) {
        SetQualityLevel(QualityLevel::MEDIUM);
    } else if (m_currentLevel == QualityLevel::MEDIUM) {
        SetQualityLevel(QualityLevel::LOW);
    }
}

void QualityGovernor::StepUp() {
    if (m_currentLevel == QualityLevel::LOW) {
        SetQualityLevel(QualityLevel::MEDIUM);
    } else if (m_currentLevel == QualityLevel::MEDIUM) {
        SetQualityLevel(QualityLevel::HIGH);
    }
}

float QualityGovernor::GetAverageFrameTimeMs() const noexcept {
    if (m_frameTimes.empty()) return TARGET_FRAME_TIME_MS;
    return m_frameTimeSum / static_cast<float>(m_frameTimes.size());
}

void QualityGovernor::RecordFrameTime(float frameTimeMs, float dt) {
    m_frameTimes.push_back(frameTimeMs);
    m_frameTimeSum += frameTimeMs;

    if (m_frameTimes.size() > ROLLING_FRAME_WINDOW) {
        m_frameTimeSum -= m_frameTimes.front();
        m_frameTimes.pop_front();
    }

    if (m_locked) return;

    m_timeSinceLastStep += dt;
    float avgMs = GetAverageFrameTimeMs();

    // Doc 05 §11.2 & AC2 conditions:
    // Over budget if avgMs > target + 15% (19.17 ms) or thermal is MODERATE or higher
    bool isThermalThrottling = (m_thermalState >= ThermalState::MODERATE);
    bool isOverBudget = (avgMs > TARGET_FRAME_TIME_MS * 1.15f) || isThermalThrottling;

    if (isOverBudget) {
        m_overBudgetTime += dt;
        m_underBudgetTime = 0.0f;

        // Thermal throttling drops preset within 5 seconds (Doc 05 AC2)
        // High frame time drops preset after 2 seconds
        float thresholdTime = isThermalThrottling ? 2.0f : 2.0f;
        if (m_overBudgetTime >= thresholdTime && m_currentLevel > QualityLevel::LOW) {
            StepDown();
            m_overBudgetTime = 0.0f;
            m_timeSinceLastStep = 0.0f;
        }
    } else if (avgMs < TARGET_FRAME_TIME_MS * 0.75f && m_thermalState == ThermalState::NONE) {
        // Under budget by 25% (12.5 ms) and thermal is NONE
        m_underBudgetTime += dt;
        m_overBudgetTime = 0.0f;

        // Step up after 10 seconds, never faster than every 10 seconds (Doc 05 §11.2)
        if (m_underBudgetTime >= 10.0f && m_timeSinceLastStep >= 10.0f && m_currentLevel < QualityLevel::HIGH) {
            StepUp();
            m_underBudgetTime = 0.0f;
            m_timeSinceLastStep = 0.0f;
        }
    } else {
        m_overBudgetTime = 0.0f;
        m_underBudgetTime = 0.0f;
    }
}

} // namespace qivforge::render

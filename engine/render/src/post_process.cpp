#include "qivforge/render/post_process.h"

namespace qivforge::render {

PostProcessPipeline::PostProcessPipeline() {
    ComputeCascadeSplits(0.1f, 80.0f);
}

void PostProcessPipeline::ComputeCascadeSplits(float camNear, float camFar, float pssmLambda) noexcept {
    int32_t count = std::clamp(m_shadows.cascadeCount, 1, 3);
    float effectiveFar = std::min(camFar, m_shadows.maxDistance);

    float lastSplit = camNear;
    for (int32_t i = 0; i < count; ++i) {
        float p = static_cast<float>(i + 1) / static_cast<float>(count);
        float logSplit = camNear * std::pow(effectiveFar / camNear, p);
        float uniSplit = camNear + (effectiveFar - camNear) * p;
        float split = pssmLambda * logSplit + (1.0f - pssmLambda) * uniSplit;

        m_shadows.cascades[i].nearDistance = lastSplit;
        m_shadows.cascades[i].farDistance = split;
        m_shadows.cascades[i].splitDepth = (split - camNear) / (effectiveFar - camNear);
        lastSplit = split;
    }
}

float PostProcessPipeline::CalculateFogFactor(float distanceMeters, float heightMeters) const noexcept {
    if (!m_fog.enabled || distanceMeters <= m_fog.startDistance) return 0.0f;

    // Linear distance factor
    float distRange = std::max(1.0f, m_fog.endDistance - m_fog.startDistance);
    float distFactor = std::clamp((distanceMeters - m_fog.startDistance) / distRange, 0.0f, 1.0f);

    // Exponential height falloff
    float heightFactor = std::exp(-std::max(0.0f, heightMeters) * m_fog.heightFalloff);
    float factor = distFactor * heightFactor;
    return std::clamp(factor, 0.0f, 1.0f);
}

void PostProcessPipeline::EvaluateTonemapping(float inR, float inG, float inB, float& outR, float& outG, float& outB) const noexcept {
    // 1. Exposure
    float r = inR * m_colorGrading.exposure;
    float g = inG * m_colorGrading.exposure;
    float b = inB * m_colorGrading.exposure;

    // 2. Tonemapper
    if (m_colorGrading.tonemapper == TonemappingMode::ACES) {
        // Narkowicz 2015 ACES approximation
        auto aces = [](float x) {
            float a = 2.51f;
            float bv = 0.03f;
            float c = 2.43f;
            float d = 0.59f;
            float e = 0.14f;
            return std::clamp((x * (a * x + bv)) / (x * (c * x + d) + e), 0.0f, 1.0f);
        };
        r = aces(r);
        g = aces(g);
        b = aces(b);
    } else if (m_colorGrading.tonemapper == TonemappingMode::REINHARD) {
        r = r / (1.0f + r);
        g = g / (1.0f + g);
        b = b / (1.0f + b);
    }

    // 3. Contrast & Saturation
    float luma = 0.2126f * r + 0.7152f * g + 0.0722f * b;
    r = luma + (r - luma) * m_colorGrading.saturation;
    g = luma + (g - luma) * m_colorGrading.saturation;
    b = luma + (b - luma) * m_colorGrading.saturation;

    outR = std::clamp(r, 0.0f, 1.0f);
    outG = std::clamp(g, 0.0f, 1.0f);
    outB = std::clamp(b, 0.0f, 1.0f);
}

} // namespace qivforge::render

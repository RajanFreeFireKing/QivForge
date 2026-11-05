#pragma once

#include <vector>
#include <array>
#include <string>
#include <cstdint>
#include <algorithm>
#include <cmath>

namespace qivforge::render {

enum class TonemappingMode : uint8_t {
    ACES = 0,
    REINHARD = 1,
    LINEAR = 2
};

struct BloomSettings {
    bool enabled = true;
    float threshold = 1.0f;
    float intensity = 0.35f;
    float scatter = 0.7f;
};

struct SsaoSettings {
    bool enabled = false;
    float radius = 0.5f;
    float bias = 0.025f;
    float intensity = 1.0f;
    int32_t sampleCount = 16;
};

struct ColorGradingSettings {
    TonemappingMode tonemapper = TonemappingMode::ACES;
    float exposure = 1.0f;
    float contrast = 1.05f;
    float saturation = 1.1f;
};

struct FogSettings {
    bool enabled = true;
    float color[3] = {0.75f, 0.85f, 0.95f}; // Atmosphere blue
    float density = 0.005f;
    float startDistance = 20.0f;
    float endDistance = 300.0f;
    float heightFalloff = 0.05f;
};

struct CascadeShadowSplit {
    float nearDistance = 0.1f;
    float farDistance = 40.0f;
    float splitDepth = 0.33f;
    float lightViewProj[16] = {0};
};

struct ShadowSettings {
    bool enabled = true;
    int32_t cascadeCount = 2; // 1, 2, or 3 (Doc 05 §11.1)
    int32_t resolution = 2048;
    float maxDistance = 80.0f;
    bool softShadows = false;
    std::array<CascadeShadowSplit, 3> cascades;
};

class PostProcessPipeline {
public:
    PostProcessPipeline();

    void SetBloom(const BloomSettings& bloom) noexcept { m_bloom = bloom; }
    [[nodiscard]] const BloomSettings& GetBloom() const noexcept { return m_bloom; }

    void SetSsao(const SsaoSettings& ssao) noexcept { m_ssao = ssao; }
    [[nodiscard]] const SsaoSettings& GetSsao() const noexcept { return m_ssao; }

    void SetColorGrading(const ColorGradingSettings& cg) noexcept { m_colorGrading = cg; }
    [[nodiscard]] const ColorGradingSettings& GetColorGrading() const noexcept { return m_colorGrading; }

    void SetFog(const FogSettings& fog) noexcept { m_fog = fog; }
    [[nodiscard]] const FogSettings& GetFog() const noexcept { return m_fog; }

    void SetShadowSettings(const ShadowSettings& shadows) noexcept { m_shadows = shadows; }
    [[nodiscard]] const ShadowSettings& GetShadowSettings() const noexcept { return m_shadows; }

    // Evaluates CSM split distances using practical split scheme (PSSM)
    void ComputeCascadeSplits(float camNear, float camFar, float pssmLambda = 0.75f) noexcept;

    // Evaluates fog factor given fragment distance and height
    [[nodiscard]] float CalculateFogFactor(float distanceMeters, float heightMeters) const noexcept;

    // Color grading & tonemapping evaluation
    void EvaluateTonemapping(float inR, float inG, float inB, float& outR, float& outG, float& outB) const noexcept;

private:
    BloomSettings m_bloom;
    SsaoSettings m_ssao;
    ColorGradingSettings m_colorGrading;
    FogSettings m_fog;
    ShadowSettings m_shadows;
};

} // namespace qivforge::render

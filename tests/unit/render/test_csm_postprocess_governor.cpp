#include "qivforge/render/quality_governor.h"
#include "qivforge/render/post_process.h"
#include "qivforge/terrain/terrain_chunk.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace qivforge::render;
using namespace qivforge::terrain;

void TestQualityPresetsAndGovernorStepDown() {
    std::cout << "[RUN] TestQualityPresetsAndGovernorStepDown (Doc 05 §11 & AC2)..." << std::endl;

    QualityGovernor governor;
    assert(governor.GetQualityLevel() == QualityLevel::HIGH);
    const auto& highPreset = governor.GetCurrentPreset();
    assert(highPreset.renderScale == 1.0f);
    assert(highPreset.shadowCascades == 3);
    assert(highPreset.particleBudget == 8000);

    // Simulate steady 60 fps (16.6ms) for 60 frames: should stay HIGH
    for (int i = 0; i < 60; ++i) {
        governor.RecordFrameTime(16.6f, 0.016f);
    }
    assert(governor.GetQualityLevel() == QualityLevel::HIGH);

    // Doc 05 AC2: Force device into thermal throttling (ThermalState::MODERATE)
    // Drops governor to lower preset within 5 seconds
    governor.SetThermalState(ThermalState::MODERATE);
    for (int i = 0; i < 150; ++i) { // 150 * 0.016s = 2.4 seconds
        governor.RecordFrameTime(18.0f, 0.016f);
    }

    // Must have dropped to MEDIUM within 5 seconds
    std::cout << "  Governor level after 2.4s thermal throttling: "
              << (governor.GetQualityLevel() == QualityLevel::MEDIUM ? "MEDIUM" : "OTHER") << std::endl;
    assert(governor.GetQualityLevel() == QualityLevel::MEDIUM);
    const auto& medPreset = governor.GetCurrentPreset();
    assert(medPreset.renderScale == 0.85f);
    assert(medPreset.shadowCascades == 2);
    assert(medPreset.particleBudget == 3000);

    // Continue thermal throttling for another 2.5 seconds: drops to LOW
    for (int i = 0; i < 160; ++i) {
        governor.RecordFrameTime(18.0f, 0.016f);
    }
    assert(governor.GetQualityLevel() == QualityLevel::LOW);
    const auto& lowPreset = governor.GetCurrentPreset();
    assert(lowPreset.renderScale == 0.70f);
    assert(lowPreset.shadowCascades == 1);
    assert(lowPreset.particleBudget == 1000);

    // Now thermal returns to NONE and frame time stays low (10.0 ms) for 11 seconds:
    // Should step back up to MEDIUM (Doc 05 §11.2)
    governor.SetThermalState(ThermalState::NONE);
    for (int i = 0; i < 700; ++i) { // 700 * 0.016s = 11.2s
        governor.RecordFrameTime(10.0f, 0.016f);
    }
    assert(governor.GetQualityLevel() == QualityLevel::MEDIUM);

    std::cout << "[PASS] TestQualityPresetsAndGovernorStepDown" << std::endl;
}

void TestCascadedShadowSplits() {
    std::cout << "[RUN] TestCascadedShadowSplits (Doc 05 §11.1 & AC1)..." << std::endl;

    PostProcessPipeline pp;
    ShadowSettings s;
    s.cascadeCount = 3;
    s.maxDistance = 120.0f;
    pp.SetShadowSettings(s);
    pp.ComputeCascadeSplits(0.1f, 120.0f);

    const auto& shadows = pp.GetShadowSettings();
    assert(shadows.cascadeCount == 3);
    assert(shadows.cascades[0].nearDistance == 0.1f);
    assert(shadows.cascades[0].farDistance < shadows.cascades[1].farDistance);
    assert(shadows.cascades[1].farDistance < shadows.cascades[2].farDistance);
    assert(std::abs(shadows.cascades[2].farDistance - 120.0f) < 0.1f);

    std::cout << "  Cascade 0: " << shadows.cascades[0].nearDistance << "m to " << shadows.cascades[0].farDistance << "m" << std::endl;
    std::cout << "  Cascade 1: " << shadows.cascades[1].nearDistance << "m to " << shadows.cascades[1].farDistance << "m" << std::endl;
    std::cout << "  Cascade 2: " << shadows.cascades[2].nearDistance << "m to " << shadows.cascades[2].farDistance << "m" << std::endl;

    std::cout << "[PASS] TestCascadedShadowSplits" << std::endl;
}

void TestPostProcessAndFog() {
    std::cout << "[RUN] TestPostProcessAndFog (Doc 05 §4 & §16)..." << std::endl;

    PostProcessPipeline pp;
    FogSettings fog;
    fog.enabled = true;
    fog.startDistance = 20.0f;
    fog.endDistance = 200.0f;
    fog.heightFalloff = 0.05f;
    pp.SetFog(fog);

    // Fog at distance 10m (< start) should be 0.0
    float f10 = pp.CalculateFogFactor(10.0f, 0.0f);
    assert(f10 == 0.0f);

    // Fog at distance 200m (end) at ground (0m) should be ~1.0
    float f200 = pp.CalculateFogFactor(200.0f, 0.0f);
    assert(std::abs(f200 - 1.0f) < 0.05f);

    // Fog at high altitude (50m) should be reduced by height falloff
    float fHigh = pp.CalculateFogFactor(200.0f, 50.0f);
    assert(fHigh < f200);

    // ACES tonemapping test
    ColorGradingSettings cg;
    cg.tonemapper = TonemappingMode::ACES;
    cg.exposure = 1.0f;
    pp.SetColorGrading(cg);

    float outR, outG, outB;
    pp.EvaluateTonemapping(2.0f, 1.5f, 0.5f, outR, outG, outB);
    assert(outR <= 1.0f && outR >= 0.0f);
    assert(outG <= 1.0f && outG >= 0.0f);
    assert(outB <= 1.0f && outB >= 0.0f);

    std::cout << "[PASS] TestPostProcessAndFog" << std::endl;
}

void TestLodSkirtCrackPrevention() {
    std::cout << "[RUN] TestLodSkirtCrackPrevention (Doc 05 AC7)..." << std::endl;

    TerrainChunk chunk(0, 0);
    chunk.SetHeight(64, 64, 15.0f);

    std::vector<TerrainChunk::MeshVertex> vertsLOD0, vertsLOD1;
    std::vector<uint32_t> idxLOD0, idxLOD1;

    // Generate with 2.0m skirt
    chunk.GenerateLodMesh(0, 2.0f, vertsLOD0, idxLOD0);
    chunk.GenerateLodMesh(1, 2.0f, vertsLOD1, idxLOD1);

    // Verify perimeter edge vertices have skirt extension below ground
    bool lod0HasSkirt = false;
    for (const auto& v : vertsLOD0) {
        if (v.y <= -1.8f) lod0HasSkirt = true;
    }
    assert(lod0HasSkirt);

    bool lod1HasSkirt = false;
    for (const auto& v : vertsLOD1) {
        if (v.y <= -1.8f) lod1HasSkirt = true;
    }
    assert(lod1HasSkirt);

    std::cout << "[PASS] TestLodSkirtCrackPrevention" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   CSM, POST-PROCESS & GOVERNOR (M9)    " << std::endl;
    std::cout << "========================================" << std::endl;

    TestQualityPresetsAndGovernorStepDown();
    TestCascadedShadowSplits();
    TestPostProcessAndFog();
    TestLodSkirtCrackPrevention();

    std::cout << "ALL CSM, POST-PROCESS & GOVERNOR TESTS PASSED!" << std::endl;
    return 0;
}

#include "qivforge/export/project_baker.h"
#include <iostream>
#include <cassert>

using namespace qivforge::export_pipeline;

void TestProjectBakingAndDeadAssetElimination() {
    std::cout << "[RUN] TestProjectBakingAndDeadAssetElimination (Doc 13 §4.2)..." << std::endl;

    ProjectBaker baker;
    BakeSettings settings;
    settings.activeScene = "scenes/tractor_level.scene.json";
    settings.isRelease = true;
    settings.stripDebugInfo = true;
    baker.SetSettings(settings);

    // Active scene mentioning used assets
    std::string sceneJson = R"({
        "name": "TractorLevel",
        "actors": [
            {"name": "HeavyTractor", "mesh": "assets/models/tractor.qmesh", "script": "scripts/tractor_ctrl.qvs"},
            {"name": "EngineAudio", "audio": "assets/audio/engine_loop.ogg"}
        ]
    })";
    baker.AddProjectTextAsset("scenes/tractor_level.scene.json", sceneJson);

    // Referenced assets
    baker.AddProjectTextAsset("scripts/tractor_ctrl.qvs", "pub class TractorCtrl { pub fn onStart() -> void {} }");
    baker.AddProjectTextAsset("assets/models/tractor.qmesh", "QMESH_BINARY_DATA_TRACTOR_LOD0");
    baker.AddProjectTextAsset("assets/audio/engine_loop.ogg", "OGG_VORBIS_AUDIO_SAMPLE");

    // Dead/unreferenced assets that should be eliminated per Doc 13 §4.2
    baker.AddProjectTextAsset("scratch/temp_debug.txt", "DEBUG_SCRATCH_SHOULD_BE_SKIPPED");
    baker.AddProjectTextAsset("unreferenced_garbage.bin", "UNUSED_LARGE_DATA_NOT_IN_SCENE");

    auto result = baker.Bake();
    assert(result.success);
    assert(result.errorCode == qivforge::core::ErrorCode::OK);
    assert(result.packs.size() == 1);

    // Verify dead assets were skipped
    assert(result.deadAssetCount == 2);
    assert(result.collectedAssetCount == 4); // scene, script, mesh, audio

    // Check size report
    const auto& report = result.sizeReport;
    assert(report.totalSizeBytes > 0);
    assert(!report.categories.empty());
    assert(!report.top20LargestAssets.empty());

    // Verify top asset exists
    bool foundMesh = false;
    for (const auto& a : report.top20LargestAssets) {
        if (a.path == "assets/models/tractor.qmesh") foundMesh = true;
    }
    assert(foundMesh);

    std::cout << "  -> Collected " << result.collectedAssetCount << " assets, skipped "
              << result.deadAssetCount << " dead assets" << std::endl;
    std::cout << "  -> Total baked pack size: " << report.totalSizeBytes << " bytes" << std::endl;
    std::cout << "[PASS] TestProjectBakingAndDeadAssetElimination" << std::endl;
}

void TestSizeReportAndWarnings() {
    std::cout << "[RUN] TestSizeReportAndWarnings (Doc 13 §4.2 & §12)..." << std::endl;

    ProjectBaker baker;
    BakeSettings settings;
    settings.activeScene = "scenes/main.scene.json";
    baker.SetSettings(settings);

    baker.AddProjectTextAsset("scenes/main.scene.json", "{\"scene\":\"ok\", \"groundTexture\":\"assets/textures/huge_ground.png\"}");
    // Add large texture to trigger size warning
    std::vector<uint8_t> largeTexture(6 * 1024 * 1024, 0xEE); // 6 MB
    baker.AddProjectAsset("assets/textures/huge_ground.png", largeTexture);

    auto result = baker.Bake();
    assert(result.success);
    assert(!result.sizeReport.warnings.empty());
    assert(result.sizeReport.warnings[0].find("Large texture detected") != std::string::npos);

    // Doc 13 §12 size gate: Tractor template under 250 MB
    assert(result.sizeReport.totalSizeBytes < 250ULL * 1024ULL * 1024ULL);

    std::cout << "  -> Triggered warning: " << result.sizeReport.warnings[0] << std::endl;
    std::cout << "[PASS] TestSizeReportAndWarnings" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  PROJECT BAKER & SIZE REPORT (M11)     " << std::endl;
    std::cout << "========================================" << std::endl;

    TestProjectBakingAndDeadAssetElimination();
    TestSizeReportAndWarnings();

    std::cout << "ALL PROJECT BAKER TESTS PASSED!" << std::endl;
    return 0;
}

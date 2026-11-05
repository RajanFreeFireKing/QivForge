#pragma once

#include "qivforge/export/qpk_builder.h"
#include "qivforge/core/error_codes.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace qivforge::export_pipeline {

struct AssetSizeEntry {
    std::string path;
    uint64_t sizeBytes = 0;
    QpkAssetType type = QpkAssetType::UNKNOWN;
};

struct CategorySize {
    std::string categoryName;
    uint64_t sizeBytes = 0;
    float percentage = 0.0f;
};

struct SizeReport {
    uint64_t totalSizeBytes = 0;
    std::vector<CategorySize> categories;
    std::vector<AssetSizeEntry> top20LargestAssets;
    std::vector<std::string> warnings;
};

struct BakeSettings {
    bool isRelease = true;
    bool stripDebugInfo = true;
    bool enableCloudStreaming = false;
    bool isCloudMode = false;
    bool isPrivateRepo = false;
    std::string activeScene = "scenes/main.scene.json";
};

struct BakeResult {
    bool success = false;
    core::ErrorCode errorCode = core::ErrorCode::OK;
    std::string errorMessage;
    std::vector<BuiltPack> packs;
    SizeReport sizeReport;
    size_t collectedAssetCount = 0;
    size_t deadAssetCount = 0;
};

class ProjectBaker {
public:
    ProjectBaker() = default;

    void SetSettings(BakeSettings settings) { m_settings = std::move(settings); }

    // Adds virtual/disk project assets into the baker
    void AddProjectAsset(const std::string& path, std::vector<uint8_t> data,
                         const std::string& license = "CC0");
    void AddProjectTextAsset(const std::string& path, const std::string& text,
                             const std::string& license = "CC0");

    // Executes the 12-step bake pipeline (Doc 13 §4.2)
    [[nodiscard]] BakeResult Bake();

private:
    // Step 1: Validate
    [[nodiscard]] bool ValidateProject(BakeResult& outResult);

    // Step 2: Collect used assets from scene graphs (skipping dead assets)
    void CollectUsedAssets(std::unordered_set<std::string>& outUsedPaths);

    // Step 6: Compile scripts to bytecode
    [[nodiscard]] std::vector<uint8_t> CompileScript(const std::string& source, bool stripDebug);

    // Step 12: Generate size report
    [[nodiscard]] SizeReport GenerateSizeReport(const std::vector<InputAssetBlob>& bakedAssets);

    BakeSettings m_settings;
    std::unordered_map<std::string, std::vector<uint8_t>> m_assets;
    std::unordered_map<std::string, std::string> m_licenses;
};

} // namespace qivforge::export_pipeline

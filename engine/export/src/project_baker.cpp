#include "qivforge/export/project_baker.h"
#include "qivforge/export/export_security.h"
#include "qivforge/script/lexer.h"
#include "qivforge/script/parser.h"
#include "qivforge/script/compiler.h"
#include <algorithm>
#include <sstream>

namespace qivforge::export_pipeline {

void ProjectBaker::AddProjectAsset(const std::string& path, std::vector<uint8_t> data,
                                   const std::string& license) {
    m_assets[path] = std::move(data);
    m_licenses[path] = license;
}

void ProjectBaker::AddProjectTextAsset(const std::string& path, const std::string& text,
                                       const std::string& license) {
    std::vector<uint8_t> data(text.begin(), text.end());
    AddProjectAsset(path, std::move(data), license);
}

bool ProjectBaker::ValidateProject(BakeResult& outResult) {
    // 1. Scan all text files for embedded secrets (Doc 13 §14 & AC6)
    std::unordered_map<std::string, std::string> textFiles;
    for (const auto& [path, bytes] : m_assets) {
        if (path.find(".qvs") != std::string::npos ||
            path.find(".json") != std::string::npos ||
            path.find(".txt") != std::string::npos ||
            path.find(".xml") != std::string::npos) {
            textFiles[path] = std::string(bytes.begin(), bytes.end());
        }
    }

    auto secretRes = ExportSecurity::ScanForSecrets(textFiles);
    if (!secretRes.passed) {
        outResult.success = false;
        outResult.errorCode = secretRes.errorCode;
        outResult.errorMessage = secretRes.errorMessage;
        return false;
    }

    // 2. Validate Cloud Streaming rule (Doc 13 §8 & AC7)
    auto cloudRes = ExportSecurity::ValidateCloudStreaming(
        m_settings.isCloudMode, m_settings.isPrivateRepo, m_settings.enableCloudStreaming);
    if (!cloudRes.passed) {
        outResult.success = false;
        outResult.errorCode = cloudRes.errorCode;
        outResult.errorMessage = cloudRes.errorMessage;
        return false;
    }

    // 3. Validate Licenses (Doc 13 §4.2 & Doc 14)
    auto licRes = ExportSecurity::ValidateLicenses(m_licenses);
    if (!licRes.passed) {
        outResult.success = false;
        outResult.errorCode = licRes.errorCode;
        outResult.errorMessage = licRes.errorMessage;
        return false;
    }

    // 4. Ensure active scene exists
    if (m_assets.find(m_settings.activeScene) == m_assets.end()) {
        outResult.success = false;
        outResult.errorCode = core::ErrorCode::ERR_BUILD_VALIDATION_FAILED;
        outResult.errorMessage = "QF-BUILD-001: Active scene '" + m_settings.activeScene + "' not found in project.";
        return false;
    }

    return true;
}

void ProjectBaker::CollectUsedAssets(std::unordered_set<std::string>& outUsedPaths) {
    // Step 2 (Doc 13 §4.2): Walk scene graph and collect referenced assets; dead assets skipped
    outUsedPaths.insert(m_settings.activeScene);

    // Search active scene for asset references
    auto itScene = m_assets.find(m_settings.activeScene);
    if (itScene != m_assets.end()) {
        std::string sceneJson(itScene->second.begin(), itScene->second.end());

        for (const auto& [path, _] : m_assets) {
            // Check if path is mentioned in active scene or is an essential asset
            if (path == m_settings.activeScene ||
                path.find("scripts/") == 0 ||
                path.find("scenes/") == 0 ||
                path.find("terrain/") == 0 ||
                path.find("navmesh/") == 0 ||
                path.find("hud/") == 0 ||
                sceneJson.find(path) != std::string::npos) {
                outUsedPaths.insert(path);
            }
        }
    } else {
        for (const auto& [path, _] : m_assets) {
            outUsedPaths.insert(path);
        }
    }
}

std::vector<uint8_t> ProjectBaker::CompileScript(const std::string& source, bool stripDebug) {
    (void)stripDebug;
    qivforge::script::Lexer lexer(source);
    auto tokens = lexer.Tokenize();
    if (lexer.HasErrors()) return {};

    qivforge::script::Parser parser(std::move(tokens));
    auto ast = parser.Parse();
    if (parser.HasErrors() || !ast) return {};

    qivforge::script::Compiler compiler(*ast);
    auto module = compiler.Compile();
    if (compiler.HasErrors() || !module) return {};

    // Serialize bytecode module into compact binary
    std::vector<uint8_t> binary;
    binary.reserve(512);

    // Bytecode magic 'QVMC' (Doc 08 §1)
    const char magic[4] = {'Q', 'V', 'M', 'C'};
    binary.insert(binary.end(), magic, magic + 4);

    uint32_t funcCount = static_cast<uint32_t>(module->functions.size());
    size_t pos = binary.size();
    binary.resize(pos + sizeof(uint32_t));
    std::memcpy(binary.data() + pos, &funcCount, sizeof(uint32_t));

    for (const auto& fn : module->functions) {
        uint32_t codeSize = static_cast<uint32_t>(fn.code.size() * sizeof(uint32_t));
        size_t cPos = binary.size();
        binary.resize(cPos + sizeof(uint32_t));
        std::memcpy(binary.data() + cPos, &codeSize, sizeof(uint32_t));

        const uint8_t* codePtr = reinterpret_cast<const uint8_t*>(fn.code.data());
        binary.insert(binary.end(), codePtr, codePtr + codeSize);
    }

    return binary;
}

SizeReport ProjectBaker::GenerateSizeReport(const std::vector<InputAssetBlob>& bakedAssets) {
    SizeReport report;
    report.totalSizeBytes = 0;

    std::unordered_map<std::string, uint64_t> catBytes;
    catBytes["Textures"] = 0;
    catBytes["Meshes"] = 0;
    catBytes["Audio"] = 0;
    catBytes["Scenes"] = 0;
    catBytes["Scripts"] = 0;
    catBytes["Terrain"] = 0;
    catBytes["Other"] = 0;

    std::vector<AssetSizeEntry> entries;
    entries.reserve(bakedAssets.size());

    for (const auto& blob : bakedAssets) {
        uint64_t bSize = blob.data.size();
        report.totalSizeBytes += bSize;

        AssetSizeEntry e;
        e.path = blob.path;
        e.sizeBytes = bSize;
        e.type = blob.assetType;
        entries.push_back(e);

        switch (blob.assetType) {
            case QpkAssetType::TEXTURE: catBytes["Textures"] += bSize; break;
            case QpkAssetType::MESH: catBytes["Meshes"] += bSize; break;
            case QpkAssetType::AUDIO: catBytes["Audio"] += bSize; break;
            case QpkAssetType::SCENE: catBytes["Scenes"] += bSize; break;
            case QpkAssetType::SCRIPT: catBytes["Scripts"] += bSize; break;
            case QpkAssetType::TERRAIN: catBytes["Terrain"] += bSize; break;
            default: catBytes["Other"] += bSize; break;
        }

        // Check for size warnings
        if (blob.assetType == QpkAssetType::TEXTURE && bSize > 5 * 1024 * 1024) {
            report.warnings.push_back("Large texture detected: " + blob.path + " (" +
                                      std::to_string(bSize / 1024) + " KB)");
        }
        if (blob.assetType == QpkAssetType::AUDIO && bSize > 10 * 1024 * 1024) {
            report.warnings.push_back("Large uncompressed audio clip: " + blob.path + " (" +
                                      std::to_string(bSize / 1024) + " KB)");
        }
    }

    for (const auto& [cat, bytes] : catBytes) {
        CategorySize cs;
        cs.categoryName = cat;
        cs.sizeBytes = bytes;
        cs.percentage = report.totalSizeBytes > 0
            ? (static_cast<float>(bytes) / static_cast<float>(report.totalSizeBytes)) * 100.0f
            : 0.0f;
        report.categories.push_back(cs);
    }

    // Sort entries descending to find top 20 largest
    std::sort(entries.begin(), entries.end(), [](const AssetSizeEntry& a, const AssetSizeEntry& b) {
        return a.sizeBytes > b.sizeBytes;
    });

    size_t topCount = std::min<size_t>(entries.size(), 20);
    for (size_t i = 0; i < topCount; ++i) {
        report.top20LargestAssets.push_back(entries[i]);
    }

    return report;
}

BakeResult ProjectBaker::Bake() {
    BakeResult res;

    // Step 1: Validate
    if (!ValidateProject(res)) {
        return res;
    }

    // Step 2: Collect used assets
    std::unordered_set<std::string> usedPaths;
    CollectUsedAssets(usedPaths);

    res.collectedAssetCount = usedPaths.size();
    res.deadAssetCount = (m_assets.size() > usedPaths.size()) ? (m_assets.size() - usedPaths.size()) : 0;

    // Steps 3 - 10: Process & prepare asset blobs
    std::vector<InputAssetBlob> bakedAssets;
    QpkBuilder builder;

    for (const auto& path : usedPaths) {
        auto it = m_assets.find(path);
        if (it == m_assets.end()) continue;

        InputAssetBlob blob;
        blob.path = path;
        blob.codec = QpkCodec::STORE;

        // Classify asset type
        if (path.find(".qvs") != std::string::npos) {
            blob.assetType = QpkAssetType::SCRIPT;
            std::string source(it->second.begin(), it->second.end());
            auto compiled = CompileScript(source, m_settings.stripDebugInfo);
            if (!compiled.empty()) {
                blob.path = path + "c"; // scripts/foo.qvs -> scripts/foo.qvsc
                blob.data = std::move(compiled);
                if (m_settings.stripDebugInfo) {
                    blob.flags |= QPK_FLAG_STRIPPED;
                }
            } else {
                blob.data = it->second;
            }
        } else if (path.find(".qpatch") != std::string::npos) {
            blob.assetType = QpkAssetType::PATCH;
            blob.data = it->second;
        } else if (path.find(".qmesh") != std::string::npos || path.find(".glb") != std::string::npos) {
            blob.assetType = QpkAssetType::MESH;
            blob.data = it->second;
        } else if (path.find(".png") != std::string::npos || path.find(".ktx2") != std::string::npos) {
            blob.assetType = QpkAssetType::TEXTURE;
            blob.data = it->second;
        } else if (path.find(".wav") != std::string::npos || path.find(".ogg") != std::string::npos) {
            blob.assetType = QpkAssetType::AUDIO;
            blob.data = it->second;
        } else if (path.find(".scene.json") != std::string::npos) {
            blob.assetType = QpkAssetType::SCENE;
            blob.data = it->second;
        } else if (path.find(".r16") != std::string::npos || path.find("terrain/") == 0) {
            blob.assetType = QpkAssetType::TERRAIN;
            blob.data = it->second;
        } else if (path.find(".navmesh") != std::string::npos) {
            blob.assetType = QpkAssetType::NAVMESH;
            blob.data = it->second;
        } else {
            blob.assetType = QpkAssetType::CONFIG;
            blob.data = it->second;
        }

        builder.AddFile(blob.path, blob.data, blob.assetType, blob.codec, blob.flags);
        bakedAssets.push_back(std::move(blob));
    }

    // Step 11: Pack archives (multi-pack chunking at 1.5 GB limit)
    res.packs = builder.BuildPacks("game");

    // Step 12: Generate size report
    res.sizeReport = GenerateSizeReport(bakedAssets);

    res.success = true;
    res.errorCode = core::ErrorCode::OK;
    return res;
}

} // namespace qivforge::export_pipeline

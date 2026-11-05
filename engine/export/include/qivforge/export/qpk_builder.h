#pragma once

#include "qivforge/export/qpk_format.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace qivforge::export_pipeline {

struct InputAssetBlob {
    std::string path;
    std::vector<uint8_t> data;
    QpkAssetType assetType = QpkAssetType::UNKNOWN;
    QpkCodec codec = QpkCodec::STORE;
    uint16_t flags = 0;
    std::string sha256;
    std::string remoteUrl;
};

struct BuiltPack {
    std::string filename; // e.g. "game_0.qpk"
    std::vector<uint8_t> bytes;
    size_t assetCount = 0;
    uint64_t uncompressedBytes = 0;
};

class QpkBuilder {
public:
    QpkBuilder() = default;

    void AddFile(const std::string& path, std::vector<uint8_t> data,
                 QpkAssetType type = QpkAssetType::UNKNOWN,
                 QpkCodec codec = QpkCodec::STORE,
                 uint16_t flags = 0,
                 const std::string& sha256 = "");

    void AddFileString(const std::string& path, const std::string& text,
                       QpkAssetType type = QpkAssetType::UNKNOWN,
                       uint16_t flags = 0);

    void AddRemoteFile(const std::string& path, const std::string& remoteUrl,
                       uint64_t expectedRawSize, const std::string& sha256,
                       QpkAssetType type = QpkAssetType::UNKNOWN);

    [[nodiscard]] size_t GetFileCount() const noexcept { return m_blobs.size(); }
    [[nodiscard]] uint64_t GetTotalUncompressedSize() const noexcept;

    // Builds the archive. If total size > 1.5 GB, splits into game_0.qpk, game_1.qpk, etc.
    [[nodiscard]] std::vector<BuiltPack> BuildPacks(const std::string& baseName = "game");

    // Single pack build helper
    [[nodiscard]] std::vector<uint8_t> BuildSinglePack(const std::vector<InputAssetBlob>& blobs);

    static uint32_t CalculateChecksum(const uint8_t* data, size_t length) noexcept;

private:
    std::vector<InputAssetBlob> m_blobs;
};

} // namespace qivforge::export_pipeline

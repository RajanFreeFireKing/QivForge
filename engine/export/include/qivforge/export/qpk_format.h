#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace qivforge::export_pipeline {

constexpr uint32_t QPK_MAGIC = 0x314B5051; // 'Q' 'P' 'K' '1' little-endian
constexpr uint32_t QPK_FORMAT_VERSION = 1;
constexpr size_t QPK_BLOB_ALIGNMENT = 16;
constexpr uint64_t QPK_MAX_PACK_SIZE = 1500ULL * 1024ULL * 1024ULL; // 1.5 GB limit per Doc 13 §4.1

enum class QpkCodec : uint8_t {
    STORE = 0,
    LZ4   = 1,
    ZSTD  = 2
};

enum class QpkAssetType : uint8_t {
    UNKNOWN = 0,
    SCENE   = 1,
    SCRIPT  = 2,
    PATCH   = 3,
    MESH    = 4,
    TEXTURE = 5,
    AUDIO   = 6,
    TERRAIN = 7,
    NAVMESH = 8,
    CONFIG  = 9
};

enum QpkEntryFlags : uint16_t {
    QPK_FLAG_NONE     = 0x0000,
    QPK_FLAG_REMOTE   = 0x0001, // Asset streamed from cloud per Doc 13 §4.1
    QPK_FLAG_STRIPPED = 0x0002  // Debug line tables stripped in release
};

#pragma pack(push, 1)
struct QpkHeader {
    uint32_t magic = QPK_MAGIC;
    uint32_t version = QPK_FORMAT_VERSION;
    uint64_t indexOffset = 0;
    uint32_t indexSize = 0;
    uint32_t flags = 0;
};

struct QpkIndexEntryBinary {
    uint64_t pathHash = 0;
    uint64_t offset = 0;
    uint64_t compressedSize = 0;
    uint64_t rawSize = 0;
    uint8_t codec = 0;
    uint8_t assetType = 0;
    uint16_t flags = 0;
    uint16_t pathLength = 0;
    uint8_t sha256[32] = {0};
};

struct QpkFooter {
    uint32_t checksum = 0;
};
#pragma pack(pop)

struct QpkEntry {
    uint64_t pathHash = 0;
    std::string path;
    uint64_t offset = 0;
    uint64_t compressedSize = 0;
    uint64_t rawSize = 0;
    QpkCodec codec = QpkCodec::STORE;
    QpkAssetType assetType = QpkAssetType::UNKNOWN;
    uint16_t flags = 0;
    std::string sha256;
    std::string remoteUrl; // When QPK_FLAG_REMOTE is set
};

// 64-bit FNV-1a Hash for fast microsecond asset lookup
[[nodiscard]] inline uint64_t HashPath(const std::string& path) noexcept {
    uint64_t hash = 14695981039346656037ULL;
    for (char c : path) {
        char norm = (c == '\\') ? '/' : (c >= 'A' && c <= 'Z' ? static_cast<char>(c + ('a' - 'A')) : c);
        hash ^= static_cast<uint64_t>(static_cast<unsigned char>(norm));
        hash *= 1099511628211ULL;
    }
    return hash;
}

} // namespace qivforge::export_pipeline

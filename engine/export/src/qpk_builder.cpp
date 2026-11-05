#include "qivforge/export/qpk_builder.h"
#include <algorithm>
#include <cstring>

namespace qivforge::export_pipeline {

uint32_t QpkBuilder::CalculateChecksum(const uint8_t* data, size_t length) noexcept {
    // Standard Adler-32 checksum algorithm
    uint32_t a = 1;
    uint32_t b = 0;
    constexpr uint32_t MOD_ADLER = 65521;

    for (size_t i = 0; i < length; ++i) {
        a = (a + data[i]) % MOD_ADLER;
        b = (b + a) % MOD_ADLER;
    }
    return (b << 16) | a;
}

void QpkBuilder::AddFile(const std::string& path, std::vector<uint8_t> data,
                         QpkAssetType type, QpkCodec codec,
                         uint16_t flags, const std::string& sha256) {
    InputAssetBlob blob;
    blob.path = path;
    blob.data = std::move(data);
    blob.assetType = type;
    blob.codec = codec;
    blob.flags = flags;
    blob.sha256 = sha256;
    m_blobs.push_back(std::move(blob));
}

void QpkBuilder::AddFileString(const std::string& path, const std::string& text,
                               QpkAssetType type, uint16_t flags) {
    std::vector<uint8_t> data(text.begin(), text.end());
    AddFile(path, std::move(data), type, QpkCodec::STORE, flags);
}

void QpkBuilder::AddRemoteFile(const std::string& path, const std::string& remoteUrl,
                               uint64_t expectedRawSize, const std::string& sha256,
                               QpkAssetType type) {
    InputAssetBlob blob;
    blob.path = path;
    blob.remoteUrl = remoteUrl;
    blob.assetType = type;
    blob.codec = QpkCodec::STORE;
    blob.flags = QPK_FLAG_REMOTE;
    blob.sha256 = sha256;
    blob.data.resize(static_cast<size_t>(expectedRawSize)); // Placeholder size
    m_blobs.push_back(std::move(blob));
}

uint64_t QpkBuilder::GetTotalUncompressedSize() const noexcept {
    uint64_t total = 0;
    for (const auto& b : m_blobs) {
        total += b.data.size();
    }
    return total;
}

std::vector<BuiltPack> QpkBuilder::BuildPacks(const std::string& baseName) {
    std::vector<BuiltPack> result;
    if (m_blobs.empty()) {
        BuiltPack emptyPack;
        emptyPack.filename = baseName + ".qpk";
        emptyPack.bytes = BuildSinglePack({});
        emptyPack.assetCount = 0;
        emptyPack.uncompressedBytes = 0;
        result.push_back(std::move(emptyPack));
        return result;
    }

    std::vector<InputAssetBlob> currentChunk;
    uint64_t currentSize = 0;
    int packIndex = 0;

    for (size_t i = 0; i < m_blobs.size(); ++i) {
        uint64_t blobSize = m_blobs[i].data.size();
        if (!currentChunk.empty() && (currentSize + blobSize > QPK_MAX_PACK_SIZE)) {
            BuiltPack pack;
            pack.filename = baseName + "_" + std::to_string(packIndex++) + ".qpk";
            pack.assetCount = currentChunk.size();
            pack.uncompressedBytes = currentSize;
            pack.bytes = BuildSinglePack(currentChunk);
            result.push_back(std::move(pack));

            currentChunk.clear();
            currentSize = 0;
        }

        currentChunk.push_back(m_blobs[i]);
        currentSize += blobSize;
    }

    if (!currentChunk.empty()) {
        BuiltPack pack;
        if (result.empty()) {
            pack.filename = baseName + ".qpk";
        } else {
            pack.filename = baseName + "_" + std::to_string(packIndex) + ".qpk";
        }
        pack.assetCount = currentChunk.size();
        pack.uncompressedBytes = currentSize;
        pack.bytes = BuildSinglePack(currentChunk);
        result.push_back(std::move(pack));
    }

    return result;
}

std::vector<uint8_t> QpkBuilder::BuildSinglePack(const std::vector<InputAssetBlob>& blobs) {
    std::vector<uint8_t> buffer;
    buffer.reserve(1024 * 64);

    // 1. Placeholder Header
    QpkHeader header;
    header.magic = QPK_MAGIC;
    header.version = QPK_FORMAT_VERSION;
    header.flags = 0;
    buffer.resize(sizeof(QpkHeader));
    std::memcpy(buffer.data(), &header, sizeof(QpkHeader));

    std::vector<QpkEntry> entries;
    entries.reserve(blobs.size());

    // 2. Write Data Blobs (aligned to 16 bytes per Doc 13 §4.1)
    for (const auto& blob : blobs) {
        QpkEntry entry;
        entry.pathHash = HashPath(blob.path);
        entry.path = blob.path;
        entry.codec = blob.codec;
        entry.assetType = blob.assetType;
        entry.flags = blob.flags;
        entry.sha256 = blob.sha256;
        entry.remoteUrl = blob.remoteUrl;
        entry.rawSize = blob.data.size();

        if (blob.flags & QPK_FLAG_REMOTE) {
            entry.offset = 0;
            entry.compressedSize = 0;
        } else {
            // Align to 16 bytes
            size_t padding = (QPK_BLOB_ALIGNMENT - (buffer.size() % QPK_BLOB_ALIGNMENT)) % QPK_BLOB_ALIGNMENT;
            for (size_t p = 0; p < padding; ++p) {
                buffer.push_back(0);
            }

            entry.offset = buffer.size();
            entry.compressedSize = blob.data.size(); // In STORE codec
            buffer.insert(buffer.end(), blob.data.begin(), blob.data.end());
        }

        entries.push_back(std::move(entry));
    }

    // 3. Sort entries by pathHash for fast binary search
    std::sort(entries.begin(), entries.end(), [](const QpkEntry& a, const QpkEntry& b) {
        return a.pathHash < b.pathHash;
    });

    // 4. Write Index
    uint64_t indexOffset = buffer.size();
    for (const auto& entry : entries) {
        QpkIndexEntryBinary bin;
        bin.pathHash = entry.pathHash;
        bin.offset = entry.offset;
        bin.compressedSize = entry.compressedSize;
        bin.rawSize = entry.rawSize;
        bin.codec = static_cast<uint8_t>(entry.codec);
        bin.assetType = static_cast<uint8_t>(entry.assetType);
        bin.flags = entry.flags;
        bin.pathLength = static_cast<uint16_t>(entry.path.size());

        // Copy up to 32 bytes of sha256
        size_t shaLen = std::min<size_t>(entry.sha256.size(), 32);
        std::memcpy(bin.sha256, entry.sha256.data(), shaLen);

        size_t prevSize = buffer.size();
        buffer.resize(prevSize + sizeof(QpkIndexEntryBinary));
        std::memcpy(buffer.data() + prevSize, &bin, sizeof(QpkIndexEntryBinary));

        // Write path string
        buffer.insert(buffer.end(), entry.path.begin(), entry.path.end());

        // Write remote url if remote
        if (entry.flags & QPK_FLAG_REMOTE) {
            uint16_t urlLen = static_cast<uint16_t>(entry.remoteUrl.size());
            size_t uPos = buffer.size();
            buffer.resize(uPos + sizeof(uint16_t));
            std::memcpy(buffer.data() + uPos, &urlLen, sizeof(uint16_t));
            buffer.insert(buffer.end(), entry.remoteUrl.begin(), entry.remoteUrl.end());
        }
    }

    uint32_t indexSize = static_cast<uint32_t>(buffer.size() - indexOffset);

    // 5. Compute Checksum of Index
    uint32_t checksum = CalculateChecksum(buffer.data() + indexOffset, indexSize);

    // 6. Write Footer
    QpkFooter footer;
    footer.checksum = checksum;
    size_t fPos = buffer.size();
    buffer.resize(fPos + sizeof(QpkFooter));
    std::memcpy(buffer.data() + fPos, &footer, sizeof(QpkFooter));

    // 7. Update Header with Index metadata
    header.indexOffset = indexOffset;
    header.indexSize = indexSize;
    std::memcpy(buffer.data(), &header, sizeof(QpkHeader));

    return buffer;
}

} // namespace qivforge::export_pipeline

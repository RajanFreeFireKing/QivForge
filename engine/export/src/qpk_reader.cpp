#include "qivforge/export/qpk_reader.h"
#include "qivforge/export/qpk_builder.h"
#include <algorithm>
#include <cstring>

namespace qivforge::export_pipeline {

QpkReadStatus QpkReader::LoadFromVector(std::vector<uint8_t> bytes) {
    m_ownedData = std::move(bytes);
    return LoadFromBuffer(m_ownedData.data(), m_ownedData.size());
}

QpkReadStatus QpkReader::LoadFromBuffer(const uint8_t* data, size_t size) {
    m_entries.clear();
    m_loaded = false;
    m_data = data;
    m_size = size;

    if (!data || size < sizeof(QpkHeader) + sizeof(QpkFooter)) {
        m_lastStatus = QpkReadStatus::FILE_TRUNCATED;
        return m_lastStatus;
    }

    // 1. Validate Header
    std::memcpy(&m_header, data, sizeof(QpkHeader));
    if (m_header.magic != QPK_MAGIC) {
        m_lastStatus = QpkReadStatus::CORRUPT_MAGIC;
        return m_lastStatus;
    }

    if (m_header.version != QPK_FORMAT_VERSION) {
        m_lastStatus = QpkReadStatus::UNSUPPORTED_VERSION;
        return m_lastStatus;
    }

    // 2. Validate Index Offset and Bounds
    if (m_header.indexOffset < sizeof(QpkHeader) ||
        m_header.indexOffset + m_header.indexSize + sizeof(QpkFooter) > size) {
        m_lastStatus = QpkReadStatus::CORRUPT_INDEX_OFFSET;
        return m_lastStatus;
    }

    // 3. Validate Footer & Index Checksum (Doc 13 AC9 corruption defense)
    const uint8_t* indexPtr = data + m_header.indexOffset;
    uint32_t computedChecksum = QpkBuilder::CalculateChecksum(indexPtr, m_header.indexSize);

    QpkFooter footer;
    std::memcpy(&footer, data + m_header.indexOffset + m_header.indexSize, sizeof(QpkFooter));
    if (footer.checksum != computedChecksum) {
        m_lastStatus = QpkReadStatus::CHECKSUM_MISMATCH;
        return m_lastStatus;
    }

    // 4. Parse Index Entries
    size_t cursor = 0;
    while (cursor < m_header.indexSize) {
        if (cursor + sizeof(QpkIndexEntryBinary) > m_header.indexSize) {
            m_lastStatus = QpkReadStatus::FILE_TRUNCATED;
            return m_lastStatus;
        }

        QpkIndexEntryBinary bin;
        std::memcpy(&bin, indexPtr + cursor, sizeof(QpkIndexEntryBinary));
        cursor += sizeof(QpkIndexEntryBinary);

        if (cursor + bin.pathLength > m_header.indexSize) {
            m_lastStatus = QpkReadStatus::FILE_TRUNCATED;
            return m_lastStatus;
        }

        std::string path(reinterpret_cast<const char*>(indexPtr + cursor), bin.pathLength);
        cursor += bin.pathLength;

        std::string remoteUrl;
        if (bin.flags & QPK_FLAG_REMOTE) {
            if (cursor + sizeof(uint16_t) > m_header.indexSize) {
                m_lastStatus = QpkReadStatus::FILE_TRUNCATED;
                return m_lastStatus;
            }
            uint16_t urlLen = 0;
            std::memcpy(&urlLen, indexPtr + cursor, sizeof(uint16_t));
            cursor += sizeof(uint16_t);

            if (cursor + urlLen > m_header.indexSize) {
                m_lastStatus = QpkReadStatus::FILE_TRUNCATED;
                return m_lastStatus;
            }
            remoteUrl.assign(reinterpret_cast<const char*>(indexPtr + cursor), urlLen);
            cursor += urlLen;
        }

        // Validate data offset bounds
        if (!(bin.flags & QPK_FLAG_REMOTE)) {
            if (bin.offset + bin.compressedSize > m_header.indexOffset) {
                m_lastStatus = QpkReadStatus::CORRUPT_INDEX_OFFSET;
                return m_lastStatus;
            }
        }

        QpkEntry entry;
        entry.pathHash = bin.pathHash;
        entry.path = std::move(path);
        entry.offset = bin.offset;
        entry.compressedSize = bin.compressedSize;
        entry.rawSize = bin.rawSize;
        entry.codec = static_cast<QpkCodec>(bin.codec);
        entry.assetType = static_cast<QpkAssetType>(bin.assetType);
        entry.flags = bin.flags;
        entry.sha256 = std::string(reinterpret_cast<const char*>(bin.sha256), 32);
        entry.remoteUrl = std::move(remoteUrl);

        m_entries.push_back(std::move(entry));
    }

    m_loaded = true;
    m_lastStatus = QpkReadStatus::OK;
    return m_lastStatus;
}

const QpkEntry* QpkReader::FindEntry(uint64_t pathHash) const noexcept {
    if (!m_loaded || m_entries.empty()) return nullptr;

    auto it = std::lower_bound(m_entries.begin(), m_entries.end(), pathHash,
        [](const QpkEntry& item, uint64_t val) {
            return item.pathHash < val;
        });

    if (it != m_entries.end() && it->pathHash == pathHash) {
        return &(*it);
    }
    return nullptr;
}

const QpkEntry* QpkReader::FindEntry(const std::string& path) const noexcept {
    return FindEntry(HashPath(path));
}

std::vector<uint8_t> QpkReader::ReadAssetData(const QpkEntry& entry) const {
    if (!m_loaded || !m_data) return {};
    if (entry.flags & QPK_FLAG_REMOTE) return {}; // Remote must be fetched via streamer

    if (entry.offset + entry.compressedSize > m_size) {
        return {};
    }

    std::vector<uint8_t> out(entry.compressedSize);
    std::memcpy(out.data(), m_data + entry.offset, entry.compressedSize);
    return out;
}

std::string QpkReader::ReadAssetText(const std::string& path) const {
    const auto* entry = FindEntry(path);
    if (!entry) return "";

    auto data = ReadAssetData(*entry);
    return std::string(data.begin(), data.end());
}

std::string QpkReader::GetStatusString() const noexcept {
    switch (m_lastStatus) {
        case QpkReadStatus::OK: return "OK";
        case QpkReadStatus::CORRUPT_MAGIC: return "QF-BUILD-005: Corrupt pack magic";
        case QpkReadStatus::UNSUPPORTED_VERSION: return "QF-BUILD-006: Unsupported pack version";
        case QpkReadStatus::CORRUPT_INDEX_OFFSET: return "QF-BUILD-005: Corrupt index offset";
        case QpkReadStatus::CHECKSUM_MISMATCH: return "QF-BUILD-005: Pack checksum mismatch (corrupted data)";
        case QpkReadStatus::FILE_TRUNCATED: return "QF-BUILD-005: Pack file is truncated";
        case QpkReadStatus::NOT_FOUND: return "Pack not loaded";
    }
    return "Unknown error";
}

} // namespace qivforge::export_pipeline

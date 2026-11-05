#pragma once

#include "qivforge/export/qpk_format.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace qivforge::export_pipeline {

enum class QpkReadStatus {
    OK = 0,
    CORRUPT_MAGIC = 1,
    UNSUPPORTED_VERSION = 2,
    CORRUPT_INDEX_OFFSET = 3,
    CHECKSUM_MISMATCH = 4,
    FILE_TRUNCATED = 5,
    NOT_FOUND = 6
};

class QpkReader {
public:
    QpkReader() = default;

    // Load from memory buffer (memory-mapped file or in-memory)
    [[nodiscard]] QpkReadStatus LoadFromBuffer(const uint8_t* data, size_t size);
    [[nodiscard]] QpkReadStatus LoadFromVector(std::vector<uint8_t> bytes);

    [[nodiscard]] bool IsLoaded() const noexcept { return m_loaded; }
    [[nodiscard]] size_t GetAssetCount() const noexcept { return m_entries.size(); }
    [[nodiscard]] const std::vector<QpkEntry>& GetEntries() const noexcept { return m_entries; }

    // Fast O(log N) lookup by path hash (<1 microsecond)
    [[nodiscard]] const QpkEntry* FindEntry(uint64_t pathHash) const noexcept;
    [[nodiscard]] const QpkEntry* FindEntry(const std::string& path) const noexcept;

    // Read asset bytes safely
    [[nodiscard]] std::vector<uint8_t> ReadAssetData(const QpkEntry& entry) const;
    [[nodiscard]] std::string ReadAssetText(const std::string& path) const;

    [[nodiscard]] std::string GetStatusString() const noexcept;

private:
    std::vector<uint8_t> m_ownedData;
    const uint8_t* m_data = nullptr;
    size_t m_size = 0;
    bool m_loaded = false;
    QpkReadStatus m_lastStatus = QpkReadStatus::NOT_FOUND;

    QpkHeader m_header;
    std::vector<QpkEntry> m_entries;
};

} // namespace qivforge::export_pipeline

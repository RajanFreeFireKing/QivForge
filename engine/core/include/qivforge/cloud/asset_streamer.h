#pragma once

#include "qivforge/cloud/hf_client.h"
#include "qivforge/cloud/sha256.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <memory>
#include <cstdint>

namespace qivforge::cloud {

struct StreamedAsset {
    std::string path;
    std::string sha256;
    std::string data;
    bool isFromCache = false;
    bool isPlaceholder = false;
};

class LruDiskCache {
public:
    explicit LruDiskCache(size_t maxBytes = 2ULL * 1024 * 1024 * 1024); // 2 GB default

    bool Has(const std::string& sha256) const;
    std::optional<std::string> Get(const std::string& sha256);
    void Put(const std::string& sha256, const std::string& data);

    void Pin(const std::string& sha256);
    void Unpin(const std::string& sha256);

    [[nodiscard]] size_t GetCurrentBytes() const noexcept { return m_currentBytes; }
    [[nodiscard]] size_t GetMaxBytes() const noexcept { return m_maxBytes; }
    void SetMaxBytes(size_t maxBytes);
    void Clear();

private:
    void EvictIfNeeded();

    size_t m_maxBytes;
    size_t m_currentBytes = 0;
    std::list<std::pair<std::string, std::string>> m_lruList; // pair: <sha256, data>
    std::unordered_map<std::string, std::list<std::pair<std::string, std::string>>::iterator> m_cacheMap;
    std::unordered_set<std::string> m_pinned;
};

class AssetStreamer {
public:
    AssetStreamer(std::shared_ptr<HfClient> client, std::shared_ptr<LruDiskCache> cache = nullptr);

    void SetNetworkAvailable(bool available) noexcept { m_networkAvailable = available; }
    [[nodiscard]] bool IsNetworkAvailable() const noexcept { return m_networkAvailable; }

    [[nodiscard]] std::pair<core::ErrorCode, StreamedAsset> RequestAsset(
        const std::string& repoId,
        const std::string& path,
        const std::string& expectedSha256,
        size_t rangeStart = 0,
        size_t rangeEnd = 0
    );

    [[nodiscard]] std::shared_ptr<LruDiskCache> GetCache() const noexcept { return m_cache; }

private:
    std::shared_ptr<HfClient> m_client;
    std::shared_ptr<LruDiskCache> m_cache;
    bool m_networkAvailable = true;
};

} // namespace qivforge::cloud

#include "qivforge/cloud/asset_streamer.h"

namespace qivforge::cloud {

LruDiskCache::LruDiskCache(size_t maxBytes)
    : m_maxBytes(maxBytes) {}

bool LruDiskCache::Has(const std::string& sha256) const {
    return m_cacheMap.find(sha256) != m_cacheMap.end();
}

std::optional<std::string> LruDiskCache::Get(const std::string& sha256) {
    auto it = m_cacheMap.find(sha256);
    if (it == m_cacheMap.end()) return std::nullopt;

    // Move to front (most recently used)
    m_lruList.splice(m_lruList.begin(), m_lruList, it->second);
    return it->second->second;
}

void LruDiskCache::Put(const std::string& sha256, const std::string& data) {
    auto it = m_cacheMap.find(sha256);
    if (it != m_cacheMap.end()) {
        m_currentBytes -= it->second->second.size();
        m_lruList.erase(it->second);
        m_cacheMap.erase(it);
    }

    m_lruList.push_front({sha256, data});
    m_cacheMap[sha256] = m_lruList.begin();
    m_currentBytes += data.size();

    EvictIfNeeded();
}

void LruDiskCache::Pin(const std::string& sha256) {
    m_pinned.insert(sha256);
}

void LruDiskCache::Unpin(const std::string& sha256) {
    m_pinned.erase(sha256);
}

void LruDiskCache::SetMaxBytes(size_t maxBytes) {
    m_maxBytes = maxBytes;
    EvictIfNeeded();
}

void LruDiskCache::Clear() {
    m_lruList.clear();
    m_cacheMap.clear();
    m_pinned.clear();
    m_currentBytes = 0;
}

void LruDiskCache::EvictIfNeeded() {
    while (m_currentBytes > m_maxBytes && !m_lruList.empty()) {
        // Find least recently used item from back that is NOT pinned
        auto it = m_lruList.rbegin();
        bool evicted = false;

        while (it != m_lruList.rend()) {
            if (m_pinned.find(it->first) == m_pinned.end()) {
                // Not pinned: evict
                m_currentBytes -= it->second.size();
                std::string key = it->first;
                m_cacheMap.erase(key);
                m_lruList.erase(std::next(it).base());
                evicted = true;
                break;
            }
            ++it;
        }

        if (!evicted) {
            // All remaining cached assets are pinned! Cannot evict further.
            break;
        }
    }
}

AssetStreamer::AssetStreamer(std::shared_ptr<HfClient> client, std::shared_ptr<LruDiskCache> cache)
    : m_client(std::move(client)),
      m_cache(cache ? std::move(cache) : std::make_shared<LruDiskCache>()) {}

std::pair<core::ErrorCode, StreamedAsset> AssetStreamer::RequestAsset(
    const std::string& repoId,
    const std::string& path,
    const std::string& expectedSha256,
    size_t rangeStart,
    size_t rangeEnd
) {
    StreamedAsset asset;
    asset.path = path;
    asset.sha256 = expectedSha256;

    // 1. Check disk cache
    if (!expectedSha256.empty()) {
        auto cached = m_cache->Get(expectedSha256);
        if (cached.has_value()) {
            asset.data = std::move(cached.value());
            asset.isFromCache = true;
            return {core::ErrorCode::OK, asset};
        }
    }

    // 2. Offline check (Doc 11 §4.4)
    if (!m_networkAvailable) {
        asset.isPlaceholder = true;
        // Return lightweight placeholder payload without crashing
        asset.data = "{\"placeholder\":true,\"type\":\"mesh_cube\"}";
        return {core::ErrorCode::ERR_CLOUD_NETWORK_UNAVAILABLE, asset};
    }

    // 3. Network fetch
    if (!m_client) {
        return {core::ErrorCode::ERR_CLOUD_NETWORK_UNAVAILABLE, asset};
    }

    auto [err, data] = m_client->ReadFile(repoId, path, "main", rangeStart, rangeEnd);
    if (err != core::ErrorCode::OK) {
        asset.isPlaceholder = true;
        return {err, asset};
    }

    // 4. SHA-256 Hash verification (Doc 11 §4.2 step 5 & §6 QF-CLOUD-004)
    if (!expectedSha256.empty() && rangeStart == 0 && rangeEnd == 0) {
        std::string computedHash = Sha256::Compute(data);
        if (computedHash != expectedSha256) {
            return {core::ErrorCode::ERR_CLOUD_HASH_MISMATCH, asset};
        }
    }

    // 5. Store in LRU cache
    if (!expectedSha256.empty()) {
        m_cache->Put(expectedSha256, data);
    }

    asset.data = std::move(data);
    asset.isFromCache = false;
    asset.isPlaceholder = false;
    return {core::ErrorCode::OK, asset};
}

} // namespace qivforge::cloud

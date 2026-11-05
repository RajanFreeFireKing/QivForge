#include "qivforge/cloud/asset_streamer.h"
#include <iostream>
#include <cassert>

using namespace qivforge::cloud;
using namespace qivforge::core;

class MockStreamerTransport : public IHttpTransport {
public:
    HttpResponse Request(const std::string& method,
                         const std::string& url,
                         const std::vector<std::pair<std::string, std::string>>& headers,
                         const std::string& body) override {
        (void)method; (void)headers; (void)body;
        HttpResponse resp;
        resp.statusCode = 200;

        if (url.find("corrupt.glb") != std::string::npos) {
            resp.body = "CORRUPT_BYTES";
        } else {
            resp.body = "VALID_GLB_PAYLOAD_CHUNK_DATA_1234567890";
        }
        return resp;
    }
};

void TestLruDiskCacheEvictionAndPinning() {
    std::cout << "[RUN] TestLruDiskCacheEvictionAndPinning (Doc 11 §4.3)..." << std::endl;

    // Small cache: 100 bytes limit
    LruDiskCache cache(100);

    std::string dataA(30, 'A');
    std::string dataB(30, 'B');
    std::string dataC(30, 'C');
    std::string dataD(30, 'D');

    cache.Put("hashA", dataA);
    cache.Put("hashB", dataB);
    cache.Put("hashC", dataC);
    assert(cache.GetCurrentBytes() == 90);

    // Pin hashA (active in current scene per Doc 11 §4.3)
    cache.Pin("hashA");

    // Add dataD (30 bytes). Total would be 120 > 100.
    // hashB is the least recently used UNPINNED item, so hashB must be evicted!
    cache.Put("hashD", dataD);

    assert(cache.Has("hashA")); // Pinned: kept!
    assert(!cache.Has("hashB")); // Unpinned: evicted!
    assert(cache.Has("hashC"));
    assert(cache.Has("hashD"));

    std::cout << "[PASS] TestLruDiskCacheEvictionAndPinning" << std::endl;
}

void TestAssetStreamerHashVerificationAndOfflineFallback() {
    std::cout << "[RUN] TestAssetStreamerHashVerificationAndOfflineFallback (Doc 11 §4.4 AC7 & QF-CLOUD-004)..." << std::endl;

    auto transport = std::make_shared<MockStreamerTransport>();
    auto client = std::make_shared<HfClient>(transport);
    AssetStreamer streamer(client);

    std::string validData = "VALID_GLB_PAYLOAD_CHUNK_DATA_1234567890";
    std::string validSha256 = Sha256::Compute(validData);

    // 1. Request with matching hash -> succeeds and populates cache
    auto [err1, asset1] = streamer.RequestAsset("qivora/test-game", "assets/cube.glb", validSha256);
    assert(err1 == ErrorCode::OK);
    assert(asset1.data == validData);
    assert(!asset1.isFromCache);
    assert(!asset1.isPlaceholder);

    // 2. Second request -> Cache hit!
    auto [err2, asset2] = streamer.RequestAsset("qivora/test-game", "assets/cube.glb", validSha256);
    assert(err2 == ErrorCode::OK);
    assert(asset2.isFromCache);
    assert(asset2.data == validData);

    // 3. Hash mismatch -> returns QF-CLOUD-004
    std::string expectedDiffHash = Sha256::Compute("EXPECTED_DIFFERENT_CONTENT");
    auto [errMismatch, assetMismatch] = streamer.RequestAsset("qivora/test-game", "assets/corrupt.glb", expectedDiffHash);
    assert(errMismatch == ErrorCode::ERR_CLOUD_HASH_MISMATCH);
    assert(std::string(GetDocErrorCode(errMismatch)) == "QF-CLOUD-004");

    // 4. Offline mode (Doc 11 §4.4 & AC7: Disabling network returns low-poly placeholder without crashing)
    streamer.SetNetworkAvailable(false);
    auto [errOffline, assetOffline] = streamer.RequestAsset("qivora/test-game", "assets/uncached_mountain.glb", "hash_uncached");
    assert(errOffline == ErrorCode::ERR_CLOUD_NETWORK_UNAVAILABLE);
    assert(assetOffline.isPlaceholder);
    assert(assetOffline.data.find("placeholder") != std::string::npos);

    std::cout << "[PASS] TestAssetStreamerHashVerificationAndOfflineFallback" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  STREAMING & LRU CACHE TEST SUITE (M7) " << std::endl;
    std::cout << "========================================" << std::endl;

    TestLruDiskCacheEvictionAndPinning();
    TestAssetStreamerHashVerificationAndOfflineFallback();

    std::cout << "ALL STREAMING & LRU CACHE TESTS PASSED!" << std::endl;
    return 0;
}

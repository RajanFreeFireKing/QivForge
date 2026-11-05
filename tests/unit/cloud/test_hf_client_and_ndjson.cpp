#include "qivforge/cloud/hf_client.h"
#include "qivforge/cloud/ndjson_builder.h"
#include <iostream>
#include <cassert>

using namespace qivforge::cloud;
using namespace qivforge::core;

class MockHttpTransport : public IHttpTransport {
public:
    HttpResponse Request(const std::string& method,
                         const std::string& url,
                         const std::vector<std::pair<std::string, std::string>>& headers,
                         const std::string& body) override {
        lastMethod = method;
        lastUrl = url;
        lastHeaders = headers;
        lastBody = body;

        HttpResponse resp;
        if (triggerRateLimit) {
            resp.statusCode = 429;
            resp.retryAfterHeader = "30";
            resp.body = "{\"error\":\"Too many requests\"}";
            return resp;
        }

        if (triggerBranchMoved) {
            resp.statusCode = 409;
            resp.body = "{\"error\":\"Parent commit does not match remote head, branch moved\"}";
            return resp;
        }

        if (url.find("/api/whoami-v2") != std::string::npos) {
            resp.statusCode = 200;
            resp.body = "{\"name\":\"qivora-creator\",\"email\":\"creator@qivora.io\",\"avatarUrl\":\"https://hf.co/avatar.png\"}";
            return resp;
        }

        if (url.find("/api/repos/create") != std::string::npos) {
            resp.statusCode = 200;
            resp.body = "{\"name\":\"qf-tractor-sim\",\"private\":true}";
            return resp;
        }

        if (url.find("/tree/main") != std::string::npos) {
            resp.statusCode = 200;
            resp.body = "[{\"path\":\"scenes/main.scene.json\",\"type\":\"file\",\"size\":1024,\"oid\":\"a1b2c3d4\"},"
                        "{\"path\":\"assets/tractor.glb\",\"type\":\"file\",\"size\":2048576,\"oid\":\"e5f6a7b8\"}]";
            return resp;
        }

        if (url.find("/resolve/main/") != std::string::npos) {
            resp.statusCode = 200;
            resp.body = "{\"scene\":\"content_data\"}";
            return resp;
        }

        if (url.find("/commit/main") != std::string::npos) {
            resp.statusCode = 200;
            resp.body = "{\"success\":true,\"commitId\":\"c0ffee1234567890\"}";
            return resp;
        }

        if (url.find("/commits/main") != std::string::npos) {
            resp.statusCode = 200;
            resp.body = "[{\"commitId\":\"c0ffee123\",\"title\":\"Initial sync\"}]";
            return resp;
        }

        if (url.find("cdn-lfs.huggingface.co") != std::string::npos) {
            resp.statusCode = 200;
            resp.body = "OK";
            return resp;
        }

        resp.statusCode = 200;
        resp.body = "{}";
        return resp;
    }

    std::string lastMethod;
    std::string lastUrl;
    std::vector<std::pair<std::string, std::string>> lastHeaders;
    std::string lastBody;
    bool triggerRateLimit = false;
    bool triggerBranchMoved = false;
};

void TestNdjsonBuilderAndParser() {
    std::cout << "[RUN] TestNdjsonBuilderAndParser (Doc 11 §3.4)..." << std::endl;

    NdjsonCommitBuilder builder;
    builder.SetHeader("Update tractor scene", "Changed tractor wheel radius", "sha_parent_001");
    builder.AddInlineFile("scenes/main.scene.json", "{\"wheelRadius\":0.65}");
    builder.AddLfsFile("assets/models/tractor.glb", "d41d8cd98f00b204e9800998ecf8427e", 18234567);
    builder.AddDeletedFile("scenes/old_map.scene.json");

    assert(builder.GetOperationCount() == 3);
    assert(!builder.ShouldSplit());

    std::string ndjson = builder.Build();
    assert(ndjson.find("\"key\":\"header\"") != std::string::npos);
    assert(ndjson.find("\"key\":\"file\"") != std::string::npos);
    assert(ndjson.find("\"key\":\"lfsFile\"") != std::string::npos);
    assert(ndjson.find("\"key\":\"deletedFile\"") != std::string::npos);

    // Verify round-trip parsing
    CommitHeader parsedHeader;
    std::vector<CommitFileOp> parsedOps;
    bool parseOk = NdjsonCommitBuilder::Parse(ndjson, parsedHeader, parsedOps);
    assert(parseOk);
    assert(parsedHeader.summary == "Update tractor scene");
    assert(parsedHeader.parentCommit == "sha_parent_001");
    assert(parsedOps.size() == 3);
    assert(parsedOps[0].path == "scenes/main.scene.json");
    assert(parsedOps[0].type == CommitFileOp::Type::INLINE_FILE);
    assert(NdjsonCommitBuilder::Base64Decode(parsedOps[0].base64Content) == "{\"wheelRadius\":0.65}");
    assert(parsedOps[1].path == "assets/models/tractor.glb");
    assert(parsedOps[1].type == CommitFileOp::Type::LFS_FILE);
    assert(parsedOps[1].size == 18234567);
    assert(parsedOps[2].path == "scenes/old_map.scene.json");
    assert(parsedOps[2].type == CommitFileOp::Type::DELETED_FILE);

    std::cout << "[PASS] TestNdjsonBuilderAndParser" << std::endl;
}

void TestHfClientApiAndSecurity() {
    std::cout << "[RUN] TestHfClientApiAndSecurity..." << std::endl;

    auto transport = std::make_shared<MockHttpTransport>();
    HfClient client(transport);
    client.SetToken("hf_SuperSecretToken12345678");

    // 1. WhoAmI
    auto [errWho, user] = client.WhoAmI();
    assert(errWho == ErrorCode::OK);
    assert(user.isValid);
    assert(user.username == "qivora-creator");
    assert(user.email == "creator@qivora.io");

    // Verify authorization header was sent
    bool authFound = false;
    for (const auto& [k, v] : transport->lastHeaders) {
        if (k == "Authorization") {
            assert(v == "Bearer hf_SuperSecretToken12345678");
            authFound = true;
        }
    }
    assert(authFound);

    // 2. CreateRepo
    auto errCreate = client.CreateRepo("qf-tractor-sim", RepoType::DATASET, true);
    assert(errCreate == ErrorCode::OK);

    // 3. ListTree
    auto [errTree, tree] = client.ListTree("qivora-creator/qf-tractor-sim");
    assert(errTree == ErrorCode::OK);
    assert(tree.size() == 2);
    assert(tree[0].path == "scenes/main.scene.json");
    assert(tree[1].path == "assets/tractor.glb");

    // 4. Rate limit mapping QF-CLOUD-003
    transport->triggerRateLimit = true;
    auto errRate = client.CreateRepo("qf-test");
    assert(errRate == ErrorCode::ERR_CLOUD_RATE_LIMITED);
    assert(std::string(GetDocErrorCode(errRate)) == "QF-CLOUD-003");
    transport->triggerRateLimit = false;

    // 5. Branch moved conflict mapping QF-CLOUD-006
    transport->triggerBranchMoved = true;
    auto errBranch = client.CommitNdjson("qivora-creator/qf-tractor-sim", "{}");
    assert(errBranch == ErrorCode::ERR_CLOUD_BRANCH_MOVED);
    assert(std::string(GetDocErrorCode(errBranch)) == "QF-CLOUD-006");
    transport->triggerBranchMoved = false;

    // 6. Security: Token stripped on cross-domain redirect (Doc 14 §3)
    (void)client.UploadLfsObject("qivora-creator/qf-tractor-sim", "sha123", 1024, "binary");
    // Verify CDN PUT request did NOT include Authorization
    if (transport->lastUrl.find("cdn-lfs.huggingface.co") != std::string::npos) {
        for (const auto& [k, _] : transport->lastHeaders) {
            assert(k != "Authorization"); // Must be stripped on cross-domain CDN!
        }
    }

    // 7. Security: SanitizeAuthHeaders redacts secrets
    std::string sanitized = HfClient::SanitizeAuthHeaders(transport->lastHeaders);
    assert(sanitized.find("hf_SuperSecretToken12345678") == std::string::npos);

    std::cout << "[PASS] TestHfClientApiAndSecurity" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  HF CLIENT & NDJSON TEST SUITE (M7)    " << std::endl;
    std::cout << "========================================" << std::endl;

    TestNdjsonBuilderAndParser();
    TestHfClientApiAndSecurity();

    std::cout << "ALL HF CLIENT & NDJSON TESTS PASSED!" << std::endl;
    return 0;
}

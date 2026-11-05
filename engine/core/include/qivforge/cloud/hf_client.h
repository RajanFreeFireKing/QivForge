#pragma once

#include "qivforge/cloud/hf_types.h"
#include "qivforge/core/error_codes.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace qivforge::cloud {

class IHttpTransport {
public:
    virtual ~IHttpTransport() = default;
    virtual HttpResponse Request(const std::string& method,
                                const std::string& url,
                                const std::vector<std::pair<std::string, std::string>>& headers,
                                const std::string& body) = 0;
};

class HfClient {
public:
    explicit HfClient(std::shared_ptr<IHttpTransport> transport = nullptr);

    void SetToken(const std::string& token);
    [[nodiscard]] bool HasToken() const noexcept { return !m_token.empty(); }

    // API operations (Doc 11 §3.3)
    [[nodiscard]] std::pair<core::ErrorCode, HfUser> WhoAmI();
    [[nodiscard]] core::ErrorCode CreateRepo(const std::string& repoName, RepoType type = RepoType::DATASET, bool isPrivate = true);
    [[nodiscard]] std::pair<core::ErrorCode, std::vector<RepoTreeEntry>> ListTree(const std::string& repoId, const std::string& revision = "main");
    [[nodiscard]] std::pair<core::ErrorCode, std::string> ReadFile(const std::string& repoId, const std::string& path, const std::string& revision = "main", size_t rangeStart = 0, size_t rangeEnd = 0);
    [[nodiscard]] std::pair<core::ErrorCode, std::vector<PreUploadDecision>> PreUploadCheck(const std::string& repoId, const std::vector<PreUploadFile>& files, const std::string& revision = "main");
    [[nodiscard]] core::ErrorCode CommitNdjson(const std::string& repoId, const std::string& ndjsonContent, const std::string& revision = "main");
    [[nodiscard]] std::pair<core::ErrorCode, std::vector<CommitInfo>> GetCommits(const std::string& repoId, const std::string& revision = "main");

    // LFS upload flow
    [[nodiscard]] core::ErrorCode UploadLfsObject(const std::string& repoId, const std::string& sha256Oid, size_t sizeBytes, const std::string& binaryData);

    // Helpers
    static std::string SanitizeAuthHeaders(const std::vector<std::pair<std::string, std::string>>& headers);

private:
    [[nodiscard]] std::vector<std::pair<std::string, std::string>> BuildHeaders(const std::string& targetUrl, const std::string& contentType = "application/json") const;

    std::shared_ptr<IHttpTransport> m_transport;
    std::string m_token;
    std::string m_baseUrl = "https://huggingface.co";
};

} // namespace qivforge::cloud

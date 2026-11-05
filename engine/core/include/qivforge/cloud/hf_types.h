#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <optional>

namespace qivforge::cloud {

enum class RepoType {
    DATASET,
    MODEL
};

inline const char* RepoTypeToString(RepoType type) noexcept {
    return (type == RepoType::DATASET) ? "dataset" : "model";
}

struct HfUser {
    std::string username;
    std::string email;
    std::string avatarUrl;
    std::vector<std::string> orgs;
    bool isValid = false;
};

struct RepoTreeEntry {
    std::string path;
    std::string type; // "file" or "directory"
    size_t size = 0;
    std::string oid; // sha256 or commit oid
    bool isLfs = false;
};

struct PreUploadFile {
    std::string path;
    size_t size = 0;
    std::string sample; // initial bytes
};

struct PreUploadDecision {
    std::string path;
    std::string uploadMode; // "regular" or "lfs"
};

struct LfsObjectRequest {
    std::string oid; // SHA-256 hex string
    size_t size = 0;
};

struct LfsUploadAction {
    std::string href;
    std::string headerAuth;
};

struct LfsObjectResponse {
    std::string oid;
    size_t size = 0;
    bool authenticated = false;
    std::optional<LfsUploadAction> upload;
    std::optional<LfsUploadAction> verify;
};

struct CommitHeader {
    std::string summary;
    std::string description;
    std::string parentCommit;
};

struct CommitFileOp {
    enum class Type { INLINE_FILE, LFS_FILE, DELETED_FILE };
    Type type = Type::INLINE_FILE;
    std::string path;
    std::string base64Content;
    std::string oid;
    size_t size = 0;
};

struct CommitInfo {
    std::string commitId;
    std::string message;
    std::string author;
    std::string date;
    size_t fileCount = 0;
};

struct HttpResponse {
    int statusCode = 0;
    std::string body;
    std::string retryAfterHeader;
    bool isSuccess() const noexcept { return statusCode >= 200 && statusCode < 300; }
};

} // namespace qivforge::cloud

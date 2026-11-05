#pragma once

#include "qivforge/cloud/hf_types.h"
#include <string>
#include <vector>

namespace qivforge::cloud {

class NdjsonCommitBuilder {
public:
    NdjsonCommitBuilder() = default;

    void SetHeader(const std::string& summary, const std::string& description, const std::string& parentCommit);
    void AddInlineFile(const std::string& path, const std::string& rawContent);
    void AddInlineFileBase64(const std::string& path, const std::string& base64Content);
    void AddLfsFile(const std::string& path, const std::string& sha256Oid, size_t sizeBytes);
    void AddDeletedFile(const std::string& path);

    [[nodiscard]] std::string Build() const;
    [[nodiscard]] size_t GetOperationCount() const noexcept { return m_operations.size(); }
    [[nodiscard]] size_t GetInlineBytes() const noexcept { return m_inlineBytes; }
    [[nodiscard]] bool ShouldSplit() const noexcept { return m_operations.size() >= 100 || m_inlineBytes >= (50 * 1024 * 1024); }

    void Clear();

    // Helpers
    static std::string Base64Encode(const std::string& in);
    static std::string Base64Decode(const std::string& in);

    // Static parser for tests & verification
    static bool Parse(const std::string& ndjson, CommitHeader& outHeader, std::vector<CommitFileOp>& outOps);

private:
    CommitHeader m_header;
    std::vector<CommitFileOp> m_operations;
    size_t m_inlineBytes = 0;
};

} // namespace qivforge::cloud

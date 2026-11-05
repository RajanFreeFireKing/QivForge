#pragma once

#include "qivforge/scene/local_store.h"
#include <unordered_set>

namespace qivforge::scene {

class HfStore : public LocalStore {
public:
    HfStore(std::string projectRoot, std::string repoId, std::string revision = "main");

    core::ErrorCode WriteAsset(const std::string& path, const std::string& content) override;
    core::ErrorCode DeleteAsset(const std::string& path) override;

    bool IsCloudMode() const noexcept override { return true; }
    bool HasPendingChanges() const noexcept override { return !m_pendingFiles.empty() || !m_pendingDeletions.empty(); }
    size_t GetPendingCount() const noexcept override { return m_pendingFiles.size() + m_pendingDeletions.size(); }

    [[nodiscard]] const std::string& GetRepoId() const noexcept { return m_repoId; }
    [[nodiscard]] const std::string& GetRevision() const noexcept { return m_revision; }
    [[nodiscard]] const std::string& GetLastSyncedCommit() const noexcept { return m_lastSyncedCommit; }
    void SetLastSyncedCommit(std::string commitId) noexcept { m_lastSyncedCommit = std::move(commitId); }

    [[nodiscard]] std::vector<std::string> GetPendingFiles() const;
    [[nodiscard]] std::vector<std::string> GetPendingDeletions() const;
    void ClearPending() noexcept;

private:
    std::string m_repoId;
    std::string m_revision;
    std::string m_lastSyncedCommit;
    std::unordered_set<std::string> m_pendingFiles;
    std::unordered_set<std::string> m_pendingDeletions;
};

} // namespace qivforge::scene

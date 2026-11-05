#include "qivforge/scene/hf_store.h"

namespace qivforge::scene {

HfStore::HfStore(std::string projectRoot, std::string repoId, std::string revision)
    : LocalStore(std::move(projectRoot)),
      m_repoId(std::move(repoId)),
      m_revision(std::move(revision)) {}

core::ErrorCode HfStore::WriteAsset(const std::string& path, const std::string& content) {
    // Write locally first
    auto err = LocalStore::WriteAsset(path, content);
    if (err != core::ErrorCode::OK) return err;

    // Track as pending for explicit cloud Sync (Rule R4: no background sync!)
    m_pendingFiles.insert(path);
    m_pendingDeletions.erase(path);
    return core::ErrorCode::OK;
}

core::ErrorCode HfStore::DeleteAsset(const std::string& path) {
    auto err = LocalStore::DeleteAsset(path);
    if (err != core::ErrorCode::OK) return err;

    m_pendingFiles.erase(path);
    m_pendingDeletions.insert(path);
    return core::ErrorCode::OK;
}

std::vector<std::string> HfStore::GetPendingFiles() const {
    return std::vector<std::string>(m_pendingFiles.begin(), m_pendingFiles.end());
}

std::vector<std::string> HfStore::GetPendingDeletions() const {
    return std::vector<std::string>(m_pendingDeletions.begin(), m_pendingDeletions.end());
}

void HfStore::ClearPending() noexcept {
    m_pendingFiles.clear();
    m_pendingDeletions.clear();
}

} // namespace qivforge::scene

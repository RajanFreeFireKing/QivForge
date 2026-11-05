#pragma once

#include "qivforge/scene/asset_store.h"
#include <string>
#include <unordered_map>
#include <filesystem>

namespace qivforge::scene {

class LocalStore : public AssetStore {
public:
    explicit LocalStore(std::string projectRoot = "");
    ~LocalStore() override;

    std::pair<core::ErrorCode, std::string> ReadAsset(const std::string& path) override;
    core::ErrorCode WriteAsset(const std::string& path, const std::string& content) override;
    core::ErrorCode DeleteAsset(const std::string& path) override;
    bool HasAsset(const std::string& path) const override;
    std::vector<std::string> ListAssets() const override;

    bool IsCloudMode() const noexcept override { return false; }
    bool HasPendingChanges() const noexcept override { return false; }
    size_t GetPendingCount() const noexcept override { return 0; }

    // Disk and space utilities
    void SetAvailableDiskSpaceMb(uint64_t mb) noexcept { m_availableDiskSpaceMb = mb; }
    [[nodiscard]] uint64_t GetAvailableDiskSpaceMb() const noexcept { return m_availableDiskSpaceMb; }
    [[nodiscard]] const std::string& GetProjectRoot() const noexcept { return m_projectRoot; }

    bool AcquireLock();
    void ReleaseLock();

private:
    std::string m_projectRoot;
    uint64_t m_availableDiskSpaceMb = 1024; // Default 1024 MB free
    bool m_hasLock = false;
    std::unordered_map<std::string, std::string> m_memoryFiles; // Virtual fallback
};

} // namespace qivforge::scene

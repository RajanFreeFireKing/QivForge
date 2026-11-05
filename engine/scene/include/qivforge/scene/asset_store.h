#pragma once

#include "qivforge/core/error_codes.h"
#include <string>
#include <vector>
#include <memory>

namespace qivforge::scene {

/**
 * Unified AssetStore interface (Doc 11 §1)
 * Game code and editor work through AssetStore without needing to know whether
 * LocalStore or HfStore is active.
 */
class AssetStore {
public:
    virtual ~AssetStore() = default;

    virtual std::pair<core::ErrorCode, std::string> ReadAsset(const std::string& path) = 0;
    virtual core::ErrorCode WriteAsset(const std::string& path, const std::string& content) = 0;
    virtual core::ErrorCode DeleteAsset(const std::string& path) = 0;
    virtual bool HasAsset(const std::string& path) const = 0;
    virtual std::vector<std::string> ListAssets() const = 0;

    virtual bool IsCloudMode() const noexcept = 0;
    virtual bool HasPendingChanges() const noexcept = 0;
    virtual size_t GetPendingCount() const noexcept = 0;
};

} // namespace qivforge::scene

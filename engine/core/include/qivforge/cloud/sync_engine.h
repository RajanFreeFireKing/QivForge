#pragma once

#include "qivforge/core/error_codes.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>

namespace qivforge::cloud {

enum class SyncFileState {
    UNCHANGED,
    LOCAL_ONLY_CHANGED,   // Push to remote
    REMOTE_ONLY_CHANGED,  // Pull from remote
    BOTH_CHANGED_CONFLICT // Needs semantic merge or user resolution
};

enum class ConflictAction {
    KEEP_MINE,
    KEEP_REMOTE,
    KEEP_BOTH
};

struct FileHashSnapshot {
    std::string path;
    std::string sha256;
    size_t size = 0;
};

struct ConflictItem {
    std::string path;
    bool isBinary = false;
    std::string conflictReason;
    std::string mergedContent; // Present if automatically merged with conflict markers
    bool requiresManualChoice = true;
};

struct SyncPlan {
    std::vector<std::string> toUpload;
    std::vector<std::string> toDownload;
    std::vector<ConflictItem> conflicts;
    bool hasConflicts() const noexcept { return !conflicts.empty(); }
};

class SyncEngine {
public:
    SyncEngine() = default;

    // 1. Three-way comparison (Doc 11 §3.6)
    [[nodiscard]] static SyncPlan ComputeSyncPlan(
        const std::unordered_map<std::string, std::string>& baseHashes,
        const std::unordered_map<std::string, std::string>& localHashes,
        const std::unordered_map<std::string, std::string>& remoteHashes
    );

    // 2. Semantic text merge for QivScript (.qvs) (Doc 11 §3.7)
    [[nodiscard]] static std::pair<bool, std::string> MergeScript(
        const std::string& baseText,
        const std::string& localText,
        const std::string& remoteText
    );

    // 3. Semantic JSON merge for Scenes (.scene.json, .hud.json) (Doc 11 §3.7)
    [[nodiscard]] static std::pair<bool, std::string> MergeSceneJson(
        const std::string& baseJson,
        const std::string& localJson,
        const std::string& remoteJson,
        std::vector<std::string>& outConflictDetails
    );

    // 4. Binary conflict resolution (Doc 11 §3.7)
    [[nodiscard]] static std::string ResolveBinaryConflict(
        ConflictAction action,
        const std::string& localData,
        const std::string& remoteData,
        const std::string& originalPath,
        std::string& outKeepBothPath
    );
};

} // namespace qivforge::cloud

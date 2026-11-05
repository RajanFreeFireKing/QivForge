#include "qivforge/cloud/sync_engine.h"
#include <sstream>
#include <unordered_set>
#include <algorithm>

namespace qivforge::cloud {

static bool IsBinaryFile(const std::string& path) {
    auto dot = path.rfind('.');
    if (dot == std::string::npos) return false;
    std::string ext = path.substr(dot);
    return ext == ".glb" || ext == ".gltf" || ext == ".qmesh" || 
           ext == ".png" || ext == ".jpg" || ext == ".jpeg" || 
           ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".ktx2";
}

SyncPlan SyncEngine::ComputeSyncPlan(
    const std::unordered_map<std::string, std::string>& baseHashes,
    const std::unordered_map<std::string, std::string>& localHashes,
    const std::unordered_map<std::string, std::string>& remoteHashes
) {
    SyncPlan plan;
    std::unordered_set<std::string> allFiles;
    for (const auto& [p, _] : baseHashes) allFiles.insert(p);
    for (const auto& [p, _] : localHashes) allFiles.insert(p);
    for (const auto& [p, _] : remoteHashes) allFiles.insert(p);

    for (const auto& path : allFiles) {
        std::string bHash = baseHashes.count(path) ? baseHashes.at(path) : "";
        std::string lHash = localHashes.count(path) ? localHashes.at(path) : "";
        std::string rHash = remoteHashes.count(path) ? remoteHashes.at(path) : "";

        if (lHash == rHash) {
            // Unchanged or identical update
            continue;
        }

        if (lHash != bHash && rHash == bHash) {
            // Local only changed
            plan.toUpload.push_back(path);
        } else if (lHash == bHash && rHash != bHash) {
            // Remote only changed
            plan.toDownload.push_back(path);
        } else {
            // Both changed! Conflict
            ConflictItem conflict;
            conflict.path = path;
            conflict.isBinary = IsBinaryFile(path);
            conflict.conflictReason = "Modified on both local and remote branch";
            conflict.requiresManualChoice = conflict.isBinary;
            plan.conflicts.push_back(std::move(conflict));
        }
    }

    return plan;
}

std::pair<bool, std::string> SyncEngine::MergeScript(
    const std::string& baseText,
    const std::string& localText,
    const std::string& remoteText
) {
    // Line-based 3-way merge
    std::vector<std::string> baseLines, localLines, remoteLines;
    auto split = [](const std::string& str, std::vector<std::string>& lines) {
        std::istringstream ss(str);
        std::string line;
        while (std::getline(ss, line)) lines.push_back(line);
    };
    split(baseText, baseLines);
    split(localText, localLines);
    split(remoteText, remoteLines);

    std::ostringstream merged;
    bool hasConflicts = false;
    size_t maxLen = std::max({baseLines.size(), localLines.size(), remoteLines.size()});

    for (size_t i = 0; i < maxLen; ++i) {
        std::string b = (i < baseLines.size()) ? baseLines[i] : "";
        std::string l = (i < localLines.size()) ? localLines[i] : "";
        std::string r = (i < remoteLines.size()) ? remoteLines[i] : "";

        if (l == r) {
            merged << l << "\n";
        } else if (l != b && r == b) {
            merged << l << "\n";
        } else if (l == b && r != b) {
            merged << r << "\n";
        } else {
            // Conflict on line i
            hasConflicts = true;
            merged << "<<<<<<< LOCAL\n"
                   << l << "\n"
                   << "=======\n"
                   << r << "\n"
                   << ">>>>>>> REMOTE\n";
        }
    }

    return {!hasConflicts, merged.str()};
}

static std::unordered_map<uint32_t, std::string> ExtractActorsMap(const std::string& sceneJson) {
    std::unordered_map<uint32_t, std::string> map;
    size_t pos = 0;
    while ((pos = sceneJson.find("{\"id\":", pos)) != std::string::npos) {
        size_t idStart = pos + 6;
        size_t idEnd = sceneJson.find_first_of(",}", idStart);
        if (idEnd == std::string::npos) break;
        uint32_t id = std::stoul(sceneJson.substr(idStart, idEnd - idStart));

        // Find closing brace of this actor object
        size_t end = sceneJson.find("}", pos);
        if (end == std::string::npos) break;
        std::string actorJson = sceneJson.substr(pos, end - pos + 1);
        map[id] = actorJson;
        pos = end + 1;
    }
    return map;
}

std::pair<bool, std::string> SyncEngine::MergeSceneJson(
    const std::string& baseJson,
    const std::string& localJson,
    const std::string& remoteJson,
    std::vector<std::string>& outConflictDetails
) {
    auto baseActors = ExtractActorsMap(baseJson);
    auto localActors = ExtractActorsMap(localJson);
    auto remoteActors = ExtractActorsMap(remoteJson);

    std::unordered_set<uint32_t> allIds;
    for (const auto& [id, _] : baseActors) allIds.insert(id);
    for (const auto& [id, _] : localActors) allIds.insert(id);
    for (const auto& [id, _] : remoteActors) allIds.insert(id);

    std::vector<uint32_t> sortedIds(allIds.begin(), allIds.end());
    std::sort(sortedIds.begin(), sortedIds.end());

    std::ostringstream merged;
    merged << "{\"actors\":[";
    bool first = true;
    bool hasConflicts = false;

    for (uint32_t id : sortedIds) {
        std::string b = baseActors.count(id) ? baseActors[id] : "";
        std::string l = localActors.count(id) ? localActors[id] : "";
        std::string r = remoteActors.count(id) ? remoteActors[id] : "";

        std::string chosen;
        if (l == r) {
            chosen = l;
        } else if (l != b && r == b) {
            chosen = l; // Local modification or addition
        } else if (l == b && r != b) {
            chosen = r; // Remote modification or addition
        } else {
            // Both sides modified the same actor with differing values!
            hasConflicts = true;
            outConflictDetails.push_back("Actor ID " + std::to_string(id) + " property conflict between local and remote");
            // Default to local while recording conflict detail
            chosen = l.empty() ? r : l;
        }

        if (!chosen.empty()) {
            if (!first) merged << ",";
            merged << chosen;
            first = false;
        }
    }
    merged << "]}";

    return {!hasConflicts, merged.str()};
}

std::string SyncEngine::ResolveBinaryConflict(
    ConflictAction action,
    const std::string& localData,
    const std::string& remoteData,
    const std::string& originalPath,
    std::string& outKeepBothPath
) {
    switch (action) {
        case ConflictAction::KEEP_MINE:
            return localData;
        case ConflictAction::KEEP_REMOTE:
            return remoteData;
        case ConflictAction::KEEP_BOTH: {
            auto dot = originalPath.rfind('.');
            if (dot != std::string::npos) {
                outKeepBothPath = originalPath.substr(0, dot) + ".remote" + originalPath.substr(dot);
            } else {
                outKeepBothPath = originalPath + ".remote";
            }
            return localData; // Local kept at original path, remote saved at outKeepBothPath
        }
    }
    return localData;
}

} // namespace qivforge::cloud

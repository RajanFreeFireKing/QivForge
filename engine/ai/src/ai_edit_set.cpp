#include "qivforge/ai/ai_edit_set.h"
#include <algorithm>

namespace qivforge::ai {

EditSetManager::EditSetManager() = default;

void EditSetManager::StageEditSet(const EditSet& editSet) {
    m_stagedSet = editSet;
}

bool EditSetManager::CanAcceptAll() const noexcept {
    // Doc 12 §6: Accept all is available only when every item compiles and is NOT high risk
    return m_stagedSet.allCompileSuccess && !m_stagedSet.hasHighRiskOps;
}

bool EditSetManager::Accept(std::unordered_map<std::string, std::string>& virtualDisk,
                           std::vector<std::string>& virtualSceneActors) {
    if (m_stagedSet.IsEmpty()) return false;

    AppliedStateSnapshot snapshot;
    snapshot.summary = m_stagedSet.summary.empty() ? "AI Edit Set" : m_stagedSet.summary;

    // 1. Apply file edits
    for (const auto& fileEdit : m_stagedSet.fileEdits) {
        auto it = virtualDisk.find(fileEdit.path);
        if (it != virtualDisk.end()) {
            snapshot.previousFileContents[fileEdit.path] = it->second;
        } else {
            snapshot.previousFileContents[fileEdit.path] = ""; // File was newly created
        }
        virtualDisk[fileEdit.path] = fileEdit.proposedContent;
    }

    // 2. Apply scene operations
    for (const auto& sceneOp : m_stagedSet.sceneOps) {
        if (sceneOp.type == SceneOpType::CREATE_ACTOR || sceneOp.type == SceneOpType::SPAWN_ASSET) {
            virtualSceneActors.push_back(sceneOp.actorName);
            snapshot.createdActors.push_back(sceneOp.actorName);
        } else if (sceneOp.type == SceneOpType::DELETE_ACTOR) {
            auto it = std::find(virtualSceneActors.begin(), virtualSceneActors.end(), sceneOp.actorName);
            if (it != virtualSceneActors.end()) {
                virtualSceneActors.erase(it);
                snapshot.deletedActors.push_back(sceneOp.actorName);
            }
        }
    }

    m_undoHistory.push_back(std::move(snapshot));
    m_stagedSet = EditSet{}; // Clear staged set
    return true;
}

void EditSetManager::Reject() noexcept {
    // Simply clear the staged set; virtualDisk and virtualSceneActors remain untouched
    m_stagedSet = EditSet{};
}

bool EditSetManager::Undo(std::unordered_map<std::string, std::string>& virtualDisk,
                          std::vector<std::string>& virtualSceneActors) {
    if (m_undoHistory.empty()) return false;

    auto snapshot = std::move(m_undoHistory.back());
    m_undoHistory.pop_back();

    // 1. Revert files
    for (const auto& [path, prevContent] : snapshot.previousFileContents) {
        if (prevContent.empty()) {
            virtualDisk.erase(path); // File was created by AI, remove it
        } else {
            virtualDisk[path] = prevContent; // Restore prior content
        }
    }

    // 2. Revert scene operations
    // Remove created actors
    for (const auto& created : snapshot.createdActors) {
        auto it = std::find(virtualSceneActors.begin(), virtualSceneActors.end(), created);
        if (it != virtualSceneActors.end()) {
            virtualSceneActors.erase(it);
        }
    }
    // Re-add deleted actors
    for (const auto& deleted : snapshot.deletedActors) {
        virtualSceneActors.push_back(deleted);
    }

    return true;
}

} // namespace qivforge::ai

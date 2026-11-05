#pragma once

#include "qivforge/ai/ai_types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace qivforge::ai {

struct AppliedStateSnapshot {
    std::string summary;
    std::unordered_map<std::string, std::string> previousFileContents;
    std::vector<std::string> createdActors;
    std::vector<std::string> deletedActors;
};

class EditSetManager {
public:
    EditSetManager();

    // Stage a new proposed edit set from the AI
    void StageEditSet(const EditSet& editSet);
    [[nodiscard]] const EditSet& GetStagedEditSet() const noexcept { return m_stagedSet; }
    [[nodiscard]] bool HasStagedEdits() const noexcept { return !m_stagedSet.IsEmpty(); }

    // User reviews and accepts/rejects edits
    bool CanAcceptAll() const noexcept; // Blocked if hasHighRiskOps (Doc 12 §6)

    // Applies all edits, storing a snapshot for single-step undo (Doc 12 AC4)
    bool Accept(std::unordered_map<std::string, std::string>& virtualDisk,
                std::vector<std::string>& virtualSceneActors);

    // Discards proposed edits; leaves project byte-identical (Doc 12 AC3)
    void Reject() noexcept;

    // Single-step undo of applied AI edits
    bool Undo(std::unordered_map<std::string, std::string>& virtualDisk,
              std::vector<std::string>& virtualSceneActors);

    [[nodiscard]] size_t GetUndoHistoryCount() const noexcept { return m_undoHistory.size(); }

private:
    EditSet m_stagedSet;
    std::vector<AppliedStateSnapshot> m_undoHistory;
};

} // namespace qivforge::ai

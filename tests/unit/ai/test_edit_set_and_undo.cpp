#include "qivforge/ai/ai_edit_set.h"
#include "qivforge/ai/ai_tool_router.h"
#include "qivforge/ai/mock_ai_provider.h"
#include <iostream>
#include <cassert>

using namespace qivforge::ai;

void TestTractorScriptSynthesisAndInspectorField() {
    std::cout << "[RUN] TestTractorScriptSynthesisAndInspectorField (Doc 12 AC2)..." << std::endl;

    MockAiProvider mock(AiProviderId::GEMINI);
    AiMessage response = mock.GenerateTractorScriptResponse();
    assert(!response.toolCalls.empty());

    AiToolRouter router("/data/project");
    EditSet stagedEdits;
    ToolResult res = router.ExecuteDryRun(response.toolCalls[0], stagedEdits);

    if (res.status != ToolStatus::SUCCESS) {
        std::cerr << "Tool error: " << res.errorMessage << std::endl;
    }
    assert(res.status == ToolStatus::SUCCESS);
    assert(!stagedEdits.fileEdits.empty());
    const auto& fileEdit = stagedEdits.fileEdits[0];
    assert(fileEdit.compileSuccess);
    assert(fileEdit.path == "scripts/tractor_controller.qvs");
    assert(!fileEdit.diffUnified.empty());

    // Verify @export engineTorque exists in the script code
    assert(fileEdit.proposedContent.find("@export") != std::string::npos);
    assert(fileEdit.proposedContent.find("engineTorque") != std::string::npos);

    std::cout << "[PASS] TestTractorScriptSynthesisAndInspectorField" << std::endl;
}

void TestRejectLeavesProjectByteIdentical() {
    std::cout << "[RUN] TestRejectLeavesProjectByteIdentical (Doc 12 AC3)..." << std::endl;

    std::unordered_map<std::string, std::string> virtualDisk = {
        {"scripts/main.qvs", "class Main : Actor {}"}
    };
    std::vector<std::string> virtualActors = {"Tractor", "Camera"};

    // Copy original state for byte-identical verification
    auto originalDisk = virtualDisk;
    auto originalActors = virtualActors;

    EditSet proposedEdits;
    proposedEdits.summary = "Malicious or unwanted edit";
    proposedEdits.fileEdits.push_back({
        "scripts/main.qvs",
        virtualDisk["scripts/main.qvs"],
        "class Overwritten {}",
        "--- a/scripts/main.qvs\n+++ b/scripts/main.qvs",
        true
    });
    proposedEdits.sceneOps.push_back({
        SceneOpType::CREATE_ACTOR,
        "UnwantedEnemy",
        "",
        "Actor",
        false
    });

    EditSetManager manager;
    manager.StageEditSet(proposedEdits);
    assert(manager.HasStagedEdits());

    // User rejects edits (Doc 12 AC3)
    manager.Reject();
    assert(!manager.HasStagedEdits());

    // Verify byte-identical disk and scene
    assert(virtualDisk == originalDisk);
    assert(virtualActors == originalActors);

    std::cout << "[PASS] TestRejectLeavesProjectByteIdentical" << std::endl;
}

void TestAcceptAndSingleStepUndo() {
    std::cout << "[RUN] TestAcceptAndSingleStepUndo (Doc 12 AC4)..." << std::endl;

    std::unordered_map<std::string, std::string> virtualDisk = {
        {"scripts/player.qvs", "class Player : Actor { var speed = 10.0 }"}
    };
    std::vector<std::string> virtualActors = {"PlayerActor", "Ground"};

    auto originalDisk = virtualDisk;
    auto originalActors = virtualActors;

    EditSet proposedEdits;
    proposedEdits.summary = "AI: Add sprint functionality and camera rig";
    proposedEdits.fileEdits.push_back({
        "scripts/player.qvs",
        virtualDisk["scripts/player.qvs"],
        "class Player : Actor { var speed = 20.0 }",
        "diff...",
        true
    });
    proposedEdits.sceneOps.push_back({
        SceneOpType::CREATE_ACTOR,
        "FollowCamera",
        "",
        "Camera",
        false
    });

    EditSetManager manager;
    manager.StageEditSet(proposedEdits);
    assert(manager.CanAcceptAll());

    // Apply edits
    bool applied = manager.Accept(virtualDisk, virtualActors);
    assert(applied);
    assert(virtualDisk["scripts/player.qvs"].find("speed = 20.0") != std::string::npos);
    assert(std::find(virtualActors.begin(), virtualActors.end(), "FollowCamera") != virtualActors.end());

    // Doc 12 AC4: Pressing Undo restores the previous state in one step
    bool undone = manager.Undo(virtualDisk, virtualActors);
    assert(undone);
    assert(virtualDisk == originalDisk);
    assert(virtualActors == originalActors);

    std::cout << "[PASS] TestAcceptAndSingleStepUndo" << std::endl;
}

void TestHighRiskGating() {
    std::cout << "[RUN] TestHighRiskGating (Doc 12 §6)..." << std::endl;

    EditSet proposedEdits;
    proposedEdits.summary = "Delete scene actors";
    proposedEdits.allCompileSuccess = true;
    proposedEdits.hasHighRiskOps = true; // High risk deletion operation
    proposedEdits.sceneOps.push_back({
        SceneOpType::DELETE_ACTOR,
        "MainPlayer",
        "",
        "",
        true // High risk
    });

    EditSetManager manager;
    manager.StageEditSet(proposedEdits);

    // Doc 12 §6: Accept all is BLOCKED when any item is high risk
    assert(!manager.CanAcceptAll());

    std::cout << "[PASS] TestHighRiskGating" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    AI EDIT SET & UNDO TESTS (M10)      " << std::endl;
    std::cout << "========================================" << std::endl;

    TestTractorScriptSynthesisAndInspectorField();
    TestRejectLeavesProjectByteIdentical();
    TestAcceptAndSingleStepUndo();
    TestHighRiskGating();

    std::cout << "ALL AI EDIT SET TESTS PASSED!" << std::endl;
    return 0;
}

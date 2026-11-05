#include "qivforge/cloud/sync_engine.h"
#include <iostream>
#include <cassert>

using namespace qivforge::cloud;

void TestThreeWayHashClassification() {
    std::cout << "[RUN] TestThreeWayHashClassification (Doc 11 §3.6)..." << std::endl;

    std::unordered_map<std::string, std::string> baseHashes = {
        {"scenes/main.scene.json", "hash_base_scene"},
        {"scripts/Player.qvs", "hash_base_script"},
        {"assets/rock.glb", "hash_base_rock"},
        {"assets/tree.glb", "hash_base_tree"}
    };

    std::unordered_map<std::string, std::string> localHashes = {
        {"scenes/main.scene.json", "hash_local_scene_modified"}, // Local modified
        {"scripts/Player.qvs", "hash_base_script"},              // Unchanged
        {"assets/rock.glb", "hash_base_rock"},                   // Remote will modify
        {"assets/tree.glb", "hash_local_tree_modified"},         // Both will modify!
        {"scripts/NewUtil.qvs", "hash_local_new_util"}           // Local added
    };

    std::unordered_map<std::string, std::string> remoteHashes = {
        {"scenes/main.scene.json", "hash_base_scene"},           // Unchanged on remote
        {"scripts/Player.qvs", "hash_base_script"},              // Unchanged
        {"assets/rock.glb", "hash_remote_rock_modified"},        // Remote modified
        {"assets/tree.glb", "hash_remote_tree_modified"},        // Both modified! Conflict
        {"assets/grass.png", "hash_remote_new_grass"}            // Remote added
    };

    SyncPlan plan = SyncEngine::ComputeSyncPlan(baseHashes, localHashes, remoteHashes);

    // Verify local only changed files are queued to upload
    assert(std::find(plan.toUpload.begin(), plan.toUpload.end(), "scenes/main.scene.json") != plan.toUpload.end());
    assert(std::find(plan.toUpload.begin(), plan.toUpload.end(), "scripts/NewUtil.qvs") != plan.toUpload.end());

    // Verify remote only changed files are queued to download
    assert(std::find(plan.toDownload.begin(), plan.toDownload.end(), "assets/rock.glb") != plan.toDownload.end());
    assert(std::find(plan.toDownload.begin(), plan.toDownload.end(), "assets/grass.png") != plan.toDownload.end());

    // Verify both changed files are classified as conflicts
    assert(plan.hasConflicts());
    assert(plan.conflicts.size() == 1);
    assert(plan.conflicts[0].path == "assets/tree.glb");
    assert(plan.conflicts[0].isBinary);

    std::cout << "[PASS] TestThreeWayHashClassification" << std::endl;
}

void TestSemanticSceneJsonMerge() {
    std::cout << "[RUN] TestSemanticSceneJsonMerge (Doc 11 §3.7 AC5)..." << std::endl;

    // Doc 11 AC5: Two devices editing different actors in the same scene merge cleanly!
    std::string baseScene = "{\"actors\":[{\"id\":101,\"name\":\"Tractor\",\"speed\":20},{\"id\":202,\"name\":\"Camera\",\"fov\":60}]}";
    
    // User A on Device 1 edits Tractor (Actor 101) speed to 35
    std::string localScene = "{\"actors\":[{\"id\":101,\"name\":\"Tractor\",\"speed\":35},{\"id\":202,\"name\":\"Camera\",\"fov\":60}]}";

    // User B on Device 2 edits Camera (Actor 202) fov to 75
    std::string remoteScene = "{\"actors\":[{\"id\":101,\"name\":\"Tractor\",\"speed\":20},{\"id\":202,\"name\":\"Camera\",\"fov\":75}]}";

    std::vector<std::string> conflictDetails;
    auto [success, merged] = SyncEngine::MergeSceneJson(baseScene, localScene, remoteScene, conflictDetails);

    assert(success);
    assert(conflictDetails.empty());

    // Verify both actor 101 speed=35 AND actor 202 fov=75 are preserved!
    assert(merged.find("\"speed\":35") != std::string::npos);
    assert(merged.find("\"fov\":75") != std::string::npos);

    // Now test conflict when both modify the SAME actor 101 with differing properties
    std::string remoteConflicting = "{\"actors\":[{\"id\":101,\"name\":\"Tractor\",\"speed\":50},{\"id\":202,\"name\":\"Camera\",\"fov\":60}]}";
    auto [confSuccess, confMerged] = SyncEngine::MergeSceneJson(baseScene, localScene, remoteConflicting, conflictDetails);

    assert(!confSuccess); // Conflict reported!
    assert(!conflictDetails.empty());
    assert(conflictDetails[0].find("Actor ID 101") != std::string::npos);

    std::cout << "[PASS] TestSemanticSceneJsonMerge" << std::endl;
}

void TestScriptThreeWayMerge() {
    std::cout << "[RUN] TestScriptThreeWayMerge (Doc 11 §3.7)..." << std::endl;

    // 1. Non-overlapping edits merge cleanly
    std::string baseScript = "var a = 1;\nvar b = 2;\nvar c = 3;\n";
    std::string localScript = "var a = 100;\nvar b = 2;\nvar c = 3;\n";
    std::string remoteScript = "var a = 1;\nvar b = 2;\nvar c = 300;\n";

    auto [cleanSuccess, cleanMerged] = SyncEngine::MergeScript(baseScript, localScript, remoteScript);
    assert(cleanSuccess);
    assert(cleanMerged.find("var a = 100;") != std::string::npos);
    assert(cleanMerged.find("var c = 300;") != std::string::npos);

    // 2. Overlapping conflict produces conflict markers
    std::string remoteConflict = "var a = 999;\nvar b = 2;\nvar c = 3;\n";
    auto [confSuccess, confMerged] = SyncEngine::MergeScript(baseScript, localScript, remoteConflict);
    assert(!confSuccess);
    assert(confMerged.find("<<<<<<< LOCAL") != std::string::npos);
    assert(confMerged.find("var a = 100;") != std::string::npos);
    assert(confMerged.find("=======") != std::string::npos);
    assert(confMerged.find("var a = 999;") != std::string::npos);
    assert(confMerged.find(">>>>>>> REMOTE") != std::string::npos);

    std::cout << "[PASS] TestScriptThreeWayMerge" << std::endl;
}

void TestBinaryConflictResolution() {
    std::cout << "[RUN] TestBinaryConflictResolution..." << std::endl;

    std::string localBin = "LOCAL_GLB_BYTES";
    std::string remoteBin = "REMOTE_GLB_BYTES";
    std::string path = "assets/models/character.glb";
    std::string keepBothPath;

    // KEEP_MINE
    assert(SyncEngine::ResolveBinaryConflict(ConflictAction::KEEP_MINE, localBin, remoteBin, path, keepBothPath) == localBin);

    // KEEP_REMOTE
    assert(SyncEngine::ResolveBinaryConflict(ConflictAction::KEEP_REMOTE, localBin, remoteBin, path, keepBothPath) == remoteBin);

    // KEEP_BOTH
    auto resBoth = SyncEngine::ResolveBinaryConflict(ConflictAction::KEEP_BOTH, localBin, remoteBin, path, keepBothPath);
    assert(resBoth == localBin);
    assert(keepBothPath == "assets/models/character.remote.glb");

    std::cout << "[PASS] TestBinaryConflictResolution" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  SYNC & SEMANTIC MERGE TEST SUITE (M7) " << std::endl;
    std::cout << "========================================" << std::endl;

    TestThreeWayHashClassification();
    TestSemanticSceneJsonMerge();
    TestScriptThreeWayMerge();
    TestBinaryConflictResolution();

    std::cout << "ALL SYNC & SEMANTIC MERGE TESTS PASSED!" << std::endl;
    return 0;
}

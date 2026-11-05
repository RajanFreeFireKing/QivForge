#include "qivforge/anim/skeleton.h"
#include "qivforge/anim/animation_clip.h"
#include <iostream>
#include <cassert>

using namespace qivforge::anim;

void TestSkeletonHierarchyAndLimits() {
    std::cout << "[RUN] TestSkeletonHierarchyAndLimits (Doc 05 §9 & Doc 07 §2)..." << std::endl;

    Skeleton skeleton;

    // Root bone: Hips
    BoneTransform hipsBind;
    hipsBind.translation = Vec3(0.0f, 1.0f, 0.0f);
    int32_t hipsIdx = skeleton.AddBone("Hips", -1, hipsBind);
    assert(hipsIdx == 0);

    // Child bone: Spine
    BoneTransform spineBind;
    spineBind.translation = Vec3(0.0f, 0.4f, 0.0f);
    int32_t spineIdx = skeleton.AddBone("Spine", hipsIdx, spineBind);
    assert(spineIdx == 1);

    // Child of Spine: Head
    BoneTransform headBind;
    headBind.translation = Vec3(0.0f, 0.3f, 0.0f);
    int32_t headIdx = skeleton.AddBone("Head", spineIdx, headBind);
    assert(headIdx == 2);

    assert(skeleton.GetBoneCount() == 3);
    assert(skeleton.FindBoneIndex("Hips") == 0);
    assert(skeleton.FindBoneIndex("Spine") == 1);
    assert(skeleton.FindBoneIndex("Head") == 2);
    assert(skeleton.FindBoneIndex("NonExistent") == -1);

    // Verify GPU skinning matrix computation
    std::vector<BoneTransform> currentPoses = {hipsBind, spineBind, headBind};
    std::vector<Mat4> skinMatrices;
    skeleton.ComputeSkinningMatrices(currentPoses, skinMatrices);
    assert(skinMatrices.size() == 3);

    // Verify 128 bone limit per Doc 05 §9
    for (int i = 3; i < 128; ++i) {
        skeleton.AddBone("Bone_" + std::to_string(i), hipsIdx, BoneTransform{});
    }
    assert(skeleton.GetBoneCount() == 128);
    // 129th bone must be rejected
    int32_t rejectedIdx = skeleton.AddBone("Bone_129", hipsIdx, BoneTransform{});
    assert(rejectedIdx == -1);

    std::cout << "[PASS] TestSkeletonHierarchyAndLimits" << std::endl;
}

void TestAnimationClipSamplingAndEvents() {
    std::cout << "[RUN] TestAnimationClipSamplingAndEvents (Doc 07 §3 & §9)..." << std::endl;

    AnimationClip clip("Walk", 1.0f, true);

    BoneTrack track;
    track.boneIndex = 0;
    track.boneName = "Hips";
    track.translations.push_back({0.0f, Vec3(0.0f, 1.0f, 0.0f)});
    track.translations.push_back({0.5f, Vec3(0.0f, 1.08f, 0.2f)});
    track.translations.push_back({1.0f, Vec3(0.0f, 1.0f, 0.0f)});

    track.rotations.push_back({0.0f, Quat::Identity()});
    track.rotations.push_back({0.5f, Quat::FromEuler(5.0f, 0.0f, 0.0f)});
    track.rotations.push_back({1.0f, Quat::Identity()});

    clip.AddTrack(track);

    // Add standard events per Doc 07 §9
    clip.AddEvent(0.25f, "Footstep_L", 1.0f);
    clip.AddEvent(0.75f, "Footstep_R", 1.0f);

    // Sample at t = 0.25s
    std::vector<BoneTransform> pose;
    clip.Sample(0.25f, 1, pose);
    assert(pose.size() == 1);
    assert(pose[0].translation.y > 1.0f && pose[0].translation.y < 1.08f);

    // Check event dispatching
    std::vector<std::string> firedEvents;
    clip.EvaluateEvents(0.1f, 0.3f, [&](const AnimationEvent& ev) {
        firedEvents.push_back(ev.name);
    });
    assert(firedEvents.size() == 1);
    assert(firedEvents[0] == "Footstep_L");

    firedEvents.clear();
    clip.EvaluateEvents(0.6f, 0.8f, [&](const AnimationEvent& ev) {
        firedEvents.push_back(ev.name);
    });
    assert(firedEvents.size() == 1);
    assert(firedEvents[0] == "Footstep_R");

    std::cout << "[PASS] TestAnimationClipSamplingAndEvents" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  SKELETON & ANIMATION CLIP TEST (M8)   " << std::endl;
    std::cout << "========================================" << std::endl;

    TestSkeletonHierarchyAndLimits();
    TestAnimationClipSamplingAndEvents();

    std::cout << "ALL SKELETON & ANIMATION CLIP TESTS PASSED!" << std::endl;
    return 0;
}

#include "qivforge/anim/anim_graph.h"
#include <iostream>
#include <cassert>

using namespace qivforge::anim;

void TestBlendTreeAndFootstepDeduplication() {
    std::cout << "[RUN] TestBlendTreeAndFootstepDeduplication (Doc 07 AC1 & AC4)..." << std::endl;

    auto idleClip = std::make_shared<AnimationClip>("Idle", 1.0f, true);
    auto walkClip = std::make_shared<AnimationClip>("Walk", 1.0f, true);
    auto runClip  = std::make_shared<AnimationClip>("Run", 0.8f, true);

    walkClip->AddEvent(0.25f, "Footstep_L");
    walkClip->AddEvent(0.75f, "Footstep_R");
    runClip->AddEvent(0.20f, "Footstep_L");
    runClip->AddEvent(0.60f, "Footstep_R");

    // Add bone tracks to idle, walk and run
    BoneTrack idleTrack;
    idleTrack.boneIndex = 0;
    idleTrack.translations.push_back({0.0f, Vec3(0, 1.0f, 0)});
    idleClip->AddTrack(idleTrack);

    BoneTrack walkTrack;
    walkTrack.boneIndex = 0;
    walkTrack.translations.push_back({0.0f, Vec3(0, 1.0f, 0)});
    walkTrack.translations.push_back({1.0f, Vec3(0, 1.0f, 1.8f)});
    walkClip->AddTrack(walkTrack);

    BoneTrack runTrack;
    runTrack.boneIndex = 0;
    runTrack.translations.push_back({0.0f, Vec3(0, 1.0f, 0)});
    runTrack.translations.push_back({0.8f, Vec3(0, 1.0f, 5.5f)});
    runClip->AddTrack(runTrack);

    BlendTree1D blendTree("MoveSpeed");
    blendTree.AddClip(0.0f, idleClip);
    blendTree.AddClip(1.8f, walkClip);
    blendTree.AddClip(5.5f, runClip);

    // Sample across speed range 0 to 6.5 m/s per Doc 07 AC1
    for (float speed = 0.0f; speed <= 6.5f; speed += 0.5f) {
        std::vector<BoneTransform> pose;
        blendTree.Sample(speed, 0.4f, 1, pose);
        assert(pose.size() == 1);
        assert(pose[0].translation.y >= 0.99f);
    }

    // Doc 07 AC4: Footstep events fire exactly once per step across blend of walk and run!
    // At speed = 2.0 m/s (between walk 1.8 and run 5.5, walk weight ~0.95 >= 0.5), walkClip fires, runClip does NOT fire!
    int footstepCount = 0;
    blendTree.EvaluateEvents(2.0f, 0.1f, 0.35f, [&](const AnimationEvent& ev) {
        if (ev.name.find("Footstep") != std::string::npos) {
            footstepCount++;
        }
    });
    // Must be exactly 1 event (no doubled footsteps!)
    assert(footstepCount == 1);

    std::cout << "[PASS] TestBlendTreeAndFootstepDeduplication" << std::endl;
}

void TestInterruptedCrossfadeNeverPops() {
    std::cout << "[RUN] TestInterruptedCrossfadeNeverPops (Doc 07 AC2)..." << std::endl;

    Skeleton skeleton;
    BoneTransform rootBind;
    skeleton.AddBone("Root", -1, rootBind);

    AnimGraph graph;
    AnimLayer baseLayer;
    baseLayer.name = "Base";
    baseLayer.weight = 1.0f;

    auto clipIdle = std::make_shared<AnimationClip>("Idle", 1.0f, true);
    BoneTrack trackIdle;
    trackIdle.boneIndex = 0;
    trackIdle.rotations.push_back({0.0f, Quat::FromEuler(0.0f, 0.0f, 0.0f)});
    clipIdle->AddTrack(trackIdle);

    auto clipWalk = std::make_shared<AnimationClip>("Walk", 1.0f, true);
    BoneTrack trackWalk;
    trackWalk.boneIndex = 0;
    trackWalk.rotations.push_back({0.0f, Quat::FromEuler(0.0f, 40.0f, 0.0f)});
    clipWalk->AddTrack(trackWalk);

    auto clipJump = std::make_shared<AnimationClip>("Jump", 1.0f, true);
    BoneTrack trackJump;
    trackJump.boneIndex = 0;
    trackJump.rotations.push_back({0.0f, Quat::FromEuler(0.0f, 80.0f, 0.0f)});
    clipJump->AddTrack(trackJump);

    baseLayer.states["Idle"] = AnimState{"Idle", AnimState::Type::CLIP, clipIdle, nullptr, 1.0f};
    baseLayer.states["Walk"] = AnimState{"Walk", AnimState::Type::CLIP, clipWalk, nullptr, 1.0f};
    baseLayer.states["Jump"] = AnimState{"Jump", AnimState::Type::CLIP, clipJump, nullptr, 1.0f};

    // Transition Idle -> Walk when MoveSpeed > 0.1
    StateTransition t1;
    t1.fromState = "Idle";
    t1.toState = "Walk";
    t1.duration = 0.2f; // 200 ms crossfade
    t1.conditions.push_back({"MoveSpeed", ConditionOp::GREATER, 0.1f});
    baseLayer.transitions.push_back(t1);

    // Transition Any -> Jump when trigger "Jump"
    StateTransition t2;
    t2.fromState = "Any";
    t2.toState = "Jump";
    t2.duration = 0.2f;
    t2.conditions.push_back({"Jump", ConditionOp::GREATER, 0.5f});
    baseLayer.transitions.push_back(t2);

    baseLayer.currentState = "Idle";
    graph.AddLayer(baseLayer);

    std::vector<BoneTransform> posePrev;
    graph.Update(0.016f, skeleton, posePrev);

    // 1. Trigger Idle -> Walk crossfade
    graph.SetFloat("MoveSpeed", 1.5f);
    graph.Update(0.05f, skeleton, posePrev); // Crossfade at ~25%

    // 2. Interrupt mid-crossfade with trigger "Jump"
    graph.Trigger("Jump");

    // Track rotation delta over subsequent frames
    float maxDeltaDeg = 0.0f;
    for (int frame = 0; frame < 15; ++frame) {
        std::vector<BoneTransform> poseCurr;
        graph.Update(0.016f, skeleton, poseCurr);

        float delta = Quat::AngleDegrees(posePrev[0].rotation, poseCurr[0].rotation);
        if (delta > maxDeltaDeg) maxDeltaDeg = delta;
        posePrev = poseCurr;
    }

    // Doc 07 AC2: Max bone rotation delta between frames must be below 15 degrees!
    std::cout << "  -> Interrupted crossfade max frame rotation delta: " << maxDeltaDeg << " deg (Budget: < 15 deg)" << std::endl;
    assert(maxDeltaDeg < 15.0f);

    std::cout << "[PASS] TestInterruptedCrossfadeNeverPops" << std::endl;
}

void TestLayeredUpperBodyMasking() {
    std::cout << "[RUN] TestLayeredUpperBodyMasking (Doc 07 AC3)..." << std::endl;

    Skeleton skeleton;
    int32_t hips = skeleton.AddBone("Hips", -1, BoneTransform{});
    int32_t spine = skeleton.AddBone("Spine", hips, BoneTransform{});
    int32_t arm = skeleton.AddBone("RightArm", spine, BoneTransform{});
    int32_t leg = skeleton.AddBone("RightLeg", hips, BoneTransform{});

    AnimGraph graph;

    // Base Layer: Running locomotion on all bones
    AnimLayer baseLayer;
    baseLayer.name = "Base";
    baseLayer.weight = 1.0f;
    auto runClip = std::make_shared<AnimationClip>("Run", 1.0f, true);
    BoneTrack runLegTrack;
    runLegTrack.boneIndex = leg;
    runLegTrack.translations.push_back({0.0f, Vec3(0, 0, 1.5f)});
    runClip->AddTrack(runLegTrack);
    baseLayer.states["Run"] = AnimState{"Run", AnimState::Type::CLIP, runClip, nullptr, 1.0f};
    baseLayer.currentState = "Run";
    graph.AddLayer(baseLayer);

    // Layer 1: Shoot animation with Upper Body mask (override)
    AnimLayer shootLayer;
    shootLayer.name = "Shoot";
    shootLayer.weight = 1.0f;
    shootLayer.isAdditive = false; // Override

    auto mask = std::make_shared<AvatarMask>();
    mask->SetBoneRole(hips, HumanoidRole::HIPS);
    mask->SetBoneRole(spine, HumanoidRole::SPINE);
    mask->SetBoneRole(arm, HumanoidRole::RIGHT_UPPER_ARM);
    mask->SetBoneRole(leg, HumanoidRole::RIGHT_UPPER_LEG);
    mask->ApplyPreset(MaskPreset::UPPER_BODY, skeleton.GetBoneCount());
    shootLayer.mask = mask;

    auto shootClip = std::make_shared<AnimationClip>("Shoot", 1.0f, true);
    BoneTrack shootArmTrack;
    shootArmTrack.boneIndex = arm;
    shootArmTrack.translations.push_back({0.0f, Vec3(0.5f, 0.2f, 0.0f)});
    shootClip->AddTrack(shootArmTrack);

    BoneTrack shootLegTrack; // Erroneous or stationary leg pose in shoot clip
    shootLegTrack.boneIndex = leg;
    shootLegTrack.translations.push_back({0.0f, Vec3(0, 0, 0)});
    shootClip->AddTrack(shootLegTrack);

    shootLayer.states["Shoot"] = AnimState{"Shoot", AnimState::Type::CLIP, shootClip, nullptr, 1.0f};
    shootLayer.currentState = "Shoot";
    graph.AddLayer(shootLayer);

    std::vector<BoneTransform> pose;
    graph.Update(0.1f, skeleton, pose);

    // Doc 07 AC3: Upper body arm receives shoot clip, while lower body leg keeps running!
    assert(pose[arm].translation.x == 0.5f); // Upper body: shoot overridden!
    assert(pose[leg].translation.z == 1.5f); // Lower body: run locomotion preserved!

    std::cout << "[PASS] TestLayeredUpperBodyMasking" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  ANIM GRAPH & BLEND TREE TEST (M8)     " << std::endl;
    std::cout << "========================================" << std::endl;

    TestBlendTreeAndFootstepDeduplication();
    TestInterruptedCrossfadeNeverPops();
    TestLayeredUpperBodyMasking();

    std::cout << "ALL ANIM GRAPH & BLEND TREE TESTS PASSED!" << std::endl;
    return 0;
}

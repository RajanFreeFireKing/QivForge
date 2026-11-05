#pragma once

#include "qivforge/anim/animation_clip.h"
#include "qivforge/anim/blend_tree.h"
#include "qivforge/anim/avatar_mask.h"
#include "qivforge/anim/skeleton.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <variant>

namespace qivforge::anim {

enum class ConditionOp {
    GREATER,
    LESS,
    GREATER_EQUAL,
    LESS_EQUAL,
    EQUAL,
    NOT_EQUAL
};

struct TransitionCondition {
    std::string paramName;
    ConditionOp op = ConditionOp::GREATER;
    float value = 0.0f;
};

struct StateTransition {
    std::string fromState;
    std::string toState;
    float duration = 0.15f; // Crossfade duration in seconds
    bool hasExitTime = false;
    float exitTime = 0.9f;
    std::vector<TransitionCondition> conditions;
};

struct AnimState {
    std::string id;
    enum class Type { CLIP, BLEND_TREE } type = Type::CLIP;
    std::shared_ptr<AnimationClip> clip;
    std::shared_ptr<BlendTree1D> blendTree;
    float speed = 1.0f;
};

struct AnimLayer {
    std::string name;
    float weight = 1.0f;
    bool isAdditive = false; // true = additive, false = override
    std::shared_ptr<AvatarMask> mask;

    std::unordered_map<std::string, AnimState> states;
    std::vector<StateTransition> transitions;
    std::string currentState;
    std::string targetState; // During crossfade
    float stateTime = 0.0f;
    float crossfadeTimer = 0.0f;
    float crossfadeDuration = 0.0f;
    bool inCrossfade = false;
    std::vector<BoneTransform> currentBlendedPose; // Stored to prevent popping on interruption
};

class AnimGraph {
public:
    AnimGraph();

    void AddLayer(AnimLayer layer);
    [[nodiscard]] size_t GetLayerCount() const noexcept { return m_layers.size(); }
    [[nodiscard]] AnimLayer& GetLayer(size_t index) { return m_layers[index]; }

    // Parameter APIs (Doc 07 §5)
    void SetFloat(const std::string& name, float value);
    [[nodiscard]] float GetFloat(const std::string& name) const noexcept;
    void SetInt(const std::string& name, int32_t value);
    [[nodiscard]] int32_t GetInt(const std::string& name) const noexcept;
    void SetBool(const std::string& name, bool value);
    [[nodiscard]] bool GetBool(const std::string& name) const noexcept;
    void Trigger(const std::string& name);
    void ResetTrigger(const std::string& name);

    // Frame update producing final skeleton local poses (Doc 07 §4)
    void Update(float dt, const Skeleton& skeleton, std::vector<BoneTransform>& outPose,
                const std::function<void(const AnimationEvent&)>& eventCallback = nullptr);

    // .qanim serialization & deserialization (Doc 07 §19)
    [[nodiscard]] std::string ToQanimJson() const;
    static std::shared_ptr<AnimGraph> FromQanimJson(const std::string& json);

private:
    bool CheckConditions(const std::vector<TransitionCondition>& conditions);

    std::vector<AnimLayer> m_layers;
    std::unordered_map<std::string, float> m_floatParams;
    std::unordered_map<std::string, int32_t> m_intParams;
    std::unordered_map<std::string, bool> m_boolParams;
    std::unordered_map<std::string, bool> m_triggers;
};

} // namespace qivforge::anim

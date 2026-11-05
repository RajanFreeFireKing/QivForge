#pragma once

#include "qivforge/anim/animation_clip.h"
#include <vector>
#include <memory>
#include <string>

namespace qivforge::anim {

struct BlendNode1D {
    float threshold = 0.0f;
    std::shared_ptr<AnimationClip> clip;
};

class BlendTree1D {
public:
    BlendTree1D(std::string parameterName = "MoveSpeed");

    void AddClip(float threshold, std::shared_ptr<AnimationClip> clip);

    // Evaluates pose for a given parameter value and time
    void Sample(float paramValue, float time, size_t boneCount, std::vector<BoneTransform>& outPose) const;

    // Evaluates events only for clips with weight > 0.5 per Doc 07 §9
    void EvaluateEvents(float paramValue, float previousTime, float currentTime, const std::function<void(const AnimationEvent&)>& callback) const;

    [[nodiscard]] const std::string& GetParameterName() const noexcept { return m_parameterName; }
    [[nodiscard]] const std::vector<BlendNode1D>& GetNodes() const noexcept { return m_nodes; }

private:
    std::string m_parameterName;
    std::vector<BlendNode1D> m_nodes;
};

} // namespace qivforge::anim

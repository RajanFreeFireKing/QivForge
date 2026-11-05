#include "qivforge/anim/blend_tree.h"
#include <algorithm>

namespace qivforge::anim {

BlendTree1D::BlendTree1D(std::string parameterName)
    : m_parameterName(std::move(parameterName)) {}

void BlendTree1D::AddClip(float threshold, std::shared_ptr<AnimationClip> clip) {
    m_nodes.push_back({threshold, std::move(clip)});
    std::sort(m_nodes.begin(), m_nodes.end(), [](const BlendNode1D& a, const BlendNode1D& b) {
        return a.threshold < b.threshold;
    });
}

void BlendTree1D::Sample(float paramValue, float time, size_t boneCount, std::vector<BoneTransform>& outPose) const {
    outPose.resize(boneCount);
    if (m_nodes.empty()) return;

    if (m_nodes.size() == 1 || paramValue <= m_nodes.front().threshold) {
        m_nodes.front().clip->Sample(time, boneCount, outPose);
        return;
    }

    if (paramValue >= m_nodes.back().threshold) {
        m_nodes.back().clip->Sample(time, boneCount, outPose);
        return;
    }

    // Find interval [i, i+1]
    for (size_t i = 0; i + 1 < m_nodes.size(); ++i) {
        if (paramValue >= m_nodes[i].threshold && paramValue <= m_nodes[i + 1].threshold) {
            float range = m_nodes[i + 1].threshold - m_nodes[i].threshold;
            float weightB = (range > 1e-5f) ? (paramValue - m_nodes[i].threshold) / range : 0.0f;
            float weightA = 1.0f - weightB;

            std::vector<BoneTransform> poseA, poseB;
            m_nodes[i].clip->Sample(time, boneCount, poseA);
            m_nodes[i + 1].clip->Sample(time, boneCount, poseB);

            for (size_t b = 0; b < boneCount; ++b) {
                outPose[b] = BoneTransform::Lerp(poseA[b], poseB[b], weightB);
            }
            return;
        }
    }

    m_nodes.back().clip->Sample(time, boneCount, outPose);
}

void BlendTree1D::EvaluateEvents(float paramValue, float previousTime, float currentTime, const std::function<void(const AnimationEvent&)>& callback) const {
    if (m_nodes.empty() || !callback) return;

    if (m_nodes.size() == 1) {
        m_nodes.front().clip->EvaluateEvents(previousTime, currentTime, callback);
        return;
    }

    // Doc 07 §9: "Events inside a blend tree fire only from clips with weight above 0.5 to avoid doubled footsteps."
    for (size_t i = 0; i + 1 < m_nodes.size(); ++i) {
        if (paramValue >= m_nodes[i].threshold && paramValue <= m_nodes[i + 1].threshold) {
            float range = m_nodes[i + 1].threshold - m_nodes[i].threshold;
            float weightB = (range > 1e-5f) ? (paramValue - m_nodes[i].threshold) / range : 0.0f;
            float weightA = 1.0f - weightB;

            if (weightA >= 0.5f) {
                m_nodes[i].clip->EvaluateEvents(previousTime, currentTime, callback);
            } else {
                m_nodes[i + 1].clip->EvaluateEvents(previousTime, currentTime, callback);
            }
            return;
        }
    }

    if (paramValue > m_nodes.back().threshold) {
        m_nodes.back().clip->EvaluateEvents(previousTime, currentTime, callback);
    } else {
        m_nodes.front().clip->EvaluateEvents(previousTime, currentTime, callback);
    }
}

} // namespace qivforge::anim

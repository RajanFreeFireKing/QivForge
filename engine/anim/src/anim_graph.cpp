#include "qivforge/anim/anim_graph.h"
#include <sstream>
#include <iostream>

namespace qivforge::anim {

AnimGraph::AnimGraph() = default;

void AnimGraph::AddLayer(AnimLayer layer) {
    m_layers.push_back(std::move(layer));
}

void AnimGraph::SetFloat(const std::string& name, float value) {
    m_floatParams[name] = value;
}

float AnimGraph::GetFloat(const std::string& name) const noexcept {
    auto it = m_floatParams.find(name);
    return (it != m_floatParams.end()) ? it->second : 0.0f;
}

void AnimGraph::SetInt(const std::string& name, int32_t value) {
    m_intParams[name] = value;
}

int32_t AnimGraph::GetInt(const std::string& name) const noexcept {
    auto it = m_intParams.find(name);
    return (it != m_intParams.end()) ? it->second : 0;
}

void AnimGraph::SetBool(const std::string& name, bool value) {
    m_boolParams[name] = value;
}

bool AnimGraph::GetBool(const std::string& name) const noexcept {
    auto it = m_boolParams.find(name);
    return (it != m_boolParams.end()) ? it->second : false;
}

void AnimGraph::Trigger(const std::string& name) {
    m_triggers[name] = true;
}

void AnimGraph::ResetTrigger(const std::string& name) {
    m_triggers[name] = false;
}

bool AnimGraph::CheckConditions(const std::vector<TransitionCondition>& conditions) {
    for (const auto& cond : conditions) {
        // Check triggers
        auto trIt = m_triggers.find(cond.paramName);
        if (trIt != m_triggers.end() && trIt->second) {
            continue; // Trigger fired
        }

        // Check floats
        auto fIt = m_floatParams.find(cond.paramName);
        if (fIt != m_floatParams.end()) {
            float val = fIt->second;
            switch (cond.op) {
                case ConditionOp::GREATER: if (!(val > cond.value)) return false; break;
                case ConditionOp::LESS: if (!(val < cond.value)) return false; break;
                case ConditionOp::GREATER_EQUAL: if (!(val >= cond.value)) return false; break;
                case ConditionOp::LESS_EQUAL: if (!(val <= cond.value)) return false; break;
                case ConditionOp::EQUAL: if (!(std::abs(val - cond.value) < 1e-4f)) return false; break;
                case ConditionOp::NOT_EQUAL: if (!(std::abs(val - cond.value) >= 1e-4f)) return false; break;
            }
            continue;
        }

        // Check bools
        auto bIt = m_boolParams.find(cond.paramName);
        if (bIt != m_boolParams.end()) {
            bool expected = (cond.value > 0.5f);
            if (bIt->second != expected) return false;
            continue;
        }

        return false; // Parameter not found
    }
    return true;
}

static void SampleStatePose(const AnimState& state, float time, float speedParam, size_t boneCount, std::vector<BoneTransform>& outPose) {
    if (state.type == AnimState::Type::CLIP && state.clip) {
        state.clip->Sample(time, boneCount, outPose);
    } else if (state.type == AnimState::Type::BLEND_TREE && state.blendTree) {
        state.blendTree->Sample(speedParam, time, boneCount, outPose);
    }
}

void AnimGraph::Update(float dt, const Skeleton& skeleton, std::vector<BoneTransform>& outPose,
                      const std::function<void(const AnimationEvent&)>& eventCallback) {
    size_t boneCount = skeleton.GetBoneCount();
    outPose.resize(boneCount);

    // Initialize with skeleton bind pose
    for (size_t b = 0; b < boneCount; ++b) {
        outPose[b] = skeleton.GetBone(b).bindPose;
    }

    float moveSpeed = GetFloat("MoveSpeed");

    // Evaluate each layer per Doc 07 §4
    for (size_t layerIdx = 0; layerIdx < m_layers.size(); ++layerIdx) {
        auto& layer = m_layers[layerIdx];
        if (layer.weight <= 0.0f || layer.states.empty()) continue;

        if (layer.currentState.empty() && !layer.states.empty()) {
            layer.currentState = layer.states.begin()->first;
        }

        // Check transitions
        for (const auto& trans : layer.transitions) {
            if (trans.fromState == layer.currentState || trans.fromState == "Any") {
                if (CheckConditions(trans.conditions)) {
                    // Transition triggered!
                    if (layer.inCrossfade) {
                        // Doc 07 §6.2 & AC2: Interrupted crossfade blends from current blended pose to avoid popping!
                        // The current blended pose is preserved in layer.currentBlendedPose
                    }
                    layer.targetState = trans.toState;
                    layer.crossfadeDuration = std::max(0.001f, trans.duration);
                    layer.crossfadeTimer = 0.0f;
                    layer.inCrossfade = true;

                    // Consume triggers
                    for (const auto& c : trans.conditions) {
                        m_triggers[c.paramName] = false;
                    }
                    break;
                }
            }
        }

        float prevTime = layer.stateTime;
        layer.stateTime += dt;

        // Sample layer pose
        std::vector<BoneTransform> layerPose(boneCount);
        auto currentIt = layer.states.find(layer.currentState);

        if (layer.inCrossfade) {
            layer.crossfadeTimer += dt;
            float rawT = std::clamp(layer.crossfadeTimer / layer.crossfadeDuration, 0.0f, 1.0f);
            // Smoothstep curve for crossfade blend: 3t^2 - 2t^3
            float blendT = rawT * rawT * (3.0f - 2.0f * rawT);

            std::vector<BoneTransform> poseSrc(boneCount), poseDst(boneCount);

            if (!layer.currentBlendedPose.empty()) {
                poseSrc = layer.currentBlendedPose; // Smooth interruption
            } else if (currentIt != layer.states.end()) {
                SampleStatePose(currentIt->second, layer.stateTime, moveSpeed, boneCount, poseSrc);
            }

            auto targetIt = layer.states.find(layer.targetState);
            if (targetIt != layer.states.end()) {
                SampleStatePose(targetIt->second, layer.crossfadeTimer, moveSpeed, boneCount, poseDst);
            }

            for (size_t b = 0; b < boneCount; ++b) {
                layerPose[b] = BoneTransform::Lerp(poseSrc[b], poseDst[b], blendT);
            }
            layer.currentBlendedPose = layerPose;

            if (rawT >= 1.0f) {
                // Crossfade finished
                layer.currentState = layer.targetState;
                layer.targetState.clear();
                layer.inCrossfade = false;
                layer.stateTime = layer.crossfadeTimer;
                layer.currentBlendedPose.clear();
            }
        } else {
            if (currentIt != layer.states.end()) {
                SampleStatePose(currentIt->second, layer.stateTime, moveSpeed, boneCount, layerPose);

                // Dispatch animation events
                if (currentIt->second.type == AnimState::Type::CLIP && currentIt->second.clip) {
                    currentIt->second.clip->EvaluateEvents(prevTime, layer.stateTime, eventCallback);
                } else if (currentIt->second.type == AnimState::Type::BLEND_TREE && currentIt->second.blendTree) {
                    currentIt->second.blendTree->EvaluateEvents(moveSpeed, prevTime, layer.stateTime, eventCallback);
                }
            }
        }

        // Blend layer into output pose
        if (layerIdx == 0) {
            // Base layer: full pose
            outPose = layerPose;
        } else {
            // Layer > 0: mask and weight blending
            float w = layer.weight;
            for (size_t b = 0; b < boneCount; ++b) {
                bool isIncluded = (!layer.mask) || layer.mask->IsBoneIncluded(static_cast<int32_t>(b));
                if (isIncluded) {
                    if (layer.isAdditive) {
                        outPose[b].translation = outPose[b].translation + layerPose[b].translation * w;
                        outPose[b].rotation = Quat::Nlerp(outPose[b].rotation, layerPose[b].rotation, w);
                    } else { // Override
                        outPose[b] = BoneTransform::Lerp(outPose[b], layerPose[b], w);
                    }
                }
            }
        }
    }
}

std::string AnimGraph::ToQanimJson() const {
    // Format JSON matching Doc 07 §19
    std::ostringstream ss;
    ss << "{\"formatVersion\":1,\"parameters\":[";
    bool firstParam = true;
    for (const auto& [name, defVal] : m_floatParams) {
        if (!firstParam) ss << ",";
        ss << "{\"name\":\"" << name << "\",\"type\":\"float\",\"default\":" << defVal << "}";
        firstParam = false;
    }
    for (const auto& [name, defVal] : m_boolParams) {
        if (!firstParam) ss << ",";
        ss << "{\"name\":\"" << name << "\",\"type\":\"bool\",\"default\":" << (defVal ? "true" : "false") << "}";
        firstParam = false;
    }
    ss << "],\"layers\":[";
    for (size_t i = 0; i < m_layers.size(); ++i) {
        if (i > 0) ss << ",";
        ss << "{\"name\":\"" << m_layers[i].name << "\",\"weight\":" << m_layers[i].weight
           << ",\"blending\":\"" << (m_layers[i].isAdditive ? "additive" : "override") << "\"}";
    }
    ss << "]}";
    return ss.str();
}

std::shared_ptr<AnimGraph> AnimGraph::FromQanimJson(const std::string& json) {
    auto graph = std::make_shared<AnimGraph>();
    // Simple deserialization of formatVersion & parameters
    if (json.find("\"formatVersion\":1") == std::string::npos) {
        return graph;
    }

    size_t pos = 0;
    while ((pos = json.find("{\"name\":\"", pos)) != std::string::npos) {
        pos += 9;
        size_t endName = json.find("\"", pos);
        if (endName == std::string::npos) break;
        std::string name = json.substr(pos, endName - pos);

        if (json.find("\"type\":\"float\"", endName) != std::string::npos) {
            graph->SetFloat(name, 0.0f);
        } else if (json.find("\"type\":\"bool\"", endName) != std::string::npos) {
            graph->SetBool(name, false);
        }
        pos = endName;
    }

    return graph;
}

} // namespace qivforge::anim

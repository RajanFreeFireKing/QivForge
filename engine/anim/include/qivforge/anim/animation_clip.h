#pragma once

#include "qivforge/anim/anim_math.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace qivforge::anim {

struct AnimationEvent {
    float time = 0.0f; // in seconds
    std::string name;
    float floatPayload = 0.0f;
    std::string stringPayload;
};

struct KeyframeVec3 {
    float time = 0.0f;
    Vec3 value;
};

struct KeyframeQuat {
    float time = 0.0f;
    Quat value;
};

struct BoneTrack {
    int32_t boneIndex = -1;
    std::string boneName;
    std::vector<KeyframeVec3> translations;
    std::vector<KeyframeQuat> rotations;
    std::vector<KeyframeVec3> scales;

    [[nodiscard]] BoneTransform Sample(float time) const;
};

class AnimationClip {
public:
    explicit AnimationClip(std::string name = "", float duration = 1.0f, bool loop = true);

    void AddTrack(BoneTrack track);
    void AddEvent(float time, const std::string& name, float floatPayload = 0.0f, const std::string& stringPayload = "");

    void Sample(float time, size_t boneCount, std::vector<BoneTransform>& outPose) const;

    // Checks and triggers events that occurred between previousTime and currentTime
    void EvaluateEvents(float previousTime, float currentTime, const std::function<void(const AnimationEvent&)>& callback) const;

    [[nodiscard]] const std::string& GetName() const noexcept { return m_name; }
    [[nodiscard]] float GetDuration() const noexcept { return m_duration; }
    [[nodiscard]] bool IsLooping() const noexcept { return m_loop; }
    [[nodiscard]] const std::vector<BoneTrack>& GetTracks() const noexcept { return m_tracks; }
    [[nodiscard]] const std::vector<AnimationEvent>& GetEvents() const noexcept { return m_events; }

private:
    std::string m_name;
    float m_duration = 1.0f;
    bool m_loop = true;
    std::vector<BoneTrack> m_tracks;
    std::vector<AnimationEvent> m_events;
};

} // namespace qivforge::anim

#include "qivforge/anim/animation_clip.h"
#include <algorithm>

namespace qivforge::anim {

AnimationClip::AnimationClip(std::string name, float duration, bool loop)
    : m_name(std::move(name)), m_duration(std::max(0.001f, duration)), m_loop(loop) {}

void AnimationClip::AddTrack(BoneTrack track) {
    m_tracks.push_back(std::move(track));
}

void AnimationClip::AddEvent(float time, const std::string& name, float floatPayload, const std::string& stringPayload) {
    m_events.push_back({time, name, floatPayload, stringPayload});
}

static Vec3 SampleVec3Curve(const std::vector<KeyframeVec3>& keys, float time, const Vec3& defaultVal) {
    if (keys.empty()) return defaultVal;
    if (keys.size() == 1 || time <= keys.front().time) return keys.front().value;
    if (time >= keys.back().time) return keys.back().value;

    for (size_t i = 0; i + 1 < keys.size(); ++i) {
        if (time >= keys[i].time && time <= keys[i + 1].time) {
            float dt = keys[i + 1].time - keys[i].time;
            float t = (dt > 1e-6f) ? (time - keys[i].time) / dt : 0.0f;
            return Vec3::Lerp(keys[i].value, keys[i + 1].value, t);
        }
    }
    return keys.back().value;
}

static Quat SampleQuatCurve(const std::vector<KeyframeQuat>& keys, float time, const Quat& defaultVal) {
    if (keys.empty()) return defaultVal;
    if (keys.size() == 1 || time <= keys.front().time) return keys.front().value;
    if (time >= keys.back().time) return keys.back().value;

    for (size_t i = 0; i + 1 < keys.size(); ++i) {
        if (time >= keys[i].time && time <= keys[i + 1].time) {
            float dt = keys[i + 1].time - keys[i].time;
            float t = (dt > 1e-6f) ? (time - keys[i].time) / dt : 0.0f;
            return Quat::Nlerp(keys[i].value, keys[i + 1].value, t);
        }
    }
    return keys.back().value;
}

BoneTransform BoneTrack::Sample(float time) const {
    BoneTransform bt;
    bt.translation = SampleVec3Curve(translations, time, Vec3(0, 0, 0));
    bt.rotation = SampleQuatCurve(rotations, time, Quat::Identity());
    bt.scale = SampleVec3Curve(scales, time, Vec3(1, 1, 1));
    return bt;
}

void AnimationClip::Sample(float time, size_t boneCount, std::vector<BoneTransform>& outPose) const {
    outPose.resize(boneCount);

    // Normalize or loop time
    float t = time;
    if (m_loop) {
        t = std::fmod(time, m_duration);
        if (t < 0.0f) t += m_duration;
    } else {
        t = std::clamp(time, 0.0f, m_duration);
    }

    for (const auto& track : m_tracks) {
        if (track.boneIndex >= 0 && static_cast<size_t>(track.boneIndex) < boneCount) {
            outPose[track.boneIndex] = track.Sample(t);
        }
    }
}

void AnimationClip::EvaluateEvents(float previousTime, float currentTime, const std::function<void(const AnimationEvent&)>& callback) const {
    if (!callback) return;

    for (const auto& ev : m_events) {
        if (m_loop) {
            float p = std::fmod(previousTime, m_duration);
            float c = std::fmod(currentTime, m_duration);
            if (p <= c) {
                if (ev.time >= p && ev.time < c) callback(ev);
            } else { // Wrapped around loop boundary
                if (ev.time >= p || ev.time < c) callback(ev);
            }
        } else {
            if (ev.time >= previousTime && ev.time < currentTime) {
                callback(ev);
            }
        }
    }
}

} // namespace qivforge::anim

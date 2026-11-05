#pragma once

#include "qivforge/audio/audio_clip.h"
#include <memory>
#include <cmath>
#include <algorithm>

namespace qivforge::audio {

class AudioEmitter {
public:
    AudioEmitter() = default;

    void SetPosition(float x, float y, float z) noexcept {
        m_posX = x; m_posY = y; m_posZ = z;
    }

    void SetVolume(float vol) noexcept { m_volume = std::clamp(vol, 0.0f, 1.0f); }
    [[nodiscard]] float GetVolume() const noexcept { return m_volume; }

    void SetPitch(float pitch) noexcept { m_pitch = std::clamp(pitch, 0.1f, 3.0f); }
    [[nodiscard]] float GetPitch() const noexcept { return m_pitch; }

    void SetMinDistance(float minD) noexcept { m_minDistance = std::max(0.1f, minD); }
    void SetMaxDistance(float maxD) noexcept { m_maxDistance = std::max(m_minDistance, maxD); }

    // 3D Spatial Attenuation (Doc 10 §13)
    [[nodiscard]] float CalculateGain(float listenerX, float listenerY, float listenerZ) const noexcept {
        float dx = m_posX - listenerX;
        float dy = m_posY - listenerY;
        float dz = m_posZ - listenerZ;
        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

        if (dist <= m_minDistance) return m_volume;
        if (dist >= m_maxDistance) return 0.0f;

        float attenuation = m_minDistance / (m_minDistance + (dist - m_minDistance));
        return std::clamp(m_volume * attenuation, 0.0f, 1.0f);
    }

    // Vehicle engine pitch scaling (Doc 08 §15 & Doc 15 M6)
    // Formula: pitch = 0.9 + speedKmh / 55.0
    [[nodiscard]] static float CalculateEnginePitch(float speedKmh) noexcept {
        float p = 0.9f + (speedKmh / 55.0f);
        return std::clamp(p, 0.7f, 3.0f);
    }

private:
    float m_posX = 0.0f;
    float m_posY = 0.0f;
    float m_posZ = 0.0f;

    float m_volume = 1.0f;
    float m_pitch = 1.0f;
    float m_minDistance = 2.0f;
    float m_maxDistance = 50.0f;
};

} // namespace qivforge::audio

#include "qivforge/audio/audio_manager.h"
#include <cmath>

namespace qivforge::audio {

void EngineSoundBlender::Update(float rpm, float redline, float throttle, float dt) noexcept {
    float normRpm = std::clamp(rpm / std::max(100.0f, redline), 0.0f, 1.0f);

    // Doc 10 §C4 formula: pitch = 0.6 + rpm / redline * 1.6
    float targetPitch = 0.6f + normRpm * 1.6f;
    float targetVol = 0.5f + std::clamp(throttle, 0.0f, 1.0f) * 0.5f;

    // Smooth exponential filtering to prevent clicks/pops (Doc 10 AC5)
    float blendFactor = 1.0f - std::exp(-15.0f * dt);
    currentPitch += (targetPitch - currentPitch) * blendFactor;
    currentVolume += (targetVol - currentVolume) * blendFactor;

    // 3-sample blend: idle (0..0.4), mid (0.2..0.8), high (0.6..1.0)
    if (normRpm <= 0.35f) {
        float t = normRpm / 0.35f;
        idleSampleWeight = 1.0f - t;
        midSampleWeight = t;
        highSampleWeight = 0.0f;
    } else if (normRpm <= 0.75f) {
        float t = (normRpm - 0.35f) / 0.40f;
        idleSampleWeight = 0.0f;
        midSampleWeight = 1.0f - t;
        highSampleWeight = t;
    } else {
        idleSampleWeight = 0.0f;
        midSampleWeight = 0.0f;
        highSampleWeight = 1.0f;
    }
}

AudioManager::AudioManager() {
    for (auto& v : m_voices) {
        v.active = false;
        v.voiceId = -1;
    }
}

int32_t AudioManager::PlaySound(const std::string& name, AudioBusType bus, float volume, float pitch, int32_t priority) {
    return PlaySound3D(name, bus, m_listenerX, m_listenerY, m_listenerZ, volume, pitch, 2.0f, 50.0f, DistanceModel::INVERSE, priority);
}

int32_t AudioManager::PlaySound3D(const std::string& name, AudioBusType bus,
                                  float x, float y, float z,
                                  float volume, float pitch,
                                  float minD, float maxD,
                                  DistanceModel model,
                                  int32_t priority) {
    // 1. Look for free voice
    int32_t slot = -1;
    for (size_t i = 0; i < MAX_VOICES; ++i) {
        if (!m_voices[i].active) {
            slot = static_cast<int32_t>(i);
            break;
        }
    }

    // 2. If all 32 voices occupied, perform voice stealing (Doc 10 AC6)
    float dx = x - m_listenerX;
    float dy = y - m_listenerY;
    float dz = z - m_listenerZ;
    float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

    if (slot < 0) {
        slot = StealVoice(priority, dist);
    }

    if (slot < 0) {
        return -1; // Sound is lower priority than all current active voices
    }

    int32_t id = m_nextVoiceId++;
    auto& v = m_voices[slot];
    v.voiceId = id;
    v.soundName = name;
    v.bus = bus;
    v.posX = x; v.posY = y; v.posZ = z;
    v.volume = std::clamp(volume, 0.0f, 1.0f);
    v.pitch = std::clamp(pitch, 0.1f, 3.0f);
    v.priority = priority;
    v.is3D = true;
    v.minDistance = minD;
    v.maxDistance = maxD;
    v.distanceModel = model;
    v.active = true;
    v.playbackPos = 0.0f;

    // Trigger ducking if playing Voice bus
    if (bus == AudioBusType::VOICE) {
        m_mixer.SetMusicDucking(true);
    }

    return id;
}

int32_t AudioManager::StealVoice(int32_t newPriority, float newDist) {
    int32_t victimSlot = -1;
    int32_t lowestPriority = newPriority;
    float largestDist = newDist;

    for (size_t i = 0; i < MAX_VOICES; ++i) {
        const auto& v = m_voices[i];
        if (!v.active) return static_cast<int32_t>(i);

        float dx = v.posX - m_listenerX;
        float dy = v.posY - m_listenerY;
        float dz = v.posZ - m_listenerZ;
        float d = std::sqrt(dx * dx + dy * dy + dz * dz);

        if (v.priority < lowestPriority || (v.priority == lowestPriority && d > largestDist)) {
            lowestPriority = v.priority;
            largestDist = d;
            victimSlot = static_cast<int32_t>(i);
        }
    }

    return victimSlot;
}

void AudioManager::StopSound(int32_t voiceId) noexcept {
    bool hadVoice = false;
    for (auto& v : m_voices) {
        if (v.active && v.voiceId == voiceId) {
            v.active = false;
            v.voiceId = -1;
        }
        if (v.active && v.bus == AudioBusType::VOICE) {
            hadVoice = true;
        }
    }
    if (!hadVoice) {
        m_mixer.SetMusicDucking(false);
    }
}

void AudioManager::StopAll() noexcept {
    for (auto& v : m_voices) {
        v.active = false;
        v.voiceId = -1;
    }
    m_mixer.SetMusicDucking(false);
}

size_t AudioManager::GetActiveVoiceCount() const noexcept {
    size_t count = 0;
    for (const auto& v : m_voices) {
        if (v.active) count++;
    }
    return count;
}

float AudioManager::ComputeSpatialGain(const ActiveVoice& voice) const noexcept {
    if (!voice.is3D) return voice.volume;

    float dx = voice.posX - m_listenerX;
    float dy = voice.posY - m_listenerY;
    float dz = voice.posZ - m_listenerZ;
    float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

    if (dist <= voice.minDistance) return voice.volume;
    if (dist >= voice.maxDistance) return 0.0f;

    float attenuation = 1.0f;
    switch (voice.distanceModel) {
        case DistanceModel::INVERSE:
            attenuation = voice.minDistance / (voice.minDistance + (dist - voice.minDistance));
            break;
        case DistanceModel::LINEAR:
            attenuation = 1.0f - (dist - voice.minDistance) / (voice.maxDistance - voice.minDistance);
            break;
        case DistanceModel::LOGARITHMIC:
            attenuation = 1.0f - std::log10(1.0f + 9.0f * (dist - voice.minDistance) / (voice.maxDistance - voice.minDistance));
            break;
    }

    float busGain = m_mixer.GetEffectiveBusGain(voice.bus);
    return std::clamp(voice.volume * attenuation * busGain, 0.0f, 1.0f);
}

void AudioManager::PlayMusic(const std::string& trackName, float fadeDuration) {
    if (m_currentTrack.empty()) {
        m_currentTrack = trackName;
        m_musicFadeGain = 1.0f;
    } else {
        m_fadingTrack = m_currentTrack;
        m_currentTrack = trackName;
        m_musicFadeDuration = std::max(0.1f, fadeDuration);
        m_musicFadeTimer = m_musicFadeDuration;
        m_musicFadeGain = 0.0f;
    }
}

void AudioManager::Update(float deltaTime) {
    // 1. Crossfade music
    if (m_musicFadeTimer > 0.0f) {
        m_musicFadeTimer -= deltaTime;
        float progress = 1.0f - std::clamp(m_musicFadeTimer / m_musicFadeDuration, 0.0f, 1.0f);
        m_musicFadeGain = progress; // Fades new track in
        if (m_musicFadeTimer <= 0.0f) {
            m_fadingTrack.clear();
            m_musicFadeGain = 1.0f;
        }
    }

    // 2. Check active voice expiration
    bool hasVoice = false;
    for (auto& v : m_voices) {
        if (v.active) {
            v.playbackPos += deltaTime * v.pitch;
            if (v.bus == AudioBusType::VOICE) hasVoice = true;
        }
    }
    if (!hasVoice && m_mixer.IsMusicDucking()) {
        m_mixer.SetMusicDucking(false);
    }
}

} // namespace qivforge::audio

#pragma once

#include "qivforge/audio/audio_mixer.h"
#include "qivforge/audio/audio_clip.h"
#include "qivforge/audio/audio_emitter.h"
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <cmath>

namespace qivforge::audio {

enum class DistanceModel : uint8_t {
    INVERSE = 0,
    LINEAR = 1,
    LOGARITHMIC = 2
};

struct ActiveVoice {
    int32_t voiceId = -1;
    std::string soundName;
    AudioBusType bus = AudioBusType::SFX;
    float posX = 0.0f, posY = 0.0f, posZ = 0.0f;
    float volume = 1.0f;
    float pitch = 1.0f;
    int32_t priority = 128; // Higher = more important
    bool is3D = false;
    float minDistance = 2.0f;
    float maxDistance = 50.0f;
    DistanceModel distanceModel = DistanceModel::INVERSE;
    bool active = false;
    float playbackPos = 0.0f;
};

// 3-sample blend for vehicle engine sound (Doc 10 §C4 & AC5)
struct EngineSoundBlender {
    float idleSampleWeight = 1.0f;
    float midSampleWeight = 0.0f;
    float highSampleWeight = 0.0f;
    float currentPitch = 1.0f;
    float currentVolume = 1.0f;

    void Update(float rpm, float redline, float throttle, float dt) noexcept;
};

class AudioManager {
public:
    static constexpr size_t MAX_VOICES = 32; // Doc 10 §C4 & AC6

    AudioManager();

    [[nodiscard]] AudioMixer& GetMixer() noexcept { return m_mixer; }
    [[nodiscard]] const AudioMixer& GetMixer() const noexcept { return m_mixer; }

    void SetListenerPosition(float x, float y, float z) noexcept {
        m_listenerX = x; m_listenerY = y; m_listenerZ = z;
    }

    // Voice playback with priority stealing (Doc 10 AC6)
    int32_t PlaySound(const std::string& name, AudioBusType bus, float volume = 1.0f,
                      float pitch = 1.0f, int32_t priority = 128);

    int32_t PlaySound3D(const std::string& name, AudioBusType bus,
                        float x, float y, float z,
                        float volume = 1.0f, float pitch = 1.0f,
                        float minD = 2.0f, float maxD = 50.0f,
                        DistanceModel model = DistanceModel::INVERSE,
                        int32_t priority = 128);

    void StopSound(int32_t voiceId) noexcept;
    void StopAll() noexcept;

    [[nodiscard]] size_t GetActiveVoiceCount() const noexcept;
    [[nodiscard]] const std::array<ActiveVoice, MAX_VOICES>& GetVoices() const noexcept { return m_voices; }

    // Evaluates spatial gain and distance model
    [[nodiscard]] float ComputeSpatialGain(const ActiveVoice& voice) const noexcept;

    // Music playlist with crossfade (Doc 10 §C4)
    void PlayMusic(const std::string& trackName, float fadeDuration = 2.0f);
    void Update(float deltaTime);

    [[nodiscard]] const std::string& GetCurrentMusicTrack() const noexcept { return m_currentTrack; }
    [[nodiscard]] float GetMusicCrossfadeGain() const noexcept { return m_musicFadeGain; }

    [[nodiscard]] EngineSoundBlender& GetEngineSound() noexcept { return m_engineBlender; }

private:
    AudioMixer m_mixer;
    std::array<ActiveVoice, MAX_VOICES> m_voices;
    int32_t m_nextVoiceId = 1;

    float m_listenerX = 0.0f;
    float m_listenerY = 0.0f;
    float m_listenerZ = 0.0f;

    // Music playlist crossfade state
    std::string m_currentTrack;
    std::string m_fadingTrack;
    float m_musicFadeDuration = 2.0f;
    float m_musicFadeTimer = 0.0f;
    float m_musicFadeGain = 1.0f;

    EngineSoundBlender m_engineBlender;

    int32_t StealVoice(int32_t newPriority, float newDist);
};

} // namespace qivforge::audio

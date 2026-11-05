#pragma once

#include <string>
#include <array>
#include <algorithm>
#include <cstdint>

namespace qivforge::audio {

enum class AudioBusType : uint8_t {
    MASTER = 0,
    MUSIC = 1,
    SFX = 2,
    VOICE = 3,
    AMBIENT = 4,
    UI = 5,
    COUNT = 6
};

struct AudioBus {
    std::string name;
    float volume = 1.0f;
    bool muted = false;
    bool solo = false;
};

class AudioMixer {
public:
    AudioMixer();

    void SetBusVolume(AudioBusType bus, float volume) noexcept;
    [[nodiscard]] float GetBusVolume(AudioBusType bus) const noexcept;

    void SetBusMuted(AudioBusType bus, bool muted) noexcept;
    [[nodiscard]] bool IsBusMuted(AudioBusType bus) const noexcept;

    void SetBusSolo(AudioBusType bus, bool solo) noexcept;
    [[nodiscard]] bool IsBusSolo(AudioBusType bus) const noexcept;

    // Ducking: lowers Music when Voice is playing (Doc 10 §C4)
    void SetMusicDucking(bool active, float duckAmount = 0.4f) noexcept;
    [[nodiscard]] bool IsMusicDucking() const noexcept { return m_duckingActive; }

    // Evaluates final bus gain accounting for Master, Mute, Solo, and Ducking
    [[nodiscard]] float GetEffectiveBusGain(AudioBusType bus) const noexcept;

private:
    std::array<AudioBus, static_cast<size_t>(AudioBusType::COUNT)> m_buses;
    bool m_hasSolo = false;
    bool m_duckingActive = false;
    float m_duckAmount = 0.4f;

    void UpdateSoloState() noexcept;
};

} // namespace qivforge::audio

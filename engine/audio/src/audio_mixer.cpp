#include "qivforge/audio/audio_mixer.h"

namespace qivforge::audio {

AudioMixer::AudioMixer() {
    m_buses[static_cast<size_t>(AudioBusType::MASTER)].name = "Master";
    m_buses[static_cast<size_t>(AudioBusType::MUSIC)].name = "Music";
    m_buses[static_cast<size_t>(AudioBusType::SFX)].name = "SFX";
    m_buses[static_cast<size_t>(AudioBusType::VOICE)].name = "Voice";
    m_buses[static_cast<size_t>(AudioBusType::AMBIENT)].name = "Ambient";
    m_buses[static_cast<size_t>(AudioBusType::UI)].name = "UI";
}

void AudioMixer::SetBusVolume(AudioBusType bus, float volume) noexcept {
    size_t idx = static_cast<size_t>(bus);
    if (idx < m_buses.size()) {
        m_buses[idx].volume = std::clamp(volume, 0.0f, 1.0f);
    }
}

float AudioMixer::GetBusVolume(AudioBusType bus) const noexcept {
    size_t idx = static_cast<size_t>(bus);
    if (idx < m_buses.size()) {
        return m_buses[idx].volume;
    }
    return 1.0f;
}

void AudioMixer::SetBusMuted(AudioBusType bus, bool muted) noexcept {
    size_t idx = static_cast<size_t>(bus);
    if (idx < m_buses.size()) {
        m_buses[idx].muted = muted;
    }
}

bool AudioMixer::IsBusMuted(AudioBusType bus) const noexcept {
    size_t idx = static_cast<size_t>(bus);
    if (idx < m_buses.size()) {
        return m_buses[idx].muted;
    }
    return false;
}

void AudioMixer::SetBusSolo(AudioBusType bus, bool solo) noexcept {
    size_t idx = static_cast<size_t>(bus);
    if (idx < m_buses.size()) {
        m_buses[idx].solo = solo;
        UpdateSoloState();
    }
}

bool AudioMixer::IsBusSolo(AudioBusType bus) const noexcept {
    size_t idx = static_cast<size_t>(bus);
    if (idx < m_buses.size()) {
        return m_buses[idx].solo;
    }
    return false;
}

void AudioMixer::UpdateSoloState() noexcept {
    m_hasSolo = false;
    for (const auto& b : m_buses) {
        if (b.solo) {
            m_hasSolo = true;
            break;
        }
    }
}

void AudioMixer::SetMusicDucking(bool active, float duckAmount) noexcept {
    m_duckingActive = active;
    m_duckAmount = std::clamp(duckAmount, 0.0f, 1.0f);
}

float AudioMixer::GetEffectiveBusGain(AudioBusType bus) const noexcept {
    size_t masterIdx = static_cast<size_t>(AudioBusType::MASTER);
    if (m_buses[masterIdx].muted) return 0.0f;

    size_t idx = static_cast<size_t>(bus);
    if (idx >= m_buses.size()) return 0.0f;

    const auto& currentBus = m_buses[idx];
    if (currentBus.muted) return 0.0f;

    // Solo logic: if any bus is solo, non-solo buses are silenced (Master is exempt)
    if (m_hasSolo && !currentBus.solo && bus != AudioBusType::MASTER) {
        return 0.0f;
    }

    float gain = m_buses[masterIdx].volume * currentBus.volume;

    // Apply ducking to Music bus if voice is playing
    if (bus == AudioBusType::MUSIC && m_duckingActive) {
        gain *= m_duckAmount;
    }

    return std::clamp(gain, 0.0f, 1.0f);
}

} // namespace qivforge::audio

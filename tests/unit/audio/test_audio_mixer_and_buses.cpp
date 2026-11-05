#include "qivforge/audio/audio_mixer.h"
#include "qivforge/audio/audio_manager.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace qivforge::audio;

void Test6BusesGainAndSoloDucking() {
    std::cout << "[RUN] Test6BusesGainAndSoloDucking (Doc 10 §C4)..." << std::endl;

    AudioMixer mixer;

    // Default: all buses have volume 1.0, not muted
    assert(mixer.GetEffectiveBusGain(AudioBusType::MASTER) == 1.0f);
    assert(mixer.GetEffectiveBusGain(AudioBusType::MUSIC) == 1.0f);
    assert(mixer.GetEffectiveBusGain(AudioBusType::SFX) == 1.0f);

    // Set Master volume to 0.8
    mixer.SetBusVolume(AudioBusType::MASTER, 0.8f);
    assert(std::abs(mixer.GetEffectiveBusGain(AudioBusType::SFX) - 0.8f) < 0.001f);

    // Mute SFX
    mixer.SetBusMuted(AudioBusType::SFX, true);
    assert(mixer.GetEffectiveBusGain(AudioBusType::SFX) == 0.0f);
    assert(std::abs(mixer.GetEffectiveBusGain(AudioBusType::MUSIC) - 0.8f) < 0.001f);
    mixer.SetBusMuted(AudioBusType::SFX, false);

    // Test Solo: Soloing SFX should silence all non-solo buses
    mixer.SetBusSolo(AudioBusType::SFX, true);
    assert(std::abs(mixer.GetEffectiveBusGain(AudioBusType::SFX) - 0.8f) < 0.001f);
    assert(mixer.GetEffectiveBusGain(AudioBusType::MUSIC) == 0.0f);
    assert(mixer.GetEffectiveBusGain(AudioBusType::AMBIENT) == 0.0f);
    mixer.SetBusSolo(AudioBusType::SFX, false);

    // Test Ducking: When Voice ducking is active, Music bus gain drops
    mixer.SetMusicDucking(true, 0.4f);
    float duckedMusic = mixer.GetEffectiveBusGain(AudioBusType::MUSIC);
    // 0.8 * 1.0 * 0.4 = 0.32
    assert(std::abs(duckedMusic - 0.32f) < 0.001f);
    assert(std::abs(mixer.GetEffectiveBusGain(AudioBusType::SFX) - 0.8f) < 0.001f); // SFX unaffected
    mixer.SetMusicDucking(false);

    std::cout << "[PASS] Test6BusesGainAndSoloDucking" << std::endl;
}

void TestEnginePitchAndVolumeSmoothing() {
    std::cout << "[RUN] TestEnginePitchAndVolumeSmoothing (Doc 10 AC5)..." << std::endl;

    EngineSoundBlender blender;

    // Simulate aggressive vehicle acceleration: RPM ramps from 800 to 6000 (redline 6000)
    float lastPitch = blender.currentPitch;
    float maxPitchDelta = 0.0f;

    for (int frame = 0; frame < 120; ++frame) {
        float rpm = 800.0f + (static_cast<float>(frame) / 119.0f) * 5200.0f;
        blender.Update(rpm, 6000.0f, 1.0f, 0.016f);

        float delta = std::abs(blender.currentPitch - lastPitch);
        maxPitchDelta = std::max(maxPitchDelta, delta);
        lastPitch = blender.currentPitch;
    }

    // Hold at redline for 20 frames to reach steady state
    for (int frame = 0; frame < 20; ++frame) {
        blender.Update(6000.0f, 6000.0f, 1.0f, 0.016f);
    }

    // Doc 10 AC5: Smooth RPM pitch transition with no pops or clicks
    // Max frame-to-frame delta must be small (< 0.1)
    std::cout << "  Max pitch delta per frame during acceleration: " << maxPitchDelta << std::endl;
    assert(maxPitchDelta < 0.10f);

    // At redline, pitch formula 0.6 + (rpm / redline) * 1.6 = 2.2
    assert(std::abs(blender.currentPitch - 2.2f) < 0.05f);
    assert(blender.highSampleWeight > 0.8f);

    std::cout << "[PASS] TestEnginePitchAndVolumeSmoothing" << std::endl;
}

void Test32VoiceLimitAndStealing() {
    std::cout << "[RUN] Test32VoiceLimitAndStealing (Doc 10 AC6)..." << std::endl;

    AudioManager audioMgr;
    audioMgr.SetListenerPosition(0, 0, 0);

    // Trigger 32 sounds
    for (int i = 0; i < 32; ++i) {
        // Priority 100, placed at distance (i * 2) meters
        int32_t id = audioMgr.PlaySound3D("Sound_" + std::to_string(i), AudioBusType::SFX,
                                          static_cast<float>(i * 2), 0, 0, 1.0f, 1.0f,
                                          2.0f, 50.0f, DistanceModel::INVERSE, 100);
        assert(id > 0);
    }
    assert(audioMgr.GetActiveVoiceCount() == 32);

    // Trigger 8 MORE sounds (total 40 sounds triggered) with HIGH priority 200 close to listener
    for (int i = 0; i < 8; ++i) {
        int32_t id = audioMgr.PlaySound3D("HighPriority_" + std::to_string(i), AudioBusType::SFX,
                                          1.0f, 0, 0, 1.0f, 1.0f,
                                          2.0f, 50.0f, DistanceModel::INVERSE, 200);
        assert(id > 0);
    }

    // Doc 10 AC6: Voice count stays capped at exactly 32 (never underruns)
    assert(audioMgr.GetActiveVoiceCount() == 32);

    // Verify that the high priority sounds were accommodated and the farthest low-priority sounds were stolen
    size_t highPriorityCount = 0;
    for (const auto& v : audioMgr.GetVoices()) {
        if (v.active && v.priority == 200) {
            highPriorityCount++;
        }
    }
    assert(highPriorityCount == 8);

    std::cout << "[PASS] Test32VoiceLimitAndStealing" << std::endl;
}

void TestMusicPlaylistCrossfade() {
    std::cout << "[RUN] TestMusicPlaylistCrossfade (Doc 10 §C4)..." << std::endl;

    AudioManager audioMgr;
    audioMgr.PlayMusic("TrackA.wav", 2.0f);
    assert(audioMgr.GetCurrentMusicTrack() == "TrackA.wav");
    assert(audioMgr.GetMusicCrossfadeGain() == 1.0f);

    // Switch to Track B with 2.0s fade
    audioMgr.PlayMusic("TrackB.wav", 2.0f);
    assert(audioMgr.GetCurrentMusicTrack() == "TrackB.wav");
    assert(audioMgr.GetMusicCrossfadeGain() == 0.0f);

    // Step 1.0s (halfway)
    audioMgr.Update(1.0f);
    assert(std::abs(audioMgr.GetMusicCrossfadeGain() - 0.5f) < 0.05f);

    // Step another 1.0s (complete)
    audioMgr.Update(1.0f);
    assert(audioMgr.GetMusicCrossfadeGain() == 1.0f);

    std::cout << "[PASS] TestMusicPlaylistCrossfade" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "     AUDIO MIXER & VOICES TEST (M9)     " << std::endl;
    std::cout << "========================================" << std::endl;

    Test6BusesGainAndSoloDucking();
    TestEnginePitchAndVolumeSmoothing();
    Test32VoiceLimitAndStealing();
    TestMusicPlaylistCrossfade();

    std::cout << "ALL AUDIO MIXER TESTS PASSED!" << std::endl;
    return 0;
}

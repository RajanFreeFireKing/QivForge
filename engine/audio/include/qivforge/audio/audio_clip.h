#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <cmath>
#include <algorithm>

namespace qivforge::audio {

struct WavHeader {
    char riff[4];
    uint32_t fileSize;
    char wave[4];
    char fmt[4];
    uint32_t fmtSize;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char data[4];
    uint32_t dataSize;
};

class AudioClip {
public:
    std::string name;
    uint32_t sampleRate = 44100;
    uint16_t channels = 2;
    uint16_t bitsPerSample = 16;
    float durationSeconds = 0.0f;
    std::vector<uint8_t> pcmData;

    static bool ParseWavHeader(const uint8_t* bytes, size_t size, AudioClip& outClip) {
        if (!bytes || size < sizeof(WavHeader)) return false;

        const auto* hdr = reinterpret_cast<const WavHeader*>(bytes);
        if (std::memcmp(hdr->riff, "RIFF", 4) != 0 || std::memcmp(hdr->wave, "WAVE", 4) != 0) {
            return false;
        }

        outClip.sampleRate = hdr->sampleRate;
        outClip.channels = hdr->numChannels;
        outClip.bitsPerSample = hdr->bitsPerSample;

        if (hdr->byteRate > 0) {
            outClip.durationSeconds = static_cast<float>(hdr->dataSize) / static_cast<float>(hdr->byteRate);
        }

        size_t dataOffset = sizeof(WavHeader);
        if (size >= dataOffset + hdr->dataSize) {
            outClip.pcmData.assign(bytes + dataOffset, bytes + dataOffset + hdr->dataSize);
        }

        return true;
    }
};

} // namespace qivforge::audio

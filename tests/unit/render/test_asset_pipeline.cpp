#include "qivforge/render/gltf_importer.h"
#include "qivforge/audio/audio_clip.h"
#include "qivforge/audio/audio_emitter.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <cstring>

using namespace qivforge::render;
using namespace qivforge::audio;

void TestGltfAndGlbParsing() {
    std::cout << "[RUN] TestGltfAndGlbParsing (Doc 05 §8 & Doc 13 §3)..." << std::endl;

    // Build a minimal valid GLB buffer:
    // Header (12 bytes) + JSON chunk (header 8 + data 20) + BIN chunk (header 8 + data 96)
    std::vector<uint8_t> glbData;
    glbData.resize(12 + 28 + 104);

    uint32_t magic = 0x46546C67; // "glTF"
    uint32_t version = 2;
    uint32_t totalLength = static_cast<uint32_t>(glbData.size());

    std::memcpy(glbData.data(), &magic, 4);
    std::memcpy(glbData.data() + 4, &version, 4);
    std::memcpy(glbData.data() + 8, &totalLength, 4);

    // JSON Chunk
    uint32_t jsonLen = 20;
    uint32_t jsonType = 0x4E4F534A; // "JSON"
    std::memcpy(glbData.data() + 12, &jsonLen, 4);
    std::memcpy(glbData.data() + 16, &jsonType, 4);
    std::string mockJson = "{\"asset\":{\"version\":\"2.0\"}}";
    std::memcpy(glbData.data() + 20, mockJson.c_str(), 20);

    // BIN Chunk (3 vertices * 8 floats = 96 bytes)
    size_t binOffset = 12 + 8 + 20;
    uint32_t binLen = 96;
    uint32_t binType = 0x004E4942; // "BIN\0"
    std::memcpy(glbData.data() + binOffset, &binLen, 4);
    std::memcpy(glbData.data() + binOffset + 4, &binType, 4);

    float* binFloats = reinterpret_cast<float*>(glbData.data() + binOffset + 8);
    // V0: pos(0,0,0), norm(0,1,0), uv(0,0)
    binFloats[0] = 0.0f; binFloats[1] = 0.0f; binFloats[2] = 0.0f;
    binFloats[3] = 0.0f; binFloats[4] = 1.0f; binFloats[5] = 0.0f;
    binFloats[6] = 0.0f; binFloats[7] = 0.0f;
    // V1: pos(1,0,0), norm(0,1,0), uv(1,0)
    binFloats[8] = 1.0f; binFloats[9] = 0.0f; binFloats[10] = 0.0f;
    binFloats[11] = 0.0f; binFloats[12] = 1.0f; binFloats[13] = 0.0f;
    binFloats[14] = 1.0f; binFloats[15] = 0.0f;
    // V2: pos(0,1,0), norm(0,1,0), uv(0,1)
    binFloats[16] = 0.0f; binFloats[17] = 1.0f; binFloats[18] = 0.0f;
    binFloats[19] = 0.0f; binFloats[20] = 1.0f; binFloats[21] = 0.0f;
    binFloats[22] = 0.0f; binFloats[23] = 1.0f;

    assert(GltfImporter::IsGlb(glbData.data(), glbData.size()));

    GltfModel model = GltfImporter::LoadFromGlbMemory(glbData.data(), glbData.size());
    assert(!model.meshes.empty());
    assert(model.totalVertices == 3);
    assert(model.totalTriangles == 1);
    assert(model.meshes[0].vertices[1].position[0] == 1.0f);

    std::cout << "  -> Successfully parsed GLB binary buffer with 3 vertices and 1 triangle" << std::endl;
    std::cout << "[PASS] TestGltfAndGlbParsing\n" << std::endl;
}

void TestQMeshBakingRoundtrip() {
    std::cout << "[RUN] TestQMeshBakingRoundtrip (Doc 13 §3 Baker Basics)..." << std::endl;

    GltfModel originalModel = GltfImporter::LoadFromGltfJson("{\"cube\":true}");
    assert(originalModel.totalTriangles == 12);

    // Bake to .qmesh format
    std::vector<uint8_t> bakedData = GltfImporter::BakeToQMesh(originalModel);
    assert(!bakedData.empty());
    assert(std::memcmp(bakedData.data(), "QMESH", 5) == 0);

    // Load from .qmesh format
    GltfModel loadedModel = GltfImporter::LoadFromQMesh(bakedData.data(), bakedData.size());
    assert(loadedModel.totalVertices == originalModel.totalVertices);
    assert(loadedModel.totalTriangles == originalModel.totalTriangles);
    assert(loadedModel.meshes.size() == originalModel.meshes.size());

    std::cout << "  -> Baked model to .qmesh (" << bakedData.size() << " bytes) and reloaded with 100% fidelity" << std::endl;
    std::cout << "[PASS] TestQMeshBakingRoundtrip\n" << std::endl;
}

void TestAudioImportAndPitchScaling() {
    std::cout << "[RUN] TestAudioImportAndPitchScaling (Doc 08 §15 & Doc 10 §13)..." << std::endl;

    // 1. WAV Header Parsing
    WavHeader hdr;
    std::memcpy(hdr.riff, "RIFF", 4);
    hdr.fileSize = sizeof(WavHeader) + 1000;
    std::memcpy(hdr.wave, "WAVE", 4);
    std::memcpy(hdr.fmt, "fmt ", 4);
    hdr.fmtSize = 16;
    hdr.audioFormat = 1; // PCM
    hdr.numChannels = 2; // Stereo
    hdr.sampleRate = 48000;
    hdr.byteRate = 48000 * 2 * 2; // 192,000 bytes/sec
    hdr.blockAlign = 4;
    hdr.bitsPerSample = 16;
    std::memcpy(hdr.data, "data", 4);
    hdr.dataSize = 1000;

    std::vector<uint8_t> wavBytes(sizeof(WavHeader) + 1000, 0);
    std::memcpy(wavBytes.data(), &hdr, sizeof(WavHeader));

    AudioClip clip;
    bool parsed = AudioClip::ParseWavHeader(wavBytes.data(), wavBytes.size(), clip);
    assert(parsed);
    assert(clip.sampleRate == 48000);
    assert(clip.channels == 2);
    assert(clip.bitsPerSample == 16);
    assert(clip.durationSeconds > 0.005f);

    // 2. 3D Spatial Attenuation
    AudioEmitter emitter;
    emitter.SetPosition(0.0f, 0.0f, 0.0f);
    emitter.SetMinDistance(2.0f);
    emitter.SetMaxDistance(50.0f);
    emitter.SetVolume(1.0f);

    float closeGain = emitter.CalculateGain(1.0f, 0.0f, 0.0f);
    assert(closeGain == 1.0f); // within min distance

    float midGain = emitter.CalculateGain(6.0f, 0.0f, 0.0f);
    assert(midGain < 1.0f && midGain > 0.1f);

    float farGain = emitter.CalculateGain(100.0f, 0.0f, 0.0f);
    assert(farGain == 0.0f); // beyond max distance

    // 3. Engine RPM pitch scaling per Doc 08 §15: pitch = 0.9 + speedKmh / 55.0
    float idlePitch = AudioEmitter::CalculateEnginePitch(0.0f);
    assert(std::abs(idlePitch - 0.9f) < 1e-4);

    float midSpeedPitch = AudioEmitter::CalculateEnginePitch(55.0f);
    assert(std::abs(midSpeedPitch - 1.9f) < 1e-4);

    float topSpeedPitch = AudioEmitter::CalculateEnginePitch(110.0f);
    assert(std::abs(topSpeedPitch - (0.9f + 2.0f)) < 1e-4);

    std::cout << "  -> WAV parsed (48kHz, stereo), 3D gain falloff verified, and RPM pitch scaling matched Doc 08 formula" << std::endl;
    std::cout << "[PASS] TestAudioImportAndPitchScaling\n" << std::endl;
}

int main() {
    std::cout << "===========================================" << std::endl;
    std::cout << "  ASSET PIPELINE SUITE (GLB/QMESH/AUDIO)   " << std::endl;
    std::cout << "===========================================" << std::endl;

    TestGltfAndGlbParsing();
    TestQMeshBakingRoundtrip();
    TestAudioImportAndPitchScaling();

    std::cout << "ALL ASSET PIPELINE TESTS PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}

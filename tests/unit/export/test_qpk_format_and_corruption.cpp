#include "qivforge/export/qpk_format.h"
#include "qivforge/export/qpk_builder.h"
#include "qivforge/export/qpk_reader.h"
#include <iostream>
#include <cassert>
#include <chrono>

using namespace qivforge::export_pipeline;

void TestQpkPackAndMicrosecondLookup() {
    std::cout << "[RUN] TestQpkPackAndMicrosecondLookup (Doc 13 §4.1)..." << std::endl;

    QpkBuilder builder;
    builder.AddFileString("scripts/tractor.qvsc", "BYTECODE_DATA_FOR_TRACTOR", QpkAssetType::SCRIPT);
    builder.AddFileString("scenes/main.scene.json", "{\"actors\":[\"Tractor\",\"Camera\"]}", QpkAssetType::SCENE);
    builder.AddFileString("assets/textures/ground.png", "TEXTURE_BYTES_PNG", QpkAssetType::TEXTURE);
    builder.AddRemoteFile("assets/models/streamed_mesh.glb", "https://huggingface.co/repo/resolve/main/mesh.glb",
                          2048, "a1b2c3d4", QpkAssetType::MESH);

    assert(builder.GetFileCount() == 4);
    assert(builder.GetTotalUncompressedSize() > 0);

    auto packs = builder.BuildPacks("game");
    assert(packs.size() == 1);
    assert(packs[0].filename == "game.qpk");
    assert(!packs[0].bytes.empty());

    // Load pack in QpkReader
    QpkReader reader;
    auto status = reader.LoadFromBuffer(packs[0].bytes.data(), packs[0].bytes.size());
    assert(status == QpkReadStatus::OK);
    assert(reader.IsLoaded());
    assert(reader.GetAssetCount() == 4);

    // Microsecond lookup performance check
    auto start = std::chrono::high_resolution_clock::now();
    const auto* entry = reader.FindEntry("scripts/tractor.qvsc");
    auto end = std::chrono::high_resolution_clock::now();

    double lookupMicros = std::chrono::duration<double, std::micro>(end - start).count();
    std::cout << "  -> Looked up 'scripts/tractor.qvsc' in " << lookupMicros << " microseconds (budget < 10 us)" << std::endl;
    assert(entry != nullptr);
    assert(entry->assetType == QpkAssetType::SCRIPT);
    assert(lookupMicros < 10.0);

    std::string text = reader.ReadAssetText("scripts/tractor.qvsc");
    assert(text == "BYTECODE_DATA_FOR_TRACTOR");

    // Remote asset lookup
    const auto* remoteEntry = reader.FindEntry("assets/models/streamed_mesh.glb");
    assert(remoteEntry != nullptr);
    assert(remoteEntry->flags & QPK_FLAG_REMOTE);
    assert(remoteEntry->remoteUrl.find("https://huggingface.co") != std::string::npos);

    std::cout << "[PASS] TestQpkPackAndMicrosecondLookup" << std::endl;
}

void TestQpkCorruptionDefense() {
    std::cout << "[RUN] TestQpkCorruptionDefense (Doc 13 AC9)..." << std::endl;

    // Doc 13 AC9: Deleting or corrupting a pack file at random bytes shows a safe error screen
    // and zero crash or ANR
    QpkBuilder builder;
    builder.AddFileString("scenes/level01.scene.json", "{\"scene\":\"valid\"}", QpkAssetType::SCENE);
    builder.AddFileString("scripts/game.qvsc", "BYTECODE", QpkAssetType::SCRIPT);

    auto packs = builder.BuildPacks("game");
    assert(!packs.empty());
    auto originalBytes = packs[0].bytes;

    // 1. Truncated pack file
    auto truncatedBytes = originalBytes;
    truncatedBytes.resize(truncatedBytes.size() / 2);

    QpkReader truncReader;
    auto truncStatus = truncReader.LoadFromBuffer(truncatedBytes.data(), truncatedBytes.size());
    assert(truncStatus != QpkReadStatus::OK);
    assert(!truncReader.IsLoaded());
    std::cout << "  -> Truncated pack handled safely: " << truncReader.GetStatusString() << std::endl;

    // 2. Corrupt random bytes inside the index
    auto corruptedBytes = originalBytes;
    size_t corruptIndex = corruptedBytes.size() - 8; // Corrupt footer/index area
    corruptedBytes[corruptIndex] ^= 0xFF;

    QpkReader corruptReader;
    auto corruptStatus = corruptReader.LoadFromBuffer(corruptedBytes.data(), corruptedBytes.size());
    assert(corruptStatus == QpkReadStatus::CHECKSUM_MISMATCH || corruptStatus == QpkReadStatus::CORRUPT_INDEX_OFFSET);
    assert(!corruptReader.IsLoaded());
    std::cout << "  -> Corrupted index handled safely: " << corruptReader.GetStatusString() << std::endl;

    // 3. Corrupt magic header
    auto badMagicBytes = originalBytes;
    badMagicBytes[0] = 'X'; badMagicBytes[1] = 'X';

    QpkReader magicReader;
    auto magicStatus = magicReader.LoadFromBuffer(badMagicBytes.data(), badMagicBytes.size());
    assert(magicStatus == QpkReadStatus::CORRUPT_MAGIC);
    assert(!magicReader.IsLoaded());
    std::cout << "  -> Bad magic handled safely: " << magicReader.GetStatusString() << std::endl;

    std::cout << "[PASS] TestQpkCorruptionDefense (Zero crashes across all corruption scenarios)" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    QPK PACK & CORRUPTION TESTS (M11)   " << std::endl;
    std::cout << "========================================" << std::endl;

    TestQpkPackAndMicrosecondLookup();
    TestQpkCorruptionDefense();

    std::cout << "ALL QPK PACK & CORRUPTION TESTS PASSED!" << std::endl;
    return 0;
}

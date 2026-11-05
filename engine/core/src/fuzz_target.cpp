#include "qivforge/core/fuzz_target.h"
#include "qivforge/export/qpk_builder.h"
#include "qivforge/export/qpk_reader.h"
#include "qivforge/render/gltf_importer.h"
#include "qivforge/anim/anim_graph.h"
#include "qivforge/script/qvm.h"
#include <random>
#include <vector>

namespace qivforge::core {

namespace {

// Deterministic PRNG for reproducible fuzzing runs
std::mt19937_64 g_rng(133742);

void MutateBytes(std::vector<uint8_t>& bytes) {
    if (bytes.empty()) return;
    std::uniform_int_distribution<size_t> posDist(0, bytes.size() - 1);
    std::uniform_int_distribution<uint32_t> valDist(0, 255);
    std::uniform_int_distribution<int> modeDist(0, 4);

    int mode = modeDist(g_rng);
    switch (mode) {
        case 0: // Bit flip
            bytes[posDist(g_rng)] ^= (1 << (valDist(g_rng) % 8));
            break;
        case 1: // Byte overwrite
            bytes[posDist(g_rng)] = static_cast<uint8_t>(valDist(g_rng));
            break;
        case 2: // Zero byte
            bytes[posDist(g_rng)] = 0;
            break;
        case 3: // Truncate
            if (bytes.size() > 4) {
                bytes.resize(posDist(g_rng));
            }
            break;
        case 4: // Insert random byte
            bytes.insert(bytes.begin() + posDist(g_rng), static_cast<uint8_t>(valDist(g_rng)));
            break;
    }
}

} // anonymous namespace

FuzzReport FuzzEngine::FuzzQpkReader(size_t iterations) {
    FuzzReport report;

    // Create a base valid QPK archive
    export_pipeline::QpkBuilder builder;
    builder.AddFileString("test/data.txt", "Hello Fuzzing World", export_pipeline::QpkAssetType::CONFIG);
    auto packs = builder.BuildPacks("fuzz");
    if (packs.empty()) return report;
    const auto basePack = packs[0].bytes;

    for (size_t i = 0; i < iterations; ++i) {
        report.iterationsRan++;
        auto mutated = basePack;
        // Apply 1 to 5 mutations per iteration
        size_t numMutations = 1 + (i % 5);
        for (size_t m = 0; m < numMutations; ++m) {
            MutateBytes(mutated);
        }

        try {
            export_pipeline::QpkReader reader;
            auto status = reader.LoadFromBuffer(mutated.data(), mutated.size());
            if (status != export_pipeline::QpkReadStatus::OK) {
                report.safeRejections++;
            }
        } catch (...) {
            report.unhandledExceptions++;
        }
    }

    return report;
}

FuzzReport FuzzEngine::FuzzQmeshParser(size_t iterations) {
    FuzzReport report;

    // Base valid .qmesh binary buffer
    render::GltfModel testModel;
    render::GltfMesh testMesh;
    testMesh.vertices.resize(3);
    testMesh.indices = {0, 1, 2};
    testModel.meshes.push_back(testMesh);
    auto baseQmesh = render::GltfImporter::BakeToQMesh(testModel);

    for (size_t i = 0; i < iterations; ++i) {
        report.iterationsRan++;
        auto mutated = baseQmesh;
        MutateBytes(mutated);

        try {
            auto model = render::GltfImporter::LoadFromQMesh(mutated.data(), mutated.size());
            if (model.meshes.empty() || model.totalVertices == 0) {
                report.safeRejections++;
            }
        } catch (...) {
            report.unhandledExceptions++;
        }
    }

    return report;
}

FuzzReport FuzzEngine::FuzzQanimParser(size_t iterations) {
    FuzzReport report;

    anim::AnimGraph baseGraph;
    baseGraph.SetFloat("speed", 1.0f);
    baseGraph.SetBool("isGrounded", true);
    std::string baseJson = baseGraph.ToQanimJson();

    for (size_t i = 0; i < iterations; ++i) {
        report.iterationsRan++;
        std::vector<uint8_t> mutated(baseJson.begin(), baseJson.end());
        MutateBytes(mutated);
        std::string mutatedStr(mutated.begin(), mutated.end());

        try {
            auto graph = anim::AnimGraph::FromQanimJson(mutatedStr);
            if (!graph || graph->GetFloat("speed") != 1.0f) {
                report.safeRejections++;
            }
        } catch (...) {
            report.unhandledExceptions++;
        }
    }

    return report;
}

FuzzReport FuzzEngine::FuzzQvmExecution(size_t iterations) {
    FuzzReport report;

    for (size_t i = 0; i < iterations; ++i) {
        report.iterationsRan++;

        // Generate synthetic bytecode function with randomized opcodes and arguments
        script::CompiledFunction fn;
        fn.name = "fuzz_fn_" + std::to_string(i);
        fn.numRegisters = 16;

        size_t numInst = 10 + (i % 20);
        for (size_t j = 0; j < numInst; ++j) {
            uint32_t randomInst = static_cast<uint32_t>(g_rng());
            fn.code.push_back(randomInst);
        }
        // Ensure function returns eventually if not trapped
        fn.code.push_back(script::EncodeABC(script::Opcode::RET_VOID, 0, 0, 0));

        try {
            script::Qvm qvm;
            qvm.SetInstructionBudget(500); // Gas limit budget Doc 08 §13.2
            auto res = qvm.Execute(fn);
            // Either executed or safely trapped/gas exhausted
            if (!res.success) {
                report.safeRejections++;
            }
        } catch (...) {
            report.unhandledExceptions++;
        }
    }

    return report;
}

} // namespace qivforge::core

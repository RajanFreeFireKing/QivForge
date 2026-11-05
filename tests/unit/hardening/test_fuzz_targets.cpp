#include "qivforge/core/fuzz_target.h"
#include "qivforge/export/qpk_reader.h"
#include "qivforge/render/gltf_importer.h"
#include "qivforge/anim/anim_graph.h"
#include "qivforge/script/qvm.h"
#include <cassert>
#include <iostream>

using namespace qivforge;

void TestQpkFuzzing() {
    std::cout << "[M12] Running QpkReader fuzzing suite (500 iterations)..." << std::endl;
    auto report = core::FuzzEngine::FuzzQpkReader(500);
    std::cout << "  Iterations: " << report.iterationsRan
              << ", Safe Rejections: " << report.safeRejections
              << ", Crashes: " << report.crashesDetected
              << ", Unhandled Exceptions: " << report.unhandledExceptions << std::endl;

    assert(report.iterationsRan == 500);
    assert(report.crashesDetected == 0);
    assert(report.unhandledExceptions == 0);
    assert(report.safeRejections > 0);

    // Boundary edge cases
    export_pipeline::QpkReader reader;
    assert(reader.LoadFromBuffer(nullptr, 0) == export_pipeline::QpkReadStatus::FILE_TRUNCATED);
    uint8_t garbage[sizeof(export_pipeline::QpkHeader) + sizeof(export_pipeline::QpkFooter)] = {0};
    assert(reader.LoadFromBuffer(garbage, sizeof(garbage)) == export_pipeline::QpkReadStatus::CORRUPT_MAGIC);
    std::cout << "  -> QPK Reader fuzzing passed 100%!" << std::endl;
}

void TestQmeshFuzzing() {
    std::cout << "[M12] Running Qmesh parser fuzzing suite (500 iterations)..." << std::endl;
    auto report = core::FuzzEngine::FuzzQmeshParser(500);
    std::cout << "  Iterations: " << report.iterationsRan
              << ", Safe Rejections: " << report.safeRejections
              << ", Crashes: " << report.crashesDetected
              << ", Unhandled Exceptions: " << report.unhandledExceptions << std::endl;

    assert(report.iterationsRan == 500);
    assert(report.crashesDetected == 0);
    assert(report.unhandledExceptions == 0);
    assert(report.safeRejections > 0);

    // Boundary edge cases
    auto modelNull = render::GltfImporter::LoadFromQMesh(nullptr, 0);
    assert(modelNull.meshes.empty());
    uint8_t shortBuffer[5] = {'Q', 'M', 'E', 'S', 'H'};
    auto modelShort = render::GltfImporter::LoadFromQMesh(shortBuffer, 5);
    assert(modelShort.meshes.empty());
    std::cout << "  -> Qmesh Parser fuzzing passed 100%!" << std::endl;
}

void TestQanimFuzzing() {
    std::cout << "[M12] Running Qanim parser fuzzing suite (500 iterations)..." << std::endl;
    auto report = core::FuzzEngine::FuzzQanimParser(500);
    std::cout << "  Iterations: " << report.iterationsRan
              << ", Safe Rejections: " << report.safeRejections
              << ", Crashes: " << report.crashesDetected
              << ", Unhandled Exceptions: " << report.unhandledExceptions << std::endl;

    assert(report.iterationsRan == 500);
    assert(report.crashesDetected == 0);
    assert(report.unhandledExceptions == 0);
    assert(report.safeRejections > 0);

    // Boundary edge cases
    auto graphEmpty = anim::AnimGraph::FromQanimJson("");
    assert(graphEmpty != nullptr);
    auto graphCorrupt = anim::AnimGraph::FromQanimJson("{ broken: json [[[");
    assert(graphCorrupt != nullptr);
    std::cout << "  -> Qanim Parser fuzzing passed 100%!" << std::endl;
}

void TestQvmFuzzing() {
    std::cout << "[M12] Running QVM bytecode interpreter fuzzing suite (500 iterations)..." << std::endl;
    auto report = core::FuzzEngine::FuzzQvmExecution(500);
    std::cout << "  Iterations: " << report.iterationsRan
              << ", Safe Rejections: " << report.safeRejections
              << ", Crashes: " << report.crashesDetected
              << ", Unhandled Exceptions: " << report.unhandledExceptions << std::endl;

    assert(report.iterationsRan == 500);
    assert(report.crashesDetected == 0);
    assert(report.unhandledExceptions == 0);

    // Gas exhaustion edge case
    script::CompiledFunction infiniteLoopFn;
    infiniteLoopFn.name = "infinite_loop";
    infiniteLoopFn.numRegisters = 2;
    // JMP -1 (infinite loop jumping back to instruction 0)
    infiniteLoopFn.code.push_back(script::EncodeAImm(script::Opcode::JMP, 0, static_cast<uint16_t>(-1)));
    script::Qvm qvm;
    qvm.SetInstructionBudget(50);
    auto res = qvm.Execute(infiniteLoopFn);
    assert(!res.success);
    assert(res.errorCode == "QS-RUN-002"); // Gas limit exceeded (Doc 08 §13.1)
    assert(res.instructionsExecuted == 51);

    std::cout << "  -> QVM fuzzing & gas budget governance passed 100%!" << std::endl;
}

int main() {
    std::cout << "=== Running Milestone M12 Fuzz Hardening Tests ===" << std::endl;
    TestQpkFuzzing();
    TestQmeshFuzzing();
    TestQanimFuzzing();
    TestQvmFuzzing();
    std::cout << "=== All M12 Fuzz Hardening Tests PASSED Successfully! ===" << std::endl;
    return 0;
}

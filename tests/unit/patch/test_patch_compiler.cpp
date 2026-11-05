#include "qivforge/patch/patch_compiler.h"
#include "qivforge/script/qvm.h"
#include <iostream>
#include <cassert>
#include <chrono>

using namespace qivforge::patch;
using namespace qivforge::script;

void TestCompileFireWeaponPatch() {
    std::cout << "[RUN] TestCompileFireWeaponPatch (Doc 09 §8 & §11)..." << std::endl;

    PatchGraph graph("logic");

    PatchNode n1;
    n1.id = "n1";
    n1.type = "event.OnTouchButtonDown";
    n1.constants["button"] = "FireButton";
    graph.AddNode(n1);

    PatchNode n2;
    n2.id = "n2";
    n2.type = "physics.RaycastLine";
    n2.constants["distance"] = "300.0";
    graph.AddNode(n2);

    PatchNode n3;
    n3.id = "n3";
    n3.type = "combat.ApplyDamage";
    n3.constants["amount"] = "35.0";
    graph.AddNode(n3);

    graph.AddWire("n1", "exec", "n2", "exec");
    graph.AddWire("n2", "hit", "n3", "exec");

    // Measure compile time (Doc 09 §8 budget: under 50 ms)
    auto tStart = std::chrono::high_resolution_clock::now();
    PatchCompiler compiler;
    auto res = compiler.Compile(graph);
    auto tEnd = std::chrono::high_resolution_clock::now();
    double compileMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();

    std::cout << "  -> Compiled Fire Weapon patch in " << compileMs << " ms (Budget: < 50 ms)" << std::endl;
    assert(res.success);
    assert(res.module != nullptr);
    assert(compileMs < 50.0);

    // Verify generated function
    const auto* fn = res.module->FindFunction("onTouchButtonDown");
    assert(fn != nullptr);
    assert(!fn->code.empty());
    assert(fn->constants.size() >= 2);

    // Execute in QVM
    Qvm qvm;
    auto execRes = qvm.Execute(*fn, {});
    assert(execRes.success);

    std::cout << "[PASS] TestCompileFireWeaponPatch" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  PATCH COMPILER TEST SUITE (M8)        " << std::endl;
    std::cout << "========================================" << std::endl;

    TestCompileFireWeaponPatch();

    std::cout << "ALL PATCH COMPILER TESTS PASSED!" << std::endl;
    return 0;
}

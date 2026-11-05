#include "qivforge/patch/patch_converter.h"
#include <iostream>
#include <cassert>

using namespace qivforge::patch;

void TestScriptToPatchConversion() {
    std::cout << "[RUN] TestScriptToPatchConversion (Doc 09 §12 & Rule R2)..." << std::endl;

    std::string script = 
        "@export var maxSpeed: float = 45.0;\n\n"
        "fn onTick(dt: float) {\n"
        "    PhysicsWorld.raycast(origin, dir, 300.0);\n"
        "    applyDamage(target, 35.0);\n"
        "}\n";

    // 1. Convert QivScript to Visual Patch
    auto graph = PatchConverter::ScriptToPatch(script);
    assert(graph != nullptr);

    // Verify exported variable
    assert(graph->GetVariables().size() == 1);
    assert(graph->GetVariables()[0].name == "maxSpeed");
    assert(graph->GetVariables()[0].exported);

    // Verify nodes created: OnFrameTick -> RaycastLine -> ApplyDamage
    assert(graph->GetNodes().size() == 3);
    assert(graph->GetNodes()[0].type == "event.OnFrameTick");
    assert(graph->GetNodes()[1].type == "physics.RaycastLine");
    assert(graph->GetNodes()[2].type == "combat.ApplyDamage");

    // Verify wires connected
    assert(graph->GetWires().size() == 2);
    assert(graph->GetWires()[0].fromNode == "n1");
    assert(graph->GetWires()[0].toNode == "n2");
    assert(graph->GetWires()[1].fromNode == "n2");
    assert(graph->GetWires()[1].toNode == "n3");

    // 2. Convert Patch back to clean Script
    std::string generatedScript = PatchConverter::PatchToScript(*graph);
    assert(generatedScript.find("@export var maxSpeed: float") != std::string::npos);
    assert(generatedScript.find("fn onTick(dt: float)") != std::string::npos);

    std::cout << "[PASS] TestScriptToPatchConversion" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  SCRIPT TO PATCH CONVERSION TEST (M8)  " << std::endl;
    std::cout << "========================================" << std::endl;

    TestScriptToPatchConversion();

    std::cout << "ALL SCRIPT TO PATCH CONVERSION TESTS PASSED!" << std::endl;
    return 0;
}

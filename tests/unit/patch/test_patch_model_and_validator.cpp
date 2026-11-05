#include "qivforge/patch/patch_model.h"
#include "qivforge/patch/patch_validator.h"
#include <iostream>
#include <cassert>

using namespace qivforge::patch;

void TestPatchGraphSerialization() {
    std::cout << "[RUN] TestPatchGraphSerialization (Doc 09 §10)..." << std::endl;

    PatchGraph graph("logic");

    PatchVariable varAmmo;
    varAmmo.name = "ammo";
    varAmmo.type = PinType::INT;
    varAmmo.defaultValue = "30";
    varAmmo.exported = true;
    graph.AddVariable(varAmmo);

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

    graph.AddWire("n1", "exec", "n2", "exec");

    PatchComment c1;
    c1.title = "Fire Action";
    c1.x = 0; c1.y = 0; c1.width = 600; c1.height = 200;
    graph.AddComment(c1);

    std::string json = graph.ToQpatchJson();
    assert(json.find("\"kind\":\"logic\"") != std::string::npos);
    assert(json.find("\"ammo\"") != std::string::npos);
    assert(json.find("\"event.OnTouchButtonDown\"") != std::string::npos);
    assert(json.find("\"physics.RaycastLine\"") != std::string::npos);

    // Round-trip deserialization
    auto deserialized = PatchGraph::FromQpatchJson(json);
    assert(deserialized != nullptr);
    assert(deserialized->GetNodes().size() == 2);
    assert(deserialized->GetWires().size() == 1);
    assert(deserialized->GetWires()[0].fromNode == "n1");
    assert(deserialized->GetWires()[0].toNode == "n2");

    std::cout << "[PASS] TestPatchGraphSerialization" << std::endl;
}

void TestPatchValidatorDiagnostics() {
    std::cout << "[RUN] TestPatchValidatorDiagnostics (Doc 09 §15 QP0001-QP0005)..." << std::endl;

    PatchValidator validator;

    // 1. Missing required input pin without default (QP0001)
    PatchGraph g1("logic");
    PatchNode nAdd;
    nAdd.id = "add1";
    nAdd.type = "math.Add";
    nAdd.pins.push_back({"in_a", "a", PinDirection::IN, PinKind::DATA, PinType::FLOAT, ""});
    g1.AddNode(nAdd);

    auto diags1 = validator.Validate(g1);
    assert(validator.HasErrors(diags1));
    bool foundQp0001 = false;
    for (const auto& d : diags1) {
        if (d.code == "QP0001") foundQp0001 = true;
    }
    assert(foundQp0001);

    // 2. Type mismatch (QP0002): connect Vec3 to int
    PatchGraph g2("logic");
    PatchNode nOut;
    nOut.id = "src";
    nOut.type = "custom.Source";
    nOut.pins.push_back({"v_out", "vector", PinDirection::OUT, PinKind::DATA, PinType::VEC3, ""});
    g2.AddNode(nOut);

    PatchNode nIn;
    nIn.id = "dst";
    nIn.type = "custom.Target";
    nIn.pins.push_back({"i_in", "count", PinDirection::IN, PinKind::DATA, PinType::INT, "0"});
    g2.AddNode(nIn);

    g2.AddWire("src", "v_out", "dst", "i_in");

    auto diags2 = validator.Validate(g2);
    assert(validator.HasErrors(diags2));
    bool foundQp0002 = false;
    for (const auto& d : diags2) {
        if (d.code == "QP0002") foundQp0002 = true;
    }
    assert(foundQp0002);

    // 3. Missing node type placeholder (QP0004)
    PatchGraph g4("logic");
    PatchNode nMiss;
    nMiss.id = "missing_node";
    nMiss.type = "unknown.PluginNode";
    nMiss.isMissingType = true;
    g4.AddNode(nMiss);

    auto diags4 = validator.Validate(g4);
    bool foundQp0004 = false;
    for (const auto& d : diags4) {
        if (d.code == "QP0004") foundQp0004 = true;
    }
    assert(foundQp0004);

    // 4. Duplicate single-instance event node (QP0005)
    PatchGraph g5("logic");
    PatchNode ev1{"ev1", "event.OnGameStart", 0, 0, {}, {}, false};
    PatchNode ev2{"ev2", "event.OnGameStart", 100, 0, {}, {}, false};
    g5.AddNode(ev1);
    g5.AddNode(ev2);

    auto diags5 = validator.Validate(g5);
    bool foundQp0005 = false;
    for (const auto& d : diags5) {
        if (d.code == "QP0005") foundQp0005 = true;
    }
    assert(foundQp0005);

    std::cout << "[PASS] TestPatchValidatorDiagnostics" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  PATCH MODEL & VALIDATOR TEST (M8)     " << std::endl;
    std::cout << "========================================" << std::endl;

    TestPatchGraphSerialization();
    TestPatchValidatorDiagnostics();

    std::cout << "ALL PATCH MODEL & VALIDATOR TESTS PASSED!" << std::endl;
    return 0;
}

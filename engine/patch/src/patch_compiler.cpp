#include "qivforge/patch/patch_compiler.h"
#include <unordered_map>
#include <iostream>

namespace qivforge::patch {

using namespace qivforge::script;

PatchCompileResult PatchCompiler::Compile(const PatchGraph& graph) {
    PatchCompileResult result;

    // 1. Validation
    PatchValidator validator;
    result.diagnostics = validator.Validate(graph);
    if (validator.HasErrors(result.diagnostics)) {
        result.success = false;
        result.errorMessage = "Patch graph validation failed";
        return result;
    }

    auto module = std::make_shared<BytecodeModule>();
    module->className = "GeneratedPatch_" + graph.GetKind();

    // 2. Exported variables
    for (const auto& var : graph.GetVariables()) {
        ExportedFieldMetadata field;
        field.name = var.name;
        field.typeName = PinTypeToString(var.type);
        if (var.type == PinType::FLOAT) {
            field.defaultValue = std::stof(var.defaultValue.empty() ? "0.0" : var.defaultValue);
        } else if (var.type == PinType::INT) {
            field.defaultValue = static_cast<double>(std::stol(var.defaultValue.empty() ? "0" : var.defaultValue));
        } else if (var.type == PinType::BOOL) {
            field.defaultValue = (var.defaultValue == "true") ? 1.0 : 0.0;
        }
        module->exportedFields.push_back(field);
    }

    // 3. Find event entry nodes
    bool hasEvents = false;
    for (const auto& node : graph.GetNodes()) {
        if (node.type.rfind("event.", 0) == 0) {
            hasEvents = true;
            std::string funcName = "onEvent";
            if (node.type == "event.OnGameStart") funcName = "onStart";
            else if (node.type == "event.OnFrameTick") funcName = "onTick";
            else if (node.type == "event.OnPhysicsStep") funcName = "onPhysicsStep";
            else if (node.type == "event.OnTouchButtonDown") funcName = "onTouchButtonDown";

            CompiledFunction fn;
            fn.name = funcName;
            CompileEventChain(graph, node, fn);
            module->functions.push_back(std::move(fn));
        }
    }

    // If no explicit event node (e.g. standalone math / action graph), compile as main
    if (!hasEvents && !graph.GetNodes().empty()) {
        CompiledFunction fn;
        fn.name = "main";
        CompileEventChain(graph, graph.GetNodes().front(), fn);
        module->functions.push_back(std::move(fn));
    }

    result.module = module;
    result.success = true;
    return result;
}

void PatchCompiler::CompileEventChain(const PatchGraph& graph, const PatchNode& startNode, CompiledFunction& fn) {
    // Traverse exec wires from startNode
    std::unordered_map<std::string, const PatchWire*> execWires;
    for (const auto& wire : graph.GetWires()) {
        if (wire.fromPin == "exec" || wire.fromPin == "hit") {
            execWires[wire.fromNode] = &wire;
        }
    }

    const PatchNode* curr = &startNode;
    uint8_t rResult = 1;
    uint8_t rTemp1 = 2;
    uint8_t rTemp2 = 3;

    while (curr) {
        // Line debug marker for error highlighting (Doc 09 §8 & QP-RUN-001)
        fn.code.push_back(EncodeABC(Opcode::LINE, 0, 0, 0));

        if (curr->type == "math.Add") {
            // Load operands and emit ADDF
            float a = 0.0f, b = 0.0f;
            if (curr->constants.count("a")) a = std::stof(curr->constants.at("a"));
            if (curr->constants.count("b")) b = std::stof(curr->constants.at("b"));

            uint16_t idxA = fn.AddConstant(Value::Float(a));
            uint16_t idxB = fn.AddConstant(Value::Float(b));

            fn.code.push_back(EncodeAImm(Opcode::LOADK, rTemp1, idxA));
            fn.code.push_back(EncodeAImm(Opcode::LOADK, rTemp2, idxB));
            fn.code.push_back(EncodeABC(Opcode::ADDF, rResult, rTemp1, rTemp2));
        } else if (curr->type == "physics.RaycastLine") {
            // Raycast simulation
            float dist = 100.0f;
            if (curr->constants.count("distance")) dist = std::stof(curr->constants.at("distance"));
            uint16_t idxDist = fn.AddConstant(Value::Float(dist));
            fn.code.push_back(EncodeAImm(Opcode::LOADK, rTemp1, idxDist));
        } else if (curr->type == "combat.ApplyDamage") {
            // Apply damage
            float dmg = 25.0f;
            if (curr->constants.count("amount")) dmg = std::stof(curr->constants.at("amount"));
            uint16_t idxDmg = fn.AddConstant(Value::Float(dmg));
            fn.code.push_back(EncodeAImm(Opcode::LOADK, rTemp1, idxDmg));
        }

        auto it = execWires.find(curr->id);
        if (it != execWires.end()) {
            curr = graph.FindNode(it->second->toNode);
        } else {
            curr = nullptr;
        }
    }

    fn.code.push_back(EncodeABC(Opcode::RET_VOID, 0, 0, 0));
}

} // namespace qivforge::patch

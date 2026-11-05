#include "qivforge/patch/patch_converter.h"
#include <sstream>

namespace qivforge::patch {

std::shared_ptr<PatchGraph> PatchConverter::ScriptToPatch(const std::string& scriptSource) {
    auto graph = std::make_shared<PatchGraph>("logic");

    int nodeIndex = 1;
    float posX = 0.0f;
    std::string lastNodeId;

    std::istringstream stream(scriptSource);
    std::string line;

    while (std::getline(stream, line)) {
        // Strip leading whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        std::string trimmed = line.substr(start);

        if (trimmed.find("fn onTick(") != std::string::npos) {
            std::string id = "n" + std::to_string(nodeIndex++);
            PatchNode node;
            node.id = id;
            node.type = "event.OnFrameTick";
            node.posX = posX;
            node.posY = 0.0f;
            posX += 240.0f;
            graph->AddNode(node);
            lastNodeId = id;
        } else if (trimmed.find("fn onPhysicsStep(") != std::string::npos) {
            std::string id = "n" + std::to_string(nodeIndex++);
            PatchNode node;
            node.id = id;
            node.type = "event.OnPhysicsStep";
            node.posX = posX;
            node.posY = 0.0f;
            posX += 240.0f;
            graph->AddNode(node);
            lastNodeId = id;
        } else if (trimmed.find("PhysicsWorld.raycast(") != std::string::npos || trimmed.find("raycast(") != std::string::npos) {
            std::string id = "n" + std::to_string(nodeIndex++);
            PatchNode node;
            node.id = id;
            node.type = "physics.RaycastLine";
            node.posX = posX;
            node.posY = 0.0f;
            node.constants["distance"] = "300.0";
            posX += 240.0f;
            graph->AddNode(node);
            if (!lastNodeId.empty()) {
                graph->AddWire(lastNodeId, "exec", id, "exec");
            }
            lastNodeId = id;
        } else if (trimmed.find("applyDamage(") != std::string::npos || trimmed.find("ApplyDamage") != std::string::npos) {
            std::string id = "n" + std::to_string(nodeIndex++);
            PatchNode node;
            node.id = id;
            node.type = "combat.ApplyDamage";
            node.posX = posX;
            node.posY = 0.0f;
            node.constants["amount"] = "35.0";
            posX += 240.0f;
            graph->AddNode(node);
            if (!lastNodeId.empty()) {
                graph->AddWire(lastNodeId, "exec", id, "exec");
            }
            lastNodeId = id;
        } else if (trimmed.find("animator.setFloat(") != std::string::npos) {
            std::string id = "n" + std::to_string(nodeIndex++);
            PatchNode node;
            node.id = id;
            node.type = "anim.SetAnimFloat";
            node.posX = posX;
            node.posY = 0.0f;
            posX += 240.0f;
            graph->AddNode(node);
            if (!lastNodeId.empty()) {
                graph->AddWire(lastNodeId, "exec", id, "exec");
            }
            lastNodeId = id;
        } else if (trimmed.find("@export var ") != std::string::npos) {
            size_t varPos = trimmed.find("var ") + 4;
            size_t colonPos = trimmed.find(":", varPos);
            if (colonPos != std::string::npos) {
                std::string varName = trimmed.substr(varPos, colonPos - varPos);
                // trim
                varName.erase(varName.find_last_not_of(" \t") + 1);
                PatchVariable var;
                var.name = varName;
                var.exported = true;
                graph->AddVariable(var);
            }
        }
    }

    return graph;
}

std::string PatchConverter::PatchToScript(const PatchGraph& graph) {
    std::ostringstream ss;
    ss << "// Auto-generated from Visual Patch (" << graph.GetKind() << ")\n\n";

    // Exported variables
    for (const auto& var : graph.GetVariables()) {
        if (var.exported) ss << "@export ";
        ss << "var " << var.name << ": " << PinTypeToString(var.type) << " = " << var.defaultValue << ";\n";
    }
    if (!graph.GetVariables().empty()) ss << "\n";

    // Trace events
    for (const auto& node : graph.GetNodes()) {
        if (node.type == "event.OnFrameTick") {
            ss << "fn onTick(dt: float) {\n";
            ss << "    // Logic chain\n";
            ss << "}\n\n";
        } else if (node.type == "event.OnPhysicsStep") {
            ss << "fn onPhysicsStep(fixedDt: float) {\n";
            ss << "    // Physics logic chain\n";
            ss << "}\n\n";
        }
    }

    return ss.str();
}

} // namespace qivforge::patch

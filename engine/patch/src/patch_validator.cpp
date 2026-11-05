#include "qivforge/patch/patch_validator.h"
#include <unordered_set>
#include <unordered_map>

namespace qivforge::patch {

bool PatchValidator::CanConvert(PinType from, PinType to) noexcept {
    if (from == to) return true;
    if (to == PinType::ANY || from == PinType::ANY) return true;
    // Doc 09 §2: Implicit conversions: int to float, float to int, bool to int, any to string
    if (from == PinType::INT && to == PinType::FLOAT) return true;
    if (from == PinType::FLOAT && to == PinType::INT) return true;
    if (from == PinType::BOOL && to == PinType::INT) return true;
    if (to == PinType::STRING) return true;
    return false;
}

std::vector<PatchDiagnostic> PatchValidator::Validate(const PatchGraph& graph) const {
    std::vector<PatchDiagnostic> diags;

    std::unordered_set<std::string> knownEvents;
    std::unordered_map<std::string, const PatchPin*> pinMap; // key: "nodeId.pinId"

    // Index all pins and check missing types (QP0004) and duplicate events (QP0005)
    for (const auto& node : graph.GetNodes()) {
        if (node.isMissingType) {
            diags.push_back({"QP0004", node.id, "", "Missing node type: loaded as placeholder", false});
        }

        // Duplicate event check (Doc 09 §15 QP0005)
        if (node.type.rfind("event.", 0) == 0) {
            if (node.type == "event.OnGameStart" || node.type == "event.OnFrameTick" || node.type == "event.OnPhysicsStep") {
                if (knownEvents.find(node.type) != knownEvents.end()) {
                    diags.push_back({"QP0005", node.id, "", "Duplicate single-instance event node: " + node.type, true});
                } else {
                    knownEvents.insert(node.type);
                }
            }
        }

        for (const auto& pin : node.pins) {
            std::string key = node.id + "." + pin.id;
            pinMap[key] = &pin;
        }
    }

    // Check connections and type compatibility (QP0002)
    std::unordered_set<std::string> connectedInputs;
    for (const auto& wire : graph.GetWires()) {
        std::string fromKey = wire.fromNode + "." + wire.fromPin;
        std::string toKey = wire.toNode + "." + wire.toPin;

        connectedInputs.insert(toKey);

        auto fromIt = pinMap.find(fromKey);
        auto toIt = pinMap.find(toKey);
        if (fromIt != pinMap.end() && toIt != pinMap.end()) {
            const auto* fromPin = fromIt->second;
            const auto* toPin = toIt->second;

            if (fromPin->kind != toPin->kind) {
                diags.push_back({"QP0002", wire.toNode, wire.toPin, "Cannot connect Exec pin to Data pin", false});
            } else if (fromPin->kind == PinKind::DATA) {
                if (!CanConvert(fromPin->type, toPin->type)) {
                    diags.push_back({"QP0002", wire.toNode, wire.toPin, 
                        "Type mismatch: cannot convert " + std::string(PinTypeToString(fromPin->type)) + 
                        " to " + std::string(PinTypeToString(toPin->type)), false});
                }
            }
        }
    }

    // Check required input pins (QP0001)
    for (const auto& node : graph.GetNodes()) {
        for (const auto& pin : node.pins) {
            if (pin.direction == PinDirection::IN && pin.kind == PinKind::DATA) {
                std::string key = node.id + "." + pin.id;
                bool isConnected = connectedInputs.find(key) != connectedInputs.end();
                bool hasConstant = node.constants.find(pin.name) != node.constants.end() || !pin.defaultValue.empty();

                if (!isConnected && !hasConstant) {
                    diags.push_back({"QP0001", node.id, pin.id, "Required input pin '" + pin.name + "' is not connected and has no default value", false});
                }
            }
        }
    }

    return diags;
}

bool PatchValidator::HasErrors(const std::vector<PatchDiagnostic>& diags) const {
    for (const auto& d : diags) {
        if (!d.isWarning) return true;
    }
    return false;
}

} // namespace qivforge::patch

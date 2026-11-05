#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <variant>

namespace qivforge::patch {

enum class PinKind {
    EXEC,
    DATA
};

enum class PinType {
    EXEC,
    BOOL,
    INT,
    FLOAT,
    STRING,
    VEC3,
    QUAT,
    ACTOR,
    ANY
};

inline const char* PinTypeToString(PinType type) noexcept {
    switch (type) {
        case PinType::EXEC: return "exec";
        case PinType::BOOL: return "bool";
        case PinType::INT: return "int";
        case PinType::FLOAT: return "float";
        case PinType::STRING: return "string";
        case PinType::VEC3: return "Vec3";
        case PinType::QUAT: return "Quat";
        case PinType::ACTOR: return "Actor";
        case PinType::ANY: return "any";
        default: return "unknown";
    }
}

inline PinType StringToPinType(const std::string& str) noexcept {
    if (str == "exec") return PinType::EXEC;
    if (str == "bool") return PinType::BOOL;
    if (str == "int") return PinType::INT;
    if (str == "float") return PinType::FLOAT;
    if (str == "string") return PinType::STRING;
    if (str == "Vec3") return PinType::VEC3;
    if (str == "Quat") return PinType::QUAT;
    if (str == "Actor") return PinType::ACTOR;
    return PinType::ANY;
}

enum class PinDirection {
    IN,
    OUT
};

struct PatchPin {
    std::string id;
    std::string name;
    PinDirection direction = PinDirection::IN;
    PinKind kind = PinKind::DATA;
    PinType type = PinType::FLOAT;
    std::string defaultValue;
};

struct PatchNode {
    std::string id;
    std::string type; // e.g. "event.OnTouchButtonDown", "physics.RaycastLine", "combat.ApplyDamage"
    float posX = 0.0f;
    float posY = 0.0f;
    std::unordered_map<std::string, std::string> constants;
    std::vector<PatchPin> pins;
    bool isMissingType = false; // Loaded as missing placeholder per Doc 09 §10 & QP0004
};

struct PatchWire {
    std::string fromNode;
    std::string fromPin;
    std::string toNode;
    std::string toPin;
};

struct PatchVariable {
    std::string name;
    PinType type = PinType::FLOAT;
    std::string defaultValue = "0.0";
    bool exported = false; // Visible in Property Inspector per Doc 09 §9
};

struct PatchComment {
    std::string title;
    float x = 0.0f;
    float y = 0.0f;
    float width = 200.0f;
    float height = 150.0f;
    std::string colorToken = "nodeEvent";
};

class PatchGraph {
public:
    PatchGraph(std::string kind = "logic");

    void AddNode(PatchNode node);
    void AddWire(const std::string& fromNode, const std::string& fromPin, const std::string& toNode, const std::string& toPin);
    void AddVariable(PatchVariable var);
    void AddComment(PatchComment comment);

    [[nodiscard]] const PatchNode* FindNode(const std::string& id) const noexcept;
    [[nodiscard]] PatchNode* FindNode(const std::string& id) noexcept;
    [[nodiscard]] const std::vector<PatchNode>& GetNodes() const noexcept { return m_nodes; }
    [[nodiscard]] const std::vector<PatchWire>& GetWires() const noexcept { return m_wires; }
    [[nodiscard]] const std::vector<PatchVariable>& GetVariables() const noexcept { return m_variables; }
    [[nodiscard]] const std::vector<PatchComment>& GetComments() const noexcept { return m_comments; }
    [[nodiscard]] const std::string& GetKind() const noexcept { return m_kind; }

    // Serialization & Deserialization (.qpatch format per Doc 09 §10)
    [[nodiscard]] std::string ToQpatchJson() const;
    static std::shared_ptr<PatchGraph> FromQpatchJson(const std::string& json);

private:
    std::string m_kind; // "logic" or "animgraph"
    std::vector<PatchNode> m_nodes;
    std::vector<PatchWire> m_wires;
    std::vector<PatchVariable> m_variables;
    std::vector<PatchComment> m_comments;
};

} // namespace qivforge::patch

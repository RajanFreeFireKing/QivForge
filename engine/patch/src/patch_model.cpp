#include "qivforge/patch/patch_model.h"
#include <sstream>
#include <algorithm>

namespace qivforge::patch {

PatchGraph::PatchGraph(std::string kind) : m_kind(std::move(kind)) {}

void PatchGraph::AddNode(PatchNode node) {
    m_nodes.push_back(std::move(node));
}

void PatchGraph::AddWire(const std::string& fromNode, const std::string& fromPin, const std::string& toNode, const std::string& toPin) {
    m_wires.push_back({fromNode, fromPin, toNode, toPin});
}

void PatchGraph::AddVariable(PatchVariable var) {
    m_variables.push_back(std::move(var));
}

void PatchGraph::AddComment(PatchComment comment) {
    m_comments.push_back(std::move(comment));
}

const PatchNode* PatchGraph::FindNode(const std::string& id) const noexcept {
    for (const auto& n : m_nodes) {
        if (n.id == id) return &n;
    }
    return nullptr;
}

PatchNode* PatchGraph::FindNode(const std::string& id) noexcept {
    for (auto& n : m_nodes) {
        if (n.id == id) return &n;
    }
    return nullptr;
}

static std::string EscapeString(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else out += c;
    }
    return out;
}

std::string PatchGraph::ToQpatchJson() const {
    std::ostringstream ss;
    ss << "{\"formatVersion\":1,\"kind\":\"" << m_kind << "\",";

    // Variables
    ss << "\"variables\":[";
    for (size_t i = 0; i < m_variables.size(); ++i) {
        if (i > 0) ss << ",";
        ss << "{\"name\":\"" << m_variables[i].name << "\",\"type\":\"" << PinTypeToString(m_variables[i].type)
           << "\",\"default\":\"" << m_variables[i].defaultValue << "\",\"exported\":"
           << (m_variables[i].exported ? "true" : "false") << "}";
    }
    ss << "],";

    // Nodes (sorted by id for stable git diffs per Doc 09 §10)
    std::vector<PatchNode> sortedNodes = m_nodes;
    std::sort(sortedNodes.begin(), sortedNodes.end(), [](const PatchNode& a, const PatchNode& b) {
        return a.id < b.id;
    });

    ss << "\"nodes\":[";
    for (size_t i = 0; i < sortedNodes.size(); ++i) {
        if (i > 0) ss << ",";
        const auto& n = sortedNodes[i];
        ss << "{\"id\":\"" << n.id << "\",\"type\":\"" << n.type << "\",\"pos\":[" << n.posX << "," << n.posY << "]";
        if (!n.constants.empty()) {
            ss << ",\"constants\":{";
            bool firstC = true;
            for (const auto& [k, v] : n.constants) {
                if (!firstC) ss << ",";
                ss << "\"" << k << "\":\"" << EscapeString(v) << "\"";
                firstC = false;
            }
            ss << "}";
        }
        ss << "}";
    }
    ss << "],";

    // Wires
    ss << "\"wires\":[";
    for (size_t i = 0; i < m_wires.size(); ++i) {
        if (i > 0) ss << ",";
        ss << "{\"from\":\"" << m_wires[i].fromNode << "." << m_wires[i].fromPin
           << "\",\"to\":\"" << m_wires[i].toNode << "." << m_wires[i].toPin << "\"}";
    }
    ss << "],";

    // Comments
    ss << "\"comments\":[";
    for (size_t i = 0; i < m_comments.size(); ++i) {
        if (i > 0) ss << ",";
        ss << "{\"title\":\"" << m_comments[i].title << "\",\"rect\":["
           << m_comments[i].x << "," << m_comments[i].y << ","
           << m_comments[i].width << "," << m_comments[i].height
           << "],\"colorToken\":\"" << m_comments[i].colorToken << "\"}";
    }
    ss << "],\"subgraphs\":[]}";

    return ss.str();
}

static std::string ExtractField(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\":\"";
    size_t start = json.find(needle);
    if (start == std::string::npos) return "";
    start += needle.size();
    size_t end = json.find("\"", start);
    return (end == std::string::npos) ? "" : json.substr(start, end - start);
}

std::shared_ptr<PatchGraph> PatchGraph::FromQpatchJson(const std::string& json) {
    std::string kind = ExtractField(json, "kind");
    if (kind.empty()) kind = "logic";

    auto graph = std::make_shared<PatchGraph>(kind);

    // Simple parser for nodes and wires
    size_t pos = 0;
    while ((pos = json.find("{\"id\":\"", pos)) != std::string::npos) {
        pos += 7;
        size_t endId = json.find("\"", pos);
        std::string id = json.substr(pos, endId - pos);

        size_t typePos = json.find("\"type\":\"", endId);
        if (typePos != std::string::npos && typePos < json.find("}", endId)) {
            typePos += 8;
            size_t endType = json.find("\"", typePos);
            std::string type = json.substr(typePos, endType - typePos);

            PatchNode node;
            node.id = id;
            node.type = type;
            graph->AddNode(std::move(node));
        }
        pos = endId;
    }

    // Parse wires: {"from":"n1.exec","to":"n2.exec"}
    pos = 0;
    while ((pos = json.find("{\"from\":\"", pos)) != std::string::npos) {
        pos += 9;
        size_t endFrom = json.find("\"", pos);
        std::string from = json.substr(pos, endFrom - pos);

        size_t toPos = json.find("\"to\":\"", endFrom);
        if (toPos != std::string::npos && toPos < json.find("}", endFrom)) {
            toPos += 6;
            size_t endTo = json.find("\"", toPos);
            std::string to = json.substr(toPos, endTo - toPos);

            auto dotFrom = from.find('.');
            auto dotTo = to.find('.');
            if (dotFrom != std::string::npos && dotTo != std::string::npos) {
                graph->AddWire(from.substr(0, dotFrom), from.substr(dotFrom + 1),
                               to.substr(0, dotTo), to.substr(dotTo + 1));
            }
        }
        pos = endFrom;
    }

    return graph;
}

} // namespace qivforge::patch

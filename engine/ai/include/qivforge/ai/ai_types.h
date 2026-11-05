#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <optional>
#include <memory>

namespace qivforge::ai {

enum class AiProviderId : uint8_t {
    GEMINI = 0,
    GROQ = 1,
    OPENROUTER = 2,
    HUGGINGFACE = 3,
    LOCAL_GGUF = 4,
    ANTIGRAVITY = 5 // Experimental, off by default (Doc 12 §2)
};

enum class MessageRole : uint8_t {
    SYSTEM = 0,
    USER = 1,
    ASSISTANT = 2,
    TOOL = 3
};

struct ToolCall {
    std::string id;
    std::string name;
    std::string argsJson;
};

struct AiMessage {
    MessageRole role = MessageRole::USER;
    std::string content;
    std::vector<ToolCall> toolCalls;
    std::string toolCallId; // For role == TOOL
};

struct ToolSpec {
    std::string name;
    std::string description;
    std::string parametersJsonSchema;
    bool isHighRisk = false; // Deletions & external generation (Doc 12 §5.1)
};

enum class ToolStatus : uint8_t {
    SUCCESS = 0,
    ERROR = 1,
    REJECTED = 2
};

struct ToolResult {
    std::string toolCallId;
    std::string toolName;
    ToolStatus status = ToolStatus::SUCCESS;
    std::string outputJson;
    std::string errorMessage;
};

// Proposed file modification
struct ProposedFileEdit {
    std::string path;
    std::string originalContent;
    std::string proposedContent;
    std::string diffUnified;
    bool compileSuccess = true;
    std::string compileDiagnostics;
    bool isHighRisk = false;
};

// Proposed scene actor operation
enum class SceneOpType : uint8_t {
    CREATE_ACTOR = 0,
    SET_PROPERTY = 1,
    DELETE_ACTOR = 2,
    SPAWN_ASSET = 3
};

struct ProposedSceneOp {
    SceneOpType type = SceneOpType::CREATE_ACTOR;
    std::string actorName;
    std::string propertyPath;
    std::string value;
    bool isHighRisk = false;
};

struct EditSet {
    std::string summary;
    std::vector<ProposedFileEdit> fileEdits;
    std::vector<ProposedSceneOp> sceneOps;
    bool hasHighRiskOps = false;
    bool allCompileSuccess = true;

    [[nodiscard]] bool IsEmpty() const noexcept {
        return fileEdits.empty() && sceneOps.empty();
    }
};

struct AiError {
    std::string code; // QF-AI-001 through QF-AI-008
    std::string message;
    std::optional<int64_t> retryAfterMs;
};

} // namespace qivforge::ai

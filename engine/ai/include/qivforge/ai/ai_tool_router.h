#pragma once

#include "qivforge/ai/ai_types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

namespace qivforge::ai {

class AiToolRouter {
public:
    static constexpr size_t MAX_TOOL_CALLS_PER_REQUEST = 25; // Doc 12 §5.2
    static constexpr size_t MAX_FILE_WRITES_PER_REQUEST = 8;
    static constexpr size_t MAX_SCRIPT_SIZE_BYTES = 40960;   // 40 KB max per file
    static constexpr size_t MAX_READ_SIZE_BYTES = 102400;    // 100 KB max read

    AiToolRouter(std::string projectRoot = "/project");

    void SetProjectRoot(std::string root) { m_projectRoot = std::move(root); }
    [[nodiscard]] const std::string& GetProjectRoot() const noexcept { return m_projectRoot; }

    [[nodiscard]] const std::vector<ToolSpec>& GetRegisteredTools() const noexcept {
        return m_tools;
    }

    [[nodiscard]] bool HasTool(const std::string& name) const noexcept;

    // Security check for file path traversal (Doc 12 §14)
    [[nodiscard]] bool IsPathAllowed(const std::string& relativePath) const noexcept;

    // Dry Run pipeline: produces an EditSet without committing changes (Doc 12 §5.2)
    ToolResult ExecuteDryRun(const ToolCall& call, EditSet& outEditSet);

    // Resets call counters for a new user request
    void ResetRequestCounters() noexcept {
        m_toolCallCount = 0;
        m_fileWriteCount = 0;
    }

    [[nodiscard]] size_t GetToolCallCount() const noexcept { return m_toolCallCount; }
    [[nodiscard]] size_t GetFileWriteCount() const noexcept { return m_fileWriteCount; }

private:
    std::string m_projectRoot;
    std::vector<ToolSpec> m_tools;
    size_t m_toolCallCount = 0;
    size_t m_fileWriteCount = 0;

    void RegisterBuiltinTools();

    // Individual tool handlers
    ToolResult HandleSceneCreateActor(const std::string& argsJson, EditSet& outEditSet);
    ToolResult HandleSceneSetProperty(const std::string& argsJson, EditSet& outEditSet);
    ToolResult HandleSceneDeleteActor(const std::string& argsJson, EditSet& outEditSet);
    ToolResult HandleSceneSpawnAsset(const std::string& argsJson, EditSet& outEditSet);
    ToolResult HandleScriptWrite(const std::string& argsJson, EditSet& outEditSet);
    ToolResult HandleScriptCompileCheck(const std::string& argsJson);
    ToolResult HandlePatchCreate(const std::string& argsJson, EditSet& outEditSet);
    ToolResult HandleProjectReadFile(const std::string& argsJson);
    ToolResult HandleProjectListFiles(const std::string& argsJson);
    ToolResult HandleDocsLookup(const std::string& argsJson);
};

} // namespace qivforge::ai

#pragma once

#include "qivforge/ai/ai_types.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace qivforge::ai {

struct ContextFocus {
    std::string activeScriptPath;
    std::string activeScriptContent;
    std::string selectedActorName;
    std::string selectedActorProperties;
    std::vector<std::string> recentDiagnostics;
    std::vector<std::string> recentConsoleLogs;
};

class AiContextBuilder {
public:
    static constexpr size_t DEFAULT_TOKEN_BUDGET = 24000;
    static constexpr size_t LOCAL_GGUF_TOKEN_BUDGET = 4096;

    AiContextBuilder(size_t tokenBudget = DEFAULT_TOKEN_BUDGET);

    void SetTokenBudget(size_t budget) noexcept { m_tokenBudget = budget; }
    [[nodiscard]] size_t GetTokenBudget() const noexcept { return m_tokenBudget; }

    void SetProjectRoot(std::string rootPath) { m_projectRoot = std::move(rootPath); }

    // Adds layers
    void SetSystemPrompt(std::string prompt) { m_systemPrompt = std::move(prompt); }
    void SetApiSummary(std::string summary) { m_apiSummary = std::move(summary); }
    void SetProjectHierarchy(std::string hierarchy) { m_projectHierarchy = std::move(hierarchy); }
    void SetFocus(const ContextFocus& focus) { m_focus = focus; }

    // Redaction Filter (Doc 12 §10: keys, tokens, emails never leave device)
    [[nodiscard]] static std::string RedactSensitiveData(const std::string& input);

    // Assembles final prompt messages ready for provider transport
    [[nodiscard]] std::vector<AiMessage> BuildContext(const std::vector<AiMessage>& conversationHistory) const;

    // Approximate token counter (4 characters per token heuristic)
    [[nodiscard]] static size_t EstimateTokens(const std::string& text) noexcept {
        return (text.size() + 3) / 4;
    }

private:
    size_t m_tokenBudget;
    std::string m_projectRoot;
    std::string m_systemPrompt;
    std::string m_apiSummary;
    std::string m_projectHierarchy;
    ContextFocus m_focus;

    [[nodiscard]] std::string BuildDefaultSystemPrompt() const;
    [[nodiscard]] std::string BuildDefaultApiSummary() const;
};

} // namespace qivforge::ai

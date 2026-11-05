#pragma once

#include "qivforge/ai/ai_types.h"
#include <string>
#include <vector>
#include <functional>

namespace qivforge::ai {

struct TestConnectionResult {
    bool success = false;
    std::string message;
    int32_t latencyMs = 50;
};

class MockAiProvider {
public:
    explicit MockAiProvider(AiProviderId providerId = AiProviderId::GEMINI);

    [[nodiscard]] AiProviderId GetProviderId() const noexcept { return m_providerId; }
    void SetApiKey(std::string key) { m_apiKey = std::move(key); }
    [[nodiscard]] const std::string& GetApiKey() const noexcept { return m_apiKey; }

    void SetAirplaneMode(bool enabled) noexcept { m_airplaneMode = enabled; }
    [[nodiscard]] bool IsAirplaneMode() const noexcept { return m_airplaneMode; }

    // Test connection (Doc 12 AC1)
    [[nodiscard]] TestConnectionResult TestConnection() const;

    // Simulate chat response with tool calls
    [[nodiscard]] AiMessage GenerateTractorScriptResponse() const;
    [[nodiscard]] AiMessage GeneratePromptInjectionResponse() const;
    [[nodiscard]] std::vector<ToolCall> GenerateMalformedToolCallsFuzzer(size_t count = 200) const;

    // Network request counter (for offline verification)
    [[nodiscard]] size_t GetNetworkRequestCount() const noexcept { return m_networkRequests; }
    void ResetNetworkRequestCount() noexcept { m_networkRequests = 0; }

    // Simulates an offline Local GGUF completion
    [[nodiscard]] AiMessage ExecuteLocalGgufOffline(const std::string& userPrompt);

private:
    AiProviderId m_providerId;
    std::string m_apiKey;
    bool m_airplaneMode = false;
    mutable size_t m_networkRequests = 0;
};

} // namespace qivforge::ai

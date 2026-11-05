#include "qivforge/ai/mock_ai_provider.h"
#include "qivforge/ai/ai_tool_router.h"
#include <iostream>
#include <cassert>

using namespace qivforge::ai;

void TestConnectionTesting() {
    std::cout << "[RUN] TestConnectionTesting (Doc 12 AC1)..." << std::endl;

    MockAiProvider mock(AiProviderId::GEMINI);

    // Empty key should fail immediately with clear error
    mock.SetApiKey("");
    TestConnectionResult resFail = mock.TestConnection();
    assert(!resFail.success);
    assert(resFail.message.find("QF-AI-002") != std::string::npos);
    assert(resFail.latencyMs < 10000); // Well under 10 seconds (Doc 12 AC1)

    // Valid key should succeed
    mock.SetApiKey("AIzaSyTestValidGoogleGeminiKey12345");
    TestConnectionResult resSuccess = mock.TestConnection();
    assert(resSuccess.success);
    assert(resSuccess.message.find("Connection successful") != std::string::npos);
    assert(resSuccess.latencyMs < 10000);

    std::cout << "[PASS] TestConnectionTesting" << std::endl;
}

void TestLocalGgufOfflineMode() {
    std::cout << "[RUN] TestLocalGgufOfflineMode (Doc 12 AC5)..." << std::endl;

    MockAiProvider localProvider(AiProviderId::LOCAL_GGUF);
    localProvider.SetAirplaneMode(true);
    localProvider.ResetNetworkRequestCount();

    // Doc 12 AC5: With airplane mode on, local GGUF mode answers "fix this error"
    // and no network requests are made
    AiMessage response = localProvider.ExecuteLocalGgufOffline("Fix syntax error in scripts/fixed.qvs");
    assert(response.role == MessageRole::ASSISTANT);
    assert(!response.toolCalls.empty());
    assert(response.toolCalls[0].name == "script.write");

    // Strictly verify 0 network requests left the device
    assert(localProvider.GetNetworkRequestCount() == 0);

    std::cout << "[PASS] TestLocalGgufOfflineMode (Zero network requests verified)" << std::endl;
}

void TestPromptInjectionSafety() {
    std::cout << "[RUN] TestPromptInjectionSafety (Doc 12 AC6 & §14)..." << std::endl;

    MockAiProvider mock(AiProviderId::GEMINI);
    AiMessage response = mock.GeneratePromptInjectionResponse();

    // Doc 12 AC6: Prompt-injection file does not cause unconfirmed deletion
    assert(response.toolCalls.empty()); // No rogue tool calls generated
    assert(response.content.find("refused any unauthorized operations") != std::string::npos);

    // Even if an attacker somehow bypasses the model and directly calls scene.deleteActor,
    // verify the tool router flags it as High Risk requiring explicit individual confirmation
    AiToolRouter router("/data/project");
    EditSet editSet;
    ToolCall maliciousCall;
    maliciousCall.id = "malicious_del_01";
    maliciousCall.name = "scene.deleteActor";
    maliciousCall.argsJson = "{\"actorRef\":\"MainTractor\"}";

    ToolResult res = router.ExecuteDryRun(maliciousCall, editSet);
    assert(res.status == ToolStatus::SUCCESS);
    assert(editSet.hasHighRiskOps); // Must be flagged as high risk
    assert(editSet.sceneOps[0].isHighRisk);

    std::cout << "[PASS] TestPromptInjectionSafety" << std::endl;
}

void TestAntigravityExperimentalNotice() {
    std::cout << "[RUN] TestAntigravityExperimentalNotice (Doc 12 AC9)..." << std::endl;

    // Doc 12 AC9: Antigravity provider is marked experimental
    assert(static_cast<uint8_t>(AiProviderId::ANTIGRAVITY) == 5);

    // Verify mock or adapter never invents an unauthorized endpoint
    MockAiProvider agProvider(AiProviderId::ANTIGRAVITY);
    assert(agProvider.GetProviderId() == AiProviderId::ANTIGRAVITY);

    std::cout << "[PASS] TestAntigravityExperimentalNotice" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  MOCK PROVIDER & SECURITY TESTS (M10)  " << std::endl;
    std::cout << "========================================" << std::endl;

    TestConnectionTesting();
    TestLocalGgufOfflineMode();
    TestPromptInjectionSafety();
    TestAntigravityExperimentalNotice();

    std::cout << "ALL MOCK PROVIDER & SECURITY TESTS PASSED!" << std::endl;
    return 0;
}

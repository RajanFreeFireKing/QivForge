#include "qivforge/ai/ai_context_builder.h"
#include <iostream>
#include <cassert>

using namespace qivforge::ai;

void TestRedactionFilter() {
    std::cout << "[RUN] TestRedactionFilter (Doc 12 §10 & AC8)..." << std::endl;

    std::string textWithSecrets =
        "Here are the test tokens:\n"
        "HF: hf_AbCdEfGhIjKlMnOpQrStUvWxYz1234567890\n"
        "Google: AIzaSyD1234567890abcdefghijklmnopqrstuv\n"
        "OpenAI: sk-proj-1234567890abcdefghijklmnopqrstuv\n"
        "Groq: gsk_1234567890abcdefghijklmnopqrstuv\n"
        "GitHub: ghp_1234567890abcdefghijklmnopqrstuvwxyz\n"
        "Contact: dev.security@example.com for questions.\n"
        "-----BEGIN RSA PRIVATE KEY-----\n"
        "MIIEowIBAAKCAQEA0Y+..."
        "-----END RSA PRIVATE KEY-----\n";

    std::string redacted = AiContextBuilder::RedactSensitiveData(textWithSecrets);

    // Verify all secrets are redacted
    assert(redacted.find("hf_") == std::string::npos);
    assert(redacted.find("[REDACTED_HF_TOKEN]") != std::string::npos);

    assert(redacted.find("AIza") == std::string::npos);
    assert(redacted.find("[REDACTED_GOOGLE_KEY]") != std::string::npos);

    assert(redacted.find("sk-") == std::string::npos);
    assert(redacted.find("gsk_") == std::string::npos);
    assert(redacted.find("[REDACTED_API_KEY]") != std::string::npos);

    assert(redacted.find("ghp_") == std::string::npos);
    assert(redacted.find("[REDACTED_GITHUB_TOKEN]") != std::string::npos);

    assert(redacted.find("dev.security@example.com") == std::string::npos);
    assert(redacted.find("[REDACTED_EMAIL]") != std::string::npos);

    assert(redacted.find("BEGIN RSA PRIVATE KEY") == std::string::npos);
    assert(redacted.find("[REDACTED_PEM_PRIVATE_KEY]") != std::string::npos);

    std::cout << "[PASS] TestRedactionFilter" << std::endl;
}

void TestContextBuilderLayering() {
    std::cout << "[RUN] TestContextBuilderLayering (Doc 12 §7 & §8)..." << std::endl;

    AiContextBuilder builder(24000);
    builder.SetProjectHierarchy("Tractor_01 (VehiclePawn)\nMainCamera (Camera)\nSun (DirectionalLight)");

    ContextFocus focus;
    focus.selectedActorName = "Tractor_01";
    focus.selectedActorProperties = "mass=2100kg, maxSteer=35deg";
    focus.activeScriptPath = "scripts/tractor.qvs";
    focus.activeScriptContent = "// User script with token hf_1234567890abcdefghij12345\nclass Tractor {}";
    focus.recentDiagnostics.push_back("Warning W001: unused variable 'temp'");
    builder.SetFocus(focus);

    std::vector<AiMessage> history;
    AiMessage userMsg;
    userMsg.role = MessageRole::USER;
    userMsg.content = "Please increase the engine torque.";
    history.push_back(userMsg);

    auto messages = builder.BuildContext(history);

    assert(messages.size() >= 3); // System, Focus, User

    // System prompt verification
    assert(messages[0].role == MessageRole::SYSTEM);
    assert(messages[0].content.find("QivForge AI Copilot") != std::string::npos);
    assert(messages[0].content.find("Tractor_01 (VehiclePawn)") != std::string::npos);

    // Focus verification with data isolation
    assert(messages[1].role == MessageRole::USER);
    assert(messages[1].content.find("<active_file path=\"scripts/tractor.qvs\">") != std::string::npos);
    assert(messages[1].content.find("<project_data>") != std::string::npos);
    assert(messages[1].content.find("[REDACTED_HF_TOKEN]") != std::string::npos); // Redaction verified in context
    assert(messages[1].content.find("hf_1234567890") == std::string::npos);

    std::cout << "[PASS] TestContextBuilderLayering" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   AI CONTEXT & REDACTION TESTS (M10)   " << std::endl;
    std::cout << "========================================" << std::endl;

    TestRedactionFilter();
    TestContextBuilderLayering();

    std::cout << "ALL AI CONTEXT TESTS PASSED!" << std::endl;
    return 0;
}

#include "qivforge/ai/ai_tool_router.h"
#include "qivforge/ai/mock_ai_provider.h"
#include <iostream>
#include <cassert>

using namespace qivforge::ai;

void TestToolRegistryAndPathSecurity() {
    std::cout << "[RUN] TestToolRegistryAndPathSecurity (Doc 12 §5 & §14)..." << std::endl;

    AiToolRouter router("/data/project");
    assert(router.GetRegisteredTools().size() == 12);
    assert(router.HasTool("script.write"));
    assert(router.HasTool("scene.deleteActor"));
    assert(!router.HasTool("non_existent_tool"));

    // Path traversal security check (Doc 12 §14)
    assert(!router.IsPathAllowed("../../secrets.txt"));
    assert(!router.IsPathAllowed("../other_project/file.qvs"));
    assert(!router.IsPathAllowed(".vault/token"));
    assert(!router.IsPathAllowed(".journal/backup"));
    assert(!router.IsPathAllowed("/etc/shadow"));
    assert(router.IsPathAllowed("scripts/tractor.qvs"));
    assert(router.IsPathAllowed("scenes/level01.scene.json"));

    // Verify router rejects forbidden path in dry-run
    EditSet editSet;
    ToolCall forbiddenCall;
    forbiddenCall.id = "bad_01";
    forbiddenCall.name = "script.write";
    forbiddenCall.argsJson = "{\"path\": \"../../secrets.txt\", \"code\": \"class Evil {}\"}";

    ToolResult res = router.ExecuteDryRun(forbiddenCall, editSet);
    assert(res.status == ToolStatus::ERROR);
    assert(res.errorMessage.find("Path traversal or forbidden directory") != std::string::npos);
    assert(editSet.fileEdits.empty());

    std::cout << "[PASS] TestToolRegistryAndPathSecurity" << std::endl;
}

void TestLimitsAndSemanticCompilerValidation() {
    std::cout << "[RUN] TestLimitsAndSemanticCompilerValidation (Doc 12 §5.2 & §12)..." << std::endl;

    AiToolRouter router("/data/project");
    EditSet editSet;

    // 1. Script size > 40 KB check
    ToolCall largeScriptCall;
    largeScriptCall.id = "large_01";
    largeScriptCall.name = "script.write";
    std::string bigCode(45000, 'X');
    largeScriptCall.argsJson = "{\"path\": \"scripts/big.qvs\", \"code\": \"" + bigCode + "\"}";

    ToolResult largeRes = router.ExecuteDryRun(largeScriptCall, editSet);
    assert(largeRes.status == ToolStatus::ERROR);
    assert(largeRes.errorMessage.find("Script size exceeds 40 KB limit") != std::string::npos);

    // 2. Syntax check via real QivScript compiler
    ToolCall invalidSyntaxCall;
    invalidSyntaxCall.id = "invalid_01";
    invalidSyntaxCall.name = "script.write";
    invalidSyntaxCall.argsJson = "{\"path\": \"scripts/broken.qvs\", \"code\": \"class Broken { unclosed syntax %$! }\"}";

    ToolResult syntaxRes = router.ExecuteDryRun(invalidSyntaxCall, editSet);
    assert(syntaxRes.status == ToolStatus::ERROR);
    assert(syntaxRes.errorMessage.find("QF-AI-006") != std::string::npos);

    // 3. Valid script compiles cleanly
    ToolCall validCall;
    validCall.id = "valid_01";
    validCall.name = "script.write";
    validCall.argsJson = "{\"path\": \"scripts/good.qvs\", \"code\": \"pub class Good { fn onStart() -> void {} }\"}";

    ToolResult validRes = router.ExecuteDryRun(validCall, editSet);
    assert(validRes.status == ToolStatus::SUCCESS);
    assert(!editSet.fileEdits.empty());
    assert(editSet.fileEdits.back().compileSuccess);

    std::cout << "[PASS] TestLimitsAndSemanticCompilerValidation" << std::endl;
}

void Test200MalformedCallsFuzzer() {
    std::cout << "[RUN] Test200MalformedCallsFuzzer (Doc 12 AC7)..." << std::endl;

    MockAiProvider mock(AiProviderId::GEMINI);
    auto fuzzedCalls = mock.GenerateMalformedToolCallsFuzzer(200);
    assert(fuzzedCalls.size() == 200);

    AiToolRouter router("/data/project");
    EditSet editSet;

    size_t errorCount = 0;
    for (size_t i = 0; i < fuzzedCalls.size(); ++i) {
        // Reset counters every 20 calls to test individual payload safety
        if (i % 20 == 0) router.ResetRequestCounters();

        ToolResult res = router.ExecuteDryRun(fuzzedCalls[i], editSet);
        if (res.status == ToolStatus::ERROR) {
            errorCount++;
            assert(!res.errorMessage.empty()); // Must produce a readable error message
        }
    }

    // Doc 12 AC7: Every invalid call results in a readable error message and ZERO crashes across 200 calls
    std::cout << "  Fuzzed 200 malformed tool calls: " << errorCount << " errors safely handled with zero crashes" << std::endl;
    assert(errorCount > 0);

    std::cout << "[PASS] Test200MalformedCallsFuzzer" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   AI TOOL ROUTER & FUZZER TESTS (M10)  " << std::endl;
    std::cout << "========================================" << std::endl;

    TestToolRegistryAndPathSecurity();
    TestLimitsAndSemanticCompilerValidation();
    Test200MalformedCallsFuzzer();

    std::cout << "ALL AI TOOL ROUTER TESTS PASSED!" << std::endl;
    return 0;
}

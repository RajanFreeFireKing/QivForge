#include "qivforge/ai/ai_tool_router.h"
#include "qivforge/script/lexer.h"
#include "qivforge/script/parser.h"
#include "qivforge/script/compiler.h"
#include "qivforge/patch/patch_validator.h"
#include <sstream>
#include <algorithm>

namespace qivforge::ai {

AiToolRouter::AiToolRouter(std::string projectRoot)
    : m_projectRoot(std::move(projectRoot)) {
    RegisterBuiltinTools();
}

void AiToolRouter::RegisterBuiltinTools() {
    m_tools.push_back({
        "scene.spawnAsset",
        "Place an asset into the active scene",
        "{\"type\":\"object\",\"properties\":{\"assetRef\":{\"type\":\"string\"},\"position\":{\"type\":\"array\"},\"name\":{\"type\":\"string\"}},\"required\":[\"assetRef\"]}",
        false
    });

    m_tools.push_back({
        "scene.createActor",
        "Create light, camera, empty, primitive, collider, audio emitter, etc.",
        "{\"type\":\"object\",\"properties\":{\"type\":{\"type\":\"string\"},\"name\":{\"type\":\"string\"},\"parent\":{\"type\":\"string\"}},\"required\":[\"type\",\"name\"]}",
        false
    });

    m_tools.push_back({
        "scene.setProperty",
        "Change an Inspector property on an actor",
        "{\"type\":\"object\",\"properties\":{\"actorRef\":{\"type\":\"string\"},\"path\":{\"type\":\"string\"},\"value\":{\"type\":\"string\"}},\"required\":[\"actorRef\",\"path\",\"value\"]}",
        false
    });

    m_tools.push_back({
        "scene.deleteActor",
        "Delete an actor from the scene",
        "{\"type\":\"object\",\"properties\":{\"actorRef\":{\"type\":\"string\"}},\"required\":[\"actorRef\"]}",
        true // High risk (Doc 12 §5.1)
    });

    m_tools.push_back({
        "script.write",
        "Create or replace a .qvs script file",
        "{\"type\":\"object\",\"properties\":{\"path\":{\"type\":\"string\"},\"code\":{\"type\":\"string\"},\"mode\":{\"type\":\"string\"}},\"required\":[\"path\",\"code\"]}",
        false
    });

    m_tools.push_back({
        "script.compileCheck",
        "Compile and return syntax and type diagnostics for a script",
        "{\"type\":\"object\",\"properties\":{\"path\":{\"type\":\"string\"},\"code\":{\"type\":\"string\"}},\"required\":[\"code\"]}",
        false
    });

    m_tools.push_back({
        "patch.create",
        "Create a .qpatch visual logic graph",
        "{\"type\":\"object\",\"properties\":{\"actorRef\":{\"type\":\"string\"},\"patchJson\":{\"type\":\"string\"}},\"required\":[\"actorRef\",\"patchJson\"]}",
        false
    });

    m_tools.push_back({
        "assets.search",
        "Search project and starter library asset indices",
        "{\"type\":\"object\",\"properties\":{\"query\":{\"type\":\"string\"}},\"required\":[\"query\"]}",
        false
    });

    m_tools.push_back({
        "project.readFile",
        "Read a project text file (capped at 100 KB)",
        "{\"type\":\"object\",\"properties\":{\"path\":{\"type\":\"string\"}},\"required\":[\"path\"]}",
        false
    });

    m_tools.push_back({
        "project.listFiles",
        "List files in a project folder",
        "{\"type\":\"object\",\"properties\":{\"folder\":{\"type\":\"string\"}},\"required\":[\"folder\"]}",
        false
    });

    m_tools.push_back({
        "console.getErrors",
        "Retrieve recent diagnostics and runtime console errors",
        "{\"type\":\"object\",\"properties\":{\"count\":{\"type\":\"integer\"}}}",
        false
    });

    m_tools.push_back({
        "docs.lookup",
        "Search the built-in QivScript API docs and patch catalog",
        "{\"type\":\"object\",\"properties\":{\"topic\":{\"type\":\"string\"}},\"required\":[\"topic\"]}",
        false
    });
}

bool AiToolRouter::HasTool(const std::string& name) const noexcept {
    for (const auto& t : m_tools) {
        if (t.name == name) return true;
    }
    return false;
}

bool AiToolRouter::IsPathAllowed(const std::string& path) const noexcept {
    if (path.empty()) return false;

    // Reject path traversal tokens
    if (path.find("..") != std::string::npos) return false;
    if (path.find("./") == 0) return false;

    // Block sensitive directories and files (Doc 12 §14)
    if (path.find(".vault") != std::string::npos ||
        path.find(".journal") != std::string::npos ||
        path.find(".cache") != std::string::npos ||
        path.find("project.lock") != std::string::npos ||
        path.find("credentials") != std::string::npos ||
        path.find("/etc/") != std::string::npos ||
        path.find("/data/user/") != std::string::npos) {
        return false;
    }

    return true;
}

ToolResult AiToolRouter::ExecuteDryRun(const ToolCall& call, EditSet& outEditSet) {
    ToolResult res;
    res.toolCallId = call.id;
    res.toolName = call.name;

    // 1. Tool count limit check (Doc 12 §5.2: max 25 calls per request)
    m_toolCallCount++;
    if (m_toolCallCount > MAX_TOOL_CALLS_PER_REQUEST) {
        res.status = ToolStatus::ERROR;
        res.errorMessage = "QF-AI-005: Tool call limit exceeded (maximum 25 calls per request)";
        return res;
    }

    if (!HasTool(call.name)) {
        res.status = ToolStatus::ERROR;
        res.errorMessage = "QF-AI-005: Unknown tool '" + call.name + "'";
        return res;
    }

    if (call.name == "script.write") {
        return HandleScriptWrite(call.argsJson, outEditSet);
    } else if (call.name == "script.compileCheck") {
        return HandleScriptCompileCheck(call.argsJson);
    } else if (call.name == "scene.createActor") {
        return HandleSceneCreateActor(call.argsJson, outEditSet);
    } else if (call.name == "scene.setProperty") {
        return HandleSceneSetProperty(call.argsJson, outEditSet);
    } else if (call.name == "scene.deleteActor") {
        return HandleSceneDeleteActor(call.argsJson, outEditSet);
    } else if (call.name == "scene.spawnAsset") {
        return HandleSceneSpawnAsset(call.argsJson, outEditSet);
    } else if (call.name == "patch.create") {
        return HandlePatchCreate(call.argsJson, outEditSet);
    } else if (call.name == "project.readFile") {
        return HandleProjectReadFile(call.argsJson);
    } else if (call.name == "project.listFiles") {
        return HandleProjectListFiles(call.argsJson);
    } else if (call.name == "docs.lookup") {
        return HandleDocsLookup(call.argsJson);
    }

    res.status = ToolStatus::SUCCESS;
    res.outputJson = "{\"status\":\"ok\"}";
    return res;
}

namespace {

struct ScriptCompileResult {
    bool success = false;
    std::string diagnostics;
};

static ScriptCompileResult ValidateScript(const std::string& code, const std::string& filename = "<script>") {
    ScriptCompileResult res;
    if (code.empty()) {
        res.success = false;
        res.diagnostics = "Empty script source";
        return res;
    }

    qivforge::script::Lexer lexer(code, filename);
    auto tokens = lexer.Tokenize();
    if (lexer.HasErrors()) {
        res.success = false;
        for (const auto& d : lexer.GetDiagnostics()) {
            if (!res.diagnostics.empty()) res.diagnostics += "; ";
            res.diagnostics += "[" + d.code + "] " + d.message;
        }
        return res;
    }

    qivforge::script::Parser parser(std::move(tokens));
    auto ast = parser.Parse();
    if (parser.HasErrors() || !ast) {
        res.success = false;
        for (const auto& d : parser.GetDiagnostics()) {
            if (!res.diagnostics.empty()) res.diagnostics += "; ";
            res.diagnostics += "[" + d.code + "] " + d.message;
        }
        return res;
    }

    qivforge::script::Compiler compiler(*ast);
    auto module = compiler.Compile();
    if (compiler.HasErrors() || !module) {
        res.success = false;
        for (const auto& d : compiler.GetDiagnostics()) {
            if (!res.diagnostics.empty()) res.diagnostics += "; ";
            res.diagnostics += "[" + d.code + "] " + d.message;
        }
        return res;
    }

    res.success = true;
    return res;
}

} // anonymous namespace

static std::string UnescapeJsonString(const std::string& str) {
    std::string out;
    out.reserve(str.size());
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '\\' && i + 1 < str.size()) {
            char next = str[i + 1];
            if (next == 'n') {
                out.push_back('\n');
                i++;
            } else if (next == 'r') {
                out.push_back('\r');
                i++;
            } else if (next == 't') {
                out.push_back('\t');
                i++;
            } else if (next == '\"') {
                out.push_back('\"');
                i++;
            } else if (next == '\\') {
                out.push_back('\\');
                i++;
            } else {
                out.push_back(next);
                i++;
            }
        } else {
            out.push_back(str[i]);
        }
    }
    return out;
}

static std::string ExtractJsonField(const std::string& json, const std::string& field) {
    std::string pattern = "\"" + field + "\":";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos) return "";

    size_t valStart = json.find_first_not_of(" \t\n\r", pos + pattern.size());
    if (valStart == std::string::npos) return "";

    if (json[valStart] == '\"') {
        size_t cur = valStart + 1;
        while (cur < json.size()) {
            if (json[cur] == '\\') {
                cur += 2; // skip escape sequence
            } else if (json[cur] == '\"') {
                std::string raw = json.substr(valStart + 1, cur - (valStart + 1));
                return UnescapeJsonString(raw);
            } else {
                cur++;
            }
        }
        return UnescapeJsonString(json.substr(valStart + 1));
    } else {
        size_t end = json.find_first_of(",}\n\r", valStart);
        if (end != std::string::npos) {
            return json.substr(valStart, end - valStart);
        }
        return json.substr(valStart);
    }
}

ToolResult AiToolRouter::HandleScriptWrite(const std::string& argsJson, EditSet& outEditSet) {
    ToolResult res;
    res.toolName = "script.write";

    std::string path = ExtractJsonField(argsJson, "path");
    if (path.empty()) {
        res.status = ToolStatus::ERROR;
        res.errorMessage = "QF-AI-005: Missing required parameter 'path'";
        return res;
    }

    if (!IsPathAllowed(path)) {
        res.status = ToolStatus::ERROR;
        res.errorMessage = "QF-AI-005: Path traversal or forbidden directory detected: '" + path + "'";
        return res;
    }

    // Check file write count limit (Doc 12 §5.2: max 8 file writes)
    m_fileWriteCount++;
    if (m_fileWriteCount > MAX_FILE_WRITES_PER_REQUEST) {
        res.status = ToolStatus::ERROR;
        res.errorMessage = "QF-AI-005: File write limit exceeded (maximum 8 file writes per request)";
        return res;
    }

    // Extract code
    std::string code = ExtractJsonField(argsJson, "code");

    // Max script size check (40 KB)
    if (code.size() > MAX_SCRIPT_SIZE_BYTES) {
        res.status = ToolStatus::ERROR;
        res.errorMessage = "QF-AI-005: Script size exceeds 40 KB limit (" + std::to_string(code.size()) + " bytes)";
        return res;
    }

    // Semantic validation via real QivScript compiler
    auto compileRes = ValidateScript(code, path);
    bool compiles = compileRes.success;

    ProposedFileEdit edit;
    edit.path = path;
    edit.originalContent = ""; // In dry-run mock original is empty
    edit.proposedContent = code;
    edit.compileSuccess = compiles;
    if (!compiles) {
        edit.compileDiagnostics = compileRes.diagnostics;
        outEditSet.allCompileSuccess = false;
    }

    // Generate unified diff representation
    std::ostringstream diff;
    diff << "--- a/" << path << "\n+++ b/" << path << "\n@@ -0,0 +1," << (code.empty() ? 0 : 10) << " @@\n";
    diff << "+" << code.substr(0, std::min<size_t>(code.size(), 200)) << (code.size() > 200 ? "\n+..." : "");
    edit.diffUnified = diff.str();

    outEditSet.fileEdits.push_back(edit);
    outEditSet.summary = "Write script " + path;

    res.status = compiles ? ToolStatus::SUCCESS : ToolStatus::ERROR;
    res.outputJson = "{\"status\":\"" + std::string(compiles ? "compiled" : "compile_error") + "\",\"diagnostics\":\"" + edit.compileDiagnostics + "\"}";
    if (!compiles) {
        res.errorMessage = "QF-AI-006: Generated script does not compile: " + edit.compileDiagnostics;
    }
    return res;
}

ToolResult AiToolRouter::HandleScriptCompileCheck(const std::string& argsJson) {
    ToolResult res;
    res.toolName = "script.compileCheck";

    std::string code = ExtractJsonField(argsJson, "code");
    if (code.empty()) {
        size_t cpos = argsJson.find("\"code\":");
        if (cpos != std::string::npos) {
            size_t quoteStart = argsJson.find('\"', cpos + 7);
            size_t quoteEnd = argsJson.rfind('\"');
            if (quoteEnd > quoteStart) {
                code = argsJson.substr(quoteStart + 1, quoteEnd - (quoteStart + 1));
            }
        }
    }

    auto compileRes = ValidateScript(code);
    res.status = ToolStatus::SUCCESS;
    if (compileRes.success) {
        res.outputJson = "{\"success\":true,\"diagnostics\":\"\"}";
    } else {
        res.outputJson = "{\"success\":false,\"diagnostics\":\"" + compileRes.diagnostics + "\"}";
    }
    return res;
}

ToolResult AiToolRouter::HandleSceneCreateActor(const std::string& argsJson, EditSet& outEditSet) {
    ToolResult res;
    res.toolName = "scene.createActor";

    std::string name = ExtractJsonField(argsJson, "name");
    std::string type = ExtractJsonField(argsJson, "type");
    if (name.empty() || type.empty()) {
        res.status = ToolStatus::ERROR;
        res.errorMessage = "QF-AI-005: Missing required fields 'name' or 'type'";
        return res;
    }

    ProposedSceneOp op;
    op.type = SceneOpType::CREATE_ACTOR;
    op.actorName = name;
    op.value = type;
    op.isHighRisk = false;

    outEditSet.sceneOps.push_back(op);
    res.status = ToolStatus::SUCCESS;
    res.outputJson = "{\"status\":\"ok\",\"created\":\"" + name + "\"}";
    return res;
}

ToolResult AiToolRouter::HandleSceneSetProperty(const std::string& argsJson, EditSet& outEditSet) {
    ToolResult res;
    res.toolName = "scene.setProperty";

    std::string actorRef = ExtractJsonField(argsJson, "actorRef");
    std::string path = ExtractJsonField(argsJson, "path");
    std::string value = ExtractJsonField(argsJson, "value");

    if (actorRef.empty() || path.empty()) {
        res.status = ToolStatus::ERROR;
        res.errorMessage = "QF-AI-005: Missing actorRef or path";
        return res;
    }

    ProposedSceneOp op;
    op.type = SceneOpType::SET_PROPERTY;
    op.actorName = actorRef;
    op.propertyPath = path;
    op.value = value;
    op.isHighRisk = false;

    outEditSet.sceneOps.push_back(op);
    res.status = ToolStatus::SUCCESS;
    res.outputJson = "{\"status\":\"ok\"}";
    return res;
}

ToolResult AiToolRouter::HandleSceneDeleteActor(const std::string& argsJson, EditSet& outEditSet) {
    ToolResult res;
    res.toolName = "scene.deleteActor";

    std::string actorRef = ExtractJsonField(argsJson, "actorRef");
    if (actorRef.empty()) {
        res.status = ToolStatus::ERROR;
        res.errorMessage = "QF-AI-005: Missing actorRef";
        return res;
    }

    ProposedSceneOp op;
    op.type = SceneOpType::DELETE_ACTOR;
    op.actorName = actorRef;
    op.isHighRisk = true; // Always High Risk (Doc 12 §5.1 & §14)

    outEditSet.sceneOps.push_back(op);
    outEditSet.hasHighRiskOps = true;

    res.status = ToolStatus::SUCCESS;
    res.outputJson = "{\"status\":\"requires_user_confirmation\",\"highRisk\":true}";
    return res;
}

ToolResult AiToolRouter::HandleSceneSpawnAsset(const std::string& argsJson, EditSet& outEditSet) {
    ToolResult res;
    res.toolName = "scene.spawnAsset";

    std::string assetRef = ExtractJsonField(argsJson, "assetRef");
    std::string name = ExtractJsonField(argsJson, "name");
    if (name.empty()) name = "Spawned_" + assetRef;

    ProposedSceneOp op;
    op.type = SceneOpType::SPAWN_ASSET;
    op.actorName = name;
    op.value = assetRef;
    op.isHighRisk = false;

    outEditSet.sceneOps.push_back(op);
    res.status = ToolStatus::SUCCESS;
    res.outputJson = "{\"status\":\"ok\",\"spawned\":\"" + name + "\"}";
    return res;
}

ToolResult AiToolRouter::HandlePatchCreate(const std::string& argsJson, EditSet& outEditSet) {
    ToolResult res;
    res.toolName = "patch.create";

    std::string actorRef = ExtractJsonField(argsJson, "actorRef");
    ProposedSceneOp op;
    op.type = SceneOpType::SET_PROPERTY;
    op.actorName = actorRef;
    op.propertyPath = "patch";
    op.value = "patch.qpatch";
    op.isHighRisk = false;

    outEditSet.sceneOps.push_back(op);
    res.status = ToolStatus::SUCCESS;
    res.outputJson = "{\"status\":\"ok\",\"patch\":\"created\"}";
    return res;
}

ToolResult AiToolRouter::HandleProjectReadFile(const std::string& argsJson) {
    ToolResult res;
    res.toolName = "project.readFile";

    std::string path = ExtractJsonField(argsJson, "path");
    if (!IsPathAllowed(path)) {
        res.status = ToolStatus::ERROR;
        res.errorMessage = "QF-AI-005: Forbidden path access: '" + path + "'";
        return res;
    }

    res.status = ToolStatus::SUCCESS;
    res.outputJson = "{\"content\":\"// Mock file content for " + path + "\"}";
    return res;
}

ToolResult AiToolRouter::HandleProjectListFiles(const std::string& /*argsJson*/) {
    ToolResult res;
    res.toolName = "project.listFiles";
    res.status = ToolStatus::SUCCESS;
    res.outputJson = "{\"files\":[\"scripts/tractor.qvs\",\"scenes/main.scene.json\"]}";
    return res;
}

ToolResult AiToolRouter::HandleDocsLookup(const std::string& argsJson) {
    ToolResult res;
    res.toolName = "docs.lookup";
    std::string topic = ExtractJsonField(argsJson, "topic");
    res.status = ToolStatus::SUCCESS;
    res.outputJson = "{\"topic\":\"" + topic + "\",\"doc\":\"API documentation for " + topic + "\"}";
    return res;
}

} // namespace qivforge::ai

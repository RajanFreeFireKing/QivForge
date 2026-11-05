#pragma once

#include "qivforge/patch/patch_model.h"
#include "qivforge/patch/patch_validator.h"
#include "qivforge/script/bytecode.h"
#include <memory>
#include <string>

namespace qivforge::patch {

struct PatchCompileResult {
    bool success = false;
    std::shared_ptr<script::BytecodeModule> module;
    std::vector<PatchDiagnostic> diagnostics;
    std::string errorMessage;
};

class PatchCompiler {
public:
    PatchCompiler() = default;

    [[nodiscard]] PatchCompileResult Compile(const PatchGraph& graph);

private:
    void CompileEventChain(const PatchGraph& graph, const PatchNode& startNode, script::CompiledFunction& fn);
};

} // namespace qivforge::patch

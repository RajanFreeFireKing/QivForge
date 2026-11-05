#pragma once

#include "qivforge/script/ast.h"
#include "qivforge/script/bytecode.h"
#include <memory>
#include <unordered_map>

namespace qivforge::script {

class Compiler {
public:
    Compiler(const ScriptFile& ast);

    std::unique_ptr<BytecodeModule> Compile();
    [[nodiscard]] const std::vector<Diagnostic>& GetDiagnostics() const noexcept { return m_diagnostics; }
    [[nodiscard]] bool HasErrors() const noexcept;

private:
    void AddError(const std::string& code, const std::string& message, const SourceLocation& loc);
    void CompileClass(const ClassDecl& cls, BytecodeModule& module);
    CompiledFunction CompileMethod(const ClassDecl& cls, const MethodDecl& method);

    uint8_t AllocateRegister();
    uint8_t GetVariableRegister(const std::string& name);

    void CompileStmt(const Stmt& stmt, CompiledFunction& fn);
    void CompileBlock(const BlockStmt& block, CompiledFunction& fn);
    uint8_t CompileExpr(const Expr& expr, CompiledFunction& fn);

    const ScriptFile& m_ast;
    std::vector<Diagnostic> m_diagnostics;

    // Register allocation state for current function
    uint8_t m_nextReg = 1; // r0 is reserved for 'self'
    std::unordered_map<std::string, uint8_t> m_locals;
};

} // namespace qivforge::script

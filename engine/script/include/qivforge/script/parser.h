#pragma once

#include "qivforge/script/token.h"
#include "qivforge/script/ast.h"
#include <vector>
#include <memory>

namespace qivforge::script {

class Parser {
public:
    Parser(std::vector<Token> tokens);

    std::unique_ptr<ScriptFile> Parse();
    [[nodiscard]] const std::vector<Diagnostic>& GetDiagnostics() const noexcept { return m_diagnostics; }
    [[nodiscard]] bool HasErrors() const noexcept;

    // Helper to extract Inspector metadata from AST
    static std::vector<ExportedFieldMetadata> ExtractExportedFields(const ClassDecl& classDecl);

private:
    const Token& Peek() const noexcept;
    const Token& Previous() const noexcept;
    bool IsAtEnd() const noexcept;
    const Token& Advance() noexcept;
    bool Check(TokenType type) const noexcept;
    bool Match(TokenType type) noexcept;
    const Token& Consume(TokenType type, const std::string& code, const std::string& errorMessage);

    void Synchronize();
    void AddError(const std::string& code, const std::string& message, const SourceLocation& loc);

    // Parsing routines
    void ParseModule(ScriptFile& file);
    void ParseImport(ScriptFile& file);
    std::vector<AstDecorator> ParseDecorators();
    ClassDecl ParseClass(std::vector<AstDecorator> decorators);
    FieldDecl ParseField(std::vector<AstDecorator> decorators, bool isPub);
    MethodDecl ParseMethod(bool isPub, bool isOverride, bool isAsync);
    QivType ParseType();

    std::unique_ptr<Stmt> ParseStatement();
    std::unique_ptr<BlockStmt> ParseBlock();
    std::unique_ptr<Stmt> ParseVarDecl();
    std::unique_ptr<Stmt> ParseIfStatement();
    std::unique_ptr<Stmt> ParseWhileStatement();
    std::unique_ptr<Stmt> ParseForStatement();
    std::unique_ptr<Stmt> ParseReturnStatement();
    std::unique_ptr<Stmt> ParseTryCatchStatement();
    std::unique_ptr<Stmt> ParseExpressionStatement();

    std::unique_ptr<Expr> ParseExpression();
    std::unique_ptr<Expr> ParseAssignment();
    std::unique_ptr<Expr> ParseLogicalOr();
    std::unique_ptr<Expr> ParseLogicalAnd();
    std::unique_ptr<Expr> ParseEquality();
    std::unique_ptr<Expr> ParseComparison();
    std::unique_ptr<Expr> ParseTerm();
    std::unique_ptr<Expr> ParseFactor();
    std::unique_ptr<Expr> ParseUnary();
    std::unique_ptr<Expr> ParseCallOrMember();
    std::unique_ptr<Expr> ParsePrimary();

    std::vector<Token> m_tokens;
    size_t m_current = 0;
    std::vector<Diagnostic> m_diagnostics;
};

} // namespace qivforge::script

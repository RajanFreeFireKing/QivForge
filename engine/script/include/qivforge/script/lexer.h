#pragma once

#include "qivforge/script/token.h"
#include <string>
#include <vector>

namespace qivforge::script {

class Lexer {
public:
    Lexer(std::string source, std::string filename = "<script>");

    std::vector<Token> Tokenize();
    [[nodiscard]] const std::vector<Diagnostic>& GetDiagnostics() const noexcept { return m_diagnostics; }
    [[nodiscard]] bool HasErrors() const noexcept;

private:
    char Peek() const noexcept;
    char PeekNext() const noexcept;
    char Advance() noexcept;
    bool Match(char expected) noexcept;
    bool IsAtEnd() const noexcept;

    void SkipWhitespaceAndComments();
    Token ScanToken();

    Token ScanIdentifierOrKeyword();
    Token ScanDecorator();
    Token ScanNumber();
    Token ScanString();

    void AddError(const std::string& code, const std::string& message, uint32_t length = 1);

    std::string m_source;
    std::string m_filename;
    size_t m_start = 0;
    size_t m_current = 0;
    uint32_t m_line = 1;
    uint32_t m_column = 1;
    uint32_t m_startColumn = 1;

    std::vector<Diagnostic> m_diagnostics;
};

} // namespace qivforge::script

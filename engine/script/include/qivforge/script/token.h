#pragma once

#include <string>
#include <cstdint>
#include <string_view>

namespace qivforge::script {

enum class TokenType : uint16_t {
    // End of file / Error
    EOF_TOKEN = 0,
    ERROR_TOKEN,

    // Identifiers & Literals
    IDENTIFIER,
    INT_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,
    BOOL_LITERAL,

    // Keywords (Doc 08 §2)
    KW_MODULE,
    KW_IMPORT,
    KW_PUB,
    KW_PRIVATE,
    KW_PROTECTED,
    KW_CLASS,
    KW_INTERFACE,
    KW_ENUM,
    KW_STRUCT,
    KW_EXTENDS,
    KW_IMPLEMENTS,
    KW_OVERRIDE,
    KW_FN,
    KW_VAR,
    KW_LET,
    KW_CONST,
    KW_IF,
    KW_ELSE,
    KW_WHILE,
    KW_FOR,
    KW_IN,
    KW_DO,
    KW_SWITCH,
    KW_CASE,
    KW_DEFAULT,
    KW_BREAK,
    KW_CONTINUE,
    KW_RETURN,
    KW_ASYNC,
    KW_AWAIT,
    KW_YIELD,
    KW_SPAWN,
    KW_NEW,
    KW_SELF,
    KW_SUPER,
    KW_TRUE,
    KW_FALSE,
    KW_NULL,
    KW_IS,
    KW_AS,
    KW_STATIC,
    KW_EVENT,
    KW_EMIT,
    KW_TRY,
    KW_CATCH,
    KW_THROW,

    // Decorators / Attributes (Doc 08 §5)
    DECORATOR_COMPONENT,
    DECORATOR_EXPORT,
    DECORATOR_RANGE,
    DECORATOR_STEP,
    DECORATOR_HEADER,
    DECORATOR_TOOLTIP,
    DECORATOR_READONLY,
    DECORATOR_HIDDEN,
    DECORATOR_REQUIRE,
    DECORATOR_EXECUTE_IN_EDITOR,
    DECORATOR_FAST,
    DECORATOR_DEPRECATED,
    GENERIC_DECORATOR, // Other @identifier

    // Operators and Punctuation
    PLUS,               // +
    PLUS_PLUS,          // ++
    MINUS,              // -
    MINUS_MINUS,        // --
    STAR,               // *
    SLASH,              // /
    PERCENT,            // %
    EQUAL,              // =
    PLUS_EQUAL,         // +=
    MINUS_EQUAL,        // -=
    STAR_EQUAL,         // *=
    SLASH_EQUAL,        // /=
    EQUAL_EQUAL,        // ==
    BANG_EQUAL,         // !=
    LESS,               // <
    LESS_EQUAL,         // <=
    GREATER,            // >
    GREATER_EQUAL,      // >=
    AMP_AMP,            // &&
    PIPE_PIPE,          // ||
    BANG,               // !
    AMP,                // &
    PIPE,               // |
    CARET,              // ^
    TILDE,              // ~
    QUESTION,           // ?
    QUESTION_QUESTION,  // ??
    DOT,                // .
    DOT_QUESTION,       // ?.
    ARROW,              // ->
    COLON,              // :
    SEMICOLON,          // ;
    COMMA,              // ,
    LPAREN,             // (
    RPAREN,             // )
    LBRACE,             // {
    RBRACE,             // }
    LBRACKET,           // [
    RBRACKET            // ]
};

struct SourceLocation {
    std::string file;
    uint32_t line = 1;
    uint32_t column = 1;
    uint32_t length = 0;
};

struct Token {
    TokenType type = TokenType::EOF_TOKEN;
    std::string lexeme;
    SourceLocation location;

    int64_t intValue = 0;
    double floatValue = 0.0;
    std::string stringValue;

    [[nodiscard]] bool IsKeyword() const noexcept {
        return type >= TokenType::KW_MODULE && type <= TokenType::KW_THROW;
    }

    [[nodiscard]] bool IsDecorator() const noexcept {
        return type >= TokenType::DECORATOR_COMPONENT && type <= TokenType::GENERIC_DECORATOR;
    }
};

enum class DiagnosticSeverity : uint8_t {
    INFO = 0,
    WARNING = 1,
    ERROR = 2
};

struct Diagnostic {
    DiagnosticSeverity severity = DiagnosticSeverity::ERROR;
    std::string code;    // e.g. "QS0001", "QS0010" per Doc 08 §14.1
    std::string message;
    SourceLocation location;
    std::string fixHint; // e.g. "Did you mean 'getComponent<RigidBody>()'?"
};

} // namespace qivforge::script

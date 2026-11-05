#include "qivforge/script/lexer.h"
#include <cctype>
#include <unordered_map>
#include <sstream>

namespace qivforge::script {

static const std::unordered_map<std::string, TokenType> s_keywords = {
    {"module",     TokenType::KW_MODULE},
    {"import",     TokenType::KW_IMPORT},
    {"pub",        TokenType::KW_PUB},
    {"private",    TokenType::KW_PRIVATE},
    {"protected",  TokenType::KW_PROTECTED},
    {"class",      TokenType::KW_CLASS},
    {"interface",  TokenType::KW_INTERFACE},
    {"enum",       TokenType::KW_ENUM},
    {"struct",     TokenType::KW_STRUCT},
    {"extends",    TokenType::KW_EXTENDS},
    {"implements", TokenType::KW_IMPLEMENTS},
    {"override",   TokenType::KW_OVERRIDE},
    {"fn",         TokenType::KW_FN},
    {"var",        TokenType::KW_VAR},
    {"let",        TokenType::KW_LET},
    {"const",      TokenType::KW_CONST},
    {"if",         TokenType::KW_IF},
    {"else",       TokenType::KW_ELSE},
    {"while",      TokenType::KW_WHILE},
    {"for",        TokenType::KW_FOR},
    {"in",         TokenType::KW_IN},
    {"do",         TokenType::KW_DO},
    {"switch",     TokenType::KW_SWITCH},
    {"case",       TokenType::KW_CASE},
    {"default",    TokenType::KW_DEFAULT},
    {"break",      TokenType::KW_BREAK},
    {"continue",   TokenType::KW_CONTINUE},
    {"return",     TokenType::KW_RETURN},
    {"async",      TokenType::KW_ASYNC},
    {"await",      TokenType::KW_AWAIT},
    {"yield",      TokenType::KW_YIELD},
    {"spawn",      TokenType::KW_SPAWN},
    {"new",        TokenType::KW_NEW},
    {"self",       TokenType::KW_SELF},
    {"super",      TokenType::KW_SUPER},
    {"true",       TokenType::KW_TRUE},
    {"false",      TokenType::KW_FALSE},
    {"null",       TokenType::KW_NULL},
    {"is",         TokenType::KW_IS},
    {"as",         TokenType::KW_AS},
    {"static",     TokenType::KW_STATIC},
    {"event",      TokenType::KW_EVENT},
    {"emit",       TokenType::KW_EMIT},
    {"try",        TokenType::KW_TRY},
    {"catch",      TokenType::KW_CATCH},
    {"throw",      TokenType::KW_THROW}
};

static const std::unordered_map<std::string, TokenType> s_decorators = {
    {"component",        TokenType::DECORATOR_COMPONENT},
    {"export",           TokenType::DECORATOR_EXPORT},
    {"range",            TokenType::DECORATOR_RANGE},
    {"step",             TokenType::DECORATOR_STEP},
    {"header",           TokenType::DECORATOR_HEADER},
    {"tooltip",          TokenType::DECORATOR_TOOLTIP},
    {"readonly",         TokenType::DECORATOR_READONLY},
    {"hidden",           TokenType::DECORATOR_HIDDEN},
    {"require",          TokenType::DECORATOR_REQUIRE},
    {"executeInEditor",  TokenType::DECORATOR_EXECUTE_IN_EDITOR},
    {"fast",             TokenType::DECORATOR_FAST},
    {"deprecated",       TokenType::DECORATOR_DEPRECATED}
};

Lexer::Lexer(std::string source, std::string filename)
    : m_source(std::move(source)), m_filename(std::move(filename)) {
}

bool Lexer::HasErrors() const noexcept {
    for (const auto& diag : m_diagnostics) {
        if (diag.severity == DiagnosticSeverity::ERROR) return true;
    }
    return false;
}

char Lexer::Peek() const noexcept {
    if (IsAtEnd()) return '\0';
    return m_source[m_current];
}

char Lexer::PeekNext() const noexcept {
    if (m_current + 1 >= m_source.size()) return '\0';
    return m_source[m_current + 1];
}

char Lexer::Advance() noexcept {
    char c = m_source[m_current++];
    m_column++;
    return c;
}

bool Lexer::Match(char expected) noexcept {
    if (IsAtEnd()) return false;
    if (m_source[m_current] != expected) return false;
    m_current++;
    m_column++;
    return true;
}

bool Lexer::IsAtEnd() const noexcept {
    return m_current >= m_source.size();
}

void Lexer::AddError(const std::string& code, const std::string& message, uint32_t length) {
    Diagnostic diag;
    diag.severity = DiagnosticSeverity::ERROR;
    diag.code = code;
    diag.message = message;
    diag.location.file = m_filename;
    diag.location.line = m_line;
    diag.location.column = m_startColumn;
    diag.location.length = length;
    m_diagnostics.push_back(diag);
}

void Lexer::SkipWhitespaceAndComments() {
    while (!IsAtEnd()) {
        char c = Peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                Advance();
                break;
            case '\n':
                m_line++;
                m_column = 1;
                Advance();
                break;
            case '/':
                if (PeekNext() == '/') {
                    // Line comment
                    while (Peek() != '\n' && !IsAtEnd()) {
                        Advance();
                    }
                } else if (PeekNext() == '*') {
                    // Nested block comment per Doc 08 §2
                    Advance(); // skip '/'
                    Advance(); // skip '*'
                    int nesting = 1;
                    while (!IsAtEnd() && nesting > 0) {
                        if (Peek() == '\n') {
                            m_line++;
                            m_column = 1;
                            Advance();
                        } else if (Peek() == '/' && PeekNext() == '*') {
                            Advance();
                            Advance();
                            nesting++;
                        } else if (Peek() == '*' && PeekNext() == '/') {
                            Advance();
                            Advance();
                            nesting--;
                        } else {
                            Advance();
                        }
                    }
                    if (nesting > 0) {
                        AddError("QS0001", "Unterminated block comment");
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

std::vector<Token> Lexer::Tokenize() {
    std::vector<Token> tokens;
    tokens.reserve(m_source.size() / 5);

    while (!IsAtEnd()) {
        SkipWhitespaceAndComments();
        if (IsAtEnd()) break;

        m_start = m_current;
        m_startColumn = m_column;

        Token t = ScanToken();
        tokens.push_back(t);
    }

    Token eof;
    eof.type = TokenType::EOF_TOKEN;
    eof.location = {m_filename, m_line, m_column, 0};
    tokens.push_back(eof);

    return tokens;
}

Token Lexer::ScanToken() {
    char c = Advance();

    if (std::isalpha(c) || c == '_') {
        return ScanIdentifierOrKeyword();
    }
    if (c == '@') {
        return ScanDecorator();
    }
    if (std::isdigit(c)) {
        return ScanNumber();
    }
    if (c == '"') {
        return ScanString();
    }

    Token token;
    token.location = {m_filename, m_line, m_startColumn, 1};

    switch (c) {
        case '(': token.type = TokenType::LPAREN; break;
        case ')': token.type = TokenType::RPAREN; break;
        case '{': token.type = TokenType::LBRACE; break;
        case '}': token.type = TokenType::RBRACE; break;
        case '[': token.type = TokenType::LBRACKET; break;
        case ']': token.type = TokenType::RBRACKET; break;
        case ';': token.type = TokenType::SEMICOLON; break;
        case ',': token.type = TokenType::COMMA; break;
        case ':': token.type = TokenType::COLON; break;
        case '~': token.type = TokenType::TILDE; break;
        case '^': token.type = TokenType::CARET; break;
        case '%': token.type = TokenType::PERCENT; break;

        case '.':
            token.type = TokenType::DOT;
            break;

        case '+':
            if (Match('+')) {
                token.type = TokenType::PLUS_PLUS;
                token.location.length = 2;
            } else if (Match('=')) {
                token.type = TokenType::PLUS_EQUAL;
                token.location.length = 2;
            } else {
                token.type = TokenType::PLUS;
            }
            break;

        case '-':
            if (Match('-')) {
                token.type = TokenType::MINUS_MINUS;
                token.location.length = 2;
            } else if (Match('>')) {
                token.type = TokenType::ARROW; // ->
                token.location.length = 2;
            } else if (Match('=')) {
                token.type = TokenType::MINUS_EQUAL;
                token.location.length = 2;
            } else {
                token.type = TokenType::MINUS;
            }
            break;

        case '*':
            token.type = Match('=') ? TokenType::STAR_EQUAL : TokenType::STAR;
            if (token.type == TokenType::STAR_EQUAL) token.location.length = 2;
            break;

        case '/':
            token.type = Match('=') ? TokenType::SLASH_EQUAL : TokenType::SLASH;
            if (token.type == TokenType::SLASH_EQUAL) token.location.length = 2;
            break;

        case '=':
            token.type = Match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL;
            if (token.type == TokenType::EQUAL_EQUAL) token.location.length = 2;
            break;

        case '!':
            token.type = Match('=') ? TokenType::BANG_EQUAL : TokenType::BANG;
            if (token.type == TokenType::BANG_EQUAL) token.location.length = 2;
            break;

        case '<':
            token.type = Match('=') ? TokenType::LESS_EQUAL : TokenType::LESS;
            if (token.type == TokenType::LESS_EQUAL) token.location.length = 2;
            break;

        case '>':
            token.type = Match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER;
            if (token.type == TokenType::GREATER_EQUAL) token.location.length = 2;
            break;

        case '&':
            token.type = Match('&') ? TokenType::AMP_AMP : TokenType::AMP;
            if (token.type == TokenType::AMP_AMP) token.location.length = 2;
            break;

        case '|':
            token.type = Match('|') ? TokenType::PIPE_PIPE : TokenType::PIPE;
            if (token.type == TokenType::PIPE_PIPE) token.location.length = 2;
            break;

        case '?':
            if (Match('?')) {
                token.type = TokenType::QUESTION_QUESTION;
                token.location.length = 2;
            } else if (Match('.')) {
                token.type = TokenType::DOT_QUESTION;
                token.location.length = 2;
            } else {
                token.type = TokenType::QUESTION;
            }
            break;

        default:
            token.type = TokenType::ERROR_TOKEN;
            AddError("QS0001", std::string("Unexpected character: '") + c + "'");
            break;
    }

    token.lexeme = m_source.substr(m_start, m_current - m_start);
    return token;
}

Token Lexer::ScanIdentifierOrKeyword() {
    while (std::isalnum(Peek()) || Peek() == '_') {
        Advance();
    }

    std::string text = m_source.substr(m_start, m_current - m_start);
    Token token;
    token.location = {m_filename, m_line, m_startColumn, static_cast<uint32_t>(text.size())};
    token.lexeme = text;

    auto it = s_keywords.find(text);
    if (it != s_keywords.end()) {
        token.type = it->second;
    } else {
        token.type = TokenType::IDENTIFIER;
    }
    return token;
}

Token Lexer::ScanDecorator() {
    // Already consumed '@'
    while (std::isalnum(Peek()) || Peek() == '_') {
        Advance();
    }

    std::string decoratorName = m_source.substr(m_start + 1, m_current - (m_start + 1));
    Token token;
    token.location = {m_filename, m_line, m_startColumn, static_cast<uint32_t>(m_current - m_start)};
    token.lexeme = m_source.substr(m_start, m_current - m_start);

    auto it = s_decorators.find(decoratorName);
    if (it != s_decorators.end()) {
        token.type = it->second;
    } else {
        token.type = TokenType::GENERIC_DECORATOR;
    }
    return token;
}

Token Lexer::ScanNumber() {
    bool isHex = false;
    bool isFloat = false;

    // Check for hex '0x' or '0X'
    if (m_source[m_start] == '0' && (Peek() == 'x' || Peek() == 'X')) {
        Advance(); // consume 'x'
        isHex = true;
        while (std::isxdigit(Peek()) || Peek() == '_') {
            Advance();
        }
    } else {
        while (std::isdigit(Peek()) || Peek() == '_') {
            Advance();
        }

        // Float fraction
        if (Peek() == '.' && std::isdigit(PeekNext())) {
            isFloat = true;
            Advance(); // consume '.'
            while (std::isdigit(Peek()) || Peek() == '_') {
                Advance();
            }
        }

        // Exponent 'e' or 'E'
        if (Peek() == 'e' || Peek() == 'E') {
            isFloat = true;
            Advance();
            if (Peek() == '+' || Peek() == '-') Advance();
            while (std::isdigit(Peek()) || Peek() == '_') {
                Advance();
            }
        }

        // Float suffix 'f' or 'F'
        if (Peek() == 'f' || Peek() == 'F') {
            isFloat = true;
            Advance();
        }
    }

    std::string raw = m_source.substr(m_start, m_current - m_start);
    // Strip underscores and trailing 'f'
    std::string cleaned;
    cleaned.reserve(raw.size());
    for (char ch : raw) {
        if (ch != '_' && ch != 'f' && ch != 'F') {
            cleaned.push_back(ch);
        }
    }

    Token token;
    token.location = {m_filename, m_line, m_startColumn, static_cast<uint32_t>(raw.size())};
    token.lexeme = raw;

    if (isFloat) {
        token.type = TokenType::FLOAT_LITERAL;
        token.floatValue = std::stod(cleaned);
    } else {
        token.type = TokenType::INT_LITERAL;
        token.intValue = isHex ? std::stoll(cleaned, nullptr, 16) : std::stoll(cleaned);
        token.floatValue = static_cast<double>(token.intValue);
    }

    return token;
}

Token Lexer::ScanString() {
    // Already consumed opening quote '"'
    std::string value;
    value.reserve(32);

    while (Peek() != '"' && !IsAtEnd()) {
        if (Peek() == '\n') {
            m_line++;
            m_column = 1;
        }

        char c = Advance();
        if (c == '\\') {
            // Escape sequences
            if (IsAtEnd()) break;
            char esc = Advance();
            switch (esc) {
                case 'n': value.push_back('\n'); break;
                case 't': value.push_back('\t'); break;
                case 'r': value.push_back('\r'); break;
                case '"': value.push_back('"'); break;
                case '\\': value.push_back('\\'); break;
                default: value.push_back(esc); break;
            }
        } else {
            value.push_back(c);
        }
    }

    if (IsAtEnd()) {
        AddError("QS0001", "Unterminated string literal");
        Token err;
        err.type = TokenType::ERROR_TOKEN;
        err.location = {m_filename, m_line, m_startColumn, static_cast<uint32_t>(m_current - m_start)};
        return err;
    }

    Advance(); // Closing quote '"'

    Token token;
    token.type = TokenType::STRING_LITERAL;
    token.location = {m_filename, m_line, m_startColumn, static_cast<uint32_t>(m_current - m_start)};
    token.lexeme = m_source.substr(m_start, m_current - m_start);
    token.stringValue = value;
    return token;
}

} // namespace qivforge::script

#include "qivforge/script/parser.h"
#include <algorithm>

namespace qivforge::script {

Parser::Parser(std::vector<Token> tokens)
    : m_tokens(std::move(tokens)) {
}

bool Parser::HasErrors() const noexcept {
    for (const auto& diag : m_diagnostics) {
        if (diag.severity == DiagnosticSeverity::ERROR) return true;
    }
    return false;
}

const Token& Parser::Peek() const noexcept {
    return m_tokens[m_current];
}

const Token& Parser::Previous() const noexcept {
    return m_tokens[m_current - 1];
}

bool Parser::IsAtEnd() const noexcept {
    return Peek().type == TokenType::EOF_TOKEN;
}

const Token& Parser::Advance() noexcept {
    if (!IsAtEnd()) m_current++;
    return Previous();
}

bool Parser::Check(TokenType type) const noexcept {
    if (IsAtEnd()) return false;
    return Peek().type == type;
}

bool Parser::Match(TokenType type) noexcept {
    if (Check(type)) {
        Advance();
        return true;
    }
    return false;
}

const Token& Parser::Consume(TokenType type, const std::string& code, const std::string& errorMessage) {
    if (Check(type)) return Advance();
    AddError(code, errorMessage, Peek().location);
    return Peek();
}

void Parser::AddError(const std::string& code, const std::string& message, const SourceLocation& loc) {
    Diagnostic diag;
    diag.severity = DiagnosticSeverity::ERROR;
    diag.code = code;
    diag.message = message;
    diag.location = loc;
    m_diagnostics.push_back(diag);
}

void Parser::Synchronize() {
    Advance();
    while (!IsAtEnd()) {
        if (Previous().type == TokenType::SEMICOLON) return;
        switch (Peek().type) {
            case TokenType::KW_CLASS:
            case TokenType::KW_FN:
            case TokenType::KW_VAR:
            case TokenType::KW_FOR:
            case TokenType::KW_IF:
            case TokenType::KW_WHILE:
            case TokenType::KW_RETURN:
            case TokenType::KW_PUB:
                return;
            default:
                break;
        }
        Advance();
    }
}

std::unique_ptr<ScriptFile> Parser::Parse() {
    auto file = std::make_unique<ScriptFile>();
    file->location = Peek().location;

    while (!IsAtEnd()) {
        if (Match(TokenType::KW_MODULE)) {
            ParseModule(*file);
        } else if (Match(TokenType::KW_IMPORT)) {
            ParseImport(*file);
        } else {
            auto decorators = ParseDecorators();
            if (Check(TokenType::KW_PUB) || Check(TokenType::KW_CLASS)) {
                file->classes.push_back(ParseClass(std::move(decorators)));
            } else if (!IsAtEnd()) {
                AddError("QS0001", "Expected class declaration", Peek().location);
                Synchronize();
            }
        }
    }

    return file;
}

void Parser::ParseModule(ScriptFile& file) {
    std::string mod;
    while (!Check(TokenType::SEMICOLON) && !IsAtEnd()) {
        mod += Advance().lexeme;
    }
    Consume(TokenType::SEMICOLON, "QS0001", "Expected ';' after module name");
    file.moduleName = mod;
}

void Parser::ParseImport(ScriptFile& file) {
    std::string imp;
    while (!Check(TokenType::SEMICOLON) && !IsAtEnd()) {
        imp += Advance().lexeme;
    }
    Consume(TokenType::SEMICOLON, "QS0001", "Expected ';' after import");
    file.imports.push_back(imp);
}

std::vector<AstDecorator> Parser::ParseDecorators() {
    std::vector<AstDecorator> list;
    while (Peek().IsDecorator()) {
        Token decToken = Advance();
        AstDecorator dec;
        dec.name = decToken.lexeme.substr(1); // Strip '@'
        dec.location = decToken.location;

        if (Match(TokenType::LPAREN)) {
            while (!Check(TokenType::RPAREN) && !IsAtEnd()) {
                if (Check(TokenType::STRING_LITERAL)) {
                    dec.stringArgs.push_back(Advance().stringValue);
                } else if (Check(TokenType::FLOAT_LITERAL) || Check(TokenType::INT_LITERAL)) {
                    dec.floatArgs.push_back(Advance().floatValue);
                } else if (Check(TokenType::IDENTIFIER)) {
                    dec.stringArgs.push_back(Advance().lexeme);
                } else {
                    Advance();
                }
                Match(TokenType::COMMA);
            }
            Consume(TokenType::RPAREN, "QS0001", "Expected ')' after decorator arguments");
        }
        list.push_back(dec);
    }
    return list;
}

ClassDecl Parser::ParseClass(std::vector<AstDecorator> decorators) {
    ClassDecl cls;
    cls.decorators = std::move(decorators);
    cls.isPub = Match(TokenType::KW_PUB);

    Consume(TokenType::KW_CLASS, "QS0001", "Expected 'class'");
    const Token& nameToken = Consume(TokenType::IDENTIFIER, "QS0001", "Expected class name");
    cls.name = nameToken.lexeme;
    cls.location = nameToken.location;

    if (Match(TokenType::KW_EXTENDS)) {
        cls.baseClass = Consume(TokenType::IDENTIFIER, "QS0001", "Expected base class name").lexeme;
    }

    Consume(TokenType::LBRACE, "QS0001", "Expected '{' before class body");

    while (!Check(TokenType::RBRACE) && !IsAtEnd()) {
        auto memberDecorators = ParseDecorators();

        bool isPub = Match(TokenType::KW_PUB);
        bool isPrivate = Match(TokenType::KW_PRIVATE);
        (void)isPrivate;
        bool isOverride = Match(TokenType::KW_OVERRIDE);
        bool isAsync = Match(TokenType::KW_ASYNC);

        if (Check(TokenType::KW_VAR) || Check(TokenType::KW_LET) || Check(TokenType::KW_CONST)) {
            cls.fields.push_back(ParseField(std::move(memberDecorators), isPub));
        } else if (Match(TokenType::KW_FN) || isOverride || isAsync) {
            cls.methods.push_back(ParseMethod(isPub, isOverride, isAsync));
        } else {
            AddError("QS0001", "Unexpected token in class body", Peek().location);
            Synchronize();
        }
    }

    Consume(TokenType::RBRACE, "QS0001", "Expected '}' after class body");
    return cls;
}

FieldDecl Parser::ParseField(std::vector<AstDecorator> decorators, bool isPub) {
    FieldDecl field;
    field.decorators = std::move(decorators);
    field.isPub = isPub;

    if (Match(TokenType::KW_VAR)) {
        field.isLet = false;
    } else if (Match(TokenType::KW_LET)) {
        field.isLet = true;
    } else if (Match(TokenType::KW_CONST)) {
        field.isConst = true;
    }

    const Token& nameToken = Consume(TokenType::IDENTIFIER, "QS0001", "Expected field name");
    field.name = nameToken.lexeme;
    field.location = nameToken.location;

    if (Match(TokenType::COLON)) {
        field.type = ParseType();
    }

    if (Match(TokenType::EQUAL)) {
        field.initializer = ParseExpression();
    }

    Consume(TokenType::SEMICOLON, "QS0001", "Expected ';' after field declaration");
    return field;
}

MethodDecl Parser::ParseMethod(bool isPub, bool isOverride, bool isAsync) {
    MethodDecl method;
    method.isPub = isPub;
    method.isOverride = isOverride;
    method.isAsync = isAsync;

    if (Check(TokenType::KW_FN)) {
        Advance();
    }

    const Token& nameToken = Consume(TokenType::IDENTIFIER, "QS0001", "Expected method name");
    method.name = nameToken.lexeme;
    method.location = nameToken.location;

    Consume(TokenType::LPAREN, "QS0001", "Expected '(' after method name");

    while (!Check(TokenType::RPAREN) && !IsAtEnd()) {
        Parameter param;
        param.name = Consume(TokenType::IDENTIFIER, "QS0001", "Expected parameter name").lexeme;
        Consume(TokenType::COLON, "QS0001", "Expected ':' after parameter name");
        param.type = ParseType();
        if (Match(TokenType::EQUAL)) {
            param.defaultValue = ParseExpression();
        }
        method.parameters.push_back(std::move(param));
        if (!Match(TokenType::COMMA)) break;
    }

    Consume(TokenType::RPAREN, "QS0001", "Expected ')' after parameters");

    if (Match(TokenType::ARROW)) {
        method.returnType = ParseType();
    }

    Consume(TokenType::LBRACE, "QS0001", "Expected '{' before method body");
    method.body = ParseBlock();
    return method;
}

QivType Parser::ParseType() {
    QivType type;
    const Token& typeToken = Consume(TokenType::IDENTIFIER, "QS0001", "Expected type name");
    type.name = typeToken.lexeme;

    if (type.name == "f32") type = QivType::F32();
    else if (type.name == "i32") type = QivType::I32();
    else if (type.name == "bool") type = QivType::Bool();
    else if (type.name == "string") type = QivType::String();
    else if (type.name == "void") type = QivType::Void();
    else if (type.name == "Vec3") type = QivType::Vec3();
    else if (type.name == "Quat") type = QivType::Quat();
    else if (type.name == "Color") type = QivType::Color();
    else {
        type.kind = DataTypeKind::ENGINE_REF;
    }

    // Generic types like Array<WheelCollider, 4> or List<T>
    if (Match(TokenType::LESS)) {
        if (type.name == "Array") {
            type.kind = DataTypeKind::FIXED_ARRAY;
            type.typeArguments.push_back(ParseType());
            Consume(TokenType::COMMA, "QS0001", "Expected ',' in Array<T, N>");
            if (Check(TokenType::INT_LITERAL)) {
                type.arraySize = static_cast<uint32_t>(Advance().intValue);
            }
        } else if (type.name == "List") {
            type.kind = DataTypeKind::GENERIC_LIST;
            type.typeArguments.push_back(ParseType());
        }
        Consume(TokenType::GREATER, "QS0001", "Expected '>' after generic type argument");
    }

    if (Match(TokenType::QUESTION)) {
        type.isNullable = true;
    }

    return type;
}

std::unique_ptr<BlockStmt> Parser::ParseBlock() {
    auto block = std::make_unique<BlockStmt>();
    block->location = Previous().location;

    while (!Check(TokenType::RBRACE) && !IsAtEnd()) {
        auto stmt = ParseStatement();
        if (stmt) {
            block->statements.push_back(std::move(stmt));
        }
    }

    Consume(TokenType::RBRACE, "QS0001", "Expected '}' after block");
    return block;
}

std::unique_ptr<Stmt> Parser::ParseStatement() {
    if (Check(TokenType::KW_VAR) || Check(TokenType::KW_LET)) {
        return ParseVarDecl();
    }
    if (Match(TokenType::KW_IF)) {
        return ParseIfStatement();
    }
    if (Match(TokenType::KW_WHILE)) {
        return ParseWhileStatement();
    }
    if (Match(TokenType::KW_FOR)) {
        return ParseForStatement();
    }
    if (Match(TokenType::KW_RETURN)) {
        return ParseReturnStatement();
    }
    if (Match(TokenType::KW_TRY)) {
        return ParseTryCatchStatement();
    }
    if (Match(TokenType::LBRACE)) {
        return ParseBlock();
    }

    return ParseExpressionStatement();
}

std::unique_ptr<Stmt> Parser::ParseVarDecl() {
    auto stmt = std::make_unique<VarDeclStmt>();
    stmt->location = Advance().location;
    stmt->isLet = (Previous().type == TokenType::KW_LET);

    stmt->name = Consume(TokenType::IDENTIFIER, "QS0001", "Expected variable name").lexeme;

    if (Match(TokenType::COLON)) {
        stmt->type = ParseType();
    }

    if (Match(TokenType::EQUAL)) {
        stmt->initializer = ParseExpression();
    }

    Consume(TokenType::SEMICOLON, "QS0001", "Expected ';' after variable declaration");
    return stmt;
}

std::unique_ptr<Stmt> Parser::ParseIfStatement() {
    auto stmt = std::make_unique<IfStmt>();
    stmt->location = Previous().location;

    Consume(TokenType::LPAREN, "QS0001", "Expected '(' after 'if'");
    stmt->condition = ParseExpression();
    Consume(TokenType::RPAREN, "QS0001", "Expected ')' after if condition");

    stmt->thenBranch = ParseStatement();
    if (Match(TokenType::KW_ELSE)) {
        stmt->elseBranch = ParseStatement();
    }

    return stmt;
}

std::unique_ptr<Stmt> Parser::ParseWhileStatement() {
    auto stmt = std::make_unique<WhileStmt>();
    stmt->location = Previous().location;

    Consume(TokenType::LPAREN, "QS0001", "Expected '(' after 'while'");
    stmt->condition = ParseExpression();
    Consume(TokenType::RPAREN, "QS0001", "Expected ')' after while condition");

    stmt->body = ParseStatement();
    return stmt;
}

std::unique_ptr<Stmt> Parser::ParseForStatement() {
    auto stmt = std::make_unique<ForStmt>();
    stmt->location = Previous().location;

    Consume(TokenType::LPAREN, "QS0001", "Expected '(' after 'for'");

    if (!Match(TokenType::SEMICOLON)) {
        stmt->initializer = ParseVarDecl(); // Consumes semicolon
    }

    if (!Check(TokenType::SEMICOLON)) {
        stmt->condition = ParseExpression();
    }
    Consume(TokenType::SEMICOLON, "QS0001", "Expected ';' after loop condition");

    if (!Check(TokenType::RPAREN)) {
        stmt->increment = ParseExpression();
    }
    Consume(TokenType::RPAREN, "QS0001", "Expected ')' after for clauses");

    stmt->body = ParseStatement();
    return stmt;
}

std::unique_ptr<Stmt> Parser::ParseReturnStatement() {
    auto stmt = std::make_unique<ReturnStmt>();
    stmt->location = Previous().location;

    if (!Check(TokenType::SEMICOLON)) {
        stmt->value = ParseExpression();
    }
    Consume(TokenType::SEMICOLON, "QS0001", "Expected ';' after return value");
    return stmt;
}

std::unique_ptr<Stmt> Parser::ParseTryCatchStatement() {
    Consume(TokenType::LBRACE, "QS0001", "Expected '{' after 'try'");
    auto tryBlock = ParseBlock();

    Consume(TokenType::KW_CATCH, "QS0001", "Expected 'catch' after try block");
    if (Match(TokenType::LPAREN)) {
        Advance(); // error identifier
        Consume(TokenType::RPAREN, "QS0001", "Expected ')' after catch parameter");
    }
    Consume(TokenType::LBRACE, "QS0001", "Expected '{' after catch");
    auto catchBlock = ParseBlock();

    return tryBlock;
}

std::unique_ptr<Stmt> Parser::ParseExpressionStatement() {
    auto stmt = std::make_unique<ExprStmt>();
    stmt->expression = ParseExpression();
    Consume(TokenType::SEMICOLON, "QS0001", "Expected ';' after expression");
    return stmt;
}

// Expressions
std::unique_ptr<Expr> Parser::ParseExpression() {
    return ParseAssignment();
}

std::unique_ptr<Expr> Parser::ParseAssignment() {
    auto expr = ParseLogicalOr();

    if (Match(TokenType::QUESTION)) {
        auto trueBranch = ParseExpression();
        Consume(TokenType::COLON, "QS0001", "Expected ':' in ternary expression");
        auto falseBranch = ParseExpression();
        auto ternary = std::make_unique<TernaryExpr>();
        ternary->condition = std::move(expr);
        ternary->trueBranch = std::move(trueBranch);
        ternary->falseBranch = std::move(falseBranch);
        return ternary;
    }

    if (Match(TokenType::EQUAL) || Match(TokenType::PLUS_EQUAL) ||
        Match(TokenType::MINUS_EQUAL) || Match(TokenType::STAR_EQUAL) || Match(TokenType::SLASH_EQUAL)) {
        TokenType op = Previous().type;
        auto val = ParseAssignment();
        auto assign = std::make_unique<AssignExpr>();
        assign->target = std::move(expr);
        assign->op = op;
        assign->value = std::move(val);
        return assign;
    }

    return expr;
}

std::unique_ptr<Expr> Parser::ParseLogicalOr() {
    auto expr = ParseLogicalAnd();
    while (Match(TokenType::PIPE_PIPE)) {
        auto binary = std::make_unique<BinaryExpr>();
        binary->op = TokenType::PIPE_PIPE;
        binary->left = std::move(expr);
        binary->right = ParseLogicalAnd();
        expr = std::move(binary);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::ParseLogicalAnd() {
    auto expr = ParseEquality();
    while (Match(TokenType::AMP_AMP)) {
        auto binary = std::make_unique<BinaryExpr>();
        binary->op = TokenType::AMP_AMP;
        binary->left = std::move(expr);
        binary->right = ParseEquality();
        expr = std::move(binary);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::ParseEquality() {
    auto expr = ParseComparison();
    while (Match(TokenType::EQUAL_EQUAL) || Match(TokenType::BANG_EQUAL)) {
        auto binary = std::make_unique<BinaryExpr>();
        binary->op = Previous().type;
        binary->left = std::move(expr);
        binary->right = ParseComparison();
        expr = std::move(binary);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::ParseComparison() {
    auto expr = ParseTerm();
    while (Match(TokenType::LESS) || Match(TokenType::LESS_EQUAL) ||
           Match(TokenType::GREATER) || Match(TokenType::GREATER_EQUAL)) {
        auto binary = std::make_unique<BinaryExpr>();
        binary->op = Previous().type;
        binary->left = std::move(expr);
        binary->right = ParseTerm();
        expr = std::move(binary);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::ParseTerm() {
    auto expr = ParseFactor();
    while (Match(TokenType::PLUS) || Match(TokenType::MINUS)) {
        auto binary = std::make_unique<BinaryExpr>();
        binary->op = Previous().type;
        binary->left = std::move(expr);
        binary->right = ParseFactor();
        expr = std::move(binary);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::ParseFactor() {
    auto expr = ParseUnary();
    while (Match(TokenType::STAR) || Match(TokenType::SLASH) || Match(TokenType::PERCENT)) {
        auto binary = std::make_unique<BinaryExpr>();
        binary->op = Previous().type;
        binary->left = std::move(expr);
        binary->right = ParseUnary();
        expr = std::move(binary);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::ParseUnary() {
    if (Match(TokenType::KW_AWAIT)) {
        return ParseUnary();
    }
    if (Match(TokenType::PLUS_PLUS) || Match(TokenType::MINUS_MINUS) ||
        Match(TokenType::BANG) || Match(TokenType::MINUS)) {
        auto unary = std::make_unique<UnaryExpr>();
        unary->op = Previous().type;
        unary->operand = ParseUnary();
        return unary;
    }
    return ParseCallOrMember();
}

std::unique_ptr<Expr> Parser::ParseCallOrMember() {
    auto expr = ParsePrimary();

    while (true) {
        if (Match(TokenType::LPAREN)) {
            // Function or method call
            auto call = std::make_unique<CallExpr>();
            call->callee = std::move(expr);
            while (!Check(TokenType::RPAREN) && !IsAtEnd()) {
                call->arguments.push_back(ParseExpression());
                if (!Match(TokenType::COMMA)) break;
            }
            Consume(TokenType::RPAREN, "QS0001", "Expected ')' after arguments");
            expr = std::move(call);
        } else if (Match(TokenType::DOT) || Match(TokenType::DOT_QUESTION)) {
            // Member access (or optional chaining ?.)
            bool isOptional = (Previous().type == TokenType::DOT_QUESTION);
            const Token& member = Consume(TokenType::IDENTIFIER, "QS0001", "Expected property/method name after '.'");
            auto memberAccess = std::make_unique<MemberAccessExpr>();
            memberAccess->object = std::move(expr);
            memberAccess->memberName = member.lexeme;
            memberAccess->isOptional = isOptional;

            if (Match(TokenType::LESS)) {
                ParseType();
                Consume(TokenType::GREATER, "QS0001", "Expected '>' after generic type");
            }

            expr = std::move(memberAccess);
        } else if (Match(TokenType::LBRACKET)) {
            // Indexing expr[i]
            auto indexExpr = std::make_unique<IndexExpr>();
            indexExpr->container = std::move(expr);
            indexExpr->index = ParseExpression();
            Consume(TokenType::RBRACKET, "QS0001", "Expected ']' after index");
            expr = std::move(indexExpr);
        } else if (Match(TokenType::PLUS_PLUS) || Match(TokenType::MINUS_MINUS)) {
            auto unary = std::make_unique<UnaryExpr>();
            unary->op = Previous().type;
            unary->operand = std::move(expr);
            expr = std::move(unary);
        } else {
            break;
        }
    }

    return expr;
}

std::unique_ptr<Expr> Parser::ParsePrimary() {
    if (Match(TokenType::KW_TRUE)) {
        auto lit = std::make_unique<LiteralExpr>();
        lit->literalType = TokenType::BOOL_LITERAL;
        lit->boolVal = true;
        lit->location = Previous().location;
        return lit;
    }
    if (Match(TokenType::KW_FALSE)) {
        auto lit = std::make_unique<LiteralExpr>();
        lit->literalType = TokenType::BOOL_LITERAL;
        lit->boolVal = false;
        lit->location = Previous().location;
        return lit;
    }
    if (Match(TokenType::KW_NULL)) {
        auto lit = std::make_unique<LiteralExpr>();
        lit->literalType = TokenType::KW_NULL;
        lit->location = Previous().location;
        return lit;
    }
    if (Match(TokenType::INT_LITERAL)) {
        auto lit = std::make_unique<LiteralExpr>();
        lit->literalType = TokenType::INT_LITERAL;
        lit->intVal = Previous().intValue;
        lit->floatVal = Previous().floatValue;
        lit->location = Previous().location;
        return lit;
    }
    if (Match(TokenType::FLOAT_LITERAL)) {
        auto lit = std::make_unique<LiteralExpr>();
        lit->literalType = TokenType::FLOAT_LITERAL;
        lit->floatVal = Previous().floatValue;
        lit->location = Previous().location;
        return lit;
    }
    if (Match(TokenType::STRING_LITERAL)) {
        auto lit = std::make_unique<LiteralExpr>();
        lit->literalType = TokenType::STRING_LITERAL;
        lit->strVal = Previous().stringValue;
        lit->location = Previous().location;
        return lit;
    }
    if (Match(TokenType::IDENTIFIER) || Match(TokenType::KW_SELF) || Match(TokenType::KW_SUPER)) {
        auto id = std::make_unique<IdentifierExpr>();
        id->name = Previous().lexeme;
        id->location = Previous().location;
        return id;
    }
    if (Match(TokenType::LPAREN)) {
        auto expr = ParseExpression();
        Consume(TokenType::RPAREN, "QS0001", "Expected ')' after expression");
        return expr;
    }

    AddError("QS0001", std::string("Unexpected token: '") + Peek().lexeme + "'", Peek().location);
    Advance();
    return std::make_unique<LiteralExpr>();
}

std::vector<ExportedFieldMetadata> Parser::ExtractExportedFields(const ClassDecl& classDecl) {
    std::vector<ExportedFieldMetadata> result;

    for (const auto& field : classDecl.fields) {
        if (!field.HasDecorator("export")) continue;

        ExportedFieldMetadata meta;
        meta.name = field.name;
        meta.typeName = field.type.name;

        for (const auto& dec : field.decorators) {
            if (dec.name == "range" && dec.floatArgs.size() >= 2) {
                meta.hasRange = true;
                meta.rangeMin = dec.floatArgs[0];
                meta.rangeMax = dec.floatArgs[1];
            } else if (dec.name == "step" && !dec.floatArgs.empty()) {
                meta.step = dec.floatArgs[0];
            } else if (dec.name == "header" && !dec.stringArgs.empty()) {
                meta.header = dec.stringArgs[0];
            } else if (dec.name == "tooltip" && !dec.stringArgs.empty()) {
                meta.tooltip = dec.stringArgs[0];
            } else if (dec.name == "readonly") {
                meta.isReadonly = true;
            }
        }

        if (field.initializer) {
            if (auto* lit = dynamic_cast<LiteralExpr*>(field.initializer.get())) {
                meta.defaultValue = lit->floatVal;
            }
        }

        result.push_back(meta);
    }

    return result;
}

} // namespace qivforge::script

#pragma once

#include "qivforge/script/token.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace qivforge::script {

enum class DataTypeKind : uint8_t {
    PRIMITIVE = 0,
    VALUE_STRUCT,    // Vec2, Vec3, Vec4, Quat, Color, Transform, Ray, RayHit
    ENGINE_REF,      // Actor, Component, RigidBody, WheelCollider, AudioEmitter, etc.
    GENERIC_LIST,    // List<T>
    FIXED_ARRAY,     // Array<T, N>
    GENERIC_MAP,     // Map<K, V>
    USER_CLASS,
    VOID_TYPE
};

struct QivType {
    DataTypeKind kind = DataTypeKind::PRIMITIVE;
    std::string name = "void";
    bool isNullable = false;
    std::vector<QivType> typeArguments; // For List<T> or Array<T, N>
    uint32_t arraySize = 0;

    static QivType Void() noexcept { return {DataTypeKind::VOID_TYPE, "void", false, {}, 0}; }
    static QivType I32() noexcept { return {DataTypeKind::PRIMITIVE, "i32", false, {}, 0}; }
    static QivType I64() noexcept { return {DataTypeKind::PRIMITIVE, "i64", false, {}, 0}; }
    static QivType F32() noexcept { return {DataTypeKind::PRIMITIVE, "f32", false, {}, 0}; }
    static QivType F64() noexcept { return {DataTypeKind::PRIMITIVE, "f64", false, {}, 0}; }
    static QivType Bool() noexcept { return {DataTypeKind::PRIMITIVE, "bool", false, {}, 0}; }
    static QivType String() noexcept { return {DataTypeKind::PRIMITIVE, "string", false, {}, 0}; }
    static QivType Vec3() noexcept { return {DataTypeKind::VALUE_STRUCT, "Vec3", false, {}, 0}; }
    static QivType Quat() noexcept { return {DataTypeKind::VALUE_STRUCT, "Quat", false, {}, 0}; }
    static QivType Color() noexcept { return {DataTypeKind::VALUE_STRUCT, "Color", false, {}, 0}; }
};

struct AstDecorator {
    std::string name; // "component", "export", "range", "header", "tooltip", "require"
    std::vector<std::string> stringArgs;
    std::vector<double> floatArgs;
    SourceLocation location;
};

// Expressions
class Expr {
public:
    virtual ~Expr() = default;
    SourceLocation location;
};

class LiteralExpr : public Expr {
public:
    TokenType literalType = TokenType::INT_LITERAL;
    int64_t intVal = 0;
    double floatVal = 0.0;
    std::string strVal;
    bool boolVal = false;
};

class IdentifierExpr : public Expr {
public:
    std::string name;
};

class BinaryExpr : public Expr {
public:
    TokenType op = TokenType::PLUS;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
};

class UnaryExpr : public Expr {
public:
    TokenType op = TokenType::MINUS;
    std::unique_ptr<Expr> operand;
};

class CallExpr : public Expr {
public:
    std::unique_ptr<Expr> callee;
    std::vector<std::unique_ptr<Expr>> arguments;
};

class MemberAccessExpr : public Expr {
public:
    std::unique_ptr<Expr> object;
    std::string memberName;
    bool isOptional = false; // ?.
};

class IndexExpr : public Expr {
public:
    std::unique_ptr<Expr> container;
    std::unique_ptr<Expr> index;
};

class AssignExpr : public Expr {
public:
    std::unique_ptr<Expr> target;
    TokenType op = TokenType::EQUAL; // =, +=, -=, etc.
    std::unique_ptr<Expr> value;
};

class TernaryExpr : public Expr {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> trueBranch;
    std::unique_ptr<Expr> falseBranch;
};

// Statements
class Stmt {
public:
    virtual ~Stmt() = default;
    SourceLocation location;
};

class BlockStmt : public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> statements;
};

class ExprStmt : public Stmt {
public:
    std::unique_ptr<Expr> expression;
};

class VarDeclStmt : public Stmt {
public:
    bool isLet = false;
    bool isConst = false;
    std::string name;
    QivType type;
    std::unique_ptr<Expr> initializer;
};

class IfStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;
};

class WhileStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;
};

class ForStmt : public Stmt {
public:
    std::unique_ptr<Stmt> initializer;
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> increment;
    std::unique_ptr<Stmt> body;
};

class ReturnStmt : public Stmt {
public:
    std::unique_ptr<Expr> value;
};

// Class Members
struct Parameter {
    std::string name;
    QivType type;
    std::unique_ptr<Expr> defaultValue;
};

class MethodDecl {
public:
    bool isPub = false;
    bool isOverride = false;
    bool isAsync = false;
    bool isStatic = false;
    std::string name;
    QivType returnType = QivType::Void();
    std::vector<Parameter> parameters;
    std::unique_ptr<BlockStmt> body;
    SourceLocation location;
};

class FieldDecl {
public:
    std::vector<AstDecorator> decorators;
    bool isPub = false;
    bool isLet = false;
    bool isConst = false;
    std::string name;
    QivType type;
    std::unique_ptr<Expr> initializer;
    SourceLocation location;

    [[nodiscard]] bool HasDecorator(const std::string& decName) const noexcept {
        for (const auto& d : decorators) {
            if (d.name == decName) return true;
        }
        return false;
    }
};

class ClassDecl {
public:
    std::vector<AstDecorator> decorators;
    bool isPub = true;
    std::string name;
    std::string baseClass;
    std::vector<std::string> interfaces;
    std::vector<FieldDecl> fields;
    std::vector<MethodDecl> methods;
    SourceLocation location;

    [[nodiscard]] const MethodDecl* FindMethod(const std::string& methodName) const noexcept {
        for (const auto& m : methods) {
            if (m.name == methodName) return &m;
        }
        return nullptr;
    }
};

struct ExportedFieldMetadata {
    std::string name;
    std::string typeName;
    double rangeMin = 0.0;
    double rangeMax = 0.0;
    double step = 0.0;
    std::string header;
    std::string tooltip;
    bool hasRange = false;
    bool isReadonly = false;
    double defaultValue = 0.0;
};

class ScriptFile {
public:
    std::string moduleName;
    std::vector<std::string> imports;
    std::vector<ClassDecl> classes;
    SourceLocation location;
};

} // namespace qivforge::script

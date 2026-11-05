#include "qivforge/script/compiler.h"
#include "qivforge/script/parser.h"
#include <iostream>

namespace qivforge::script {

Compiler::Compiler(const ScriptFile& ast)
    : m_ast(ast) {
}

bool Compiler::HasErrors() const noexcept {
    for (const auto& diag : m_diagnostics) {
        if (diag.severity == DiagnosticSeverity::ERROR) return true;
    }
    return false;
}

void Compiler::AddError(const std::string& code, const std::string& message, const SourceLocation& loc) {
    Diagnostic diag;
    diag.severity = DiagnosticSeverity::ERROR;
    diag.code = code;
    diag.message = message;
    diag.location = loc;
    m_diagnostics.push_back(diag);
}

std::unique_ptr<BytecodeModule> Compiler::Compile() {
    auto module = std::make_unique<BytecodeModule>();
    module->moduleName = m_ast.moduleName;

    for (const auto& cls : m_ast.classes) {
        CompileClass(cls, *module);
    }

    return module;
}

void Compiler::CompileClass(const ClassDecl& cls, BytecodeModule& module) {
    module.className = cls.name;
    module.exportedFields = Parser::ExtractExportedFields(cls);

    // Semantic check: validate decorators per Doc 08 §5 & §14.1
    for (const auto& field : cls.fields) {
        bool hasRange = field.HasDecorator("range");
        bool isNumeric = (field.type.name == "f32" || field.type.name == "f64" ||
                          field.type.name == "i32" || field.type.name == "i64");

        if (hasRange && !isNumeric) {
            AddError("QS0031", "@range decorator is only valid on numeric fields", field.location);
        }
    }

    // Compile each method
    for (const auto& method : cls.methods) {
        module.functions.push_back(CompileMethod(cls, method));
    }
}

uint8_t Compiler::AllocateRegister() {
    if (m_nextReg >= 250) {
        // Safe cap
        return 250;
    }
    return m_nextReg++;
}

uint8_t Compiler::GetVariableRegister(const std::string& name) {
    if (name == "self") return 0;
    auto it = m_locals.find(name);
    if (it != m_locals.end()) {
        return it->second;
    }
    uint8_t reg = AllocateRegister();
    m_locals[name] = reg;
    return reg;
}

CompiledFunction Compiler::CompileMethod(const ClassDecl& cls, const MethodDecl& method) {
    (void)cls;
    CompiledFunction fn;
    fn.name = method.name;
    fn.numParameters = static_cast<uint8_t>(method.parameters.size());

    m_locals.clear();
    m_locals["self"] = 0; // r0 is self
    m_nextReg = 1;

    // Allocate parameters sequentially into r1, r2, ...
    for (const auto& param : method.parameters) {
        uint8_t reg = AllocateRegister();
        m_locals[param.name] = reg;
    }

    if (method.body) {
        CompileBlock(*method.body, fn);
    }

    // Emit final return void if last instruction is not RET
    if (fn.code.empty() || DecodeOp(fn.code.back()) != Opcode::RET) {
        fn.code.push_back(EncodeABC(Opcode::RET_VOID, 0, 0, 0));
        fn.lines.push_back(method.location.line);
    }

    fn.numRegisters = m_nextReg;
    return fn;
}

void Compiler::CompileBlock(const BlockStmt& block, CompiledFunction& fn) {
    for (const auto& stmt : block.statements) {
        CompileStmt(*stmt, fn);
    }
}

void Compiler::CompileStmt(const Stmt& stmt, CompiledFunction& fn) {
    if (auto* varDecl = dynamic_cast<const VarDeclStmt*>(&stmt)) {
        uint8_t reg = GetVariableRegister(varDecl->name);
        if (varDecl->initializer) {
            uint8_t initReg = CompileExpr(*varDecl->initializer, fn);
            fn.code.push_back(EncodeABC(Opcode::MOV, reg, initReg, 0));
            fn.lines.push_back(varDecl->location.line);
        } else {
            fn.code.push_back(EncodeABC(Opcode::LOADNULL, reg, 0, 0));
            fn.lines.push_back(varDecl->location.line);
        }
    } else if (auto* exprStmt = dynamic_cast<const ExprStmt*>(&stmt)) {
        if (exprStmt->expression) {
            CompileExpr(*exprStmt->expression, fn);
        }
    } else if (auto* ifStmt = dynamic_cast<const IfStmt*>(&stmt)) {
        uint8_t condReg = CompileExpr(*ifStmt->condition, fn);

        // Emit JMPF placeholder
        size_t jmpfIndex = fn.code.size();
        fn.code.push_back(EncodeAImm(Opcode::JMPF, condReg, 0));
        fn.lines.push_back(ifStmt->location.line);

        if (ifStmt->thenBranch) {
            CompileStmt(*ifStmt->thenBranch, fn);
        }

        if (ifStmt->elseBranch) {
            // Emit JMP placeholder over else branch
            size_t jmpIndex = fn.code.size();
            fn.code.push_back(EncodeAImm(Opcode::JMP, 0, 0));
            fn.lines.push_back(ifStmt->location.line);

            // Backpatch JMPF to start of else
            int16_t offsetToElse = static_cast<int16_t>(fn.code.size() - jmpfIndex - 1);
            fn.code[jmpfIndex] = EncodeAImm(Opcode::JMPF, condReg, static_cast<uint16_t>(offsetToElse));

            CompileStmt(*ifStmt->elseBranch, fn);

            // Backpatch JMP to end of else
            int16_t offsetToEnd = static_cast<int16_t>(fn.code.size() - jmpIndex - 1);
            fn.code[jmpIndex] = EncodeAImm(Opcode::JMP, 0, static_cast<uint16_t>(offsetToEnd));
        } else {
            // Backpatch JMPF to current end
            int16_t offsetToEnd = static_cast<int16_t>(fn.code.size() - jmpfIndex - 1);
            fn.code[jmpfIndex] = EncodeAImm(Opcode::JMPF, condReg, static_cast<uint16_t>(offsetToEnd));
        }
    } else if (auto* whileStmt = dynamic_cast<const WhileStmt*>(&stmt)) {
        size_t loopStart = fn.code.size();
        uint8_t condReg = CompileExpr(*whileStmt->condition, fn);

        size_t jmpfIndex = fn.code.size();
        fn.code.push_back(EncodeAImm(Opcode::JMPF, condReg, 0));
        fn.lines.push_back(whileStmt->location.line);

        if (whileStmt->body) {
            CompileStmt(*whileStmt->body, fn);
        }

        // Jump back to loop start (negative offset)
        int16_t loopOffset = static_cast<int16_t>(loopStart - fn.code.size() - 1);
        fn.code.push_back(EncodeAImm(Opcode::JMP, 0, static_cast<uint16_t>(loopOffset)));
        fn.lines.push_back(whileStmt->location.line);

        // Backpatch JMPF to exit
        int16_t exitOffset = static_cast<int16_t>(fn.code.size() - jmpfIndex - 1);
        fn.code[jmpfIndex] = EncodeAImm(Opcode::JMPF, condReg, static_cast<uint16_t>(exitOffset));
    } else if (auto* forStmt = dynamic_cast<const ForStmt*>(&stmt)) {
        if (forStmt->initializer) {
            CompileStmt(*forStmt->initializer, fn);
        }

        size_t loopStart = fn.code.size();
        uint8_t condReg = 0;
        size_t jmpfIndex = 0;

        if (forStmt->condition) {
            condReg = CompileExpr(*forStmt->condition, fn);
            jmpfIndex = fn.code.size();
            fn.code.push_back(EncodeAImm(Opcode::JMPF, condReg, 0));
            fn.lines.push_back(forStmt->location.line);
        }

        if (forStmt->body) {
            CompileStmt(*forStmt->body, fn);
        }

        if (forStmt->increment) {
            CompileExpr(*forStmt->increment, fn);
        }

        int16_t loopOffset = static_cast<int16_t>(loopStart - fn.code.size() - 1);
        fn.code.push_back(EncodeAImm(Opcode::JMP, 0, static_cast<uint16_t>(loopOffset)));
        fn.lines.push_back(forStmt->location.line);

        if (forStmt->condition) {
            int16_t exitOffset = static_cast<int16_t>(fn.code.size() - jmpfIndex - 1);
            fn.code[jmpfIndex] = EncodeAImm(Opcode::JMPF, condReg, static_cast<uint16_t>(exitOffset));
        }
    } else if (auto* retStmt = dynamic_cast<const ReturnStmt*>(&stmt)) {
        if (retStmt->value) {
            uint8_t valReg = CompileExpr(*retStmt->value, fn);
            fn.code.push_back(EncodeABC(Opcode::RET, valReg, 0, 0));
        } else {
            fn.code.push_back(EncodeABC(Opcode::RET_VOID, 0, 0, 0));
        }
        fn.lines.push_back(retStmt->location.line);
    } else if (auto* blockStmt = dynamic_cast<const BlockStmt*>(&stmt)) {
        CompileBlock(*blockStmt, fn);
    }
}

uint8_t Compiler::CompileExpr(const Expr& expr, CompiledFunction& fn) {
    uint8_t destReg = AllocateRegister();

    if (auto* lit = dynamic_cast<const LiteralExpr*>(&expr)) {
        if (lit->literalType == TokenType::INT_LITERAL) {
            uint16_t constIdx = fn.AddConstant(Value::Int(lit->intVal));
            fn.code.push_back(EncodeAImm(Opcode::LOADK, destReg, constIdx));
        } else if (lit->literalType == TokenType::FLOAT_LITERAL) {
            uint16_t constIdx = fn.AddConstant(Value::Float(lit->floatVal));
            fn.code.push_back(EncodeAImm(Opcode::LOADK, destReg, constIdx));
        } else if (lit->literalType == TokenType::STRING_LITERAL) {
            uint16_t constIdx = fn.AddConstant(Value::String(lit->strVal));
            fn.code.push_back(EncodeAImm(Opcode::LOADK, destReg, constIdx));
        } else if (lit->literalType == TokenType::BOOL_LITERAL) {
            fn.code.push_back(EncodeABC(lit->boolVal ? Opcode::LOADTRUE : Opcode::LOADFALSE, destReg, 0, 0));
        } else if (lit->literalType == TokenType::KW_NULL) {
            fn.code.push_back(EncodeABC(Opcode::LOADNULL, destReg, 0, 0));
        }
        fn.lines.push_back(lit->location.line);
        return destReg;
    }

    if (auto* id = dynamic_cast<const IdentifierExpr*>(&expr)) {
        uint8_t varReg = GetVariableRegister(id->name);
        fn.code.push_back(EncodeABC(Opcode::MOV, destReg, varReg, 0));
        fn.lines.push_back(id->location.line);
        return destReg;
    }

    if (auto* binary = dynamic_cast<const BinaryExpr*>(&expr)) {
        uint8_t rLeft = CompileExpr(*binary->left, fn);
        uint8_t rRight = CompileExpr(*binary->right, fn);

        Opcode op = Opcode::ADDF;
        switch (binary->op) {
            case TokenType::PLUS: op = Opcode::ADDF; break;
            case TokenType::MINUS: op = Opcode::SUBF; break;
            case TokenType::STAR: op = Opcode::MULF; break;
            case TokenType::SLASH: op = Opcode::DIVF; break;
            case TokenType::EQUAL_EQUAL: op = Opcode::EQ; break;
            case TokenType::BANG_EQUAL: op = Opcode::NE; break;
            case TokenType::LESS: op = Opcode::LT; break;
            case TokenType::LESS_EQUAL: op = Opcode::LE; break;
            case TokenType::GREATER: op = Opcode::GT; break;
            case TokenType::GREATER_EQUAL: op = Opcode::GE; break;
            default: break;
        }

        fn.code.push_back(EncodeABC(op, destReg, rLeft, rRight));
        fn.lines.push_back(binary->location.line);
        return destReg;
    }

    if (auto* unary = dynamic_cast<const UnaryExpr*>(&expr)) {
        if (unary->op == TokenType::PLUS_PLUS || unary->op == TokenType::MINUS_MINUS) {
            if (auto* id = dynamic_cast<const IdentifierExpr*>(unary->operand.get())) {
                uint8_t varReg = GetVariableRegister(id->name);
                uint8_t oneReg = AllocateRegister();
                fn.code.push_back(EncodeAImm(Opcode::LOADINT, oneReg, 1));
                if (unary->op == TokenType::PLUS_PLUS) {
                    fn.code.push_back(EncodeABC(Opcode::ADDI, varReg, varReg, oneReg));
                } else {
                    fn.code.push_back(EncodeABC(Opcode::SUBI, varReg, varReg, oneReg));
                }
                fn.code.push_back(EncodeABC(Opcode::MOV, destReg, varReg, 0));
                fn.lines.push_back(unary->location.line);
                return destReg;
            }
        }
        uint8_t rOperand = CompileExpr(*unary->operand, fn);
        if (unary->op == TokenType::MINUS) {
            fn.code.push_back(EncodeABC(Opcode::NEGF, destReg, rOperand, 0));
        } else if (unary->op == TokenType::BANG) {
            fn.code.push_back(EncodeABC(Opcode::EQ, destReg, rOperand, 0)); // cmp with false
        }
        fn.lines.push_back(unary->location.line);
        return destReg;
    }

    if (auto* assign = dynamic_cast<const AssignExpr*>(&expr)) {
        uint8_t valReg = CompileExpr(*assign->value, fn);
        if (auto* idTarget = dynamic_cast<const IdentifierExpr*>(assign->target.get())) {
            uint8_t targetReg = GetVariableRegister(idTarget->name);
            fn.code.push_back(EncodeABC(Opcode::MOV, targetReg, valReg, 0));
            fn.code.push_back(EncodeABC(Opcode::MOV, destReg, targetReg, 0));
        } else if (auto* memTarget = dynamic_cast<const MemberAccessExpr*>(assign->target.get())) {
            uint8_t objReg = CompileExpr(*memTarget->object, fn);
            uint16_t nameIdx = fn.AddConstant(Value::String(memTarget->memberName));
            fn.code.push_back(EncodeABC(Opcode::SETFIELD, objReg, valReg, static_cast<uint8_t>(nameIdx)));
            fn.code.push_back(EncodeABC(Opcode::MOV, destReg, valReg, 0));
        }
        fn.lines.push_back(assign->location.line);
        return destReg;
    }

    if (auto* call = dynamic_cast<const CallExpr*>(&expr)) {
        // Special case: Vec3 constructor Vec3(x, y, z)
        if (auto* calleeId = dynamic_cast<const IdentifierExpr*>(call->callee.get())) {
            if (calleeId->name == "Vec3" && call->arguments.size() == 3) {
                uint8_t rx = CompileExpr(*call->arguments[0], fn);
                uint8_t ry = CompileExpr(*call->arguments[1], fn);
                uint8_t rz = CompileExpr(*call->arguments[2], fn);
                uint16_t constIdx = fn.AddConstant(Value::String("Vec3"));
                fn.code.push_back(EncodeAImm(Opcode::LOADK, destReg, constIdx));
                fn.code.push_back(EncodeABC(Opcode::CALLNATIVE, destReg, rx, 3));
                return destReg;
            }
        }

        // Native API Calls (TouchHUD.getAxis, Time.time, Math.abs, etc.)
        if (auto* mem = dynamic_cast<const MemberAccessExpr*>(call->callee.get())) {
            std::string fullApiName;
            if (auto* baseId = dynamic_cast<const IdentifierExpr*>(mem->object.get())) {
                fullApiName = baseId->name + "." + mem->memberName;
            }

            uint8_t firstArgReg = 0;
            if (!call->arguments.empty()) {
                firstArgReg = CompileExpr(*call->arguments[0], fn);
                for (size_t i = 1; i < call->arguments.size(); ++i) {
                    CompileExpr(*call->arguments[i], fn);
                }
            }

            uint16_t nativeNameIdx = fn.AddConstant(Value::String(fullApiName.empty() ? mem->memberName : fullApiName));
            fn.code.push_back(EncodeAImm(Opcode::LOADK, destReg, nativeNameIdx));
            fn.code.push_back(EncodeABC(Opcode::CALLNATIVE, destReg, firstArgReg, static_cast<uint8_t>(call->arguments.size())));
            fn.lines.push_back(call->location.line);
            return destReg;
        }

        fn.code.push_back(EncodeABC(Opcode::LOADNULL, destReg, 0, 0));
        return destReg;
    }

    if (auto* mem = dynamic_cast<const MemberAccessExpr*>(&expr)) {
        uint8_t objReg = CompileExpr(*mem->object, fn);
        uint16_t nameIdx = fn.AddConstant(Value::String(mem->memberName));
        fn.code.push_back(EncodeABC(Opcode::GETFIELD, destReg, objReg, static_cast<uint8_t>(nameIdx)));
        fn.lines.push_back(mem->location.line);
        return destReg;
    }

    if (auto* idx = dynamic_cast<const IndexExpr*>(&expr)) {
        uint8_t contReg = CompileExpr(*idx->container, fn);
        uint8_t iReg = CompileExpr(*idx->index, fn);
        fn.code.push_back(EncodeABC(Opcode::LISTGET, destReg, contReg, iReg));
        fn.lines.push_back(idx->location.line);
        return destReg;
    }

    if (auto* ternary = dynamic_cast<const TernaryExpr*>(&expr)) {
        uint8_t condReg = CompileExpr(*ternary->condition, fn);
        size_t jmpFalseIdx = fn.code.size();
        fn.code.push_back(EncodeAImm(Opcode::JMPF, condReg, 0));

        uint8_t trueReg = CompileExpr(*ternary->trueBranch, fn);
        fn.code.push_back(EncodeABC(Opcode::MOV, destReg, trueReg, 0));
        size_t jmpEndIdx = fn.code.size();
        fn.code.push_back(EncodeAImm(Opcode::JMP, 0, 0));

        fn.code[jmpFalseIdx] = EncodeAImm(Opcode::JMPF, condReg, static_cast<uint16_t>(fn.code.size() - jmpFalseIdx - 1));

        uint8_t falseReg = CompileExpr(*ternary->falseBranch, fn);
        fn.code.push_back(EncodeABC(Opcode::MOV, destReg, falseReg, 0));

        fn.code[jmpEndIdx] = EncodeAImm(Opcode::JMP, 0, static_cast<uint16_t>(fn.code.size() - jmpEndIdx - 1));
        return destReg;
    }

    fn.code.push_back(EncodeABC(Opcode::LOADNULL, destReg, 0, 0));
    return destReg;
}

} // namespace qivforge::script

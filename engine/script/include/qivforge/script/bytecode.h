#pragma once

#include "qivforge/script/token.h"
#include "qivforge/script/ast.h"
#include <cstdint>
#include <vector>
#include <string>
#include <array>
#include <cmath>

namespace qivforge::script {

enum class Opcode : uint8_t {
    NOP = 0,

    // Move & Load
    MOV,
    LOADK,
    LOADNULL,
    LOADTRUE,
    LOADFALSE,
    LOADINT,

    // Integer Math
    ADDI,
    SUBI,
    MULI,
    DIVI,
    MODI,
    NEGI,

    // Float Math
    ADDF,
    SUBF,
    MULF,
    DIVF,
    NEGF,

    // Vector3 Math (Doc 08 §13.2)
    V3ADD,
    V3SUB,
    V3MULS,
    V3DOT,
    V3CROSS,
    V3LEN,
    V3NORM,

    // Compare & Jump
    EQ,
    NE,
    LT,
    LE,
    GT,
    GE,
    JMP,
    JMPT,
    JMPF,

    // Calls & Returns
    CALL,
    CALLNATIVE,
    RET,
    RET_VOID,

    // Object fields
    GETFIELD,
    SETFIELD,
    NEWOBJ,

    // Collections
    NEWLIST,
    LISTGET,
    LISTSET,
    LISTADD,
    LEN,

    // Debug
    LINE
};

// 32-bit Instruction Word: [Opcode: 8][rA: 8][rB: 8][rC: 8] or [Opcode: 8][rA: 8][imm16: 16]
inline uint32_t EncodeABC(Opcode op, uint8_t a, uint8_t b, uint8_t c) noexcept {
    return (static_cast<uint32_t>(op)) |
           (static_cast<uint32_t>(a) << 8) |
           (static_cast<uint32_t>(b) << 16) |
           (static_cast<uint32_t>(c) << 24);
}

inline uint32_t EncodeAImm(Opcode op, uint8_t a, uint16_t imm) noexcept {
    return (static_cast<uint32_t>(op)) |
           (static_cast<uint32_t>(a) << 8) |
           (static_cast<uint32_t>(imm) << 16);
}

inline Opcode DecodeOp(uint32_t inst) noexcept { return static_cast<Opcode>(inst & 0xFF); }
inline uint8_t DecodeA(uint32_t inst) noexcept { return static_cast<uint8_t>((inst >> 8) & 0xFF); }
inline uint8_t DecodeB(uint32_t inst) noexcept { return static_cast<uint8_t>((inst >> 16) & 0xFF); }
inline uint8_t DecodeC(uint32_t inst) noexcept { return static_cast<uint8_t>((inst >> 24) & 0xFF); }
inline uint16_t DecodeImm(uint32_t inst) noexcept { return static_cast<uint16_t>((inst >> 16) & 0xFFFF); }
inline int16_t DecodeSignedImm(uint32_t inst) noexcept { return static_cast<int16_t>((inst >> 16) & 0xFFFF); }

enum class ValueType : uint8_t {
    NIL = 0,
    BOOL,
    INT,
    FLOAT,
    STRING,
    VEC3,
    HANDLE // Generational handle to engine object (Doc 08 §13.1)
};

struct ScriptVec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct GenerationalHandle {
    uint32_t id = 0;
    uint32_t generation = 0;
};

struct Value {
    ValueType type = ValueType::NIL;
    union AsUnion {
        bool boolVal;
        int64_t intVal;
        double floatVal;
        ScriptVec3 vec3Val;
        GenerationalHandle handleVal;

        AsUnion() : intVal(0) {}
    } as;
    std::string strVal; // Used when type == STRING

    Value() : type(ValueType::NIL) { as.intVal = 0; }

    static Value Nil() noexcept { Value v; v.type = ValueType::NIL; return v; }
    static Value Bool(bool b) noexcept { Value v; v.type = ValueType::BOOL; v.as.boolVal = b; return v; }
    static Value Int(int64_t i) noexcept { Value v; v.type = ValueType::INT; v.as.intVal = i; return v; }
    static Value Float(double f) noexcept { Value v; v.type = ValueType::FLOAT; v.as.floatVal = f; return v; }
    static Value String(std::string s) noexcept { Value v; v.type = ValueType::STRING; v.strVal = std::move(s); return v; }
    static Value Vec3(float x, float y, float z) noexcept {
        Value v; v.type = ValueType::VEC3; v.as.vec3Val = {x, y, z}; return v;
    }
    static Value Handle(uint32_t id, uint32_t gen) noexcept {
        Value v; v.type = ValueType::HANDLE; v.as.handleVal = {id, gen}; return v;
    }

    [[nodiscard]] double AsDouble() const noexcept {
        if (type == ValueType::FLOAT) return as.floatVal;
        if (type == ValueType::INT) return static_cast<double>(as.intVal);
        return 0.0;
    }
};

struct CompiledFunction {
    std::string name;
    uint8_t numParameters = 0;
    uint8_t numRegisters = 16;
    std::vector<uint32_t> code;
    std::vector<Value> constants;
    std::vector<uint32_t> lines;

    uint16_t AddConstant(const Value& val) {
        constants.push_back(val);
        return static_cast<uint16_t>(constants.size() - 1);
    }
};

struct BytecodeModule {
    std::string moduleName;
    std::string className;
    std::vector<ExportedFieldMetadata> exportedFields;
    std::vector<CompiledFunction> functions;

    [[nodiscard]] const CompiledFunction* FindFunction(const std::string& name) const noexcept {
        for (const auto& fn : functions) {
            if (fn.name == name) return &fn;
        }
        return nullptr;
    }
};

} // namespace qivforge::script

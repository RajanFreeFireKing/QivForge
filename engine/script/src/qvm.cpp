#include "qivforge/script/qvm.h"
#include <cmath>
#include <iostream>

namespace qivforge::script {

Qvm::Qvm() noexcept {
    // Built-in standard library registrations (Doc 08 §10)
    RegisterNative("Math.abs", [](const std::vector<Value>& args) {
        if (args.empty()) return Value::Float(0.0);
        return Value::Float(std::abs(args[0].AsDouble()));
    });

    RegisterNative("Math.sin", [](const std::vector<Value>& args) {
        if (args.empty()) return Value::Float(0.0);
        return Value::Float(std::sin(args[0].AsDouble()));
    });

    RegisterNative("Math.cos", [](const std::vector<Value>& args) {
        if (args.empty()) return Value::Float(1.0);
        return Value::Float(std::cos(args[0].AsDouble()));
    });

    RegisterNative("Math.sqrt", [](const std::vector<Value>& args) {
        if (args.empty()) return Value::Float(0.0);
        double val = args[0].AsDouble();
        return Value::Float(val >= 0.0 ? std::sqrt(val) : 0.0);
    });

    RegisterNative("Math.clamp", [](const std::vector<Value>& args) {
        if (args.size() < 3) return Value::Float(0.0);
        double val = args[0].AsDouble();
        double mn = args[1].AsDouble();
        double mx = args[2].AsDouble();
        return Value::Float(std::clamp(val, mn, mx));
    });

    RegisterNative("Time.time", [](const std::vector<Value>&) {
        return Value::Float(1.5);
    });

    RegisterNative("Time.deltaTime", [](const std::vector<Value>&) {
        return Value::Float(1.0 / 60.0);
    });

    RegisterNative("TouchHUD.getAxis", [](const std::vector<Value>& args) {
        if (!args.empty() && args[0].strVal == "Vertical") return Value::Float(1.0); // Full throttle default
        if (!args.empty() && args[0].strVal == "Horizontal") return Value::Float(0.0);
        return Value::Float(0.0);
    });

    RegisterNative("TouchHUD.isPressed", [](const std::vector<Value>&) {
        return Value::Bool(false);
    });

    RegisterNative("Vec3", [](const std::vector<Value>& args) {
        float x = args.size() > 0 ? static_cast<float>(args[0].AsDouble()) : 0.0f;
        float y = args.size() > 1 ? static_cast<float>(args[1].AsDouble()) : 0.0f;
        float z = args.size() > 2 ? static_cast<float>(args[2].AsDouble()) : 0.0f;
        return Value::Vec3(x, y, z);
    });
}

void Qvm::RegisterNative(const std::string& name, NativeFunction func) {
    m_nativeFunctions[name] = std::move(func);
}

void Qvm::RegisterValidHandle(uint32_t id, uint32_t generation) {
    m_validHandles[id] = generation;
}

void Qvm::InvalidateHandle(uint32_t id) {
    m_validHandles.erase(id);
}

bool Qvm::IsHandleValid(uint32_t id, uint32_t generation) const noexcept {
    auto it = m_validHandles.find(id);
    if (it == m_validHandles.end()) return false;
    return it->second == generation;
}

ExecutionResult Qvm::Execute(const CompiledFunction& fn, const std::vector<Value>& args) {
    ExecutionResult result;
    result.success = true;
    result.returnValue = Value::Nil();

    std::array<Value, 256> registers;
    for (auto& r : registers) r = Value::Nil();

    // Populate arguments starting at r0 (r0 = self)
    for (size_t i = 0; i < args.size() && i < registers.size(); ++i) {
        registers[i] = args[i];
    }

    size_t pc = 0;
    const size_t codeSize = fn.code.size();

    while (pc < codeSize) {
        // Enforce instruction budget (Doc 08 §13.1 & AC4)
        if (++result.instructionsExecuted > m_instructionBudget) {
            result.success = false;
            result.errorCode = "QS-RUN-002";
            result.errorMessage = "Script took too long: Instruction budget exceeded";
            return result;
        }

        uint32_t inst = fn.code[pc++];
        Opcode op = DecodeOp(inst);
        uint8_t a = DecodeA(inst);
        uint8_t b = DecodeB(inst);
        uint8_t c = DecodeC(inst);
        uint16_t imm16 = DecodeImm(inst);
        int16_t sImm = DecodeSignedImm(inst);

        switch (op) {
            case Opcode::NOP:
                break;

            case Opcode::MOV:
                registers[a] = registers[b];
                break;

            case Opcode::LOADK:
                if (imm16 < fn.constants.size()) {
                    registers[a] = fn.constants[imm16];
                }
                break;

            case Opcode::LOADNULL:
                registers[a] = Value::Nil();
                break;

            case Opcode::LOADTRUE:
                registers[a] = Value::Bool(true);
                break;

            case Opcode::LOADFALSE:
                registers[a] = Value::Bool(false);
                break;

            case Opcode::LOADINT:
                registers[a] = Value::Int(sImm);
                break;

            // Integer Math
            case Opcode::ADDI:
                registers[a] = Value::Int(registers[b].as.intVal + registers[c].as.intVal);
                break;
            case Opcode::SUBI:
                registers[a] = Value::Int(registers[b].as.intVal - registers[c].as.intVal);
                break;
            case Opcode::MULI:
                registers[a] = Value::Int(registers[b].as.intVal * registers[c].as.intVal);
                break;
            case Opcode::DIVI:
                if (registers[c].as.intVal == 0) {
                    result.success = false;
                    result.errorCode = "QS-RUN-005";
                    result.errorMessage = "Divide by zero";
                    return result;
                }
                registers[a] = Value::Int(registers[b].as.intVal / registers[c].as.intVal);
                break;

            // Float Math
            case Opcode::ADDF:
                registers[a] = Value::Float(registers[b].AsDouble() + registers[c].AsDouble());
                break;
            case Opcode::SUBF:
                registers[a] = Value::Float(registers[b].AsDouble() - registers[c].AsDouble());
                break;
            case Opcode::MULF:
                registers[a] = Value::Float(registers[b].AsDouble() * registers[c].AsDouble());
                break;
            case Opcode::DIVF:
                registers[a] = Value::Float(registers[b].AsDouble() / (registers[c].AsDouble() == 0.0 ? 1e-6 : registers[c].AsDouble()));
                break;
            case Opcode::NEGF:
                registers[a] = Value::Float(-registers[b].AsDouble());
                break;

            // Vector3 Math (Doc 08 §13.2)
            case Opcode::V3ADD: {
                registers[a] = Value::Vec3(
                    registers[b].as.vec3Val.x + registers[c].as.vec3Val.x,
                    registers[b].as.vec3Val.y + registers[c].as.vec3Val.y,
                    registers[b].as.vec3Val.z + registers[c].as.vec3Val.z
                );
                break;
            }
            case Opcode::V3SUB: {
                registers[a] = Value::Vec3(
                    registers[b].as.vec3Val.x - registers[c].as.vec3Val.x,
                    registers[b].as.vec3Val.y - registers[c].as.vec3Val.y,
                    registers[b].as.vec3Val.z - registers[c].as.vec3Val.z
                );
                break;
            }
            case Opcode::V3MULS: {
                float s = static_cast<float>(registers[c].AsDouble());
                registers[a] = Value::Vec3(
                    registers[b].as.vec3Val.x * s,
                    registers[b].as.vec3Val.y * s,
                    registers[b].as.vec3Val.z * s
                );
                break;
            }
            case Opcode::V3DOT: {
                double dot = registers[b].as.vec3Val.x * registers[c].as.vec3Val.x +
                             registers[b].as.vec3Val.y * registers[c].as.vec3Val.y +
                             registers[b].as.vec3Val.z * registers[c].as.vec3Val.z;
                registers[a] = Value::Float(dot);
                break;
            }
            case Opcode::V3LEN: {
                float x = registers[b].as.vec3Val.x;
                float y = registers[b].as.vec3Val.y;
                float z = registers[b].as.vec3Val.z;
                registers[a] = Value::Float(std::sqrt(x * x + y * y + z * z));
                break;
            }
            case Opcode::V3NORM: {
                float x = registers[b].as.vec3Val.x;
                float y = registers[b].as.vec3Val.y;
                float z = registers[b].as.vec3Val.z;
                float len = std::sqrt(x * x + y * y + z * z);
                float inv = len > 1e-6f ? (1.0f / len) : 0.0f;
                registers[a] = Value::Vec3(x * inv, y * inv, z * inv);
                break;
            }

            // Compare & Jump
            case Opcode::EQ:
                registers[a] = Value::Bool(std::abs(registers[b].AsDouble() - registers[c].AsDouble()) < 1e-6);
                break;
            case Opcode::NE:
                registers[a] = Value::Bool(std::abs(registers[b].AsDouble() - registers[c].AsDouble()) >= 1e-6);
                break;
            case Opcode::LT:
                registers[a] = Value::Bool(registers[b].AsDouble() < registers[c].AsDouble());
                break;
            case Opcode::LE:
                registers[a] = Value::Bool(registers[b].AsDouble() <= registers[c].AsDouble());
                break;
            case Opcode::GT:
                registers[a] = Value::Bool(registers[b].AsDouble() > registers[c].AsDouble());
                break;
            case Opcode::GE:
                registers[a] = Value::Bool(registers[b].AsDouble() >= registers[c].AsDouble());
                break;

            case Opcode::JMP:
                pc = static_cast<size_t>(static_cast<int64_t>(pc) + sImm);
                break;

            case Opcode::JMPT:
                if (registers[a].type == ValueType::BOOL && registers[a].as.boolVal) {
                    pc = static_cast<size_t>(static_cast<int64_t>(pc) + sImm);
                }
                break;

            case Opcode::JMPF:
                if (registers[a].type == ValueType::BOOL && !registers[a].as.boolVal) {
                    pc = static_cast<size_t>(static_cast<int64_t>(pc) + sImm);
                }
                break;

            case Opcode::CALLNATIVE: {
                // a = destReg, b = firstArgReg, c = argCount
                std::string apiName;
                if (registers[a].type == ValueType::STRING) {
                    apiName = registers[a].strVal;
                } else if (imm16 < fn.constants.size()) {
                    apiName = fn.constants[imm16].strVal;
                }

                // Generational handle verification (Doc 08 §14.2 & AC6)
                if (registers[0].type == ValueType::HANDLE) {
                    if (!IsHandleValid(registers[0].as.handleVal.id, registers[0].as.handleVal.generation)) {
                        result.errorCode = "QS-RUN-003";
                        result.errorMessage = "Use of destroyed object";
                        registers[a] = Value::Nil();
                        break;
                    }
                }
                if (registers[b].type == ValueType::HANDLE) {
                    if (!IsHandleValid(registers[b].as.handleVal.id, registers[b].as.handleVal.generation)) {
                        result.errorCode = "QS-RUN-003";
                        result.errorMessage = "Use of destroyed object";
                        registers[a] = Value::Nil();
                        break;
                    }
                }

                auto it = m_nativeFunctions.find(apiName);
                if (it != m_nativeFunctions.end()) {
                    std::vector<Value> nativeArgs;
                    nativeArgs.reserve(c);
                    for (uint8_t i = 0; i < c && (b + i) < registers.size(); ++i) {
                        nativeArgs.push_back(registers[b + i]);
                    }
                    registers[a] = it->second(nativeArgs);
                } else {
                    registers[a] = Value::Nil();
                }
                break;
            }

            case Opcode::GETFIELD: {
                // a = destReg, b = objReg, c = nameConstIdx
                if (c < fn.constants.size()) {
                    const std::string& fieldName = fn.constants[c].strVal;
                    const Value& obj = registers[b];
                    if (obj.type == ValueType::HANDLE) {
                        if (!IsHandleValid(obj.as.handleVal.id, obj.as.handleVal.generation)) {
                            result.errorCode = "QS-RUN-003";
                            result.errorMessage = "Use of destroyed object";
                            registers[a] = Value::Nil();
                            break;
                        }
                    }
                    if (obj.type == ValueType::VEC3) {
                        if (fieldName == "x") registers[a] = Value::Float(obj.as.vec3Val.x);
                        else if (fieldName == "y") registers[a] = Value::Float(obj.as.vec3Val.y);
                        else if (fieldName == "z") registers[a] = Value::Float(obj.as.vec3Val.z);
                        else registers[a] = Value::Nil();
                    } else {
                        registers[a] = Value::Nil();
                    }
                }
                break;
            }

            case Opcode::SETFIELD: {
                // a = objReg, b = valReg, c = nameConstIdx
                if (c < fn.constants.size()) {
                    const std::string& fieldName = fn.constants[c].strVal;
                    Value& obj = registers[a];
                    if (obj.type == ValueType::HANDLE) {
                        if (!IsHandleValid(obj.as.handleVal.id, obj.as.handleVal.generation)) {
                            result.errorCode = "QS-RUN-003";
                            result.errorMessage = "Use of destroyed object";
                            break;
                        }
                    }
                    if (obj.type == ValueType::VEC3) {
                        float v = static_cast<float>(registers[b].AsDouble());
                        if (fieldName == "x") obj.as.vec3Val.x = v;
                        else if (fieldName == "y") obj.as.vec3Val.y = v;
                        else if (fieldName == "z") obj.as.vec3Val.z = v;
                    }
                }
                break;
            }

            case Opcode::RET:
                result.returnValue = registers[a];
                return result;

            case Opcode::RET_VOID:
                result.returnValue = Value::Nil();
                return result;

            default:
                break;
        }
    }

    return result;
}

} // namespace qivforge::script

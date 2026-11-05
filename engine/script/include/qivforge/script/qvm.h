#pragma once

#include "qivforge/script/bytecode.h"
#include <array>
#include <functional>
#include <unordered_map>
#include <string>

namespace qivforge::script {

struct ExecutionResult {
    bool success = true;
    Value returnValue = Value::Nil();
    std::string errorCode; // e.g. "QS-RUN-002", "QS-RUN-003", "QS-RUN-004"
    std::string errorMessage;
    uint32_t instructionsExecuted = 0;
};

using NativeFunction = std::function<Value(const std::vector<Value>& args)>;

class Qvm {
public:
    Qvm() noexcept;

    void SetInstructionBudget(uint32_t budget) noexcept { m_instructionBudget = budget; }
    [[nodiscard]] uint32_t GetInstructionBudget() const noexcept { return m_instructionBudget; }

    // Native API Registration (Doc 08 §10 & §13.3)
    void RegisterNative(const std::string& name, NativeFunction func);

    // Generational Handle Validation (Doc 08 §13.1 & AC6)
    void RegisterValidHandle(uint32_t id, uint32_t generation);
    void InvalidateHandle(uint32_t id);
    [[nodiscard]] bool IsHandleValid(uint32_t id, uint32_t generation) const noexcept;

    // Execution
    ExecutionResult Execute(const CompiledFunction& fn, const std::vector<Value>& args = {});

private:
    uint32_t m_instructionBudget = 100000; // 100k instructions per call default

    std::unordered_map<std::string, NativeFunction> m_nativeFunctions;
    std::unordered_map<uint32_t, uint32_t> m_validHandles; // id -> active generation
};

} // namespace qivforge::script

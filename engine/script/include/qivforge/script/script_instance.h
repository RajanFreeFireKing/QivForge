#pragma once

#include "qivforge/script/bytecode.h"
#include "qivforge/script/qvm.h"
#include <unordered_map>
#include <memory>
#include <string>

namespace qivforge::script {

class ScriptInstance {
public:
    ScriptInstance(uint32_t actorId, std::shared_ptr<BytecodeModule> module);

    [[nodiscard]] uint32_t GetActorId() const noexcept { return m_actorId; }
    [[nodiscard]] const std::string& GetClassName() const noexcept;

    // Field access for Inspector (Doc 08 AC3)
    [[nodiscard]] const std::vector<ExportedFieldMetadata>& GetExportedFields() const noexcept;
    [[nodiscard]] double GetFieldValue(const std::string& fieldName) const noexcept;
    void SetFieldValue(const std::string& fieldName, double value) noexcept;

    // Lifecycle Dispatches (Doc 08 §7)
    ExecutionResult OnAwake(Qvm& qvm);
    ExecutionResult OnStart(Qvm& qvm);
    ExecutionResult OnTick(Qvm& qvm, float dt);
    ExecutionResult OnPhysicsStep(Qvm& qvm, float fixedDt);

    // Hot-reload migration (Doc 08 §12 & AC5)
    bool HotReload(std::shared_ptr<BytecodeModule> newModule);

private:
    uint32_t m_actorId = 0;
    std::shared_ptr<BytecodeModule> m_module;
    std::unordered_map<std::string, double> m_fieldValues;
};

} // namespace qivforge::script

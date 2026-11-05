#include "qivforge/script/script_instance.h"

namespace qivforge::script {

ScriptInstance::ScriptInstance(uint32_t actorId, std::shared_ptr<BytecodeModule> module)
    : m_actorId(actorId), m_module(std::move(module)) {
    if (m_module) {
        for (const auto& meta : m_module->exportedFields) {
            m_fieldValues[meta.name] = meta.defaultValue;
        }
    }
}

const std::string& ScriptInstance::GetClassName() const noexcept {
    static const std::string s_empty = "";
    return m_module ? m_module->className : s_empty;
}

const std::vector<ExportedFieldMetadata>& ScriptInstance::GetExportedFields() const noexcept {
    static const std::vector<ExportedFieldMetadata> s_empty;
    return m_module ? m_module->exportedFields : s_empty;
}

double ScriptInstance::GetFieldValue(const std::string& fieldName) const noexcept {
    auto it = m_fieldValues.find(fieldName);
    return it != m_fieldValues.end() ? it->second : 0.0;
}

void ScriptInstance::SetFieldValue(const std::string& fieldName, double value) noexcept {
    m_fieldValues[fieldName] = value;
}

ExecutionResult ScriptInstance::OnAwake(Qvm& qvm) {
    if (!m_module) return ExecutionResult{};
    if (const auto* fn = m_module->FindFunction("onAwake")) {
        return qvm.Execute(*fn, {Value::Handle(m_actorId, 1)});
    }
    return ExecutionResult{};
}

ExecutionResult ScriptInstance::OnStart(Qvm& qvm) {
    if (!m_module) return ExecutionResult{};
    if (const auto* fn = m_module->FindFunction("onStart")) {
        return qvm.Execute(*fn, {Value::Handle(m_actorId, 1)});
    }
    return ExecutionResult{};
}

ExecutionResult ScriptInstance::OnTick(Qvm& qvm, float dt) {
    if (!m_module) return ExecutionResult{};
    if (const auto* fn = m_module->FindFunction("onTick")) {
        return qvm.Execute(*fn, {Value::Handle(m_actorId, 1), Value::Float(dt)});
    }
    return ExecutionResult{};
}

ExecutionResult ScriptInstance::OnPhysicsStep(Qvm& qvm, float fixedDt) {
    if (!m_module) return ExecutionResult{};
    if (const auto* fn = m_module->FindFunction("onPhysicsStep")) {
        return qvm.Execute(*fn, {Value::Handle(m_actorId, 1), Value::Float(fixedDt)});
    }
    return ExecutionResult{};
}

bool ScriptInstance::HotReload(std::shared_ptr<BytecodeModule> newModule) {
    if (!newModule) return false;

    // Migrate existing fields per Doc 08 §12 & AC5
    std::unordered_map<std::string, double> migratedValues;
    for (const auto& meta : newModule->exportedFields) {
        auto it = m_fieldValues.find(meta.name);
        if (it != m_fieldValues.end()) {
            migratedValues[meta.name] = it->second; // Preserve existing value
        } else {
            migratedValues[meta.name] = meta.defaultValue; // Apply new default
        }
    }

    m_fieldValues = std::move(migratedValues);
    m_module = std::move(newModule);
    return true;
}

} // namespace qivforge::script

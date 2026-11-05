#pragma once

#include "qivforge/vfx/particle_emitter.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace qivforge::vfx {

class VFXManager {
public:
    static constexpr size_t SCENE_HARD_CAP_PARTICLES = 8000; // Doc 10 §C1

    VFXManager();

    void RegisterTemplate(const EmitterConfig& config);
    [[nodiscard]] bool HasTemplate(const std::string& name) const;

    // Spawning from pool (Doc 10 AC3: zero memory growth after warm-up)
    ParticleEmitter* Spawn(const std::string& templateName, float x, float y, float z);

    void Update(float deltaTime);

    [[nodiscard]] size_t GetActiveEmitterCount() const noexcept;
    [[nodiscard]] size_t GetTotalActiveParticles() const noexcept;
    [[nodiscard]] size_t GetPoolCapacity(const std::string& templateName) const;

private:
    std::unordered_map<std::string, EmitterConfig> m_templates;
    // Map template name -> pool of emitters
    std::unordered_map<std::string, std::vector<std::unique_ptr<ParticleEmitter>>> m_emitterPools;
    std::vector<ParticleEmitter*> m_activeEmitters;

    void RegisterBuiltinLibrary();
};

} // namespace qivforge::vfx

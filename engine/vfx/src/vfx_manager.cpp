#include "qivforge/vfx/vfx_manager.h"
#include <algorithm>

namespace qivforge::vfx {

VFXManager::VFXManager() {
    RegisterBuiltinLibrary();
}

void VFXManager::RegisterTemplate(const EmitterConfig& config) {
    m_templates[config.name] = config;
}

bool VFXManager::HasTemplate(const std::string& name) const {
    return m_templates.find(name) != m_templates.end();
}

void VFXManager::RegisterBuiltinLibrary() {
    // 1. MuzzleFlash
    EmitterConfig mf;
    mf.name = "MuzzleFlash";
    mf.shape = EmitterShape::CONE;
    mf.emissionRate = 0.0f;
    mf.maxParticles = 60.0f;
    mf.burstCount = 40.0f;
    mf.lifetime = 0.08f;
    mf.speed = 25.0f;
    mf.startSize = 0.15f;
    mf.endSize = 0.05f;
    mf.gravityScale = 0.0f;
    mf.coneAngleDeg = 15.0f;
    mf.startColor[0] = 1.0f; mf.startColor[1] = 0.9f; mf.startColor[2] = 0.3f; mf.startColor[3] = 1.0f;
    mf.endColor[0] = 1.0f; mf.endColor[1] = 0.3f; mf.endColor[2] = 0.0f; mf.endColor[3] = 0.0f;
    mf.looping = false;
    RegisterTemplate(mf);

    // 2. BulletSparks
    EmitterConfig bs;
    bs.name = "BulletSparks";
    bs.shape = EmitterShape::CONE;
    bs.emissionRate = 0.0f;
    bs.maxParticles = 50.0f;
    bs.burstCount = 30.0f;
    bs.lifetime = 0.25f;
    bs.speed = 15.0f;
    bs.startSize = 0.05f;
    bs.endSize = 0.02f;
    bs.gravityScale = -9.81f;
    bs.coneAngleDeg = 45.0f;
    bs.startColor[0] = 1.0f; bs.startColor[1] = 1.0f; bs.startColor[2] = 0.6f; bs.startColor[3] = 1.0f;
    bs.endColor[0] = 1.0f; bs.endColor[1] = 0.5f; bs.endColor[2] = 0.1f; bs.endColor[3] = 0.0f;
    bs.looping = false;
    RegisterTemplate(bs);

    // 3. DirtImpact
    EmitterConfig di;
    di.name = "DirtImpact";
    di.shape = EmitterShape::SPHERE;
    di.emissionRate = 0.0f;
    di.maxParticles = 50.0f;
    di.burstCount = 35.0f;
    di.lifetime = 0.4f;
    di.speed = 8.0f;
    di.startSize = 0.1f;
    di.endSize = 0.25f;
    di.gravityScale = -12.0f;
    di.startColor[0] = 0.45f; di.startColor[1] = 0.35f; di.startColor[2] = 0.25f; di.startColor[3] = 1.0f;
    di.endColor[0] = 0.3f; di.endColor[1] = 0.25f; di.endColor[2] = 0.2f; di.endColor[3] = 0.0f;
    di.looping = false;
    RegisterTemplate(di);

    // 4. TractorExhaustSmoke
    EmitterConfig es;
    es.name = "TractorExhaustSmoke";
    es.shape = EmitterShape::CONE;
    es.emissionRate = 25.0f;
    es.maxParticles = 100.0f;
    es.lifetime = 1.2f;
    es.speed = 3.5f;
    es.startSize = 0.08f;
    es.endSize = 0.45f;
    es.gravityScale = 1.5f; // Slight upward buoyancy
    es.drag = 0.3f;
    es.coneAngleDeg = 12.0f;
    es.startColor[0] = 0.3f; es.startColor[1] = 0.3f; es.startColor[2] = 0.3f; es.startColor[3] = 0.7f;
    es.endColor[0] = 0.7f; es.endColor[1] = 0.7f; es.endColor[2] = 0.7f; es.endColor[3] = 0.0f;
    es.looping = true;
    RegisterTemplate(es);
}

ParticleEmitter* VFXManager::Spawn(const std::string& templateName, float x, float y, float z) {
    auto tIt = m_templates.find(templateName);
    if (tIt == m_templates.end()) return nullptr;

    const auto& config = tIt->second;
    auto& pool = m_emitterPools[templateName];

    // Find dormant emitter in pool
    ParticleEmitter* target = nullptr;
    for (auto& emitter : pool) {
        if (emitter->IsFinished()) {
            target = emitter.get();
            break;
        }
    }

    // If no dormant emitter found, allocate one into pool
    if (!target) {
        auto newEmitter = std::make_unique<ParticleEmitter>(config);
        target = newEmitter.get();
        pool.push_back(std::move(newEmitter));
    }

    target->SetConfig(config);
    target->SetPosition(x, y, z);
    target->Reset();

    if (config.burstCount > 0.0f) {
        target->Burst(static_cast<int32_t>(config.burstCount));
    }

    if (std::find(m_activeEmitters.begin(), m_activeEmitters.end(), target) == m_activeEmitters.end()) {
        m_activeEmitters.push_back(target);
    }

    return target;
}

void VFXManager::Update(float deltaTime) {
    for (auto it = m_activeEmitters.begin(); it != m_activeEmitters.end(); ) {
        ParticleEmitter* emitter = *it;
        emitter->Update(deltaTime);
        if (emitter->IsFinished()) {
            it = m_activeEmitters.erase(it);
        } else {
            ++it;
        }
    }
}

size_t VFXManager::GetActiveEmitterCount() const noexcept {
    return m_activeEmitters.size();
}

size_t VFXManager::GetTotalActiveParticles() const noexcept {
    size_t total = 0;
    for (const auto* emitter : m_activeEmitters) {
        total += emitter->GetActiveParticleCount();
    }
    return total;
}

size_t VFXManager::GetPoolCapacity(const std::string& templateName) const {
    auto it = m_emitterPools.find(templateName);
    if (it != m_emitterPools.end()) {
        return it->second.size();
    }
    return 0;
}

} // namespace qivforge::vfx

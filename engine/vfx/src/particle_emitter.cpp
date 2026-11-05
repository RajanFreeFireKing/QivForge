#include "qivforge/vfx/particle_emitter.h"
#include <cstdlib>

namespace qivforge::vfx {

ParticleEmitter::ParticleEmitter() {
    m_particles.resize(HARD_CAP_PARTICLES);
    Reset();
}

ParticleEmitter::ParticleEmitter(const EmitterConfig& config)
    : m_config(config) {
    m_particles.resize(HARD_CAP_PARTICLES);
    Reset();
}

void ParticleEmitter::SetConfig(const EmitterConfig& config) {
    m_config = config;
    Reset();
}

void ParticleEmitter::Reset() noexcept {
    m_activeCount = 0;
    m_emissionAccumulator = 0.0f;
    for (auto& p : m_particles) {
        p.active = false;
    }
}

void ParticleEmitter::Burst(int32_t count) {
    for (int32_t i = 0; i < count; ++i) {
        SpawnParticle();
    }
}

void ParticleEmitter::SpawnParticle() {
    size_t limit = std::min(HARD_CAP_PARTICLES, static_cast<size_t>(m_config.maxParticles));
    if (m_activeCount >= limit) return;

    // Find first inactive slot
    for (size_t i = 0; i < limit; ++i) {
        if (!m_particles[i].active) {
            auto& p = m_particles[i];
            p.active = true;
            p.age = 0.0f;
            p.maxLifetime = std::max(0.01f, m_config.lifetime);
            p.x = m_posX;
            p.y = m_posY;
            p.z = m_posZ;

            constexpr float kRandMax = static_cast<float>(RAND_MAX);
            // Generate direction based on shape
            float vx = 0.0f, vy = 1.0f, vz = 0.0f;
            if (m_config.shape == EmitterShape::CONE) {
                float angle = (static_cast<float>(std::rand()) / kRandMax - 0.5f) * (m_config.coneAngleDeg * 3.14159265f / 180.0f);
                float yaw = (static_cast<float>(std::rand()) / kRandMax) * 6.2831853f;
                vx = std::sin(angle) * std::cos(yaw);
                vy = std::cos(angle);
                vz = std::sin(angle) * std::sin(yaw);
            } else if (m_config.shape == EmitterShape::SPHERE) {
                float u = static_cast<float>(std::rand()) / kRandMax * 2.0f - 1.0f;
                float theta = static_cast<float>(std::rand()) / kRandMax * 6.2831853f;
                float r = std::sqrt(1.0f - u * u);
                vx = r * std::cos(theta);
                vy = u;
                vz = r * std::sin(theta);
            }

            p.vx = vx * m_config.speed;
            p.vy = vy * m_config.speed;
            p.vz = vz * m_config.speed;
            p.size = m_config.startSize;
            p.rotation = static_cast<float>(std::rand()) / kRandMax * 6.28f;
            p.rotationSpeed = (static_cast<float>(std::rand()) / kRandMax - 0.5f) * 2.0f;

            p.r = m_config.startColor[0];
            p.g = m_config.startColor[1];
            p.b = m_config.startColor[2];
            p.a = m_config.startColor[3];

            m_activeCount++;
            break;
        }
    }
}

void ParticleEmitter::Update(float deltaTime) {
    if (deltaTime <= 0.0f) return;

    // 1. Continuous emission
    if (m_config.looping && m_config.emissionRate > 0.0f) {
        m_emissionAccumulator += m_config.emissionRate * deltaTime;
        while (m_emissionAccumulator >= 1.0f) {
            SpawnParticle();
            m_emissionAccumulator -= 1.0f;
        }
    }

    // 2. High-performance contiguous loop
    size_t limit = std::min(HARD_CAP_PARTICLES, static_cast<size_t>(m_config.maxParticles));
    size_t newActiveCount = 0;

    for (size_t i = 0; i < limit; ++i) {
        auto& p = m_particles[i];
        if (!p.active) continue;

        p.age += deltaTime;
        if (p.age >= p.maxLifetime) {
            p.active = false;
            continue;
        }

        newActiveCount++;
        float t = p.age / p.maxLifetime; // Normalized lifetime 0..1

        // Physics integration
        p.vy += m_config.gravityScale * deltaTime;
        p.vx *= (1.0f - m_config.drag * deltaTime);
        p.vy *= (1.0f - m_config.drag * deltaTime);
        p.vz *= (1.0f - m_config.drag * deltaTime);

        p.x += p.vx * deltaTime;
        p.y += p.vy * deltaTime;
        p.z += p.vz * deltaTime;

        p.rotation += p.rotationSpeed * deltaTime;

        // Size over lifetime
        p.size = m_config.startSize + t * (m_config.endSize - m_config.startSize);

        // Color interpolation
        p.r = m_config.startColor[0] + t * (m_config.endColor[0] - m_config.startColor[0]);
        p.g = m_config.startColor[1] + t * (m_config.endColor[1] - m_config.startColor[1]);
        p.b = m_config.startColor[2] + t * (m_config.endColor[2] - m_config.startColor[2]);
        p.a = m_config.startColor[3] + t * (m_config.endColor[3] - m_config.startColor[3]);
    }

    m_activeCount = newActiveCount;
}

} // namespace qivforge::vfx

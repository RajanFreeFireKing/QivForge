#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <cmath>
#include <algorithm>

namespace qivforge::vfx {

enum class EmitterShape : uint8_t {
    POINT = 0,
    SPHERE = 1,
    CONE = 2,
    BOX = 3
};

struct Particle {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    float vx = 0.0f, vy = 0.0f, vz = 0.0f;
    float size = 1.0f;
    float rotation = 0.0f;
    float rotationSpeed = 0.0f;
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
    float age = 0.0f;
    float maxLifetime = 1.0f;
    bool active = false;
};

struct EmitterConfig {
    std::string name;
    EmitterShape shape = EmitterShape::CONE;
    float emissionRate = 50.0f;     // Particles per second
    float maxParticles = 2000.0f;   // Hard cap 2,000 per emitter (Doc 10 §C1)
    float lifetime = 1.0f;
    float speed = 5.0f;
    float startSize = 0.2f;
    float endSize = 0.5f;
    float gravityScale = -9.81f;
    float drag = 0.1f;
    float coneAngleDeg = 25.0f;

    // Start / End colors
    float startColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float endColor[4] = {1.0f, 1.0f, 1.0f, 0.0f};

    bool looping = true;
    float burstCount = 0.0f; // If > 0, fires bursts
};

class ParticleEmitter {
public:
    static constexpr size_t HARD_CAP_PARTICLES = 2000;

    ParticleEmitter();
    explicit ParticleEmitter(const EmitterConfig& config);

    void SetConfig(const EmitterConfig& config);
    [[nodiscard]] const EmitterConfig& GetConfig() const noexcept { return m_config; }

    void SetPosition(float x, float y, float z) noexcept {
        m_posX = x; m_posY = y; m_posZ = z;
    }

    void Reset() noexcept;
    void Burst(int32_t count);

    // Contiguous SIMD-friendly update loop
    void Update(float deltaTime);

    [[nodiscard]] size_t GetActiveParticleCount() const noexcept { return m_activeCount; }
    [[nodiscard]] const std::vector<Particle>& GetParticles() const noexcept { return m_particles; }

    [[nodiscard]] bool IsFinished() const noexcept {
        return !m_config.looping && m_activeCount == 0 && m_emissionAccumulator <= 0.0f;
    }

private:
    EmitterConfig m_config;
    float m_posX = 0.0f;
    float m_posY = 0.0f;
    float m_posZ = 0.0f;

    std::vector<Particle> m_particles; // Pre-allocated to HARD_CAP_PARTICLES
    size_t m_activeCount = 0;
    float m_emissionAccumulator = 0.0f;

    void SpawnParticle();
};

} // namespace qivforge::vfx

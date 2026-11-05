#pragma once

#include <cmath>
#include <cstdint>
#include <string>
#include <algorithm>

namespace qivforge::physics {

constexpr float PI = 3.14159265358979323846f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    constexpr Vec3() noexcept = default;
    constexpr Vec3(float x_, float y_, float z_) noexcept : x(x_), y(y_), z(z_) {}

    constexpr Vec3 operator+(const Vec3& r) const noexcept { return {x + r.x, y + r.y, z + r.z}; }
    constexpr Vec3 operator-(const Vec3& r) const noexcept { return {x - r.x, y - r.y, z - r.z}; }
    constexpr Vec3 operator*(float s) const noexcept { return {x * s, y * s, z * s}; }
    constexpr Vec3 operator/(float s) const noexcept { float inv = 1.0f / s; return {x * inv, y * inv, z * inv}; }
    constexpr Vec3 operator-() const noexcept { return {-x, -y, -z}; }

    Vec3& operator+=(const Vec3& r) noexcept { x += r.x; y += r.y; z += r.z; return *this; }
    Vec3& operator-=(const Vec3& r) noexcept { x -= r.x; y -= r.y; z -= r.z; return *this; }
    Vec3& operator*=(float s) noexcept { x *= s; y *= s; z *= s; return *this; }
    Vec3& operator/=(float s) noexcept { float inv = 1.0f / s; x *= inv; y *= inv; z *= inv; return *this; }

    [[nodiscard]] constexpr float Dot(const Vec3& r) const noexcept { return x * r.x + y * r.y + z * r.z; }
    [[nodiscard]] constexpr Vec3 Cross(const Vec3& r) const noexcept {
        return {y * r.z - z * r.y, z * r.x - x * r.z, x * r.y - y * r.x};
    }
    [[nodiscard]] float LengthSquared() const noexcept { return x * x + y * y + z * z; }
    [[nodiscard]] float Length() const noexcept { return std::sqrt(LengthSquared()); }
    [[nodiscard]] Vec3 Normalized() const noexcept {
        float len = Length();
        return len > 1e-6f ? (*this / len) : Vec3{0.0f, 0.0f, 0.0f};
    }

    constexpr bool operator==(const Vec3& r) const noexcept {
        return x == r.x && y == r.y && z == r.z;
    }
    constexpr bool operator!=(const Vec3& r) const noexcept { return !(*this == r); }
};

enum class BodyType : uint8_t {
    STATIC = 0,
    DYNAMIC = 1,
    KINEMATIC = 2
};

enum class ColliderShape : uint8_t {
    BOX = 0,
    SPHERE = 1,
    CAPSULE = 2,
    CYLINDER = 3,
    CONVEX = 4,
    MESH = 5,
    HEIGHTFIELD = 6
};

// Object layers (16 available, 8 named by default per Doc 06 §3)
namespace Layers {
    constexpr uint16_t STATIC     = 1 << 0;
    constexpr uint16_t DYNAMIC    = 1 << 1;
    constexpr uint16_t VEHICLE    = 1 << 2;
    constexpr uint16_t CHARACTER  = 1 << 3;
    constexpr uint16_t PROJECTILE = 1 << 4;
    constexpr uint16_t DEBRIS     = 1 << 5;
    constexpr uint16_t TRIGGER    = 1 << 6;
    constexpr uint16_t RAGDOLL    = 1 << 7;
    constexpr uint16_t ALL        = 0xFFFF;
}

enum class CombineMode : uint8_t {
    AVERAGE = 0,
    MIN = 1,
    MAX = 2,
    MULTIPLY = 3
};

struct PhysicsMaterial {
    float friction = 0.6f;
    float restitution = 0.0f;
    CombineMode combineMode = CombineMode::AVERAGE;

    static PhysicsMaterial Default() noexcept { return {0.6f, 0.0f, CombineMode::AVERAGE}; }
    static PhysicsMaterial Rubber() noexcept { return {0.9f, 0.8f, CombineMode::AVERAGE}; }
    static PhysicsMaterial Metal() noexcept { return {0.4f, 0.2f, CombineMode::MIN}; }
    static PhysicsMaterial Ice() noexcept { return {0.05f, 0.05f, CombineMode::MIN}; }
    static PhysicsMaterial Mud() noexcept { return {0.45f, 0.0f, CombineMode::AVERAGE}; }
    static PhysicsMaterial Grass() noexcept { return {0.65f, 0.1f, CombineMode::AVERAGE}; }
    static PhysicsMaterial Asphalt() noexcept { return {1.0f, 0.05f, CombineMode::AVERAGE}; }
    static PhysicsMaterial Wood() noexcept { return {0.5f, 0.2f, CombineMode::AVERAGE}; }
};

// Surface types for vehicle friction and tire audio/VFX per Doc 06 §9.4
enum class SurfaceType : uint8_t {
    ASPHALT = 0,
    DIRT = 1,
    GRASS = 2,
    MUD = 3,
    SAND = 4,
    ICE = 5
};

struct SurfaceProperties {
    float forwardMultiplier = 1.0f;
    float sidewaysMultiplier = 1.0f;
    float rollingResistance = 0.02f;
};

inline SurfaceProperties GetSurfaceProperties(SurfaceType type) noexcept {
    switch (type) {
        case SurfaceType::ASPHALT: return {1.0f, 1.0f, 0.015f};
        case SurfaceType::DIRT:    return {0.75f, 0.65f, 0.04f};
        case SurfaceType::GRASS:   return {0.65f, 0.55f, 0.05f};
        case SurfaceType::MUD:     return {0.45f, 0.40f, 0.12f};
        case SurfaceType::SAND:    return {0.55f, 0.45f, 0.08f};
        case SurfaceType::ICE:     return {0.15f, 0.10f, 0.005f};
        default:                   return {1.0f, 1.0f, 0.02f};
    }
}

enum class ContinuousDetection : uint8_t {
    OFF = 0,
    LINEAR_CAST = 1
};

struct RayHit {
    bool hasHit = false;
    Vec3 point{0.0f, 0.0f, 0.0f};
    Vec3 normal{0.0f, 1.0f, 0.0f};
    float distance = 0.0f;
    uint32_t actorId = 0;
    SurfaceType surfaceType = SurfaceType::ASPHALT;
};

struct CollisionEvent {
    uint32_t actorA = 0;
    uint32_t actorB = 0;
    Vec3 contactPoint{0.0f, 0.0f, 0.0f};
    Vec3 normal{0.0f, 1.0f, 0.0f};
    float relativeSpeed = 0.0f;
    float impulse = 0.0f;
};

struct TriggerEvent {
    uint32_t triggerActor = 0;
    uint32_t otherActor = 0;
    bool entered = true; // true = enter, false = exit
};

} // namespace qivforge::physics

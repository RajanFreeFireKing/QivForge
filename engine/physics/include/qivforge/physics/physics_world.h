#pragma once

#include "qivforge/physics/physics_types.h"
#include "qivforge/physics/rigid_body.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

namespace qivforge::physics {

class PhysicsWorld {
public:
    PhysicsWorld() noexcept;
    ~PhysicsWorld() = default;

    // Simulation stepping (Doc 06 §2)
    void Update(float deltaTime) noexcept;
    void StepFixed(float fixedDt) noexcept;

    [[nodiscard]] float GetFixedDt() const noexcept { return m_fixedDt; }
    [[nodiscard]] float GetInterpolationAlpha() const noexcept { return m_interpolationAlpha; }
    [[nodiscard]] const Vec3& GetGravity() const noexcept { return m_gravity; }
    void SetGravity(const Vec3& g) noexcept { m_gravity = g; }

    // Body Management
    RigidBody* CreateBody(uint32_t actorId, BodyType type = BodyType::DYNAMIC);
    bool DestroyBody(uint32_t actorId);
    [[nodiscard]] RigidBody* GetBody(uint32_t actorId) noexcept;
    [[nodiscard]] const RigidBody* GetBody(uint32_t actorId) const noexcept;
    void Clear() noexcept;

    [[nodiscard]] size_t GetActiveBodyCount() const noexcept;
    [[nodiscard]] size_t GetSleepingBodyCount() const noexcept;
    [[nodiscard]] size_t GetTotalBodyCount() const noexcept { return m_bodies.size(); }

    // Collision Matrix & Filtering (Doc 06 §3)
    [[nodiscard]] bool CanLayersCollide(uint16_t layerA, uint16_t layerB) const noexcept;
    void SetCollisionMask(uint16_t layer, uint16_t mask) noexcept;

    // Queries (Doc 06 §6)
    [[nodiscard]] RayHit Raycast(const Vec3& origin, const Vec3& direction, float maxDistance = 1000.0f, uint16_t layerMask = Layers::ALL) noexcept;
    [[nodiscard]] std::vector<RayHit> RaycastAll(const Vec3& origin, const Vec3& direction, float maxDistance = 1000.0f, uint16_t layerMask = Layers::ALL) noexcept;
    [[nodiscard]] RayHit SphereCast(const Vec3& origin, float radius, const Vec3& direction, float maxDistance = 1000.0f, uint16_t layerMask = Layers::ALL) noexcept;
    [[nodiscard]] std::vector<uint32_t> OverlapSphere(const Vec3& center, float radius, uint16_t layerMask = Layers::ALL) noexcept;
    [[nodiscard]] std::vector<uint32_t> OverlapBox(const Vec3& center, const Vec3& halfExtents, uint16_t layerMask = Layers::ALL) noexcept;

    // Ground plane support
    void SetGroundPlaneEnabled(bool enabled, float height = 0.0f) noexcept {
        m_hasGroundPlane = enabled;
        m_groundHeight = height;
    }
    [[nodiscard]] bool HasGroundPlane() const noexcept { return m_hasGroundPlane; }
    [[nodiscard]] float GetGroundHeight() const noexcept { return m_groundHeight; }

    // Event retrieval
    [[nodiscard]] const std::vector<CollisionEvent>& GetCollisionEvents() const noexcept { return m_collisionEvents; }
    [[nodiscard]] const std::vector<TriggerEvent>& GetTriggerEvents() const noexcept { return m_triggerEvents; }
    void ClearEvents() noexcept { m_collisionEvents.clear(); m_triggerEvents.clear(); }

    // Query stats
    [[nodiscard]] uint32_t GetRaycastCountThisFrame() const noexcept { return m_raycastsThisFrame; }

private:
    void ResolveCollisions(float dt) noexcept;
    void ResolveGroundCollisions(RigidBody& body) noexcept;
    void ResolveBodyVsBody(RigidBody& a, RigidBody& b) noexcept;

    float m_fixedDt = 1.0f / 60.0f; // 60 Hz fixed step
    float m_accumulator = 0.0f;
    float m_interpolationAlpha = 0.0f;
    Vec3 m_gravity{0.0f, -9.81f, 0.0f};

    bool m_hasGroundPlane = true;
    float m_groundHeight = 0.0f;

    std::unordered_map<uint32_t, std::unique_ptr<RigidBody>> m_bodies;
    std::unordered_map<uint16_t, uint16_t> m_layerCollisionMasks;

    std::vector<CollisionEvent> m_collisionEvents;
    std::vector<TriggerEvent> m_triggerEvents;
    uint32_t m_raycastsThisFrame = 0;
};

} // namespace qivforge::physics

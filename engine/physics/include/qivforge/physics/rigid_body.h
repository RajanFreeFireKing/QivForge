#pragma once

#include "qivforge/physics/physics_types.h"
#include <cstdint>

namespace qivforge::physics {

class RigidBody {
public:
    RigidBody(uint32_t actorId = 0, BodyType type = BodyType::DYNAMIC) noexcept;

    // Actor binding
    [[nodiscard]] uint32_t GetActorId() const noexcept { return m_actorId; }
    void SetActorId(uint32_t id) noexcept { m_actorId = id; }

    // Body type & state
    [[nodiscard]] BodyType GetBodyType() const noexcept { return m_bodyType; }
    void SetBodyType(BodyType type) noexcept;

    [[nodiscard]] float GetMass() const noexcept { return m_mass; }
    [[nodiscard]] float GetInverseMass() const noexcept { return m_invMass; }
    void SetMass(float massKg) noexcept;

    [[nodiscard]] const Vec3& GetCenterOfMass() const noexcept { return m_centerOfMass; }
    void SetCenterOfMass(const Vec3& com) noexcept { m_centerOfMass = com; }

    [[nodiscard]] float GetLinearDamping() const noexcept { return m_linearDamping; }
    void SetLinearDamping(float damping) noexcept { m_linearDamping = std::clamp(damping, 0.0f, 1.0f); }

    [[nodiscard]] float GetAngularDamping() const noexcept { return m_angularDamping; }
    void SetAngularDamping(float damping) noexcept { m_angularDamping = std::clamp(damping, 0.0f, 1.0f); }

    [[nodiscard]] float GetGravityScale() const noexcept { return m_gravityScale; }
    void SetGravityScale(float scale) noexcept { m_gravityScale = scale; }

    [[nodiscard]] ContinuousDetection GetContinuousDetection() const noexcept { return m_ccd; }
    void SetContinuousDetection(ContinuousDetection ccd) noexcept { m_ccd = ccd; }

    // Kinematics & Transforms
    [[nodiscard]] const Vec3& GetPosition() const noexcept { return m_position; }
    void SetPosition(const Vec3& pos) noexcept { m_position = pos; m_prevPosition = pos; }

    [[nodiscard]] const Vec3& GetRotation() const noexcept { return m_rotation; }
    void SetRotation(const Vec3& rotDeg) noexcept { m_rotation = rotDeg; m_prevRotation = rotDeg; }

    [[nodiscard]] const Vec3& GetPrevPosition() const noexcept { return m_prevPosition; }
    [[nodiscard]] const Vec3& GetPrevRotation() const noexcept { return m_prevRotation; }

    [[nodiscard]] Vec3 GetInterpolatedPosition(float alpha) const noexcept {
        return m_prevPosition + (m_position - m_prevPosition) * alpha;
    }

    [[nodiscard]] const Vec3& GetLinearVelocity() const noexcept { return m_linearVelocity; }
    void SetLinearVelocity(const Vec3& vel, bool wake = true) noexcept {
        m_linearVelocity = vel;
        if (wake && vel.LengthSquared() > 0.01f) Wake();
    }

    [[nodiscard]] const Vec3& GetAngularVelocity() const noexcept { return m_angularVelocity; }
    void SetAngularVelocity(const Vec3& angVel, bool wake = true) noexcept {
        m_angularVelocity = angVel;
        if (wake && angVel.LengthSquared() > 0.01f) Wake();
    }

    // Dynamic Forces & Impulses
    void AddForce(const Vec3& force) noexcept;
    void AddForceAtPosition(const Vec3& force, const Vec3& worldPoint) noexcept;
    void AddImpulse(const Vec3& impulse) noexcept;
    void AddTorque(const Vec3& torque) noexcept;

    // Sleep state (Doc 06 §4 & §15)
    [[nodiscard]] bool IsSleeping() const noexcept { return m_isSleeping; }
    [[nodiscard]] bool AllowsSleep() const noexcept { return m_allowSleep; }
    void SetAllowSleep(bool allow) noexcept { m_allowSleep = allow; if (!allow) Wake(); }
    void Wake() noexcept;
    void Sleep() noexcept;
    void CheckSleep(float dt) noexcept;

    // Collider & Material
    [[nodiscard]] ColliderShape GetColliderShape() const noexcept { return m_colliderShape; }
    void SetColliderShape(ColliderShape shape) noexcept { m_colliderShape = shape; }

    [[nodiscard]] const Vec3& GetColliderSize() const noexcept { return m_colliderSize; }
    void SetColliderSize(const Vec3& size) noexcept { m_colliderSize = size; }

    [[nodiscard]] const PhysicsMaterial& GetMaterial() const noexcept { return m_material; }
    void SetMaterial(const PhysicsMaterial& mat) noexcept { m_material = mat; }

    [[nodiscard]] uint16_t GetLayer() const noexcept { return m_layer; }
    void SetLayer(uint16_t layer) noexcept { m_layer = layer; }

    [[nodiscard]] bool IsTrigger() const noexcept { return m_isTrigger; }
    void SetTrigger(bool trigger) noexcept { m_isTrigger = trigger; }

    // Step Integration
    void Integrate(float dt, const Vec3& gravity) noexcept;

private:
    uint32_t m_actorId = 0;
    BodyType m_bodyType = BodyType::DYNAMIC;

    float m_mass = 1.0f;
    float m_invMass = 1.0f;
    Vec3 m_centerOfMass{0.0f, 0.0f, 0.0f};

    float m_linearDamping = 0.05f;
    float m_angularDamping = 0.05f;
    float m_gravityScale = 1.0f;
    ContinuousDetection m_ccd = ContinuousDetection::OFF;

    Vec3 m_position{0.0f, 0.0f, 0.0f};
    Vec3 m_rotation{0.0f, 0.0f, 0.0f};
    Vec3 m_prevPosition{0.0f, 0.0f, 0.0f};
    Vec3 m_prevRotation{0.0f, 0.0f, 0.0f};

    Vec3 m_linearVelocity{0.0f, 0.0f, 0.0f};
    Vec3 m_angularVelocity{0.0f, 0.0f, 0.0f}; // deg/s

    Vec3 m_accumulatedForce{0.0f, 0.0f, 0.0f};
    Vec3 m_accumulatedTorque{0.0f, 0.0f, 0.0f};

    bool m_allowSleep = true;
    bool m_isSleeping = false;
    float m_sleepTimer = 0.0f;

    ColliderShape m_colliderShape = ColliderShape::BOX;
    Vec3 m_colliderSize{0.5f, 0.5f, 0.5f}; // Half-extents for box
    PhysicsMaterial m_material = PhysicsMaterial::Default();
    uint16_t m_layer = Layers::DYNAMIC;
    bool m_isTrigger = false;
};

} // namespace qivforge::physics

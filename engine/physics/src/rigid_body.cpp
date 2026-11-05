#include "qivforge/physics/rigid_body.h"
#include <algorithm>

namespace qivforge::physics {

RigidBody::RigidBody(uint32_t actorId, BodyType type) noexcept
    : m_actorId(actorId), m_bodyType(type) {
    SetBodyType(type);
}

void RigidBody::SetBodyType(BodyType type) noexcept {
    m_bodyType = type;
    if (m_bodyType == BodyType::STATIC || m_bodyType == BodyType::KINEMATIC) {
        m_invMass = 0.0f;
        if (m_bodyType == BodyType::STATIC) {
            m_linearVelocity = {0.0f, 0.0f, 0.0f};
            m_angularVelocity = {0.0f, 0.0f, 0.0f};
            m_layer = Layers::STATIC;
        }
    } else {
        SetMass(m_mass);
        m_layer = Layers::DYNAMIC;
    }
    Wake();
}

void RigidBody::SetMass(float massKg) noexcept {
    // Range 0.01 to 100000 per Doc 06 §4
    m_mass = std::clamp(massKg, 0.01f, 100000.0f);
    if (m_bodyType == BodyType::DYNAMIC) {
        m_invMass = 1.0f / m_mass;
    } else {
        m_invMass = 0.0f;
    }
}

void RigidBody::AddForce(const Vec3& force) noexcept {
    if (m_bodyType != BodyType::DYNAMIC) return;
    m_accumulatedForce += force;
    Wake();
}

void RigidBody::AddForceAtPosition(const Vec3& force, const Vec3& worldPoint) noexcept {
    if (m_bodyType != BodyType::DYNAMIC) return;
    m_accumulatedForce += force;
    
    // Torque = r x F, where r = worldPoint - (position + centerOfMass)
    Vec3 r = worldPoint - (m_position + m_centerOfMass);
    Vec3 torque = r.Cross(force);
    m_accumulatedTorque += torque * RAD_TO_DEG; // Store angular in deg/s^2
    Wake();
}

void RigidBody::AddImpulse(const Vec3& impulse) noexcept {
    if (m_bodyType != BodyType::DYNAMIC) return;
    m_linearVelocity += impulse * m_invMass;
    if (impulse.LengthSquared() > 1.0f) {
        Wake();
    }
}

void RigidBody::AddTorque(const Vec3& torque) noexcept {
    if (m_bodyType != BodyType::DYNAMIC) return;
    // Simplified moment of inertia estimation (box approx: I ~ 1/6 * m * w^2)
    float approxInertia = (1.0f / 6.0f) * m_mass * (m_colliderSize.x * m_colliderSize.x + m_colliderSize.z * m_colliderSize.z);
    if (approxInertia < 0.01f) approxInertia = 0.01f;
    m_accumulatedTorque += (torque / approxInertia) * RAD_TO_DEG;
    if (torque.LengthSquared() > 1.0f) {
        Wake();
    }
}

void RigidBody::Wake() noexcept {
    m_isSleeping = false;
    m_sleepTimer = 0.0f;
}

void RigidBody::Sleep() noexcept {
    if (m_bodyType == BodyType::DYNAMIC && m_allowSleep) {
        m_isSleeping = true;
        m_linearVelocity = {0.0f, 0.0f, 0.0f};
        m_angularVelocity = {0.0f, 0.0f, 0.0f};
        m_accumulatedForce = {0.0f, 0.0f, 0.0f};
        m_accumulatedTorque = {0.0f, 0.0f, 0.0f};
    }
}

void RigidBody::Integrate(float dt, const Vec3& gravity) noexcept {
    if (m_bodyType == BodyType::STATIC || m_isSleeping) {
        return;
    }

    m_prevPosition = m_position;
    m_prevRotation = m_rotation;

    if (m_bodyType == BodyType::DYNAMIC) {
        // Accelerations
        Vec3 accel = (gravity * m_gravityScale) + (m_accumulatedForce * m_invMass);
        m_linearVelocity += accel * dt;

        // Apply damping
        m_linearVelocity *= std::max(0.0f, 1.0f - m_linearDamping * dt);

        Vec3 angAccel = m_accumulatedTorque;
        m_angularVelocity += angAccel * dt;
        m_angularVelocity *= std::max(0.0f, 1.0f - m_angularDamping * dt);

        // Update transforms
        m_position += m_linearVelocity * dt;
        m_rotation += m_angularVelocity * dt;

        // Clear forces
        m_accumulatedForce = {0.0f, 0.0f, 0.0f};
        m_accumulatedTorque = {0.0f, 0.0f, 0.0f};
    } else if (m_bodyType == BodyType::KINEMATIC) {
        m_position += m_linearVelocity * dt;
        m_rotation += m_angularVelocity * dt;
    }
}

void RigidBody::CheckSleep(float dt) noexcept {
    if (!m_allowSleep || m_bodyType != BodyType::DYNAMIC || m_isSleeping) return;

    float linSpeedSq = m_linearVelocity.LengthSquared();
    float angSpeedSq = m_angularVelocity.LengthSquared();
    float dispSq = (m_position - m_prevPosition).LengthSquared();

    // Settle threshold: speed < 0.5 m/s or frame displacement < 0.05m
    if ((linSpeedSq < 0.25f || dispSq < 0.0025f) && angSpeedSq < 0.10f) {
        m_sleepTimer += dt;
        if (m_sleepTimer >= 0.5f) {
            Sleep();
        }
    } else {
        m_sleepTimer = 0.0f;
    }
}

} // namespace qivforge::physics

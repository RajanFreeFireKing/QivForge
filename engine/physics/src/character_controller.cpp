#include "qivforge/physics/character_controller.h"
#include "qivforge/physics/physics_world.h"
#include <cmath>
#include <algorithm>

namespace qivforge::physics {

CharacterController::CharacterController(RigidBody* body) noexcept
    : m_body(body) {
    if (m_body) {
        m_body->SetBodyType(BodyType::KINEMATIC);
        m_body->SetLayer(Layers::CHARACTER);
        m_body->SetColliderShape(ColliderShape::CAPSULE);
        m_body->SetColliderSize(Vec3{m_radius, m_currentHeight * 0.5f, m_radius});
    }
}

void CharacterController::Move(const Vec3& inputDir, bool isRunning) noexcept {
    m_moveInput = inputDir.Normalized();
    m_isRunning = isRunning;
}

void CharacterController::Jump() noexcept {
    m_jumpBufferTimer = 0.1f; // 0.1s jump buffer per Doc 06 §10
}

void CharacterController::SetCrouch(bool crouch) noexcept {
    m_isCrouching = crouch;
    m_currentHeight = crouch ? m_crouchHeight : m_standingHeight;
    if (m_body) {
        m_body->SetColliderSize(Vec3{m_radius, m_currentHeight * 0.5f, m_radius});
    }
}

void CharacterController::Update(float dt, PhysicsWorld& world) noexcept {
    if (!m_body) return;

    Vec3 pos = m_body->GetPosition();
    float halfHeight = m_currentHeight * 0.5f;

    // 1. Ground check: cast downward 0.1 m below capsule per Doc 06 §10
    Vec3 groundRayOrigin = pos - Vec3{0.0f, halfHeight - 0.05f, 0.0f};
    RayHit groundHit = world.Raycast(groundRayOrigin, Vec3{0.0f, -1.0f, 0.0f}, 0.25f, Layers::STATIC);

    float slopeAngleDeg = 0.0f;
    if (groundHit.hasHit) {
        float normalY = std::clamp(groundHit.normal.y, -1.0f, 1.0f);
        slopeAngleDeg = std::acos(normalY) * RAD_TO_DEG;
    }

    // 2. Slope slide vs walkable logic (Doc 06 §10 & AC4: slope limit 50°, slide down 60° slopes)
    bool onSteepSlope = groundHit.hasHit && (slopeAngleDeg > m_slopeLimitDeg);

    if (groundHit.hasHit && !onSteepSlope) {
        m_isGrounded = true;
        m_coyoteTimer = 0.1f; // 0.1s coyote time

        // Snap to ground surface
        float targetY = groundHit.point.y + halfHeight;
        if (std::abs(pos.y - targetY) < 0.1f) {
            pos.y = targetY;
        }

        if (m_velocity.y < 0.0f) {
            m_velocity.y = 0.0f;
        }

        // Horizontal speed
        float speed = m_isCrouching ? 1.5f : (m_isRunning ? m_runSpeed : m_walkSpeed);
        m_velocity.x = m_moveInput.x * speed;
        m_velocity.z = m_moveInput.z * speed;

        // Step climbing (Doc 06 AC4: walk up 0.35m step)
        if (m_moveInput.LengthSquared() > 0.01f) {
            Vec3 forwardRayOrigin = pos - Vec3{0.0f, halfHeight - 0.05f, 0.0f};
            RayHit obstacleHit = world.Raycast(forwardRayOrigin, m_moveInput, m_radius + 0.15f, Layers::STATIC);
            if (obstacleHit.hasHit) {
                // Check if obstacle height <= step height (0.35m)
                Vec3 stepRayOrigin = obstacleHit.point + Vec3{0.0f, m_stepHeight + 0.05f, 0.0f} + m_moveInput * 0.05f;
                RayHit topClearance = world.Raycast(stepRayOrigin, Vec3{0.0f, -1.0f, 0.0f}, m_stepHeight + 0.1f, Layers::STATIC);
                if (topClearance.hasHit) {
                    float stepElevation = topClearance.point.y - (pos.y - halfHeight);
                    if (stepElevation > 0.01f && stepElevation <= m_stepHeight) {
                        pos.y += stepElevation;
                    }
                }
            }
        }

        // Jump processing
        if (m_jumpBufferTimer > 0.0f) {
            // v = sqrt(2 * |g| * h)
            m_velocity.y = std::sqrt(2.0f * std::abs(m_gravity) * m_jumpHeight);
            m_isGrounded = false;
            m_jumpBufferTimer = 0.0f;
            m_coyoteTimer = 0.0f;
        }
    } else if (onSteepSlope) {
        // Slide down steep slope!
        m_isGrounded = false;
        Vec3 n = groundHit.normal;
        Vec3 gravDir{0.0f, -1.0f, 0.0f};
        // Slide direction = gravDir - (gravDir . n) * n
        Vec3 slideDir = (gravDir - n * gravDir.Dot(n)).Normalized();
        float slideAccel = std::abs(m_gravity) * std::sin(slopeAngleDeg * DEG_TO_RAD);

        m_velocity.x += slideDir.x * slideAccel * dt;
        m_velocity.y += slideDir.y * slideAccel * dt;
        m_velocity.z += slideDir.z * slideAccel * dt;
    } else {
        // In air
        m_isGrounded = false;
        m_velocity.y += m_gravity * dt;
        m_coyoteTimer = std::max(0.0f, m_coyoteTimer - dt);

        // Mid-air steer
        float speed = m_walkSpeed;
        m_velocity.x = m_moveInput.x * speed;
        m_velocity.z = m_moveInput.z * speed;
    }

    m_jumpBufferTimer = std::max(0.0f, m_jumpBufferTimer - dt);

    // Apply movement
    pos += m_velocity * dt;
    m_body->SetPosition(pos);
}

} // namespace qivforge::physics

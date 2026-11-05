#pragma once

#include "qivforge/physics/physics_types.h"
#include "qivforge/physics/rigid_body.h"

namespace qivforge::physics {

class PhysicsWorld;

class CharacterController {
public:
    CharacterController(RigidBody* body) noexcept;

    // Movement Controls (Doc 06 §10)
    void Move(const Vec3& inputDir, bool isRunning = false) noexcept;
    void Jump() noexcept;
    void SetCrouch(bool crouch) noexcept;

    void Update(float dt, PhysicsWorld& world) noexcept;

    [[nodiscard]] bool IsGrounded() const noexcept { return m_isGrounded; }
    [[nodiscard]] bool IsCrouching() const noexcept { return m_isCrouching; }
    [[nodiscard]] const Vec3& GetVelocity() const noexcept { return m_velocity; }
    void SetVelocity(const Vec3& vel) noexcept { m_velocity = vel; }

    [[nodiscard]] float GetHeight() const noexcept { return m_currentHeight; }
    [[nodiscard]] float GetRadius() const noexcept { return m_radius; }
    [[nodiscard]] float GetSlopeLimitDeg() const noexcept { return m_slopeLimitDeg; }
    [[nodiscard]] float GetStepHeight() const noexcept { return m_stepHeight; }

    [[nodiscard]] RigidBody* GetBody() noexcept { return m_body; }

private:
    RigidBody* m_body = nullptr;

    float m_standingHeight = 1.8f;
    float m_crouchHeight = 1.1f;
    float m_currentHeight = 1.8f;
    float m_radius = 0.35f;

    float m_walkSpeed = 3.0f;
    float m_runSpeed = 6.5f;
    float m_jumpHeight = 1.2f;
    float m_gravity = -20.0f; // Snappier mobile feel per Doc 06 §10

    float m_slopeLimitDeg = 50.0f; // Limit 50°, slide above that per Doc 06 §10 & AC4
    float m_stepHeight = 0.35f;    // Climb up to 0.35m steps per Doc 06 AC4

    Vec3 m_moveInput{0.0f, 0.0f, 0.0f};
    bool m_isRunning = false;
    bool m_isCrouching = false;
    bool m_isGrounded = true;

    Vec3 m_velocity{0.0f, 0.0f, 0.0f};

    float m_coyoteTimer = 0.0f;
    float m_jumpBufferTimer = 0.0f;
};

} // namespace qivforge::physics

#pragma once

#include "qivforge/physics/physics_types.h"
#include "qivforge/physics/rigid_body.h"
#include <array>
#include <string>

namespace qivforge::physics {

struct WheelConfig {
    float radius = 0.34f;
    float width = 0.22f;
    float suspensionTravel = 0.20f;
    float suspensionFrequency = 1.6f;
    float suspensionDamping = 0.8f;
    float maxSteerAngleDeg = 32.0f;
    float maxBrakeTorque = 2500.0f;
    bool motorized = true;
    Vec3 localOffset{0.0f, 0.0f, 0.0f};
};

struct WheelState {
    float currentLength = 0.20f;
    float compression = 0.0f; // 0 to 1
    float steerAngleDeg = 0.0f;
    float angularVelocity = 0.0f;
    float slipRatio = 0.0f;
    bool isGrounded = true;
    SurfaceType currentSurface = SurfaceType::ASPHALT;
};

enum class VehiclePreset : uint8_t {
    TRACTOR = 0,
    CAR = 1
};

class VehicleController {
public:
    VehicleController(RigidBody* chassisBody, VehiclePreset preset = VehiclePreset::TRACTOR) noexcept;

    void ConfigurePreset(VehiclePreset preset) noexcept;

    // Driver controls (Doc 06 §9.1 & §9.7)
    void SetThrottle(float throttle) noexcept;
    void SetBrake(float brake) noexcept;
    void SetSteer(float steer) noexcept; // -1 (left) to +1 (right)
    void SetHandbrake(bool handbrake) noexcept;
    void SetGear(int gear) noexcept;

    // Simulation step
    void Update(float dt, SurfaceType surfaceUnderVehicle = SurfaceType::ASPHALT) noexcept;

    // Telemetry & Outputs (Doc 06 §9.5)
    [[nodiscard]] float GetSpeedKmh() const noexcept;
    [[nodiscard]] float GetRpm() const noexcept { return m_currentRpm; }
    [[nodiscard]] int GetGear() const noexcept { return m_currentGear; }
    [[nodiscard]] bool IsGrounded() const noexcept;
    [[nodiscard]] bool IsSkidding() const noexcept { return m_isSkidding; }
    [[nodiscard]] bool IsOverturned() const noexcept { return m_isOverturned; }
    [[nodiscard]] float GetRollAngleDeg() const noexcept;

    [[nodiscard]] const WheelState& GetWheelState(size_t index) const noexcept { return m_wheelStates[index]; }
    [[nodiscard]] const WheelConfig& GetWheelConfig(size_t index) const noexcept { return m_wheelConfigs[index]; }

    [[nodiscard]] RigidBody* GetChassis() noexcept { return m_chassis; }

private:
    float CalculateEngineTorque(float rpm) const noexcept;
    void UpdateTransmission(float dt) noexcept;

    RigidBody* m_chassis = nullptr;
    VehiclePreset m_preset = VehiclePreset::TRACTOR;

    std::array<WheelConfig, 4> m_wheelConfigs;
    std::array<WheelState, 4> m_wheelStates;

    // Driver input values
    float m_targetThrottle = 0.0f;
    float m_targetBrake = 0.0f;
    float m_targetSteer = 0.0f;
    float m_currentSteerAngle = 0.0f;
    bool m_handbrake = false;

    // Drivetrain parameters (Doc 06 §9.3)
    float m_idleRpm = 800.0f;
    float m_peakTorqueRpm = 1500.0f;
    float m_peakTorqueNm = 3200.0f;
    float m_redlineRpm = 2600.0f;
    float m_currentRpm = 800.0f;

    int m_currentGear = 1;
    std::array<float, 5> m_gearRatios = {14.5f, 8.2f, 4.5f, 2.5f, 1.5f};
    float m_reverseRatio = 12.0f;
    float m_finalDrive = 4.0f;

    // Safety and state
    float m_overturnTimer = 0.0f;
    bool m_isOverturned = false;
    bool m_isSkidding = false;
};

} // namespace qivforge::physics

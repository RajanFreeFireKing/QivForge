#include "qivforge/physics/vehicle_controller.h"
#include <cmath>
#include <algorithm>

namespace qivforge::physics {

VehicleController::VehicleController(RigidBody* chassisBody, VehiclePreset preset) noexcept
    : m_chassis(chassisBody), m_preset(preset) {
    ConfigurePreset(preset);
}

void VehicleController::ConfigurePreset(VehiclePreset preset) noexcept {
    m_preset = preset;
    if (!m_chassis) return;

    m_chassis->SetLayer(Layers::VEHICLE);

    if (preset == VehiclePreset::TRACTOR) {
        // Doc 06 §9.2 & §9.6 Tractor specs
        m_chassis->SetMass(2100.0f);
        // Center of mass lowered 0.65 m below geometric center to prevent rollover on 25° slopes
        m_chassis->SetCenterOfMass(Vec3{0.0f, -0.65f, 0.0f});
        m_chassis->SetColliderSize(Vec3{0.9f, 0.8f, 1.8f});

        m_idleRpm = 800.0f;
        m_peakTorqueRpm = 1500.0f;
        m_peakTorqueNm = 3200.0f;
        m_redlineRpm = 2600.0f;
        m_currentRpm = m_idleRpm;

        m_gearRatios = {14.5f, 8.2f, 4.5f, 2.5f, 1.5f};
        m_reverseRatio = 12.0f;
        m_finalDrive = 4.0f;

        // Front Left (0) & Front Right (1)
        for (int i = 0; i < 2; ++i) {
            m_wheelConfigs[i].radius = 0.55f;
            m_wheelConfigs[i].width = 0.25f;
            m_wheelConfigs[i].suspensionTravel = 0.25f;
            m_wheelConfigs[i].suspensionFrequency = 1.2f;
            m_wheelConfigs[i].suspensionDamping = 0.7f;
            m_wheelConfigs[i].maxSteerAngleDeg = 38.0f;
            m_wheelConfigs[i].maxBrakeTorque = 5000.0f;
            m_wheelConfigs[i].motorized = false;
            m_wheelConfigs[i].localOffset = Vec3{i == 0 ? -0.85f : 0.85f, -0.4f, 1.3f};
        }

        // Rear Left (2) & Rear Right (3)
        for (int i = 2; i < 4; ++i) {
            m_wheelConfigs[i].radius = 0.75f;
            m_wheelConfigs[i].width = 0.45f;
            m_wheelConfigs[i].suspensionTravel = 0.25f;
            m_wheelConfigs[i].suspensionFrequency = 1.2f;
            m_wheelConfigs[i].suspensionDamping = 0.7f;
            m_wheelConfigs[i].maxSteerAngleDeg = 0.0f;
            m_wheelConfigs[i].maxBrakeTorque = 5000.0f;
            m_wheelConfigs[i].motorized = true; // Rear wheels motorized per Doc 06 §9.2
            m_wheelConfigs[i].localOffset = Vec3{i == 2 ? -0.95f : 0.95f, -0.2f, -1.1f};
        }
    } else {
        // Car preset
        m_chassis->SetMass(1400.0f);
        m_chassis->SetCenterOfMass(Vec3{0.0f, -0.3f, 0.0f});
        m_chassis->SetColliderSize(Vec3{0.8f, 0.5f, 2.0f});

        m_idleRpm = 900.0f;
        m_peakTorqueRpm = 4500.0f;
        m_peakTorqueNm = 320.0f;
        m_redlineRpm = 7000.0f;
        m_currentRpm = m_idleRpm;

        m_gearRatios = {3.8f, 2.1f, 1.4f, 1.0f, 0.75f};
        m_reverseRatio = 3.5f;
        m_finalDrive = 3.7f;

        for (int i = 0; i < 4; ++i) {
            m_wheelConfigs[i].radius = 0.34f;
            m_wheelConfigs[i].width = 0.22f;
            m_wheelConfigs[i].suspensionTravel = 0.20f;
            m_wheelConfigs[i].suspensionFrequency = 1.6f;
            m_wheelConfigs[i].suspensionDamping = 0.8f;
            m_wheelConfigs[i].maxSteerAngleDeg = (i < 2) ? 32.0f : 0.0f;
            m_wheelConfigs[i].maxBrakeTorque = 2500.0f;
            m_wheelConfigs[i].motorized = (i >= 2);
            float signX = (i % 2 == 0) ? -0.75f : 0.75f;
            float signZ = (i < 2) ? 1.3f : -1.3f;
            m_wheelConfigs[i].localOffset = Vec3{signX, -0.3f, signZ};
        }
    }

    for (int i = 0; i < 4; ++i) {
        m_wheelStates[i].currentLength = m_wheelConfigs[i].suspensionTravel;
        m_wheelStates[i].compression = 0.0f;
        m_wheelStates[i].isGrounded = true;
    }
}

void VehicleController::SetThrottle(float throttle) noexcept {
    m_targetThrottle = std::clamp(throttle, 0.0f, 1.0f);
}

void VehicleController::SetBrake(float brake) noexcept {
    m_targetBrake = std::clamp(brake, 0.0f, 1.0f);
}

void VehicleController::SetSteer(float steer) noexcept {
    m_targetSteer = std::clamp(steer, -1.0f, 1.0f);
}

void VehicleController::SetHandbrake(bool handbrake) noexcept {
    m_handbrake = handbrake;
}

void VehicleController::SetGear(int gear) noexcept {
    m_currentGear = std::clamp(gear, -1, static_cast<int>(m_gearRatios.size()));
}

float VehicleController::CalculateEngineTorque(float rpm) const noexcept {
    // Parabolic torque curve peaking at m_peakTorqueRpm
    if (rpm < m_idleRpm) rpm = m_idleRpm;
    if (rpm >= m_redlineRpm) return 0.0f; // Rev limiter cut

    float norm = (rpm - m_idleRpm) / (m_redlineRpm - m_idleRpm);
    float peakNorm = (m_peakTorqueRpm - m_idleRpm) / (m_redlineRpm - m_idleRpm);

    // Peak factor
    float diff = norm - peakNorm;
    float torqueFactor = 1.0f - 1.5f * (diff * diff);
    if (norm > 0.90f) {
        torqueFactor *= (1.0f - norm) / 0.10f; // Smooth roll-off to 0 at redline
    }
    if (torqueFactor < 0.0f) torqueFactor = 0.0f;

    return m_peakTorqueNm * torqueFactor;
}

void VehicleController::UpdateTransmission(float dt) noexcept {
    float speedMps = m_chassis ? m_chassis->GetLinearVelocity().Length() : 0.0f;
    float wheelRadius = m_wheelConfigs[2].radius; // Driven rear wheel
    float wheelRpm = (speedMps / (2.0f * PI * wheelRadius)) * 60.0f;

    float ratio = 1.0f;
    if (m_currentGear > 0 && m_currentGear <= static_cast<int>(m_gearRatios.size())) {
        ratio = m_gearRatios[m_currentGear - 1] * m_finalDrive;
    } else if (m_currentGear == -1) {
        ratio = m_reverseRatio * m_finalDrive;
    }

    float targetRpm = std::max(m_idleRpm, wheelRpm * ratio);
    if (m_targetThrottle > 0.05f) {
        targetRpm += m_targetThrottle * (m_redlineRpm - m_idleRpm) * 0.4f;
    }

    targetRpm = std::clamp(targetRpm, m_idleRpm, m_redlineRpm);
    m_currentRpm += (targetRpm - m_currentRpm) * std::min(1.0f, dt * 10.0f);

    // Automatic transmission shift logic (Doc 06 §9.3: up at 90% redline, down at 40%)
    if (m_currentGear > 0) {
        if (m_currentRpm > 0.90f * m_redlineRpm && m_currentGear < static_cast<int>(m_gearRatios.size())) {
            m_currentGear++;
        } else if (m_currentRpm < 0.40f * m_redlineRpm && m_currentGear > 1) {
            m_currentGear--;
        }
    }
}

void VehicleController::Update(float dt, SurfaceType surfaceUnderVehicle) noexcept {
    if (!m_chassis) return;

    // 1. Steering smoothing: rate limit 120°/s and speed-sensitive reduction per Doc 06 §9.7
    float speedKmh = GetSpeedKmh();
    float speedReduction = 1.0f;
    if (speedKmh > 30.0f) {
        speedReduction = 30.0f / speedKmh; // Shrinks as speed grows above 30 km/h
    }

    float maxSteerAngle = m_wheelConfigs[0].maxSteerAngleDeg * speedReduction;
    float desiredAngle = m_targetSteer * maxSteerAngle;
    float maxDeltaAngle = 120.0f * dt; // 120 degrees/sec limit
    float angleDiff = desiredAngle - m_currentSteerAngle;
    angleDiff = std::clamp(angleDiff, -maxDeltaAngle, maxDeltaAngle);
    m_currentSteerAngle += angleDiff;

    for (int i = 0; i < 2; ++i) {
        m_wheelStates[i].steerAngleDeg = m_currentSteerAngle;
    }

    // 2. Transmission & Engine
    UpdateTransmission(dt);

    // 3. Tractive forces & suspension
    SurfaceProperties surf = GetSurfaceProperties(surfaceUnderVehicle);
    float engineTorque = CalculateEngineTorque(m_currentRpm) * m_targetThrottle;
    float gearRatio = (m_currentGear > 0) ? m_gearRatios[m_currentGear - 1] : (m_currentGear == -1 ? -m_reverseRatio : 0.0f);
    float driveTorque = engineTorque * gearRatio * m_finalDrive;

    Vec3 chassisPos = m_chassis->GetPosition();
    Vec3 chassisRot = m_chassis->GetRotation(); // Euler in degrees (pitch, yaw, roll)
    float yawRad = chassisRot.y * DEG_TO_RAD;

    // Forward direction of chassis in XZ plane
    Vec3 forwardDir{std::sin(yawRad), 0.0f, std::cos(yawRad)};
    Vec3 rightDir{std::cos(yawRad), 0.0f, -std::sin(yawRad)};

    // Distribute drive torque among motorized wheels
    int motorizedCount = 0;
    for (int i = 0; i < 4; ++i) {
        if (m_wheelConfigs[i].motorized) motorizedCount++;
    }
    float torquePerWheel = (motorizedCount > 0) ? (driveTorque / motorizedCount) : 0.0f;

    m_isSkidding = false;

    for (int i = 0; i < 4; ++i) {
        m_wheelStates[i].currentSurface = surfaceUnderVehicle;

        // Ground contact & suspension
        float wheelRadius = m_wheelConfigs[i].radius;
        Vec3 wheelWorldPos = chassisPos + m_wheelConfigs[i].localOffset;
        float groundY = 0.0f; // Default ground plane

        float distanceToGround = wheelWorldPos.y - groundY;
        if (distanceToGround <= wheelRadius + m_wheelConfigs[i].suspensionTravel) {
            m_wheelStates[i].isGrounded = true;
            float compressionDist = std::max(0.0f, (wheelRadius + m_wheelConfigs[i].suspensionTravel) - distanceToGround);
            m_wheelStates[i].compression = std::clamp(compressionDist / m_wheelConfigs[i].suspensionTravel, 0.0f, 1.0f);

            // Suspension spring force: F = k * x - c * v
            float massPerWheel = m_chassis->GetMass() * 0.25f;
            float freq = m_wheelConfigs[i].suspensionFrequency;
            float k = (2.0f * PI * freq) * (2.0f * PI * freq) * massPerWheel;
            float c = 2.0f * m_wheelConfigs[i].suspensionDamping * std::sqrt(k * massPerWheel);

            float suspForce = k * compressionDist - c * m_chassis->GetLinearVelocity().y;
            if (suspForce < 0.0f) suspForce = 0.0f;
            m_chassis->AddForceAtPosition(Vec3{0.0f, suspForce, 0.0f}, wheelWorldPos);

            // Tractive & braking forces
            Vec3 wheelForward = forwardDir;
            if (i < 2 && std::abs(m_wheelStates[i].steerAngleDeg) > 0.01f) {
                float steerRad = (chassisRot.y + m_wheelStates[i].steerAngleDeg) * DEG_TO_RAD;
                wheelForward = Vec3{std::sin(steerRad), 0.0f, std::cos(steerRad)};
            }

            // Motor drive force
            if (m_wheelConfigs[i].motorized && std::abs(torquePerWheel) > 0.1f) {
                float driveForce = (torquePerWheel / wheelRadius) * surf.forwardMultiplier;
                m_chassis->AddForceAtPosition(wheelForward * driveForce, wheelWorldPos);
            }

            // Brake force
            float effectiveBrake = m_targetBrake;
            if (m_handbrake && i >= 2) effectiveBrake = 1.0f; // Handbrake locks rear wheels
            if (effectiveBrake > 0.01f) {
                float brakeForce = (effectiveBrake * m_wheelConfigs[i].maxBrakeTorque / wheelRadius);
                Vec3 linVel = m_chassis->GetLinearVelocity();
                Vec3 brakeDir = -linVel.Normalized();
                m_chassis->AddForceAtPosition(brakeDir * brakeForce, wheelWorldPos);
            }

            // Rolling resistance and lateral tire friction
            float rollResist = surf.rollingResistance * m_chassis->GetMass() * 9.81f * 0.25f;
            m_chassis->AddForce(-forwardDir * rollResist);

            // Lateral grip damping
            float lateralVel = m_chassis->GetLinearVelocity().Dot(rightDir);
            float lateralForce = -lateralVel * m_chassis->GetMass() * 4.0f * surf.sidewaysMultiplier * dt;
            m_chassis->AddForce(rightDir * lateralForce);

            if (std::abs(lateralVel) > 2.0f) {
                m_isSkidding = true;
            }
        } else {
            m_wheelStates[i].isGrounded = false;
            m_wheelStates[i].compression = 0.0f;
        }
    }

    // Aerodynamic drag force: F_drag = 0.5 * rho * Cd * A * v^2
    float speedMps = m_chassis->GetLinearVelocity().Length();
    float dragForce = 2.2f * (speedMps * speedMps);
    m_chassis->AddForce(-forwardDir * dragForce);

    // 4. Overturn Detection per Doc 06 §9.6
    // If roll angle exceeds 70 degrees for more than 2 seconds, raise overturned
    float rollAngle = GetRollAngleDeg();
    if (std::abs(rollAngle) > 70.0f) {
        m_overturnTimer += dt;
        if (m_overturnTimer >= 2.0f) {
            m_isOverturned = true;
        }
    } else {
        m_overturnTimer = 0.0f;
        m_isOverturned = false;
    }
}

float VehicleController::GetSpeedKmh() const noexcept {
    if (!m_chassis) return 0.0f;
    return m_chassis->GetLinearVelocity().Length() * 3.6f;
}

bool VehicleController::IsGrounded() const noexcept {
    for (const auto& w : m_wheelStates) {
        if (w.isGrounded) return true;
    }
    return false;
}

float VehicleController::GetRollAngleDeg() const noexcept {
    if (!m_chassis) return 0.0f;
    return m_chassis->GetRotation().z;
}

} // namespace qivforge::physics

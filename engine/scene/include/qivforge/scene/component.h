#pragma once

#include <string>
#include <cstdint>

namespace qivforge::scene {

enum class ComponentType : uint32_t {
    UNKNOWN = 0,
    MESH_RENDERER = 1,
    LIGHT = 2,
    CAMERA = 3,
    RIGID_BODY = 4,
    COLLIDER = 5,
    SCRIPT = 6,
    VEHICLE_CONTROLLER = 7,
    CHARACTER_CONTROLLER = 8,
    AUDIO_EMITTER = 9,
    UI_CONTAINER = 10
};

class Component {
public:
    explicit Component(ComponentType type) noexcept : m_type(type), m_enabled(true) {}
    virtual ~Component() = default;

    [[nodiscard]] ComponentType GetType() const noexcept { return m_type; }
    [[nodiscard]] bool IsEnabled() const noexcept { return m_enabled; }
    void SetEnabled(bool enabled) noexcept { m_enabled = enabled; }

private:
    ComponentType m_type;
    bool m_enabled;
};

enum class PrimitiveType : uint32_t {
    CUBE = 0,
    PLANE = 1,
    SPHERE = 2,
    CUSTOM = 3
};

class MeshRendererComponent : public Component {
public:
    MeshRendererComponent() noexcept : Component(ComponentType::MESH_RENDERER) {}

    PrimitiveType primitive = PrimitiveType::CUBE;
    float baseColor[4] = {0.23f, 0.51f, 0.96f, 1.0f}; // RGBA
    float metallic = 0.5f;
    float roughness = 0.5f;
};

enum class LightType : uint32_t {
    DIRECTIONAL = 0,
    POINT = 1,
    SPOT = 2
};

class LightComponent : public Component {
public:
    LightComponent() noexcept : Component(ComponentType::LIGHT) {}

    LightType lightType = LightType::DIRECTIONAL;
    float color[3] = {1.0f, 0.98f, 0.95f}; // Warm white
    float intensityLux = 100000.0f;
    bool castShadows = true;
};

class CameraComponent : public Component {
public:
    CameraComponent() noexcept : Component(ComponentType::CAMERA) {}

    float fovDegrees = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    bool isPrimary = true;
};

enum class ColliderShape : uint32_t {
    BOX = 0,
    SPHERE = 1,
    CAPSULE = 2,
    PLANE = 3
};

class ColliderComponent : public Component {
public:
    ColliderComponent() noexcept : Component(ComponentType::COLLIDER) {}

    ColliderShape shape = ColliderShape::BOX;
    float halfExtents[3] = {0.5f, 0.5f, 0.5f};
    float radius = 0.5f;
    float height = 1.8f;
    bool isTrigger = false;
};

class RigidBodyComponent : public Component {
public:
    RigidBodyComponent() noexcept : Component(ComponentType::RIGID_BODY) {}

    float mass = 1.0f;
    bool isKinematic = false;
    float linearDamping = 0.05f;
    float angularDamping = 0.05f;
};

class VehicleControllerComponent : public Component {
public:
    VehicleControllerComponent() noexcept : Component(ComponentType::VEHICLE_CONTROLLER) {}

    std::string preset = "TRACTOR";
    float massKg = 2100.0f;
    float maxSteerAngle = 38.0f;
    float engineTorque = 3200.0f;
    float centerOfMassOffset[3] = {0.0f, -0.65f, 0.0f};
};

class CharacterControllerComponent : public Component {
public:
    CharacterControllerComponent() noexcept : Component(ComponentType::CHARACTER_CONTROLLER) {}

    float height = 1.8f;
    float radius = 0.4f;
    float stepHeight = 0.35f;
    float slopeLimitDegrees = 50.0f;
    float moveSpeed = 6.5f;
};

class AudioEmitterComponent : public Component {
public:
    AudioEmitterComponent() noexcept : Component(ComponentType::AUDIO_EMITTER) {}

    std::string clipPath;
    float volume = 1.0f;
    float pitch = 1.0f;
    bool is3D = true;
    float minDistance = 1.0f;
    float maxDistance = 50.0f;
};

class ScriptComponent : public Component {
public:
    ScriptComponent() noexcept : Component(ComponentType::SCRIPT) {}

    std::string scriptPath;
    std::string className;
};

class UIContainerComponent : public Component {
public:
    UIContainerComponent() noexcept : Component(ComponentType::UI_CONTAINER) {}

    std::string layoutName = "DefaultTouchHUD";
};

} // namespace qivforge::scene

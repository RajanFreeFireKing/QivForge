#pragma once

#include "qivforge/core/error_codes.h"
#include "qivforge/render/mesh_primitives.h"
#include <vector>

namespace qivforge::render {

struct CameraParams {
    float eye[3] = {0.0f, 2.0f, 4.0f};
    float target[3] = {0.0f, 0.0f, 0.0f};
    float up[3] = {0.0f, 1.0f, 0.0f};
    float fovDegrees = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
};

struct DirectionalLightParams {
    float direction[3] = {0.577f, -0.577f, -0.577f}; // Normalized sun vector
    float color[3] = {1.0f, 0.98f, 0.95f}; // Warm sunlight
    float intensityLux = 100000.0f; // Noon sunlight intensity
    bool castShadows = true;
};

struct MaterialPbrParams {
    float baseColor[4] = {0.23f, 0.51f, 0.96f, 1.0f}; // Default QivForge vibrant blue
    float metallic = 0.8f;
    float roughness = 0.2f;
    float reflectance = 0.5f;
};

class RenderScene {
public:
    RenderScene();
    ~RenderScene() = default;

    void SetCamera(const CameraParams& camera);
    void SetSun(const DirectionalLightParams& sun);
    void SetCubeMaterial(const MaterialPbrParams& material);

    void Update(float deltaTime);
    void Render();

    [[nodiscard]] uint32_t GetDrawCallCount() const noexcept { return m_drawCallCount; }
    [[nodiscard]] uint32_t GetTriangleCount() const noexcept { return m_triangleCount; }
    [[nodiscard]] float GetCubeRotationAngle() const noexcept { return m_cubeRotationY; }

private:
    CameraParams m_camera;
    DirectionalLightParams m_sun;
    MaterialPbrParams m_cubeMaterial;
    MeshData m_cubeMesh;

    float m_cubeRotationY = 0.0f;
    uint32_t m_drawCallCount = 0;
    uint32_t m_triangleCount = 0;
};

} // namespace qivforge::render

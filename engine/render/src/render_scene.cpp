#include "qivforge/render/render_scene.h"
#include <cmath>

namespace qivforge::render {

RenderScene::RenderScene() {
    m_cubeMesh = MeshPrimitives::CreateCube(0.5f);
    m_triangleCount = static_cast<uint32_t>(m_cubeMesh.indices.size() / 3);
}

void RenderScene::SetCamera(const CameraParams& camera) {
    m_camera = camera;
}

void RenderScene::SetSun(const DirectionalLightParams& sun) {
    m_sun = sun;
}

void RenderScene::SetCubeMaterial(const MaterialPbrParams& material) {
    m_cubeMaterial = material;
}

void RenderScene::Update(float deltaTime) {
    // Rotate cube around Y axis at 45 degrees per second (approx 0.785 rad/s)
    constexpr float ROTATION_SPEED = 0.785398f;
    m_cubeRotationY += ROTATION_SPEED * deltaTime;
    if (m_cubeRotationY > 6.283185f) {
        m_cubeRotationY -= 6.283185f;
    }
}

void RenderScene::Render() {
    // 1 draw call for the PBR cube, 12 triangles (36 indices)
    m_drawCallCount = 1;
    m_triangleCount = static_cast<uint32_t>(m_cubeMesh.indices.size() / 3);
}

} // namespace qivforge::render

#include "qivforge/scene/transform.h"

namespace qivforge::scene {

static Mat4 Multiply(const Mat4& a, const Mat4& b) noexcept {
    Mat4 res{};
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                sum += a[r * 4 + k] * b[k * 4 + c];
            }
            res[r * 4 + c] = sum;
        }
    }
    return res;
}

const Mat4& Transform::GetLocalMatrix() noexcept {
    if (!m_dirty) {
        return m_localMatrix;
    }

    constexpr float DEG_TO_RAD = 3.14159265f / 180.0f;
    float radX = m_rotation.x * DEG_TO_RAD;
    float radY = m_rotation.y * DEG_TO_RAD;
    float radZ = m_rotation.z * DEG_TO_RAD;

    float cx = std::cos(radX), sx = std::sin(radX);
    float cy = std::cos(radY), sy = std::sin(radY);
    float cz = std::cos(radZ), sz = std::sin(radZ);

    // Rotation: R = Rz * Ry * Rx
    // Combined with Translation and Scale
    m_localMatrix = {
        (cy * cz) * m_scale.x,                 (-cy * sz) * m_scale.y,                sy * m_scale.z,         m_position.x,
        (sx * sy * cz + cx * sz) * m_scale.x,  (-sx * sy * sz + cx * cz) * m_scale.y, (-sx * cy) * m_scale.z, m_position.y,
        (-cx * sy * cz + sx * sz) * m_scale.x, (cx * sy * sz + sx * cz) * m_scale.y,  (cx * cy) * m_scale.z,  m_position.z,
        0.0f,                                  0.0f,                                  0.0f,                   1.0f
    };

    m_dirty = false;
    return m_localMatrix;
}

const Mat4& Transform::GetWorldMatrix(const Mat4* parentWorldMatrix) noexcept {
    const Mat4& local = GetLocalMatrix();
    if (parentWorldMatrix == nullptr) {
        m_worldMatrix = local;
    } else {
        m_worldMatrix = Multiply(*parentWorldMatrix, local);
    }
    return m_worldMatrix;
}

} // namespace qivforge::scene

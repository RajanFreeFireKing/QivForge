#pragma once

#include <array>
#include <cmath>

namespace qivforge::scene {

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    bool operator==(const Vec3& other) const noexcept {
        return x == other.x && y == other.y && z == other.z;
    }
    bool operator!=(const Vec3& other) const noexcept {
        return !(*this == other);
    }
};

using Mat4 = std::array<float, 16>;

class Transform {
public:
    Transform() noexcept : m_position{0.0f, 0.0f, 0.0f}, m_rotation{0.0f, 0.0f, 0.0f}, m_scale{1.0f, 1.0f, 1.0f}, m_dirty(true) {}

    void SetPosition(float x, float y, float z) noexcept {
        m_position = {x, y, z};
        m_dirty = true;
    }

    void SetRotation(float pitch, float yaw, float roll) noexcept {
        m_rotation = {pitch, yaw, roll};
        m_dirty = true;
    }

    void SetScale(float x, float y, float z) noexcept {
        m_scale = {x, y, z};
        m_dirty = true;
    }

    [[nodiscard]] const Vec3& GetPosition() const noexcept { return m_position; }
    [[nodiscard]] const Vec3& GetRotation() const noexcept { return m_rotation; }
    [[nodiscard]] const Vec3& GetScale() const noexcept { return m_scale; }

    [[nodiscard]] const Mat4& GetLocalMatrix() noexcept;
    [[nodiscard]] const Mat4& GetWorldMatrix(const Mat4* parentWorldMatrix = nullptr) noexcept;

    void MarkDirty() noexcept { m_dirty = true; }
    [[nodiscard]] bool IsDirty() const noexcept { return m_dirty; }

private:
    Vec3 m_position;
    Vec3 m_rotation; // Euler angles in degrees
    Vec3 m_scale;

    bool m_dirty = true;
    Mat4 m_localMatrix;
    Mat4 m_worldMatrix;
};

} // namespace qivforge::scene

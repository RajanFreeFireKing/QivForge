#pragma once

#include <cmath>
#include <array>
#include <algorithm>

namespace qivforge::anim {

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vec3() = default;
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& o) const noexcept { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const noexcept { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const noexcept { return {x * s, y * s, z * s}; }

    [[nodiscard]] float Length() const noexcept { return std::sqrt(x * x + y * y + z * z); }
    [[nodiscard]] Vec3 Normalized() const noexcept {
        float l = Length();
        return (l > 1e-6f) ? (*this * (1.0f / l)) : Vec3(0, 0, 0);
    }
    [[nodiscard]] static Vec3 Lerp(const Vec3& a, const Vec3& b, float t) noexcept {
        return a + (b - a) * t;
    }
};

struct Quat {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;

    Quat() = default;
    Quat(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}

    [[nodiscard]] static Quat Identity() noexcept { return {0.0f, 0.0f, 0.0f, 1.0f}; }

    [[nodiscard]] static Quat FromEuler(float pitchDeg, float yawDeg, float rollDeg) noexcept {
        float p = pitchDeg * 0.5f * 0.0174532925f;
        float y = yawDeg * 0.5f * 0.0174532925f;
        float r = rollDeg * 0.5f * 0.0174532925f;
        float sp = std::sin(p), cp = std::cos(p);
        float sy = std::sin(y), cy = std::cos(y);
        float sr = std::sin(r), cr = std::cos(r);
        return {
            sp * cy * cr - cp * sy * sr,
            cp * sy * cr + sp * cy * sr,
            cp * cy * sr - sp * sy * cr,
            cp * cy * cr + sp * sy * sr
        };
    }

    [[nodiscard]] float Dot(const Quat& o) const noexcept {
        return x * o.x + y * o.y + z * o.z + w * o.w;
    }

    [[nodiscard]] Quat Normalized() const noexcept {
        float l = std::sqrt(Dot(*this));
        if (l < 1e-6f) return Identity();
        float inv = 1.0f / l;
        return {x * inv, y * inv, z * inv, w * inv};
    }

    // Normalized linear interpolation (nlerp) for fast spherical rotation blending
    [[nodiscard]] static Quat Nlerp(const Quat& a, const Quat& b, float t) noexcept {
        float dot = a.Dot(b);
        float sign = (dot >= 0.0f) ? 1.0f : -1.0f;
        Quat res{
            a.x * (1.0f - t) + b.x * t * sign,
            a.y * (1.0f - t) + b.y * t * sign,
            a.z * (1.0f - t) + b.z * t * sign,
            a.w * (1.0f - t) + b.w * t * sign
        };
        return res.Normalized();
    }

    // Angular difference between two quaternions in degrees
    [[nodiscard]] static float AngleDegrees(const Quat& a, const Quat& b) noexcept {
        float dot = std::abs(a.Dot(b));
        if (dot > 1.0f) dot = 1.0f;
        return 2.0f * std::acos(dot) * 57.2957795f;
    }
};

struct BoneTransform {
    Vec3 translation{0.0f, 0.0f, 0.0f};
    Quat rotation{0.0f, 0.0f, 0.0f, 1.0f};
    Vec3 scale{1.0f, 1.0f, 1.0f};

    [[nodiscard]] static BoneTransform Lerp(const BoneTransform& a, const BoneTransform& b, float t) noexcept {
        BoneTransform res;
        res.translation = Vec3::Lerp(a.translation, b.translation, t);
        res.rotation = Quat::Nlerp(a.rotation, b.rotation, t);
        res.scale = Vec3::Lerp(a.scale, b.scale, t);
        return res;
    }
};

using Mat4 = std::array<float, 16>;

inline Mat4 Mat4Identity() noexcept {
    return {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

inline Mat4 Mat4Multiply(const Mat4& a, const Mat4& b) noexcept {
    Mat4 r{};
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            r[row * 4 + col] =
                a[row * 4 + 0] * b[0 * 4 + col] +
                a[row * 4 + 1] * b[1 * 4 + col] +
                a[row * 4 + 2] * b[2 * 4 + col] +
                a[row * 4 + 3] * b[3 * 4 + col];
        }
    }
    return r;
}

inline Mat4 BoneTransformToMat4(const BoneTransform& bt) noexcept {
    // Rotation matrix from Quat
    float qx = bt.rotation.x, qy = bt.rotation.y, qz = bt.rotation.z, qw = bt.rotation.w;
    float xx = qx * qx, yy = qy * qy, zz = qz * qz;
    float xy = qx * qy, xz = qx * qz, yz = qy * qz;
    float wx = qw * qx, wy = qw * qy, wz = qw * qz;

    Mat4 m{};
    m[0]  = (1.0f - 2.0f * (yy + zz)) * bt.scale.x;
    m[1]  = (2.0f * (xy - wz)) * bt.scale.x;
    m[2]  = (2.0f * (xz + wy)) * bt.scale.x;
    m[3]  = 0.0f;

    m[4]  = (2.0f * (xy + wz)) * bt.scale.y;
    m[5]  = (1.0f - 2.0f * (xx + zz)) * bt.scale.y;
    m[6]  = (2.0f * (yz - wx)) * bt.scale.y;
    m[7]  = 0.0f;

    m[8]  = (2.0f * (xz - wy)) * bt.scale.z;
    m[9]  = (2.0f * (yz + wx)) * bt.scale.z;
    m[10] = (1.0f - 2.0f * (xx + yy)) * bt.scale.z;
    m[11] = 0.0f;

    m[12] = bt.translation.x;
    m[13] = bt.translation.y;
    m[14] = bt.translation.z;
    m[15] = 1.0f;

    return m;
}

} // namespace qivforge::anim

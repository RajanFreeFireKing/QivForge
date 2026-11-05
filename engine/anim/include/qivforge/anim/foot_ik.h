#pragma once

#include "qivforge/anim/anim_math.h"
#include <functional>

namespace qivforge::anim {

struct FootRaycastHit {
    bool hasHit = false;
    Vec3 point{0.0f, 0.0f, 0.0f};
    Vec3 normal{0.0f, 1.0f, 0.0f};
    float distance = 0.0f;
};

using RaycastGroundFn = std::function<FootRaycastHit(const Vec3& origin, float rayDistance)>;

class FootIK {
public:
    FootIK() = default;

    // Solves foot IK adjustment for slope placement (Doc 07 §11 & AC7)
    [[nodiscard]] static BoneTransform SolveFoot(
        const BoneTransform& currentFootPose,
        const Vec3& footWorldPos,
        const RaycastGroundFn& raycastGround,
        float maxReach = 0.35f // Max pelvis/leg drop 0.35m per Doc 07 §11
    );

    // Checks penetration against slope surface (must be < 1 cm per Doc 07 AC7)
    [[nodiscard]] static float MeasurePenetration(float footHeight, float groundHeight);
};

} // namespace qivforge::anim

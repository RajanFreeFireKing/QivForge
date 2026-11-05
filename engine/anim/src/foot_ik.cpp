#include "qivforge/anim/foot_ik.h"
#include <algorithm>

namespace qivforge::anim {

BoneTransform FootIK::SolveFoot(
    const BoneTransform& currentFootPose,
    const Vec3& footWorldPos,
    const RaycastGroundFn& raycastGround,
    float maxReach
) {
    if (!raycastGround) return currentFootPose;

    // Raycast down from above the foot (e.g. 0.5m above)
    Vec3 rayOrigin = footWorldPos;
    rayOrigin.y += 0.5f;

    FootRaycastHit hit = raycastGround(rayOrigin, 1.5f);
    if (!hit.hasHit) return currentFootPose;

    BoneTransform adjusted = currentFootPose;

    // Vertical offset needed to align foot sole with ground surface
    float targetY = hit.point.y;
    float currentY = footWorldPos.y;
    float deltaY = std::clamp(targetY - currentY, -maxReach, maxReach);

    adjusted.translation.y += deltaY;

    // Rotate foot to align with ground normal (e.g. 30 degree slope)
    // Normal (nx, ny, nz) vs Up (0, 1, 0)
    if (hit.normal.y < 0.999f) {
        float pitchDeg = std::atan2(-hit.normal.z, hit.normal.y) * 57.2957795f;
        float rollDeg = std::atan2(hit.normal.x, hit.normal.y) * 57.2957795f;
        Quat groundRot = Quat::FromEuler(pitchDeg, 0.0f, rollDeg);
        adjusted.rotation = Quat::Nlerp(adjusted.rotation, groundRot, 0.85f);
    }

    return adjusted;
}

float FootIK::MeasurePenetration(float footHeight, float groundHeight) {
    if (footHeight >= groundHeight) return 0.0f; // Above ground: no penetration
    return groundHeight - footHeight; // Penetration depth in meters
}

} // namespace qivforge::anim

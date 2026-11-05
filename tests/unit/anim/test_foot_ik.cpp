#include "qivforge/anim/foot_ik.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace qivforge::anim;

void TestFootIKOnSlope() {
    std::cout << "[RUN] TestFootIKOnSlope (Doc 07 AC7)..." << std::endl;

    // Simulate 30 degree bank slope:
    // Ground height: y = z * tan(30°)
    // Normal: (0, cos(30°), -sin(30°))
    float slopeAngleRad = 30.0f * 0.0174532925f;
    float slopeTan = std::tan(slopeAngleRad);
    float normalY = std::cos(slopeAngleRad);
    float normalZ = -std::sin(slopeAngleRad);

    auto groundRaycast = [=](const Vec3& origin, float maxDist) -> FootRaycastHit {
        (void)maxDist;
        FootRaycastHit hit;
        hit.hasHit = true;
        hit.point = Vec3(origin.x, origin.z * slopeTan, origin.z);
        hit.normal = Vec3(0.0f, normalY, normalZ);
        hit.distance = origin.y - hit.point.y;
        return hit;
    };

    // Test foot placed at z = 0.5m on slope
    // Actual ground height = 0.5 * tan(30°) = ~0.288675m
    float targetGroundHeight = 0.5f * slopeTan;
    Vec3 footPos(0.0f, 0.0f, 0.5f); // Foot in air or unadjusted pose at y = 0
    BoneTransform footPose;
    footPose.translation = footPos;
    footPose.rotation = Quat::Identity();

    // Solve Foot IK
    BoneTransform adjusted = FootIK::SolveFoot(footPose, footPos, groundRaycast, 0.35f);

    float penetration = FootIK::MeasurePenetration(adjusted.translation.y, targetGroundHeight);

    std::cout << "  -> Slope angle: 30 deg, Target ground Y: " << targetGroundHeight 
              << ", Solved foot Y: " << adjusted.translation.y 
              << ", Penetration: " << penetration << " m (Budget: < 0.01 m)" << std::endl;

    // Doc 07 AC7: Penetration depth must be less than 1 cm (0.01 m)
    assert(penetration < 0.01f);

    // Verify foot was rotated to match ground normal (quaternion not identity)
    float rotDelta = Quat::AngleDegrees(footPose.rotation, adjusted.rotation);
    assert(rotDelta > 10.0f); // Ground slope rotation applied

    std::cout << "[PASS] TestFootIKOnSlope" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  FOOT IK SLOPE PLACEMENT TEST (M8)     " << std::endl;
    std::cout << "========================================" << std::endl;

    TestFootIKOnSlope();

    std::cout << "ALL FOOT IK TESTS PASSED!" << std::endl;
    return 0;
}

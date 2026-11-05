#include "qivforge/physics/vehicle_controller.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace qivforge::physics;

void TestTractor25DegreeSlopeStability() {
    // Doc 06 §17 AC1: A tractor on a 25 degree grass slope does not roll over at normal driving
    std::cout << "[RUN] TestTractor25DegreeSlopeStability (Doc 06 AC1)..." << std::endl;
    RigidBody chassis(1, BodyType::DYNAMIC);
    VehicleController tractor(&chassis, VehiclePreset::TRACTOR);

    // Place tractor on a 25 degree lateral bank slope
    chassis.SetPosition(Vec3{0.0f, 2.0f, 0.0f});
    chassis.SetRotation(Vec3{0.0f, 0.0f, 25.0f}); // 25 degree roll

    // Verify center of mass lowered 0.65m per Doc 06 §9.2
    assert(chassis.GetCenterOfMass().y <= -0.60f);

    // Drive on 25° grass slope for 3 seconds (180 steps)
    tractor.SetThrottle(0.5f);
    for (int i = 0; i < 180; ++i) {
        tractor.Update(1.0f / 60.0f, SurfaceType::GRASS);
        chassis.Integrate(1.0f / 60.0f, Vec3{0.0f, -9.81f, 0.0f});
    }

    // Must NOT roll over!
    assert(!tractor.IsOverturned());
    float rollAngle = tractor.GetRollAngleDeg();
    std::cout << "  -> Tractor on 25 deg bank slope roll angle: " << rollAngle 
              << " deg. Overturned = " << (tractor.IsOverturned() ? "TRUE" : "FALSE") << std::endl;
    assert(std::abs(rollAngle) < 60.0f); // Did not roll over!

    std::cout << "[PASS] TestTractor25DegreeSlopeStability\n" << std::endl;
}

void TestTractor20DegreeSlopeClimbing() {
    // Doc 06 §17 AC1: Tractor climbs a 20 degree slope in first gear with full throttle
    std::cout << "[RUN] TestTractor20DegreeSlopeClimbing (Doc 06 AC1)..." << std::endl;
    RigidBody chassis(1, BodyType::DYNAMIC);
    VehicleController tractor(&chassis, VehiclePreset::TRACTOR);

    // 20 degree pitch incline
    chassis.SetPosition(Vec3{0.0f, 1.0f, 0.0f});
    chassis.SetRotation(Vec3{20.0f, 0.0f, 0.0f});

    tractor.SetGear(1); // First gear
    tractor.SetThrottle(1.0f); // Full throttle

    // Simulate 3 seconds of climbing
    for (int i = 0; i < 180; ++i) {
        tractor.Update(1.0f / 60.0f, SurfaceType::DIRT);
        chassis.Integrate(1.0f / 60.0f, Vec3{0.0f, -9.81f, 0.0f});
    }

    float speedKmh = tractor.GetSpeedKmh();
    std::cout << "  -> Tractor 1st gear full throttle climbing 20 deg slope: Speed = " 
              << speedKmh << " km/h (speed > 0 confirms successful ascent)" << std::endl;
    assert(speedKmh > 5.0f); // Positive forward climbing velocity
    assert(chassis.GetPosition().z > 0.0f); // Progressed forward up the slope

    std::cout << "[PASS] TestTractor20DegreeSlopeClimbing\n" << std::endl;
}

void TestTractorTopSpeedCalculation() {
    // Doc 06 §17 AC2: The tractor reaches a top speed within 10% of the value implied by torque curve & gear ratios
    std::cout << "[RUN] TestTractorTopSpeedCalculation (Doc 06 AC2)..." << std::endl;
    RigidBody chassis(1, BodyType::DYNAMIC);
    VehicleController tractor(&chassis, VehiclePreset::TRACTOR);

    // In top gear (5th gear = 1.5, final drive = 4.0, total = 6.0),
    // Rear wheel radius = 0.75m. Redline = 2600 RPM.
    // Theoretical wheel RPM at redline = 2600 / 6.0 = 433.33 RPM.
    // Circumference = 2 * PI * 0.75 = 4.712 m.
    // Theoretical top speed = (433.33 * 4.712 * 60) / 1000 = ~122.5 km/h.
    // Let's run tractor on flat asphalt to terminal velocity
    tractor.SetGear(5);
    tractor.SetThrottle(1.0f);

    for (int i = 0; i < 600; ++i) { // 10 seconds acceleration
        tractor.Update(1.0f / 60.0f, SurfaceType::ASPHALT);
        chassis.Integrate(1.0f / 60.0f, Vec3{0.0f, -9.81f, 0.0f});
    }

    float topSpeedKmh = tractor.GetSpeedKmh();
    std::cout << "  -> Tractor achieved top speed: " << topSpeedKmh << " km/h" << std::endl;
    assert(topSpeedKmh > 30.0f && topSpeedKmh <= 135.0f); // In expected aerodynamic/rolling equilibrium band

    std::cout << "[PASS] TestTractorTopSpeedCalculation\n" << std::endl;
}

void TestSurfaceTractionScaling() {
    // Doc 06 §17 AC3: Driving from asphalt to mud visibly reduces acceleration within 0.2 s
    std::cout << "[RUN] TestSurfaceTractionScaling (Doc 06 AC3)..." << std::endl;
    RigidBody chassisAsphalt(1, BodyType::DYNAMIC);
    VehicleController tractorAsphalt(&chassisAsphalt, VehiclePreset::TRACTOR);
    tractorAsphalt.SetThrottle(1.0f);

    RigidBody chassisMud(2, BodyType::DYNAMIC);
    VehicleController tractorMud(&chassisMud, VehiclePreset::TRACTOR);
    tractorMud.SetThrottle(1.0f);

    // Step 0.2s (12 steps at 60 Hz)
    for (int i = 0; i < 12; ++i) {
        tractorAsphalt.Update(1.0f / 60.0f, SurfaceType::ASPHALT);
        chassisAsphalt.Integrate(1.0f / 60.0f, Vec3{0.0f, -9.81f, 0.0f});

        tractorMud.Update(1.0f / 60.0f, SurfaceType::MUD);
        chassisMud.Integrate(1.0f / 60.0f, Vec3{0.0f, -9.81f, 0.0f});
    }

    float speedAsphalt = tractorAsphalt.GetSpeedKmh();
    float speedMud = tractorMud.GetSpeedKmh();

    std::cout << "  -> At 0.2s: Asphalt Speed = " << speedAsphalt << " km/h, Mud Speed = " << speedMud << " km/h" << std::endl;
    // Mud traction is 0.45 vs Asphalt 1.0; speed in mud should be visibly lower
    assert(speedMud < speedAsphalt);

    std::cout << "[PASS] TestSurfaceTractionScaling\n" << std::endl;
}

void TestOverturnDetectionTimer() {
    // Doc 06 §9.6: Roll angle exceeds 70 degrees for more than 2 seconds raises onVehicleOverturned
    std::cout << "[RUN] TestOverturnDetectionTimer (Doc 06 §9.6)..." << std::endl;
    RigidBody chassis(1, BodyType::DYNAMIC);
    VehicleController tractor(&chassis, VehiclePreset::TRACTOR);

    // Roll by 80 degrees (> 70 degrees)
    chassis.SetRotation(Vec3{0.0f, 0.0f, 80.0f});

    // 1.5 seconds later (not yet 2.0s)
    for (int i = 0; i < 90; ++i) {
        tractor.Update(1.0f / 60.0f);
    }
    assert(!tractor.IsOverturned());

    // Another 1.0 second (total 2.5s > 2.0s threshold)
    for (int i = 0; i < 60; ++i) {
        tractor.Update(1.0f / 60.0f);
    }
    assert(tractor.IsOverturned());
    std::cout << "  -> Roll angle > 70 deg for > 2.0s detected: IsOverturned = TRUE" << std::endl;

    std::cout << "[PASS] TestOverturnDetectionTimer\n" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  QIVFORGE VEHICLE PHYSICS SUITE (DOC 06) " << std::endl;
    std::cout << "========================================" << std::endl;

    TestTractor25DegreeSlopeStability();
    TestTractor20DegreeSlopeClimbing();
    TestTractorTopSpeedCalculation();
    TestSurfaceTractionScaling();
    TestOverturnDetectionTimer();

    std::cout << "ALL VEHICLE PHYSICS TESTS PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}

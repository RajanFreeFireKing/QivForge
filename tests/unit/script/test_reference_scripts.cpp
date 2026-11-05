#include "qivforge/script/lexer.h"
#include "qivforge/script/parser.h"
#include "qivforge/script/compiler.h"
#include <iostream>
#include <cassert>

using namespace qivforge::script;

// Reference Script 1: Tractor Controller (Doc 08 §15)
const char* TRACTOR_CONTROLLER_SOURCE = R"RAW(
module Game.Vehicles;

import QivForge.Core.*;
import QivForge.Physics.*;
import QivForge.Cloud.*;
import QivForge.Input.*;
import QivForge.Audio.*;

@component("Heavy Tractor Physics & Cloud Stream Controller")
@require(RigidBody)
pub class TractorController extends VehiclePawn {

    @header("Engine")
    @export @range(800.0, 6000.0)
    pub var engineTorque: f32 = 3200.0;

    @header("Steering")
    @export @range(15.0, 50.0)
    pub var maxSteerAngle: f32 = 38.0;

    @header("Body")
    @export @range(500.0, 4000.0)
    pub var tractorMassKg: f32 = 2100.0;

    @header("Wheels (front left, front right, rear left, rear right)")
    @export pub var wheels: Array<WheelCollider, 4>;

    @header("Effects")
    @export pub var engineSound: AudioEmitter;
    @export pub var exhaustSmoke: ParticleSystem;

    private var rb: RigidBody;

    override async fn onAwake() -> void {
        self.rb = self.getComponent<RigidBody>();
        self.rb.setMass(self.tractorMassKg);
        self.rb.setCenterOfMass(Vec3(0.0, -0.65, 0.0));

        try {
            var modelAsset = await AssetManager.streamModel("assets/models/tractor_heavy.glb");
            self.attachModel(modelAsset);
        } catch (e) {
            Debug.warn("Tractor model failed to load");
        }

        CameraManager.follow(self.transform, Vec3(0.0, 3.2, -6.5), 8.0);
    }

    override fn onPhysicsStep(fixedDt: f32) -> void {
        var gas: f32 = TouchHUD.getAxis("Vertical");
        var steerInput: f32 = TouchHUD.getAxis("Horizontal");
        var steerAngle: f32 = steerInput * self.maxSteerAngle;
        var motorForce: f32 = gas * self.engineTorque;

        self.wheels[0].setSteerAngle(steerAngle);
        self.wheels[1].setSteerAngle(steerAngle);
        self.wheels[2].applyMotorTorque(motorForce);
        self.wheels[3].applyMotorTorque(motorForce);

        var brake: f32 = TouchHUD.isPressed("BrakePedal") ? 5000.0 : 0.0;
        for (var i: i32 = 0; i < 4; i++) {
            self.wheels[i].setBrakeTorque(brake);
        }
    }

    override fn onTick(dt: f32) -> void {
        var speedKmh: f32 = self.rb.getLinearVelocity().length() * 3.6;
        self.engineSound.setPitch(0.9 + speedKmh / 55.0);
        self.exhaustSmoke.setRate(10.0 + speedKmh * 2.0);
        WorldStreamer.updateActiveChunks(self.transform.position, 250.0);
    }
}
)RAW";

// Reference Script 2: TPS Player (Doc 08 §16)
const char* TPS_PLAYER_SOURCE = R"RAW(
module Game.Characters;

import QivForge.Core.*;
import QivForge.Animation.*;
import QivForge.Physics.*;
import QivForge.Input.*;

@component("TPS Shooter & Survival Character")
pub class TPSPlayer extends CharacterPawn {

    @export @range(2.0, 15.0)  pub var moveSpeed: f32 = 6.5;
    @export @range(10.0, 200.0) pub var weaponDamage: f32 = 35.0;
    @export @range(0.05, 1.0)  pub var fireInterval: f32 = 0.12;
    @export pub var muzzleVFX: ParticleSystem;

    private var animGraph: SkeletalAnimator;
    private var lastFire: f32 = -10.0;

    override fn onStart() -> void {
        self.animGraph = self.getComponent<SkeletalAnimator>();
    }

    override fn onTick(dt: f32) -> void {
        var joy: Vec2 = TouchHUD.getJoystick("LeftMoveStick");
        var aiming: bool = TouchHUD.isPressed("AimButton");

        self.moveCharacter(Vec3(joy.x, 0.0, joy.y) * self.moveSpeed * dt);
        self.animGraph.setFloat("MoveSpeed", joy.length());
        self.animGraph.setBool("IsAiming", aiming);

        if (TouchHUD.isPressed("FireButton") && Time.time - self.lastFire >= self.fireInterval) {
            self.lastFire = Time.time;
            self.fireWeapon();
        }
    }

    private fn fireWeapon() -> void {
        self.animGraph.trigger("ShootRecoil");
        self.muzzleVFX.burst(15);

        var ray: Ray = CameraManager.screenCenterRay();
        var hit: RayHit = PhysicsWorld.raycast(ray, 300.0);

        if (hit.hasHit) {
            VFXManager.spawnImpact("BulletSpark", hit.point, hit.normal);
            if (hit.actor.hasTag("Enemy")) {
                hit.actor.sendMessage("applyDamage", self.weaponDamage);
            }
        }
    }
}
)RAW";

void TestReferenceScript1_Tractor() {
    std::cout << "[RUN] TestReferenceScript1_Tractor (Doc 08 §15 & AC1)..." << std::endl;

    Lexer lexer(TRACTOR_CONTROLLER_SOURCE);
    auto tokens = lexer.Tokenize();
    Parser parser(tokens);
    auto ast = parser.Parse();

    if (parser.HasErrors()) {
        std::cerr << "Parser errors in TractorController:" << std::endl;
        for (const auto& d : parser.GetDiagnostics()) {
            std::cerr << "  [" << d.code << "] Line " << d.location.line << ": " << d.message << std::endl;
        }
    }
    assert(!parser.HasErrors());
    assert(ast->classes.size() == 1);
    assert(ast->classes[0].name == "TractorController");
    assert(ast->classes[0].methods.size() == 3); // onAwake, onPhysicsStep, onTick

    Compiler compiler(*ast);
    auto module = compiler.Compile();

    if (compiler.HasErrors()) {
        std::cerr << "Compiler errors in TractorController:" << std::endl;
        for (const auto& d : compiler.GetDiagnostics()) {
            std::cerr << "  [" << d.code << "] Line " << d.location.line << ": " << d.message << std::endl;
        }
    }
    assert(!compiler.HasErrors());
    assert(module != nullptr);
    assert(module->functions.size() == 3);
    assert(module->exportedFields.size() == 6); // engineTorque, maxSteerAngle, tractorMassKg, wheels, engineSound, exhaustSmoke

    std::cout << "  -> TractorController compiled with 0 errors, 0 warnings." << std::endl;
    std::cout << "  -> Extracted " << module->exportedFields.size() << " exported fields and " 
              << module->functions.size() << " bytecode functions." << std::endl;
    std::cout << "[PASS] TestReferenceScript1_Tractor\n" << std::endl;
}

void TestReferenceScript2_TPSPlayer() {
    std::cout << "[RUN] TestReferenceScript2_TPSPlayer (Doc 08 §16 & AC1)..." << std::endl;

    Lexer lexer(TPS_PLAYER_SOURCE);
    auto tokens = lexer.Tokenize();
    Parser parser(tokens);
    auto ast = parser.Parse();

    if (parser.HasErrors()) {
        std::cerr << "Parser errors in TPSPlayer:" << std::endl;
        for (const auto& d : parser.GetDiagnostics()) {
            std::cerr << "  [" << d.code << "] Line " << d.location.line << ": " << d.message << std::endl;
        }
    }
    assert(!parser.HasErrors());
    assert(ast->classes.size() == 1);
    assert(ast->classes[0].name == "TPSPlayer");
    assert(ast->classes[0].methods.size() == 3); // onStart, onTick, fireWeapon

    Compiler compiler(*ast);
    auto module = compiler.Compile();

    if (compiler.HasErrors()) {
        std::cerr << "Compiler errors in TPSPlayer:" << std::endl;
        for (const auto& d : compiler.GetDiagnostics()) {
            std::cerr << "  [" << d.code << "] Line " << d.location.line << ": " << d.message << std::endl;
        }
    }
    assert(!compiler.HasErrors());
    assert(module != nullptr);
    assert(module->functions.size() == 3);
    assert(module->exportedFields.size() == 4); // moveSpeed, weaponDamage, fireInterval, muzzleVFX

    std::cout << "  -> TPSPlayer compiled with 0 errors, 0 warnings." << std::endl;
    std::cout << "  -> Extracted " << module->exportedFields.size() << " exported fields and " 
              << module->functions.size() << " bytecode functions." << std::endl;
    std::cout << "[PASS] TestReferenceScript2_TPSPlayer\n" << std::endl;
}

int main() {
    std::cout << "===========================================" << std::endl;
    std::cout << "  QIVSCRIPT REFERENCE SCRIPTS SUITE (DOC 08)" << std::endl;
    std::cout << "===========================================" << std::endl;

    TestReferenceScript1_Tractor();
    TestReferenceScript2_TPSPlayer();

    std::cout << "ALL REFERENCE SCRIPT TESTS PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}

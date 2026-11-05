#include "qivforge/scene/template_factory.h"
#include <iostream>
#include <cassert>

using namespace qivforge::scene;

void TestBlankTemplate() {
    std::cout << "[RUN] TestBlankTemplate (Doc 01 §7 Template 1)..." << std::endl;
    auto scene = TemplateFactory::CreateTemplateScene(TemplateId::BLANK);
    assert(scene != nullptr);
    assert(scene->GetActorCount() >= 3); // Sun, Ground, Camera

    bool hasSun = false;
    bool hasGround = false;
    bool hasCam = false;

    for (uint32_t id : scene->GetRootActorIds()) {
        Actor* a = scene->FindActor(id);
        if (a->GetName() == "DirectionalSun") hasSun = (a->GetComponent(ComponentType::LIGHT) != nullptr);
        if (a->GetName() == "GroundPlane") hasGround = (a->GetComponent(ComponentType::COLLIDER) != nullptr);
        if (a->GetName() == "MainCamera") hasCam = (a->GetComponent(ComponentType::CAMERA) != nullptr);
    }

    assert(hasSun);
    assert(hasGround);
    assert(hasCam);

    std::cout << "  -> Blank template created with Sun, Ground, and Camera" << std::endl;
    std::cout << "[PASS] TestBlankTemplate\n" << std::endl;
}

void TestTractorTemplate() {
    std::cout << "[RUN] TestTractorTemplate (Doc 01 §7 Template 2)..." << std::endl;
    auto scene = TemplateFactory::CreateTemplateScene(TemplateId::TRACTOR);
    assert(scene != nullptr);

    bool hasTractor = false;
    bool hasRamp15 = false;
    bool hasBank25 = false;

    for (uint32_t id : scene->GetRootActorIds()) {
        Actor* a = scene->FindActor(id);
        if (a->GetName() == "HeavyTractor") {
            hasTractor = true;
            auto* vehicle = dynamic_cast<VehicleControllerComponent*>(a->GetComponent(ComponentType::VEHICLE_CONTROLLER));
            assert(vehicle != nullptr);
            assert(vehicle->preset == "TRACTOR");
            assert(vehicle->massKg == 2100.0f);
            assert(vehicle->maxSteerAngle == 38.0f);
            assert(vehicle->engineTorque == 3200.0f);

            auto* script = dynamic_cast<ScriptComponent*>(a->GetComponent(ComponentType::SCRIPT));
            assert(script != nullptr);
            assert(script->className == "TractorController");
        }
        if (a->GetName() == "Ramp15Deg") hasRamp15 = true;
        if (a->GetName() == "BankSlope25Deg") hasBank25 = true;
    }

    assert(hasTractor);
    assert(hasRamp15);
    assert(hasBank25);

    std::cout << "  -> Tractor template created with HeavyTractor (2100kg, 38deg steer, 3200Nm), 15deg ramp, and 25deg bank slope" << std::endl;
    std::cout << "[PASS] TestTractorTemplate\n" << std::endl;
}

void TestTpsTemplate() {
    std::cout << "[RUN] TestTpsTemplate (Doc 01 §7 Template 3)..." << std::endl;
    auto scene = TemplateFactory::CreateTemplateScene(TemplateId::TPS);
    assert(scene != nullptr);

    bool hasHero = false;
    bool hasArena = false;

    for (uint32_t id : scene->GetRootActorIds()) {
        Actor* a = scene->FindActor(id);
        if (a->GetName() == "PlayerHero") {
            hasHero = true;
            auto* charCtrl = dynamic_cast<CharacterControllerComponent*>(a->GetComponent(ComponentType::CHARACTER_CONTROLLER));
            assert(charCtrl != nullptr);
            assert(charCtrl->height == 1.8f);
            assert(charCtrl->radius == 0.4f);
            assert(charCtrl->stepHeight == 0.35f);
            assert(charCtrl->slopeLimitDegrees == 50.0f);

            auto* script = dynamic_cast<ScriptComponent*>(a->GetComponent(ComponentType::SCRIPT));
            assert(script != nullptr);
            assert(script->className == "TPSPlayer");
        }
        if (a->GetName() == "ArenaGround") hasArena = true;
    }

    assert(hasHero);
    assert(hasArena);

    std::cout << "  -> TPS template created with PlayerHero (1.8m capsule, 0.35m step, 50deg slope limit) and Arena" << std::endl;
    std::cout << "[PASS] TestTpsTemplate\n" << std::endl;
}

void TestRacingAndHudTemplates() {
    std::cout << "[RUN] TestRacingAndHudTemplates (Doc 01 §7 Templates 4 & 5)..." << std::endl;
    auto racingScene = TemplateFactory::CreateTemplateScene(TemplateId::RACING);
    assert(racingScene != nullptr);
    assert(racingScene->GetActorCount() >= 6); // Ground, Checkpoints, Racer, Camera, Sun

    auto hudScene = TemplateFactory::CreateTemplateScene(TemplateId::HUD_PLAYGROUND);
    assert(hudScene != nullptr);
    assert(hudScene->GetActorCount() >= 3);

    std::cout << "  -> Racing and HUD Playground templates created successfully" << std::endl;
    std::cout << "[PASS] TestRacingAndHudTemplates\n" << std::endl;
}

void TestTemplateSerializationRoundtrip() {
    std::cout << "[RUN] TestTemplateSerializationRoundtrip (Doc 15 M6)..." << std::endl;

    for (int t = 0; t < 5; ++t) {
        TemplateId id = static_cast<TemplateId>(t);
        auto scene = TemplateFactory::CreateTemplateScene(id);
        size_t initialCount = scene->GetActorCount();

        std::string json = scene->SerializeToJson();
        assert(!json.empty());
        assert(json.find("\"formatVersion\":1") != std::string::npos);

        Scene reloadedScene;
        bool deserialized = reloadedScene.DeserializeFromJson(json);
        assert(deserialized);
        assert(reloadedScene.GetActorCount() == initialCount);

        std::string projJson = TemplateFactory::GenerateDefaultProjectJson(id, "TestProject");
        assert(projJson.find("\"schemaVersion\": 1") != std::string::npos);
        assert(projJson.find(TemplateFactory::GetTemplateIdString(id)) != std::string::npos);
    }

    std::cout << "  -> All 5 templates passed JSON serialization and deserialization roundtrip" << std::endl;
    std::cout << "[PASS] TestTemplateSerializationRoundtrip\n" << std::endl;
}

int main() {
    std::cout << "===========================================" << std::endl;
    std::cout << "  STARTER TEMPLATES SUITE (DOC 01 §7 & M6) " << std::endl;
    std::cout << "===========================================" << std::endl;

    TestBlankTemplate();
    TestTractorTemplate();
    TestTpsTemplate();
    TestRacingAndHudTemplates();
    TestTemplateSerializationRoundtrip();

    std::cout << "ALL STARTER TEMPLATE TESTS PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}

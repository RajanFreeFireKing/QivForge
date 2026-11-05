#include "qivforge/scene/template_factory.h"
#include <sstream>

namespace qivforge::scene {

std::string TemplateFactory::GetTemplateIdString(TemplateId id) {
    switch (id) {
        case TemplateId::BLANK: return "BLANK";
        case TemplateId::TRACTOR: return "TRACTOR";
        case TemplateId::TPS: return "TPS";
        case TemplateId::RACING: return "RACING";
        case TemplateId::HUD_PLAYGROUND: return "HUD_PLAYGROUND";
        default: return "BLANK";
    }
}

TemplateId TemplateFactory::ParseTemplateId(const std::string& str) {
    if (str == "TRACTOR") return TemplateId::TRACTOR;
    if (str == "TPS") return TemplateId::TPS;
    if (str == "RACING") return TemplateId::RACING;
    if (str == "HUD_PLAYGROUND") return TemplateId::HUD_PLAYGROUND;
    return TemplateId::BLANK;
}

std::unique_ptr<Scene> TemplateFactory::CreateTemplateScene(TemplateId id) {
    auto scene = std::make_unique<Scene>();

    // 1. Directional Sun Light (Standard for all 3D templates)
    Actor* sun = scene->CreateActor("DirectionalSun");
    sun->GetTransform().SetPosition(0.0f, 25.0f, 0.0f);
    sun->GetTransform().SetRotation(45.0f, -30.0f, 0.0f);
    auto sunLight = std::make_unique<LightComponent>();
    sunLight->lightType = LightType::DIRECTIONAL;
    sunLight->intensityLux = 100000.0f;
    sun->AddComponent(std::move(sunLight));

    switch (id) {
        case TemplateId::BLANK: {
            // Ground plane
            Actor* ground = scene->CreateActor("GroundPlane");
            ground->GetTransform().SetPosition(0.0f, -0.1f, 0.0f);
            ground->GetTransform().SetScale(50.0f, 0.2f, 50.0f);
            ground->GetFlags().isStatic = true;
            auto mesh = std::make_unique<MeshRendererComponent>();
            mesh->primitive = PrimitiveType::PLANE;
            mesh->baseColor[0] = 0.45f; mesh->baseColor[1] = 0.48f; mesh->baseColor[2] = 0.52f;
            ground->AddComponent(std::move(mesh));
            auto col = std::make_unique<ColliderComponent>();
            col->shape = ColliderShape::BOX;
            ground->AddComponent(std::move(col));

            // Perspective Camera
            Actor* cam = scene->CreateActor("MainCamera");
            cam->GetTransform().SetPosition(0.0f, 3.0f, -10.0f);
            cam->GetTransform().SetRotation(12.0f, 0.0f, 0.0f);
            cam->AddComponent(std::make_unique<CameraComponent>());
            break;
        }

        case TemplateId::TRACTOR: {
            // Open-world terrain plane (200m x 200m)
            Actor* terrain = scene->CreateActor("TerrainGround");
            terrain->GetTransform().SetPosition(0.0f, -0.1f, 0.0f);
            terrain->GetTransform().SetScale(200.0f, 0.2f, 200.0f);
            terrain->GetFlags().isStatic = true;
            auto groundMesh = std::make_unique<MeshRendererComponent>();
            groundMesh->primitive = PrimitiveType::PLANE;
            groundMesh->baseColor[0] = 0.28f; groundMesh->baseColor[1] = 0.42f; groundMesh->baseColor[2] = 0.22f; // Field green
            terrain->AddComponent(std::move(groundMesh));
            auto groundCol = std::make_unique<ColliderComponent>();
            groundCol->shape = ColliderShape::BOX;
            terrain->AddComponent(std::move(groundCol));

            // 15° Slope Ramp for climb testing
            Actor* ramp15 = scene->CreateActor("Ramp15Deg");
            ramp15->GetTransform().SetPosition(0.0f, 2.0f, 40.0f);
            ramp15->GetTransform().SetRotation(-15.0f, 0.0f, 0.0f);
            ramp15->GetTransform().SetScale(12.0f, 0.4f, 30.0f);
            ramp15->GetFlags().isStatic = true;
            ramp15->AddComponent(std::make_unique<ColliderComponent>());

            // 25° Bank Slope for roll stability verification
            Actor* bank25 = scene->CreateActor("BankSlope25Deg");
            bank25->GetTransform().SetPosition(35.0f, 2.5f, 0.0f);
            bank25->GetTransform().SetRotation(0.0f, 0.0f, 25.0f);
            bank25->GetTransform().SetScale(30.0f, 0.4f, 40.0f);
            bank25->GetFlags().isStatic = true;
            bank25->AddComponent(std::make_unique<ColliderComponent>());

            // 4-Wheel Heavy Tractor
            Actor* tractor = scene->CreateActor("HeavyTractor");
            tractor->GetTransform().SetPosition(0.0f, 1.2f, 0.0f);
            auto rb = std::make_unique<RigidBodyComponent>();
            rb->mass = 2100.0f;
            tractor->AddComponent(std::move(rb));

            auto vehicle = std::make_unique<VehicleControllerComponent>();
            vehicle->preset = "TRACTOR";
            vehicle->massKg = 2100.0f;
            vehicle->maxSteerAngle = 38.0f;
            vehicle->engineTorque = 3200.0f;
            vehicle->centerOfMassOffset[1] = -0.65f;
            tractor->AddComponent(std::move(vehicle));

            auto script = std::make_unique<ScriptComponent>();
            script->scriptPath = "scripts/TractorController.qs";
            script->className = "TractorController";
            tractor->AddComponent(std::move(script));

            auto audio = std::make_unique<AudioEmitterComponent>();
            audio->clipPath = "assets/audio/tractor_diesel.wav";
            audio->pitch = 1.0f;
            tractor->AddComponent(std::move(audio));

            // Follow Camera
            Actor* cam = scene->CreateActor("FollowCamera");
            cam->GetTransform().SetPosition(0.0f, 3.2f, -6.5f);
            cam->GetTransform().SetRotation(15.0f, 0.0f, 0.0f);
            cam->AddComponent(std::make_unique<CameraComponent>());

            // Touch HUD overlay
            Actor* hud = scene->CreateActor("TouchHUD");
            auto ui = std::make_unique<UIContainerComponent>();
            ui->layoutName = "TractorSteeringAndPedals";
            hud->AddComponent(std::move(ui));
            break;
        }

        case TemplateId::TPS: {
            // Arena Ground
            Actor* arena = scene->CreateActor("ArenaGround");
            arena->GetTransform().SetPosition(0.0f, -0.1f, 0.0f);
            arena->GetTransform().SetScale(100.0f, 0.2f, 100.0f);
            arena->GetFlags().isStatic = true;
            auto arenaMesh = std::make_unique<MeshRendererComponent>();
            arenaMesh->primitive = PrimitiveType::PLANE;
            arenaMesh->baseColor[0] = 0.35f; arenaMesh->baseColor[1] = 0.35f; arenaMesh->baseColor[2] = 0.38f;
            arena->AddComponent(std::move(arenaMesh));
            arena->AddComponent(std::make_unique<ColliderComponent>());

            // Boundary Walls
            float wallH = 3.5f;
            Actor* wallN = scene->CreateActor("Wall_North");
            wallN->GetTransform().SetPosition(0.0f, wallH * 0.5f, 50.0f);
            wallN->GetTransform().SetScale(100.0f, wallH, 1.0f);
            wallN->GetFlags().isStatic = true;
            wallN->AddComponent(std::make_unique<ColliderComponent>());

            Actor* wallS = scene->CreateActor("Wall_South");
            wallS->GetTransform().SetPosition(0.0f, wallH * 0.5f, -50.0f);
            wallS->GetTransform().SetScale(100.0f, wallH, 1.0f);
            wallS->GetFlags().isStatic = true;
            wallS->AddComponent(std::make_unique<ColliderComponent>());

            // Obstacles
            Actor* box1 = scene->CreateActor("Obstacle_Box1");
            box1->GetTransform().SetPosition(12.0f, 1.0f, 15.0f);
            box1->GetTransform().SetScale(3.0f, 2.0f, 3.0f);
            box1->GetFlags().isStatic = true;
            box1->AddComponent(std::make_unique<ColliderComponent>());

            // Hero Character
            Actor* hero = scene->CreateActor("PlayerHero");
            hero->GetTransform().SetPosition(0.0f, 1.0f, 0.0f);
            auto charCtrl = std::make_unique<CharacterControllerComponent>();
            charCtrl->height = 1.8f;
            charCtrl->radius = 0.4f;
            charCtrl->stepHeight = 0.35f;
            charCtrl->slopeLimitDegrees = 50.0f;
            charCtrl->moveSpeed = 6.5f;
            hero->AddComponent(std::move(charCtrl));

            auto script = std::make_unique<ScriptComponent>();
            script->scriptPath = "scripts/TPSPlayer.qs";
            script->className = "TPSPlayer";
            hero->AddComponent(std::move(script));

            // Shoulder Camera
            Actor* cam = scene->CreateActor("ThirdPersonCamera");
            cam->GetTransform().SetPosition(0.8f, 2.2f, -3.5f);
            cam->GetTransform().SetRotation(12.0f, 0.0f, 0.0f);
            cam->AddComponent(std::make_unique<CameraComponent>());

            // Touch HUD (Joystick + Fire Button)
            Actor* hud = scene->CreateActor("TouchHUD");
            auto ui = std::make_unique<UIContainerComponent>();
            ui->layoutName = "TPSJoystickAndFire";
            hud->AddComponent(std::move(ui));
            break;
        }

        case TemplateId::RACING: {
            // Speed Track Ground
            Actor* ground = scene->CreateActor("TrackGround");
            ground->GetTransform().SetPosition(0.0f, -0.1f, 0.0f);
            ground->GetTransform().SetScale(300.0f, 0.2f, 300.0f);
            ground->GetFlags().isStatic = true;
            ground->AddComponent(std::make_unique<ColliderComponent>());

            // Checkpoints 1 to 4
            for (int i = 0; i < 4; ++i) {
                std::string cpName = "Checkpoint_" + std::to_string(i + 1);
                Actor* cp = scene->CreateActor(cpName);
                cp->GetTransform().SetPosition(std::cos(i * 1.57f) * 60.0f, 1.5f, std::sin(i * 1.57f) * 60.0f);
                cp->GetTransform().SetScale(15.0f, 3.0f, 2.0f);
                auto col = std::make_unique<ColliderComponent>();
                col->isTrigger = true;
                cp->AddComponent(std::move(col));
            }

            // Sports Racer Car
            Actor* racer = scene->CreateActor("SportsRacer");
            racer->GetTransform().SetPosition(0.0f, 0.8f, 0.0f);
            auto rb = std::make_unique<RigidBodyComponent>();
            rb->mass = 1200.0f;
            racer->AddComponent(std::move(rb));

            auto vehicle = std::make_unique<VehicleControllerComponent>();
            vehicle->preset = "RACING";
            vehicle->massKg = 1200.0f;
            vehicle->maxSteerAngle = 32.0f;
            vehicle->engineTorque = 4500.0f;
            racer->AddComponent(std::move(vehicle));

            // Camera
            Actor* cam = scene->CreateActor("ChaseCamera");
            cam->GetTransform().SetPosition(0.0f, 2.2f, -5.5f);
            cam->AddComponent(std::make_unique<CameraComponent>());
            break;
        }

        case TemplateId::HUD_PLAYGROUND: {
            // Minimal plate for testing 2D touch interaction
            Actor* plate = scene->CreateActor("TestPlate");
            plate->GetTransform().SetPosition(0.0f, 0.0f, 0.0f);
            plate->GetTransform().SetScale(10.0f, 0.2f, 10.0f);
            plate->AddComponent(std::make_unique<ColliderComponent>());

            Actor* cam = scene->CreateActor("PreviewCamera");
            cam->GetTransform().SetPosition(0.0f, 5.0f, -8.0f);
            cam->GetTransform().SetRotation(25.0f, 0.0f, 0.0f);
            cam->AddComponent(std::make_unique<CameraComponent>());

            // Dual Joysticks & Widgets
            Actor* hud = scene->CreateActor("HUDCanvas");
            auto ui = std::make_unique<UIContainerComponent>();
            ui->layoutName = "DualJoystickAndSliders";
            hud->AddComponent(std::move(ui));
            break;
        }
    }

    return scene;
}

std::string TemplateFactory::GenerateDefaultProjectJson(TemplateId id, const std::string& projectName) {
    std::string templateStr = GetTemplateIdString(id);
    std::ostringstream ss;
    ss << "{\n"
       << "  \"schemaVersion\": 1,\n"
       << "  \"engineVersion\": \"1.0.0-alpha\",\n"
       << "  \"projectName\": \"" << projectName << "\",\n"
       << "  \"templateId\": \"" << templateStr << "\",\n"
       << "  \"targetFps\": 60,\n"
       << "  \"qualityPreset\": \"High\",\n"
       << "  \"defaultScene\": \"scenes/main.qivscene\"\n"
       << "}\n";
    return ss.str();
}

} // namespace qivforge::scene

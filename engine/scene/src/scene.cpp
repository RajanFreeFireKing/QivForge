#include "qivforge/scene/scene.h"
#include <algorithm>
#include <sstream>

namespace qivforge::scene {

Scene::Scene() = default;

Actor* Scene::CreateActor(const std::string& name, uint32_t parentId) {
    uint32_t id = m_nextActorId++;
    auto actor = std::make_unique<Actor>(id, name, parentId);
    Actor* ptr = actor.get();

    if (parentId == 0) {
        m_rootActorIds.push_back(id);
    } else {
        auto it = m_actors.find(parentId);
        if (it != m_actors.end()) {
            it->second->AddChild(id);
        } else {
            // Fallback to root if parent does not exist
            actor->SetParentId(0);
            m_rootActorIds.push_back(id);
        }
    }

    m_actors[id] = std::move(actor);
    return ptr;
}

bool Scene::DestroyActor(uint32_t actorId) {
    auto it = m_actors.find(actorId);
    if (it == m_actors.end()) {
        return false;
    }

    Actor* actor = it->second.get();
    uint32_t parentId = actor->GetParentId();

    // Remove from parent's children list
    if (parentId == 0) {
        m_rootActorIds.erase(
            std::remove(m_rootActorIds.begin(), m_rootActorIds.end(), actorId),
            m_rootActorIds.end()
        );
    } else {
        auto parentIt = m_actors.find(parentId);
        if (parentIt != m_actors.end()) {
            parentIt->second->RemoveChild(actorId);
        }
    }

    // Recursively destroy children
    std::vector<uint32_t> children = actor->GetChildren();
    for (uint32_t childId : children) {
        DestroyActor(childId);
    }

    m_actors.erase(actorId);
    return true;
}

bool Scene::ReparentActor(uint32_t actorId, uint32_t newParentId) {
    if (actorId == newParentId) return false;

    auto it = m_actors.find(actorId);
    if (it == m_actors.end()) return false;

    Actor* actor = it->second.get();
    uint32_t oldParentId = actor->GetParentId();

    // Detach from old parent
    if (oldParentId == 0) {
        m_rootActorIds.erase(
            std::remove(m_rootActorIds.begin(), m_rootActorIds.end(), actorId),
            m_rootActorIds.end()
        );
    } else {
        auto oldParentIt = m_actors.find(oldParentId);
        if (oldParentIt != m_actors.end()) {
            oldParentIt->second->RemoveChild(actorId);
        }
    }

    // Attach to new parent
    if (newParentId == 0) {
        actor->SetParentId(0);
        m_rootActorIds.push_back(actorId);
    } else {
        auto newParentIt = m_actors.find(newParentId);
        if (newParentIt == m_actors.end()) return false;

        actor->SetParentId(newParentId);
        newParentIt->second->AddChild(actorId);
    }

    actor->GetTransform().MarkDirty();
    return true;
}

Actor* Scene::FindActor(uint32_t actorId) const noexcept {
    auto it = m_actors.find(actorId);
    return (it != m_actors.end()) ? it->second.get() : nullptr;
}

void Scene::Clear() {
    m_actors.clear();
    m_rootActorIds.clear();
    m_nextActorId = 1;
}

std::string Scene::SerializeToJson() const {
    std::ostringstream ss;
    ss << "{\"formatVersion\":1,\"actors\":[";

    bool first = true;
    for (const auto& [id, actor] : m_actors) {
        if (!first) ss << ",";
        ss << actor->ToJson();
        first = false;
    }

    ss << "]}";
    return ss.str();
}

bool Scene::DeserializeFromJson(const std::string& jsonStr) {
    if (jsonStr.empty()) return false;
    Clear();

    size_t pos = 0;
    while ((pos = jsonStr.find("{\"id\":", pos)) != std::string::npos) {
        size_t namePos = jsonStr.find("\"name\":\"", pos);
        if (namePos == std::string::npos) break;
        namePos += 8;
        size_t nameEnd = jsonStr.find("\"", namePos);
        std::string name = jsonStr.substr(namePos, nameEnd - namePos);

        size_t parentPos = jsonStr.find("\"parentId\":", nameEnd);
        uint32_t parentId = 0;
        if (parentPos != std::string::npos) {
            parentId = static_cast<uint32_t>(std::stoul(jsonStr.substr(parentPos + 11)));
        }

        Actor* actor = CreateActor(name, parentId);

        // Parse pos
        size_t posArr = jsonStr.find("\"pos\":[", nameEnd);
        if (posArr != std::string::npos && posArr < jsonStr.find("}", posArr)) {
            posArr += 7;
            float px = std::stof(jsonStr.substr(posArr));
            size_t c1 = jsonStr.find(',', posArr);
            float py = std::stof(jsonStr.substr(c1 + 1));
            size_t c2 = jsonStr.find(',', c1 + 1);
            float pz = std::stof(jsonStr.substr(c2 + 1));
            actor->GetTransform().SetPosition(px, py, pz);
        }

        // Parse components
        size_t compArr = jsonStr.find("\"components\":[", nameEnd);
        if (compArr != std::string::npos) {
            compArr += 14;
            while (compArr < jsonStr.size() && jsonStr[compArr] != ']') {
                if (std::isdigit(jsonStr[compArr])) {
                    uint32_t compType = static_cast<uint32_t>(std::stoul(jsonStr.substr(compArr)));
                    if (compType == static_cast<uint32_t>(ComponentType::CAMERA)) {
                        actor->AddComponent(std::make_unique<CameraComponent>());
                    } else if (compType == static_cast<uint32_t>(ComponentType::LIGHT)) {
                        actor->AddComponent(std::make_unique<LightComponent>());
                    } else if (compType == static_cast<uint32_t>(ComponentType::MESH_RENDERER)) {
                        actor->AddComponent(std::make_unique<MeshRendererComponent>());
                    } else if (compType == static_cast<uint32_t>(ComponentType::COLLIDER)) {
                        actor->AddComponent(std::make_unique<ColliderComponent>());
                    } else if (compType == static_cast<uint32_t>(ComponentType::RIGID_BODY)) {
                        actor->AddComponent(std::make_unique<RigidBodyComponent>());
                    } else if (compType == static_cast<uint32_t>(ComponentType::VEHICLE_CONTROLLER)) {
                        actor->AddComponent(std::make_unique<VehicleControllerComponent>());
                    } else if (compType == static_cast<uint32_t>(ComponentType::CHARACTER_CONTROLLER)) {
                        actor->AddComponent(std::make_unique<CharacterControllerComponent>());
                    } else if (compType == static_cast<uint32_t>(ComponentType::SCRIPT)) {
                        actor->AddComponent(std::make_unique<ScriptComponent>());
                    } else if (compType == static_cast<uint32_t>(ComponentType::AUDIO_EMITTER)) {
                        actor->AddComponent(std::make_unique<AudioEmitterComponent>());
                    } else if (compType == static_cast<uint32_t>(ComponentType::UI_CONTAINER)) {
                        actor->AddComponent(std::make_unique<UIContainerComponent>());
                    }
                    while (compArr < jsonStr.size() && (std::isdigit(jsonStr[compArr]) || jsonStr[compArr] == ',' || jsonStr[compArr] == ' ')) compArr++;
                } else {
                    compArr++;
                }
            }
        }

        pos = nameEnd;
    }

    return true;
}

std::unique_ptr<Scene> Scene::Clone() const {
    auto clone = std::make_unique<Scene>();
    clone->m_nextActorId = m_nextActorId;

    for (const auto& [id, actor] : m_actors) {
        auto newActor = std::make_unique<Actor>(actor->GetId(), actor->GetName(), actor->GetParentId());
        newActor->GetTransform().SetPosition(
            actor->GetTransform().GetPosition().x,
            actor->GetTransform().GetPosition().y,
            actor->GetTransform().GetPosition().z
        );
        newActor->GetTransform().SetRotation(
            actor->GetTransform().GetRotation().x,
            actor->GetTransform().GetRotation().y,
            actor->GetTransform().GetRotation().z
        );
        newActor->GetTransform().SetScale(
            actor->GetTransform().GetScale().x,
            actor->GetTransform().GetScale().y,
            actor->GetTransform().GetScale().z
        );
        for (uint32_t childId : actor->GetChildren()) {
            newActor->AddChild(childId);
        }
        clone->m_actors[id] = std::move(newActor);
    }

    clone->m_rootActorIds = m_rootActorIds;
    return clone;
}

} // namespace qivforge::scene

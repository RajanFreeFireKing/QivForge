#pragma once

#include "qivforge/scene/actor.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>

namespace qivforge::scene {

class Scene {
public:
    Scene();
    ~Scene() = default;

    Actor* CreateActor(const std::string& name, uint32_t parentId = 0);
    bool DestroyActor(uint32_t actorId);
    bool ReparentActor(uint32_t actorId, uint32_t newParentId);

    [[nodiscard]] Actor* FindActor(uint32_t actorId) const noexcept;
    [[nodiscard]] const std::vector<uint32_t>& GetRootActorIds() const noexcept { return m_rootActorIds; }
    [[nodiscard]] size_t GetActorCount() const noexcept { return m_actors.size(); }

    void Clear();

    [[nodiscard]] std::string SerializeToJson() const;
    bool DeserializeFromJson(const std::string& jsonStr);

    // Deep clone for Play mode snapshot & restore (Doc 02 §9)
    [[nodiscard]] std::unique_ptr<Scene> Clone() const;

private:
    uint32_t m_nextActorId = 1;
    std::unordered_map<uint32_t, std::unique_ptr<Actor>> m_actors;
    std::vector<uint32_t> m_rootActorIds;
};

} // namespace qivforge::scene

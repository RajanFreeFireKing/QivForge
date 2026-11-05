#pragma once

#include "qivforge/scene/transform.h"
#include "qivforge/scene/component.h"
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace qivforge::scene {

struct ActorFlags {
    bool hidden = false;
    bool locked = false;
    bool isStatic = false;
};

class Actor {
public:
    Actor(uint32_t id, std::string name, uint32_t parentId = 0) noexcept;
    ~Actor() = default;

    [[nodiscard]] uint32_t GetId() const noexcept { return m_id; }
    [[nodiscard]] const std::string& GetName() const noexcept { return m_name; }
    void SetName(std::string name) noexcept { m_name = std::move(name); }

    [[nodiscard]] uint32_t GetParentId() const noexcept { return m_parentId; }
    void SetParentId(uint32_t parentId) noexcept { m_parentId = parentId; }

    [[nodiscard]] const std::vector<uint32_t>& GetChildren() const noexcept { return m_childrenIds; }
    void AddChild(uint32_t childId);
    void RemoveChild(uint32_t childId);

    Transform& GetTransform() noexcept { return m_transform; }
    [[nodiscard]] const Transform& GetTransform() const noexcept { return m_transform; }

    ActorFlags& GetFlags() noexcept { return m_flags; }
    [[nodiscard]] const ActorFlags& GetFlags() const noexcept { return m_flags; }

    void AddComponent(std::unique_ptr<Component> component);
    Component* GetComponent(ComponentType type) const noexcept;
    void RemoveComponent(ComponentType type);

    // Serialization helper
    [[nodiscard]] std::string ToJson() const;

private:
    uint32_t m_id;
    std::string m_name;
    uint32_t m_parentId;
    std::vector<uint32_t> m_childrenIds;

    Transform m_transform;
    ActorFlags m_flags;
    std::vector<std::unique_ptr<Component>> m_components;
};

} // namespace qivforge::scene

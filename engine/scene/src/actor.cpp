#include "qivforge/scene/actor.h"
#include <algorithm>
#include <sstream>

namespace qivforge::scene {

Actor::Actor(uint32_t id, std::string name, uint32_t parentId) noexcept
    : m_id(id), m_name(std::move(name)), m_parentId(parentId) {
}

void Actor::AddChild(uint32_t childId) {
    if (std::find(m_childrenIds.begin(), m_childrenIds.end(), childId) == m_childrenIds.end()) {
        m_childrenIds.push_back(childId);
    }
}

void Actor::RemoveChild(uint32_t childId) {
    m_childrenIds.erase(
        std::remove(m_childrenIds.begin(), m_childrenIds.end(), childId),
        m_childrenIds.end()
    );
}

void Actor::AddComponent(std::unique_ptr<Component> component) {
    if (component) {
        m_components.push_back(std::move(component));
    }
}

Component* Actor::GetComponent(ComponentType type) const noexcept {
    for (const auto& comp : m_components) {
        if (comp->GetType() == type) {
            return comp.get();
        }
    }
    return nullptr;
}

void Actor::RemoveComponent(ComponentType type) {
    m_components.erase(
        std::remove_if(m_components.begin(), m_components.end(),
            [type](const std::unique_ptr<Component>& comp) {
                return comp->GetType() == type;
            }),
        m_components.end()
    );
}

std::string Actor::ToJson() const {
    std::ostringstream ss;
    const auto& pos = m_transform.GetPosition();
    const auto& rot = m_transform.GetRotation();
    const auto& scale = m_transform.GetScale();

    ss << "{\"id\":" << m_id
       << ",\"name\":\"" << m_name << "\""
       << ",\"parentId\":" << m_parentId
       << ",\"transform\":{"
       << "\"pos\":[" << pos.x << "," << pos.y << "," << pos.z << "]"
       << ",\"rot\":[" << rot.x << "," << rot.y << "," << rot.z << "]"
       << ",\"scale\":[" << scale.x << "," << scale.y << "," << scale.z << "]"
       << "},\"components\":[";

    bool first = true;
    for (const auto& comp : m_components) {
        if (!first) ss << ",";
        ss << static_cast<uint32_t>(comp->GetType());
        first = false;
    }
    ss << "]}";
    return ss.str();
}

} // namespace qivforge::scene

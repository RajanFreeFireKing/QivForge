#pragma once

#include "qivforge/scene/scene.h"
#include <memory>
#include <string>

namespace qivforge::scene {

enum class TemplateId : uint8_t {
    BLANK = 0,
    TRACTOR,
    TPS,
    RACING,
    HUD_PLAYGROUND
};

class TemplateFactory {
public:
    static std::unique_ptr<Scene> CreateTemplateScene(TemplateId id);
    static std::string GetTemplateIdString(TemplateId id);
    static TemplateId ParseTemplateId(const std::string& str);
    static std::string GenerateDefaultProjectJson(TemplateId id, const std::string& projectName);
};

} // namespace qivforge::scene

#pragma once

#include "qivforge/ui2d/hud_widget.h"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>

namespace qivforge::ui2d {

enum class HudPresetType {
    RACING,
    TPS,
    TRACTOR,
    MINIMAL
};

class HudLayout {
public:
    static constexpr float REF_WIDTH = 1920.0f;
    static constexpr float REF_HEIGHT = 1080.0f;

    HudLayout(std::string name);

    [[nodiscard]] const std::string& GetName() const noexcept { return m_name; }

    // Widget management
    void AddWidget(std::shared_ptr<HudWidget> widget);
    [[nodiscard]] std::shared_ptr<HudWidget> FindWidget(const std::string& id) const;
    [[nodiscard]] size_t GetWidgetCount() const noexcept { return m_widgets.size(); }
    [[nodiscard]] const std::vector<std::shared_ptr<HudWidget>>& GetWidgets() const noexcept { return m_widgets; }

    // Multi-touch dispatch (Doc 10 §B4: up to 10 pointers, exclusive claiming)
    bool OnTouchDown(int32_t pointerId, float screenX, float screenY,
                     float screenWidth, float screenHeight, const SafeAreaInsets& insets);

    void OnTouchMove(int32_t pointerId, float screenX, float screenY,
                     float screenWidth, float screenHeight, const SafeAreaInsets& insets);

    void OnTouchUp(int32_t pointerId, float screenX, float screenY,
                   float screenWidth, float screenHeight, const SafeAreaInsets& insets);

    // Data binding engine (Doc 10 §B5 & AC4: dirty-checked, 0 allocations)
    void RegisterDataSource(const std::string& path, std::function<float()> getter);
    void UpdateBindings(); // Evaluates all dirty-checked bindings

    // Preset factories (Doc 10 §B6)
    static std::unique_ptr<HudLayout> CreatePreset(HudPresetType preset);

    // Serialization (ui/<name>.hud.json)
    [[nodiscard]] std::string SerializeToJson() const;
    bool DeserializeFromJson(const std::string& json);

private:
    std::string m_name;
    std::vector<std::shared_ptr<HudWidget>> m_widgets;
    std::unordered_map<int32_t, std::shared_ptr<HudWidget>> m_claimedPointers;
    std::unordered_map<std::string, std::function<float()>> m_dataSources;
};

} // namespace qivforge::ui2d

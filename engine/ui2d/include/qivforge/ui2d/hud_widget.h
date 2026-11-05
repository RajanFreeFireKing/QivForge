#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <memory>

namespace qivforge::ui2d {

enum class WidgetType : uint8_t {
    PANEL = 0,
    LABEL = 1,
    IMAGE = 2,
    BUTTON = 3,
    TOGGLE = 4,
    SLIDER = 5,
    PROGRESS_BAR = 6,
    JOYSTICK = 7,
    STEERING_WHEEL = 8,
    PEDAL = 9,
    SPEEDOMETER = 10,
    RPM_GAUGE = 11,
    MINIMAP = 12,
    COMPASS = 13,
    CROSSHAIR = 14,
    CONTAINER = 15
};

enum class AnchorPreset : uint8_t {
    TOP_LEFT = 0,
    TOP_CENTER = 1,
    TOP_RIGHT = 2,
    MIDDLE_LEFT = 3,
    CENTER = 4,
    MIDDLE_RIGHT = 5,
    BOTTOM_LEFT = 6,
    BOTTOM_CENTER = 7,
    BOTTOM_RIGHT = 8,
    STRETCH = 9
};

struct Rect2D {
    float x = 0.0f;
    float y = 0.0f;
    float width = 100.0f;
    float height = 100.0f;

    [[nodiscard]] bool Contains(float px, float py) const noexcept {
        return px >= x && px <= (x + width) && py >= y && py <= (y + height);
    }
};

struct SafeAreaInsets {
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
};

class HudWidget {
public:
    static constexpr float MIN_TOUCH_SIZE_DP = 48.0f; // Doc 10 §B2 & Rule R8

    HudWidget(std::string id, WidgetType type);

    [[nodiscard]] const std::string& GetId() const noexcept { return m_id; }
    [[nodiscard]] WidgetType GetType() const noexcept { return m_type; }

    // Layout configuration
    void SetAnchor(AnchorPreset anchor) noexcept { m_anchor = anchor; }
    [[nodiscard]] AnchorPreset GetAnchor() const noexcept { return m_anchor; }

    void SetPivot(float px, float py) noexcept { m_pivotX = px; m_pivotY = py; }
    void SetOffset(float ox, float oy) noexcept { m_offsetX = ox; m_offsetY = oy; }
    void SetSize(float w, float h) noexcept { m_width = w; m_height = h; }
    [[nodiscard]] float GetWidth() const noexcept { return m_width; }
    [[nodiscard]] float GetHeight() const noexcept { return m_height; }

    void SetVisible(bool vis) noexcept { m_visible = vis; }
    [[nodiscard]] bool IsVisible() const noexcept { return m_visible; }

    void SetOpacity(float op) noexcept { m_opacity = std::clamp(op, 0.0f, 1.0f); }
    [[nodiscard]] float GetOpacity() const noexcept { return m_opacity; }

    void SetZOrder(int32_t z) noexcept { m_zOrder = z; }
    [[nodiscard]] int32_t GetZOrder() const noexcept { return m_zOrder; }

    // Control binding
    void SetActionName(const std::string& action) { m_actionName = action; }
    [[nodiscard]] const std::string& GetActionName() const noexcept { return m_actionName; }

    void SetDataBinding(const std::string& expr) { m_bindingExpression = expr; }
    [[nodiscard]] const std::string& GetDataBinding() const noexcept { return m_bindingExpression; }

    // State & values
    void SetValue(float val) noexcept;
    [[nodiscard]] float GetValue() const noexcept { return m_currentValue; }

    void SetText(const std::string& text) { m_text = text; }
    [[nodiscard]] const std::string& GetText() const noexcept { return m_text; }

    // Multi-touch ownership (Doc 10 §B4: each widget claims pointer ID until release)
    [[nodiscard]] int32_t GetClaimedPointerId() const noexcept { return m_claimedPointerId; }
    void ClaimPointer(int32_t pointerId) noexcept { m_claimedPointerId = pointerId; }
    void ReleasePointer() noexcept { m_claimedPointerId = -1; }
    [[nodiscard]] bool HasClaimedPointer() const noexcept { return m_claimedPointerId >= 0; }

    // Screen geometry computation
    [[nodiscard]] Rect2D CalculateScreenRect(float screenWidth, float screenHeight, const SafeAreaInsets& insets) const noexcept;

    // Accessibility check (Rule R8: minimum 48 dp touch size)
    [[nodiscard]] bool MeetsMinimumTouchSize(float dpScale = 1.0f) const noexcept;

private:
    std::string m_id;
    WidgetType m_type;
    AnchorPreset m_anchor = AnchorPreset::CENTER;

    float m_pivotX = 0.5f;
    float m_pivotY = 0.5f;
    float m_offsetX = 0.0f;
    float m_offsetY = 0.0f;
    float m_width = 100.0f;
    float m_height = 100.0f;

    bool m_visible = true;
    float m_opacity = 1.0f;
    int32_t m_zOrder = 0;

    std::string m_actionName;
    std::string m_bindingExpression;

    float m_currentValue = 0.0f;
    std::string m_text;

    int32_t m_claimedPointerId = -1;
};

} // namespace qivforge::ui2d

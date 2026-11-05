#include "qivforge/ui2d/hud_widget.h"
#include <utility>

namespace qivforge::ui2d {

HudWidget::HudWidget(std::string id, WidgetType type)
    : m_id(std::move(id)), m_type(type) {}

void HudWidget::SetValue(float val) noexcept {
    m_currentValue = val;
}

Rect2D HudWidget::CalculateScreenRect(float screenWidth, float screenHeight, const SafeAreaInsets& insets) const noexcept {
    // Usable safe area rectangle
    float safeLeft = insets.left;
    float safeTop = insets.top;
    float safeWidth = std::max(10.0f, screenWidth - insets.left - insets.right);
    float safeHeight = std::max(10.0f, screenHeight - insets.top - insets.bottom);

    float anchorOriginX = safeLeft;
    float anchorOriginY = safeTop;

    switch (m_anchor) {
        case AnchorPreset::TOP_LEFT:
            anchorOriginX = safeLeft;
            anchorOriginY = safeTop;
            break;
        case AnchorPreset::TOP_CENTER:
            anchorOriginX = safeLeft + safeWidth * 0.5f;
            anchorOriginY = safeTop;
            break;
        case AnchorPreset::TOP_RIGHT:
            anchorOriginX = safeLeft + safeWidth;
            anchorOriginY = safeTop;
            break;
        case AnchorPreset::MIDDLE_LEFT:
            anchorOriginX = safeLeft;
            anchorOriginY = safeTop + safeHeight * 0.5f;
            break;
        case AnchorPreset::CENTER:
            anchorOriginX = safeLeft + safeWidth * 0.5f;
            anchorOriginY = safeTop + safeHeight * 0.5f;
            break;
        case AnchorPreset::MIDDLE_RIGHT:
            anchorOriginX = safeLeft + safeWidth;
            anchorOriginY = safeTop + safeHeight * 0.5f;
            break;
        case AnchorPreset::BOTTOM_LEFT:
            anchorOriginX = safeLeft;
            anchorOriginY = safeTop + safeHeight;
            break;
        case AnchorPreset::BOTTOM_CENTER:
            anchorOriginX = safeLeft + safeWidth * 0.5f;
            anchorOriginY = safeTop + safeHeight;
            break;
        case AnchorPreset::BOTTOM_RIGHT:
            anchorOriginX = safeLeft + safeWidth;
            anchorOriginY = safeTop + safeHeight;
            break;
        case AnchorPreset::STRETCH:
            return Rect2D{safeLeft, safeTop, safeWidth, safeHeight};
    }

    Rect2D rect;
    rect.width = m_width;
    rect.height = m_height;
    rect.x = anchorOriginX + m_offsetX - (m_width * m_pivotX);
    rect.y = anchorOriginY + m_offsetY - (m_height * m_pivotY);
    return rect;
}

bool HudWidget::MeetsMinimumTouchSize(float dpScale) const noexcept {
    // Only interactive widgets require 48 dp minimum touch size
    if (m_type == WidgetType::BUTTON || m_type == WidgetType::TOGGLE ||
        m_type == WidgetType::SLIDER || m_type == WidgetType::JOYSTICK ||
        m_type == WidgetType::STEERING_WHEEL || m_type == WidgetType::PEDAL) {
        float minPixelSize = MIN_TOUCH_SIZE_DP * dpScale;
        return (m_width >= minPixelSize && m_height >= minPixelSize);
    }
    return true; // Non-interactive widgets (e.g. panels, labels) don't have minimum touch requirement
}

} // namespace qivforge::ui2d

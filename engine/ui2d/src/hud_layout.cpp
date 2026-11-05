#include "qivforge/ui2d/hud_layout.h"
#include <sstream>
#include <cmath>

namespace qivforge::ui2d {

HudLayout::HudLayout(std::string name)
    : m_name(std::move(name)) {}

void HudLayout::AddWidget(std::shared_ptr<HudWidget> widget) {
    if (!widget) return;
    m_widgets.push_back(widget);
    // Keep widgets sorted by z-order
    std::sort(m_widgets.begin(), m_widgets.end(), [](const auto& a, const auto& b) {
        return a->GetZOrder() < b->GetZOrder();
    });
}

std::shared_ptr<HudWidget> HudLayout::FindWidget(const std::string& id) const {
    for (const auto& w : m_widgets) {
        if (w->GetId() == id) return w;
    }
    return nullptr;
}

bool HudLayout::OnTouchDown(int32_t pointerId, float screenX, float screenY,
                            float screenWidth, float screenHeight, const SafeAreaInsets& insets) {
    // If pointer already claimed, do not re-claim
    if (m_claimedPointers.find(pointerId) != m_claimedPointers.end()) {
        return true;
    }

    // Traverse in reverse z-order (top-most widget gets touch first)
    for (auto it = m_widgets.rbegin(); it != m_widgets.rend(); ++it) {
        auto widget = *it;
        if (!widget->IsVisible()) continue;

        Rect2D rect = widget->CalculateScreenRect(screenWidth, screenHeight, insets);
        if (rect.Contains(screenX, screenY)) {
            widget->ClaimPointer(pointerId);
            m_claimedPointers[pointerId] = widget;

            // Compute initial normalized value for interactive widgets
            if (widget->GetType() == WidgetType::BUTTON || widget->GetType() == WidgetType::PEDAL) {
                widget->SetValue(1.0f);
            } else if (widget->GetType() == WidgetType::SLIDER) {
                float val = std::clamp((screenX - rect.x) / rect.width, 0.0f, 1.0f);
                widget->SetValue(val);
            }
            return true;
        }
    }
    return false;
}

void HudLayout::OnTouchMove(int32_t pointerId, float screenX, float screenY,
                            float screenWidth, float screenHeight, const SafeAreaInsets& insets) {
    auto it = m_claimedPointers.find(pointerId);
    if (it == m_claimedPointers.end()) return;

    auto widget = it->second;
    Rect2D rect = widget->CalculateScreenRect(screenWidth, screenHeight, insets);

    if (widget->GetType() == WidgetType::STEERING_WHEEL) {
        // Calculate angle relative to center of wheel
        float centerX = rect.x + rect.width * 0.5f;
        float centerY = rect.y + rect.height * 0.5f;
        float dx = screenX - centerX;
        float dy = screenY - centerY;
        float angleRad = std::atan2(dy, dx);
        float normSteer = std::clamp(angleRad / 1.57f, -1.0f, 1.0f);
        widget->SetValue(normSteer);
    } else if (widget->GetType() == WidgetType::JOYSTICK) {
        float centerX = rect.x + rect.width * 0.5f;
        float centerY = rect.y + rect.height * 0.5f;
        float dx = screenX - centerX;
        float dy = screenY - centerY;
        float dist = std::sqrt(dx * dx + dy * dy);
        float maxR = rect.width * 0.5f;
        float normDist = std::min(1.0f, dist / maxR);
        widget->SetValue(normDist);
    } else if (widget->GetType() == WidgetType::PEDAL) {
        widget->SetValue(1.0f); // Still holding pedal
    } else if (widget->GetType() == WidgetType::SLIDER) {
        float val = std::clamp((screenX - rect.x) / rect.width, 0.0f, 1.0f);
        widget->SetValue(val);
    }
}

void HudLayout::OnTouchUp(int32_t pointerId, float /*screenX*/, float /*screenY*/,
                          float /*screenWidth*/, float /*screenHeight*/, const SafeAreaInsets& /*insets*/) {
    auto it = m_claimedPointers.find(pointerId);
    if (it == m_claimedPointers.end()) return;

    auto widget = it->second;
    if (widget->GetType() == WidgetType::BUTTON || widget->GetType() == WidgetType::PEDAL) {
        widget->SetValue(0.0f);
    } else if (widget->GetType() == WidgetType::STEERING_WHEEL || widget->GetType() == WidgetType::JOYSTICK) {
        widget->SetValue(0.0f); // Auto-return to center
    }

    widget->ReleasePointer();
    m_claimedPointers.erase(it);
}

void HudLayout::RegisterDataSource(const std::string& path, std::function<float()> getter) {
    m_dataSources[path] = getter;
}

void HudLayout::UpdateBindings() {
    for (auto& widget : m_widgets) {
        const std::string& bindExpr = widget->GetDataBinding();
        if (!bindExpr.empty()) {
            auto it = m_dataSources.find(bindExpr);
            if (it != m_dataSources.end()) {
                float val = it->second();
                if (widget->GetValue() != val) {
                    widget->SetValue(val);
                }
            }
        }
    }
}

std::unique_ptr<HudLayout> HudLayout::CreatePreset(HudPresetType preset) {
    switch (preset) {
        case HudPresetType::RACING: {
            auto layout = std::make_unique<HudLayout>("RacingHUD");
            // Steering Wheel (bottom-left)
            auto wheel = std::make_shared<HudWidget>("SteeringWheel", WidgetType::STEERING_WHEEL);
            wheel->SetAnchor(AnchorPreset::BOTTOM_LEFT);
            wheel->SetPivot(0.5f, 0.5f);
            wheel->SetOffset(150.0f, -150.0f);
            wheel->SetSize(220.0f, 220.0f);
            wheel->SetActionName("Steering");
            layout->AddWidget(wheel);

            // Brake Pedal (bottom-right inner)
            auto brake = std::make_shared<HudWidget>("BrakePedal", WidgetType::PEDAL);
            brake->SetAnchor(AnchorPreset::BOTTOM_RIGHT);
            brake->SetPivot(0.5f, 0.5f);
            brake->SetOffset(-220.0f, -120.0f);
            brake->SetSize(90.0f, 150.0f);
            brake->SetActionName("BrakePedal");
            layout->AddWidget(brake);

            // Throttle Pedal (bottom-right outer)
            auto throttle = std::make_shared<HudWidget>("GasPedal", WidgetType::PEDAL);
            throttle->SetAnchor(AnchorPreset::BOTTOM_RIGHT);
            throttle->SetPivot(0.5f, 0.5f);
            throttle->SetOffset(-90.0f, -140.0f);
            throttle->SetSize(90.0f, 180.0f);
            throttle->SetActionName("GasPedal");
            layout->AddWidget(throttle);

            // Speedometer (top-right)
            auto speedo = std::make_shared<HudWidget>("Speedometer", WidgetType::SPEEDOMETER);
            speedo->SetAnchor(AnchorPreset::TOP_RIGHT);
            speedo->SetPivot(1.0f, 0.0f);
            speedo->SetOffset(-40.0f, 30.0f);
            speedo->SetSize(160.0f, 60.0f);
            speedo->SetDataBinding("vehicle.speedKmh");
            layout->AddWidget(speedo);

            return layout;
        }
        case HudPresetType::TPS: {
            auto layout = std::make_unique<HudLayout>("TpsHUD");
            // Movement Joystick (bottom-left)
            auto joy = std::make_shared<HudWidget>("MoveStick", WidgetType::JOYSTICK);
            joy->SetAnchor(AnchorPreset::BOTTOM_LEFT);
            joy->SetPivot(0.5f, 0.5f);
            joy->SetOffset(150.0f, -150.0f);
            joy->SetSize(180.0f, 180.0f);
            joy->SetActionName("LeftMoveStick");
            layout->AddWidget(joy);

            // Fire Button (bottom-right)
            auto fire = std::make_shared<HudWidget>("FireButton", WidgetType::BUTTON);
            fire->SetAnchor(AnchorPreset::BOTTOM_RIGHT);
            fire->SetPivot(0.5f, 0.5f);
            fire->SetOffset(-120.0f, -140.0f);
            fire->SetSize(100.0f, 100.0f);
            fire->SetActionName("FireButton");
            layout->AddWidget(fire);

            // Aim Button (above fire)
            auto aim = std::make_shared<HudWidget>("AimButton", WidgetType::BUTTON);
            aim->SetAnchor(AnchorPreset::BOTTOM_RIGHT);
            aim->SetPivot(0.5f, 0.5f);
            aim->SetOffset(-120.0f, -270.0f);
            aim->SetSize(80.0f, 80.0f);
            aim->SetActionName("AimButton");
            layout->AddWidget(aim);

            // Health Bar (top-left)
            auto health = std::make_shared<HudWidget>("HealthBar", WidgetType::PROGRESS_BAR);
            health->SetAnchor(AnchorPreset::TOP_LEFT);
            health->SetPivot(0.0f, 0.0f);
            health->SetOffset(40.0f, 40.0f);
            health->SetSize(300.0f, 30.0f);
            health->SetDataBinding("player.health");
            layout->AddWidget(health);

            return layout;
        }
        case HudPresetType::TRACTOR: {
            auto layout = std::make_unique<HudLayout>("TractorHUD");
            auto wheel = std::make_shared<HudWidget>("TractorWheel", WidgetType::STEERING_WHEEL);
            wheel->SetAnchor(AnchorPreset::BOTTOM_LEFT);
            wheel->SetPivot(0.5f, 0.5f);
            wheel->SetOffset(160.0f, -160.0f);
            wheel->SetSize(240.0f, 240.0f);
            wheel->SetActionName("Steering");
            layout->AddWidget(wheel);

            auto throttle = std::make_shared<HudWidget>("ThrottlePedal", WidgetType::PEDAL);
            throttle->SetAnchor(AnchorPreset::BOTTOM_RIGHT);
            throttle->SetPivot(0.5f, 0.5f);
            throttle->SetOffset(-100.0f, -140.0f);
            throttle->SetSize(100.0f, 180.0f);
            throttle->SetActionName("Throttle");
            layout->AddWidget(throttle);

            auto brake = std::make_shared<HudWidget>("BrakePedal", WidgetType::PEDAL);
            brake->SetAnchor(AnchorPreset::BOTTOM_RIGHT);
            brake->SetPivot(0.5f, 0.5f);
            brake->SetOffset(-240.0f, -120.0f);
            brake->SetSize(90.0f, 150.0f);
            brake->SetActionName("Brake");
            layout->AddWidget(brake);

            auto rpm = std::make_shared<HudWidget>("RpmGauge", WidgetType::RPM_GAUGE);
            rpm->SetAnchor(AnchorPreset::TOP_RIGHT);
            rpm->SetPivot(1.0f, 0.0f);
            rpm->SetOffset(-40.0f, 30.0f);
            rpm->SetSize(120.0f, 120.0f);
            rpm->SetDataBinding("vehicle.rpm");
            layout->AddWidget(rpm);

            return layout;
        }
        case HudPresetType::MINIMAL:
        default: {
            auto layout = std::make_unique<HudLayout>("MinimalHUD");
            auto pause = std::make_shared<HudWidget>("PauseButton", WidgetType::BUTTON);
            pause->SetAnchor(AnchorPreset::TOP_RIGHT);
            pause->SetPivot(1.0f, 0.0f);
            pause->SetOffset(-30.0f, 30.0f);
            pause->SetSize(60.0f, 60.0f);
            pause->SetActionName("Pause");
            layout->AddWidget(pause);
            return layout;
        }
    }
}

std::string HudLayout::SerializeToJson() const {
    std::ostringstream ss;
    ss << "{\n  \"name\": \"" << m_name << "\",\n  \"widgets\": [\n";
    for (size_t i = 0; i < m_widgets.size(); ++i) {
        const auto& w = m_widgets[i];
        ss << "    {\n";
        ss << "      \"id\": \"" << w->GetId() << "\",\n";
        ss << "      \"type\": " << static_cast<int>(w->GetType()) << ",\n";
        ss << "      \"anchor\": " << static_cast<int>(w->GetAnchor()) << ",\n";
        ss << "      \"width\": " << w->GetWidth() << ",\n";
        ss << "      \"height\": " << w->GetHeight() << ",\n";
        ss << "      \"action\": \"" << w->GetActionName() << "\",\n";
        ss << "      \"binding\": \"" << w->GetDataBinding() << "\"\n";
        ss << "    }" << (i + 1 < m_widgets.size() ? ",\n" : "\n");
    }
    ss << "  ]\n}";
    return ss.str();
}

bool HudLayout::DeserializeFromJson(const std::string& json) {
    if (json.empty()) return false;
    // Simple deserializer for name
    size_t namePos = json.find("\"name\": \"");
    if (namePos != std::string::npos) {
        size_t start = namePos + 9;
        size_t end = json.find("\"", start);
        if (end != std::string::npos) {
            m_name = json.substr(start, end - start);
        }
    }
    return true;
}

} // namespace qivforge::ui2d

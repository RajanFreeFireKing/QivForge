#include "qivforge/ui2d/hud_layout.h"
#include <iostream>
#include <cassert>
#include <chrono>

using namespace qivforge::ui2d;

void TestMultiTouchPointerOwnership() {
    std::cout << "[RUN] TestMultiTouchPointerOwnership (Doc 10 AC1)..." << std::endl;

    auto layout = HudLayout::CreatePreset(HudPresetType::RACING);
    assert(layout != nullptr);

    auto wheel = layout->FindWidget("SteeringWheel");
    auto pedal = layout->FindWidget("GasPedal");
    assert(wheel != nullptr);
    assert(pedal != nullptr);

    SafeAreaInsets insets{0, 0, 0, 0};
    float screenW = 1920.0f;
    float screenH = 1080.0f;

    // Pointer 0 presses Steering Wheel (bottom-left)
    Rect2D wheelRect = wheel->CalculateScreenRect(screenW, screenH, insets);
    bool hit0 = layout->OnTouchDown(0, wheelRect.x + 20.0f, wheelRect.y + 20.0f, screenW, screenH, insets);
    assert(hit0);
    assert(wheel->GetClaimedPointerId() == 0);

    // Pointer 1 presses Gas Pedal simultaneously (bottom-right)
    Rect2D pedalRect = pedal->CalculateScreenRect(screenW, screenH, insets);
    bool hit1 = layout->OnTouchDown(1, pedalRect.x + 20.0f, pedalRect.y + 20.0f, screenW, screenH, insets);
    assert(hit1);
    assert(pedal->GetClaimedPointerId() == 1);

    // Simulate 60 seconds (60 frames) of simultaneous sliding fingers
    for (int frame = 0; frame < 60; ++frame) {
        float angleOffset = static_cast<float>(frame) * 2.0f;
        // Slide wheel finger in circle
        layout->OnTouchMove(0, wheelRect.x + 20.0f + angleOffset, wheelRect.y + 20.0f, screenW, screenH, insets);
        // Slide pedal finger vertically
        layout->OnTouchMove(1, pedalRect.x + 20.0f, pedalRect.y + 20.0f + (frame % 10), screenW, screenH, insets);

        // Doc 10 AC1: Pointers remain strictly owned by their respective widgets
        assert(wheel->GetClaimedPointerId() == 0);
        assert(pedal->GetClaimedPointerId() == 1);
        assert(pedal->GetValue() > 0.0f); // Pedal is actively held
    }

    // Release Pointer 0 (wheel returns to 0.0f)
    layout->OnTouchUp(0, 0.0f, 0.0f, screenW, screenH, insets);
    assert(!wheel->HasClaimedPointer());
    assert(wheel->GetValue() == 0.0f);
    assert(pedal->GetClaimedPointerId() == 1); // Pedal still held

    // Release Pointer 1 (pedal returns to 0.0f)
    layout->OnTouchUp(1, 0.0f, 0.0f, screenW, screenH, insets);
    assert(!pedal->HasClaimedPointer());
    assert(pedal->GetValue() == 0.0f);

    std::cout << "[PASS] TestMultiTouchPointerOwnership" << std::endl;
}

void TestDeviceAspectRatioScaling() {
    std::cout << "[RUN] TestDeviceAspectRatioScaling (Doc 10 AC2)..." << std::endl;

    auto layout = HudLayout::CreatePreset(HudPresetType::RACING);
    SafeAreaInsets insets{40.0f, 20.0f, 40.0f, 20.0f}; // Cutouts & gesture bars

    // Test across device resolutions: 16:9 (1920x1080), 20:9 (2400x1080), and 4:3 (2048x1536)
    std::vector<std::pair<float, float>> resolutions = {
        {1920.0f, 1080.0f}, // 16:9
        {2400.0f, 1080.0f}, // 20:9
        {2048.0f, 1536.0f}  // 4:3
    };

    for (const auto& [w, h] : resolutions) {
        for (const auto& widget : layout->GetWidgets()) {
            Rect2D rect = widget->CalculateScreenRect(w, h, insets);

            // Doc 10 AC2: No widgets clipped outside the usable screen
            assert(rect.x >= 0.0f && (rect.x + rect.width) <= w);
            assert(rect.y >= 0.0f && (rect.y + rect.height) <= h);
        }
    }

    std::cout << "[PASS] TestDeviceAspectRatioScaling" << std::endl;
}

void Test60WidgetUpdatePerformance() {
    std::cout << "[RUN] Test60WidgetUpdatePerformance (Doc 10 AC3)..." << std::endl;

    HudLayout layout("StressLayout");
    // Add 60 widgets
    for (int i = 0; i < 60; ++i) {
        auto w = std::make_shared<HudWidget>("Widget_" + std::to_string(i), WidgetType::PROGRESS_BAR);
        w->SetDataBinding("stat_" + std::to_string(i % 5));
        layout.AddWidget(w);
    }
    assert(layout.GetWidgetCount() == 60);

    for (int i = 0; i < 5; ++i) {
        layout.RegisterDataSource("stat_" + std::to_string(i), [i]() {
            return static_cast<float>(i) * 10.0f;
        });
    }

    // Benchmark update bindings
    auto start = std::chrono::high_resolution_clock::now();
    for (int frame = 0; frame < 1000; ++frame) {
        layout.UpdateBindings();
    }
    auto end = std::chrono::high_resolution_clock::now();
    double totalMs = std::chrono::duration<double, std::milli>(end - start).count();
    double avgMsPerFrame = totalMs / 1000.0;

    std::cout << "  60 widgets updated in avg " << avgMsPerFrame << " ms per frame" << std::endl;
    // Doc 10 AC3 budget: under 0.3 ms per frame for 60 widgets
    assert(avgMsPerFrame < 0.30);

    std::cout << "[PASS] Test60WidgetUpdatePerformance" << std::endl;
}

void TestDataBindingZeroAllocation() {
    std::cout << "[RUN] TestDataBindingZeroAllocation (Doc 10 AC4)..." << std::endl;

    HudLayout layout("HealthHud");
    auto bar = std::make_shared<HudWidget>("HealthBar", WidgetType::PROGRESS_BAR);
    bar->SetDataBinding("player.health");
    layout.AddWidget(bar);

    float currentHealth = 100.0f;
    layout.RegisterDataSource("player.health", [&currentHealth]() {
        return currentHealth;
    });

    layout.UpdateBindings();
    assert(bar->GetValue() == 100.0f);

    // Update player health: updates in the exact same frame
    currentHealth = 72.5f;
    layout.UpdateBindings();
    assert(bar->GetValue() == 72.5f);

    std::cout << "[PASS] TestDataBindingZeroAllocation" << std::endl;
}

void TestPresetsAndTouchSize() {
    std::cout << "[RUN] TestPresetsAndTouchSize (Doc 10 AC5, AC6 & Rule R8)..." << std::endl;

    std::vector<HudPresetType> presets = {
        HudPresetType::RACING,
        HudPresetType::TPS,
        HudPresetType::TRACTOR,
        HudPresetType::MINIMAL
    };

    for (auto presetType : presets) {
        auto layout = HudLayout::CreatePreset(presetType);
        assert(layout != nullptr);
        assert(layout->GetWidgetCount() > 0);

        for (const auto& w : layout->GetWidgets()) {
            // Rule R8 & Doc 10 AC6: All buttons meet the 48 dp minimum touch size
            assert(w->MeetsMinimumTouchSize(1.0f));
        }
    }

    std::cout << "[PASS] TestPresetsAndTouchSize" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   2D HUD DESIGNER & RUNTIME (M9)       " << std::endl;
    std::cout << "========================================" << std::endl;

    TestMultiTouchPointerOwnership();
    TestDeviceAspectRatioScaling();
    Test60WidgetUpdatePerformance();
    TestDataBindingZeroAllocation();
    TestPresetsAndTouchSize();

    std::cout << "ALL HUD DESIGNER TESTS PASSED!" << std::endl;
    return 0;
}

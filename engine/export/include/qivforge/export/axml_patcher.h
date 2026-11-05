#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace qivforge::export_pipeline {

struct ManifestConfig {
    std::string appName = "MyGame";
    std::string packageName = "com.qivora.mygame";
    uint32_t versionCode = 1;
    std::string versionName = "1.0.0";
    std::string orientation = "landscape"; // "landscape", "portrait", "sensor"
    bool isDebuggable = false;
    bool requestInternetPermission = false;
};

class AxmlPatcher {
public:
    AxmlPatcher() = default;

    // Validates package name against reverse-domain regex per Doc 13 §5
    [[nodiscard]] static bool ValidatePackageName(const std::string& pkg) noexcept;

    // Validates app name length (1 to 30 characters per Doc 13 §5)
    [[nodiscard]] static bool ValidateAppName(const std::string& name) noexcept;

    // Patches the binary AndroidManifest.xml buffer with user configuration
    [[nodiscard]] static std::vector<uint8_t> PatchManifest(
        const uint8_t* axmlData, size_t axmlSize,
        const ManifestConfig& config);
};

} // namespace qivforge::export_pipeline

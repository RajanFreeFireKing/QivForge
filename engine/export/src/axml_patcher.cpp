#include "qivforge/export/axml_patcher.h"
#include <regex>
#include <cstring>

namespace qivforge::export_pipeline {

bool AxmlPatcher::ValidatePackageName(const std::string& pkg) noexcept {
    // Reverse domain regex: ^[a-z][a-z0-9_]*(\.[a-z][a-z0-9_]*)+$ (Doc 13 §5)
    static const std::regex kPkgRegex(R"(^[a-z][a-z0-9_]*(\.[a-z][a-z0-9_]*)+$)");
    return std::regex_match(pkg, kPkgRegex);
}

bool AxmlPatcher::ValidateAppName(const std::string& name) noexcept {
    // App name: 1 to 30 characters (Doc 13 §5)
    return !name.empty() && name.size() <= 30;
}

std::vector<uint8_t> AxmlPatcher::PatchManifest(
    const uint8_t* axmlData, size_t axmlSize,
    const ManifestConfig& config) {
    if (!axmlData || axmlSize < 8) return {};

    std::vector<uint8_t> patched(axmlData, axmlData + axmlSize);

    // Helpers to find and replace string tokens inside binary AXML string table
    auto replaceUtf8String = [&](const std::string& target, const std::string& replacement) {
        if (target.empty() || replacement.empty()) return;

        std::string buffer(reinterpret_cast<const char*>(patched.data()), patched.size());
        size_t found = buffer.find(target);
        while (found != std::string::npos) {
            buffer.replace(found, target.size(), replacement);
            found = buffer.find(target, found + replacement.size());
        }
        patched.assign(buffer.begin(), buffer.end());
    };

    // Replace known template placeholders
    replaceUtf8String("io.qivora.qivplayer", config.packageName);
    replaceUtf8String("PACKAGE_NAME_PLACEHOLDER", config.packageName);
    replaceUtf8String("QivPlayer", config.appName);
    replaceUtf8String("APP_NAME_PLACEHOLDER", config.appName);
    replaceUtf8String("VERSION_NAME_PLACEHOLDER", config.versionName);
    replaceUtf8String("ORIENTATION_PLACEHOLDER", config.orientation);

    // Permission handling (Doc 13 §3 & §8):
    // Remove INTERNET permission if not requested
    if (!config.requestInternetPermission) {
        replaceUtf8String("android.permission.INTERNET", "android.permission.NO_OP___");
    }

    return patched;
}

} // namespace qivforge::export_pipeline

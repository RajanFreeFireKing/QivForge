#pragma once

#include "qivforge/export/axml_patcher.h"
#include "qivforge/export/apk_signer.h"
#include "qivforge/export/qpk_builder.h"
#include "qivforge/core/error_codes.h"
#include <string>
#include <vector>

namespace qivforge::export_pipeline {

struct ExportOptions {
    ManifestConfig manifest;
    KeystoreMetadata keystore;
    std::string keystorePassword;
    std::vector<BuiltPack> packs;
    std::vector<uint8_t> customIconPng;
    uint32_t splashColorRgb = 0x1E1E1E;
    bool isDebug = false;
};

struct ExportResult {
    bool success = false;
    core::ErrorCode errorCode = core::ErrorCode::OK;
    std::string errorMessage;
    std::vector<uint8_t> apkBytes;
    uint64_t totalApkSizeBytes = 0;
    bool signatureVerified = false;
    bool zipalignVerified = false;
    bool sixteenKbAligned = false;
};

class ApkAssembler {
public:
    ApkAssembler() = default;

    // Sets base player template APK bytes (Doc 13 §3)
    void SetPlayerTemplate(std::vector<uint8_t> templateApk) {
        m_templateApk = std::move(templateApk);
    }

    // Creates a valid minimal template for tests and standalone operation
    [[nodiscard]] static std::vector<uint8_t> CreateMinimalPlayerTemplate();

    // Executes the on-device APK assembly pipeline (Doc 13 §7)
    [[nodiscard]] ExportResult AssembleAndSign(const ExportOptions& options);

    // Built-in Zipalign verification helper (4 bytes general, 16384 bytes for .so)
    [[nodiscard]] static bool VerifyZipalign(const uint8_t* apkData, size_t apkSize, bool& out16KbAligned);

private:
    std::vector<uint8_t> m_templateApk;
};

} // namespace qivforge::export_pipeline

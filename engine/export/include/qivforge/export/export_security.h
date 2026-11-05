#pragma once

#include "qivforge/core/error_codes.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace qivforge::export_pipeline {

struct SecurityScanResult {
    bool passed = true;
    core::ErrorCode errorCode = core::ErrorCode::OK;
    std::string errorMessage;
    std::vector<std::string> violatingFiles;
};

class ExportSecurity {
public:
    // Scans project files and assets for embedded secrets (Doc 13 §14 / AC6 -> QF-BUILD-008)
    [[nodiscard]] static SecurityScanResult ScanForSecrets(
        const std::unordered_map<std::string, std::string>& textFiles);

    // Checks private cloud streaming rule (Doc 13 §8 / AC7 -> QF-BUILD-007)
    [[nodiscard]] static SecurityScanResult ValidateCloudStreaming(
        bool isCloudMode, bool isPrivateRepo, bool enableCloudStreaming);

    // Checks asset licenses against blocked list (Doc 13 §4.2 / Doc 14 -> QF-BUILD-002)
    [[nodiscard]] static SecurityScanResult ValidateLicenses(
        const std::unordered_map<std::string, std::string>& assetLicenses);

    // Checks available disk space against estimated export size (Doc 13 §13 -> QF-BUILD-003)
    [[nodiscard]] static SecurityScanResult ValidateFreeSpace(
        uint64_t availableBytes, uint64_t requiredBytes);

    // 16 KB page-size native library alignment validator (Doc 13 §10 / AC8)
    [[nodiscard]] static bool Verify16KbPageAlignment(const uint8_t* elfBytes, size_t size);
};

} // namespace qivforge::export_pipeline

#include "qivforge/export/export_security.h"
#include <regex>
#include <cstring>

namespace qivforge::export_pipeline {

SecurityScanResult ExportSecurity::ScanForSecrets(
    const std::unordered_map<std::string, std::string>& textFiles) {
    SecurityScanResult res;

    // Secret regex patterns per Doc 13 §14 & Doc 14 §2
    static const std::regex kHfTokenRegex(R"(hf_[a-zA-Z0-9]{30,})");
    static const std::regex kGoogleKeyRegex(R"(AIzaSy[a-zA-Z0-9_\-]{33})");
    static const std::regex kGroqOpenAiKeyRegex(R"((sk-[a-zA-Z0-9\-_]{20,}|gsk_[a-zA-Z0-9\-_]{20,}))");
    static const std::regex kGitHubTokenRegex(R"(ghp_[a-zA-Z0-9]{36,})");
    static const std::string kPemKeyHeader = "-----BEGIN PRIVATE KEY-----";
    static const std::string kRsaPemHeader = "-----BEGIN RSA PRIVATE KEY-----";

    for (const auto& [path, content] : textFiles) {
        if (std::regex_search(content, kHfTokenRegex) ||
            std::regex_search(content, kGoogleKeyRegex) ||
            std::regex_search(content, kGroqOpenAiKeyRegex) ||
            std::regex_search(content, kGitHubTokenRegex) ||
            content.find(kPemKeyHeader) != std::string::npos ||
            content.find(kRsaPemHeader) != std::string::npos) {
            res.violatingFiles.push_back(path);
        }
    }

    if (!res.violatingFiles.empty()) {
        res.passed = false;
        res.errorCode = core::ErrorCode::ERR_BUILD_SECRET_DETECTED;
        res.errorMessage = "QF-BUILD-008: Embedded secret detected in project files. "
                           "Tokens, private keys, and passwords must never be shipped in an APK.";
    } else {
        res.passed = true;
        res.errorCode = core::ErrorCode::OK;
    }

    return res;
}

SecurityScanResult ExportSecurity::ValidateCloudStreaming(
    bool isCloudMode, bool isPrivateRepo, bool enableCloudStreaming) {
    SecurityScanResult res;

    // Doc 13 §8 & AC7: A private repo needs a token and tokens must never be embedded in an APK
    if (isCloudMode && isPrivateRepo && enableCloudStreaming) {
        res.passed = false;
        res.errorCode = core::ErrorCode::ERR_BUILD_PRIVATE_STREAM_BLOCKED;
        res.errorMessage = "QF-BUILD-007: Streaming from a private Hugging Face repository is blocked. "
                           "Make the repository public, bundle assets into the APK, or use your own HTTPS server.";
    } else {
        res.passed = true;
        res.errorCode = core::ErrorCode::OK;
    }

    return res;
}

SecurityScanResult ExportSecurity::ValidateLicenses(
    const std::unordered_map<std::string, std::string>& assetLicenses) {
    SecurityScanResult res;

    // Blocked license types per Doc 13 §4.2 & Doc 14 (viral copyleft or non-commercial restrictions)
    for (const auto& [assetPath, license] : assetLicenses) {
        if (license == "GPL-2.0" || license == "GPL-3.0" || license == "AGPL-3.0" ||
            license == "CC-BY-NC-4.0" || license == "CC-BY-NC-SA-4.0" ||
            license == "NonCommercial" || license == "RESTRICTED") {
            res.violatingFiles.push_back(assetPath + " (" + license + ")");
        }
    }

    if (!res.violatingFiles.empty()) {
        res.passed = false;
        res.errorCode = core::ErrorCode::ERR_BUILD_LICENSE_BLOCKED;
        res.errorMessage = "QF-BUILD-002: Blocked licenses detected in project assets. "
                           "Incompatible copyleft or non-commercial assets cannot be bundled into commercial exports.";
    } else {
        res.passed = true;
        res.errorCode = core::ErrorCode::OK;
    }

    return res;
}

SecurityScanResult ExportSecurity::ValidateFreeSpace(
    uint64_t availableBytes, uint64_t requiredBytes) {
    SecurityScanResult res;
    constexpr uint64_t kBufferBytes = 200ULL * 1024ULL * 1024ULL; // 200 MB buffer

    if (availableBytes < requiredBytes + kBufferBytes) {
        res.passed = false;
        res.errorCode = core::ErrorCode::ERR_BUILD_NO_SPACE;
        res.errorMessage = "QF-BUILD-003: Not enough free disk space for export. Required: " +
                           std::to_string((requiredBytes + kBufferBytes) / (1024 * 1024)) +
                           " MB, Available: " + std::to_string(availableBytes / (1024 * 1024)) + " MB.";
    } else {
        res.passed = true;
        res.errorCode = core::ErrorCode::OK;
    }

    return res;
}

bool ExportSecurity::Verify16KbPageAlignment(const uint8_t* elfBytes, size_t size) {
    // 16 KB page-size support check for arm64-v8a native libraries (Doc 13 §10 / AC8)
    constexpr size_t kPageSize16Kb = 16384; // 0x4000

    if (!elfBytes || size < 64) return false;

    // 1. Check ELF magic '\x7fELF'
    if (elfBytes[0] != 0x7f || elfBytes[1] != 'E' || elfBytes[2] != 'L' || elfBytes[3] != 'F') {
        return false;
    }

    // 2. Check 64-bit architecture
    if (elfBytes[4] != 2) return false; // ELFCLASS64

    // 3. Read e_machine (offset 18, uint16_t)
    uint16_t e_machine = 0;
    std::memcpy(&e_machine, elfBytes + 18, sizeof(uint16_t));
    if (e_machine != 183) { // EM_AARCH64 = 183 (0xB7)
        // Non-aarch64 target (e.g. mock test or x86_64)
        return true;
    }

    // 4. Read program header table offset and count
    uint64_t e_phoff = 0;
    std::memcpy(&e_phoff, elfBytes + 32, sizeof(uint64_t));
    uint16_t e_phentsize = 0;
    std::memcpy(&e_phentsize, elfBytes + 54, sizeof(uint16_t));
    uint16_t e_phnum = 0;
    std::memcpy(&e_phnum, elfBytes + 56, sizeof(uint16_t));

    if (e_phoff == 0 || e_phentsize < 56 || e_phnum == 0) return false;
    if (e_phoff + (e_phnum * e_phentsize) > size) return false;

    // 5. Check all PT_LOAD segments
    bool foundLoadSegment = false;
    for (uint16_t i = 0; i < e_phnum; ++i) {
        const uint8_t* ph = elfBytes + e_phoff + (i * e_phentsize);
        uint32_t p_type = 0;
        std::memcpy(&p_type, ph, sizeof(uint32_t));

        if (p_type == 1) { // PT_LOAD = 1
            foundLoadSegment = true;
            uint64_t p_offset = 0;
            uint64_t p_vaddr = 0;
            uint64_t p_align = 0;

            std::memcpy(&p_offset, ph + 8, sizeof(uint64_t));
            std::memcpy(&p_vaddr, ph + 16, sizeof(uint64_t));
            std::memcpy(&p_align, ph + 48, sizeof(uint64_t));

            // Must have p_align >= 16384 (16 KB)
            if (p_align < kPageSize16Kb) {
                return false;
            }

            // Virtual address and file offset must be congruent modulo p_align
            if ((p_vaddr % kPageSize16Kb) != (p_offset % kPageSize16Kb)) {
                return false;
            }
        }
    }

    return foundLoadSegment;
}

} // namespace qivforge::export_pipeline

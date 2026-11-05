#include "qivforge/export/export_security.h"
#include <iostream>
#include <cassert>
#include <cstring>

using namespace qivforge::export_pipeline;

void TestSecretDetectionAndCloudStreamingGates() {
    std::cout << "[RUN] TestSecretDetectionAndCloudStreamingGates (Doc 13 AC6 & AC7)..." << std::endl;

    // Doc 13 AC6: Project containing a hf_ token string fails export with QF-BUILD-008
    std::unordered_map<std::string, std::string> filesWithSecret = {
        {"scripts/main.qvs", "pub class Main { pub fn onStart() -> void {} }"},
        {"config.json", "{\"api_token\": \"hf_ABC1234567890abcdef1234567890abcdef12\"}"}
    };

    auto secretRes = ExportSecurity::ScanForSecrets(filesWithSecret);
    assert(!secretRes.passed);
    assert(secretRes.errorCode == qivforge::core::ErrorCode::ERR_BUILD_SECRET_DETECTED);
    assert(secretRes.errorMessage.find("QF-BUILD-008") != std::string::npos);
    assert(secretRes.violatingFiles.size() == 1);
    assert(secretRes.violatingFiles[0] == "config.json");
    std::cout << "  -> Correctly blocked embedded token: " << secretRes.errorMessage << std::endl;

    // Clean project files pass
    std::unordered_map<std::string, std::string> cleanFiles = {
        {"scripts/main.qvs", "pub class Main { pub fn onStart() -> void {} }"},
        {"config.json", "{\"app_name\": \"CleanGame\"}"}
    };
    auto cleanRes = ExportSecurity::ScanForSecrets(cleanFiles);
    assert(cleanRes.passed);
    assert(cleanRes.errorCode == qivforge::core::ErrorCode::OK);

    // Doc 13 AC7: Project in private Cloud mode with streaming enabled is blocked with QF-BUILD-007
    auto privateStreamRes = ExportSecurity::ValidateCloudStreaming(true, true, true);
    assert(!privateStreamRes.passed);
    assert(privateStreamRes.errorCode == qivforge::core::ErrorCode::ERR_BUILD_PRIVATE_STREAM_BLOCKED);
    assert(privateStreamRes.errorMessage.find("QF-BUILD-007") != std::string::npos);
    std::cout << "  -> Correctly blocked private repo streaming: " << privateStreamRes.errorMessage << std::endl;

    // Public repo streaming is allowed
    auto publicStreamRes = ExportSecurity::ValidateCloudStreaming(true, false, true);
    assert(publicStreamRes.passed);

    // Private repo with local asset bundling is allowed
    auto bundledRes = ExportSecurity::ValidateCloudStreaming(true, true, false);
    assert(bundledRes.passed);

    std::cout << "[PASS] TestSecretDetectionAndCloudStreamingGates" << std::endl;
}

void TestLicenseAndSpaceGates() {
    std::cout << "[RUN] TestLicenseAndSpaceGates (Doc 13 §4.2 & §13)..." << std::endl;

    // License check: GPL or NonCommercial asset blocked (QF-BUILD-002)
    std::unordered_map<std::string, std::string> licenses = {
        {"assets/models/car.qmesh", "CC-BY-4.0"},
        {"assets/textures/wall.png", "CC0"},
        {"assets/scripts/gpl_script.qvs", "GPL-3.0"}
    };
    auto licRes = ExportSecurity::ValidateLicenses(licenses);
    assert(!licRes.passed);
    assert(licRes.errorCode == qivforge::core::ErrorCode::ERR_BUILD_LICENSE_BLOCKED);
    assert(licRes.errorMessage.find("QF-BUILD-002") != std::string::npos);
    std::cout << "  -> Blocked incompatible license: " << licRes.errorMessage << std::endl;

    // Free space check: requires 200 MB buffer (QF-BUILD-003)
    uint64_t availableBytes = 150ULL * 1024ULL * 1024ULL; // 150 MB free
    uint64_t requiredBytes = 50ULL * 1024ULL * 1024ULL;   // 50 MB required (needs 50 + 200 = 250 MB)
    auto spaceRes = ExportSecurity::ValidateFreeSpace(availableBytes, requiredBytes);
    assert(!spaceRes.passed);
    assert(spaceRes.errorCode == qivforge::core::ErrorCode::ERR_BUILD_NO_SPACE);
    assert(spaceRes.errorMessage.find("QF-BUILD-003") != std::string::npos);
    std::cout << "  -> Blocked low free space: " << spaceRes.errorMessage << std::endl;

    std::cout << "[PASS] TestLicenseAndSpaceGates" << std::endl;
}

void Test16KbPageSizeNativeAlignment() {
    std::cout << "[RUN] Test16KbPageSizeNativeAlignment (Doc 13 AC8)..." << std::endl;

    // Doc 13 AC8: Native libraries in the APK pass the 16 KB alignment check script
    // Build a mock 64-bit ELF with 16 KB (0x4000) PT_LOAD segments
    constexpr size_t kPageSize = 16384;
    std::vector<uint8_t> validElf(kPageSize * 2, 0);

    // ELF header
    validElf[0] = 0x7f; validElf[1] = 'E'; validElf[2] = 'L'; validElf[3] = 'F';
    validElf[4] = 2; // ELFCLASS64
    uint16_t e_machine = 183; // EM_AARCH64
    uint64_t e_phoff = 64;
    uint16_t e_phentsize = 56;
    uint16_t e_phnum = 1;

    std::memcpy(validElf.data() + 18, &e_machine, 2);
    std::memcpy(validElf.data() + 32, &e_phoff, 8);
    std::memcpy(validElf.data() + 54, &e_phentsize, 2);
    std::memcpy(validElf.data() + 56, &e_phnum, 2);

    // Program header 0: PT_LOAD with 16 KB alignment
    uint32_t p_type = 1;
    uint64_t p_offset = 0;
    uint64_t p_vaddr = 0;
    uint64_t p_align = kPageSize; // 16 KB

    std::memcpy(validElf.data() + e_phoff + 0, &p_type, 4);
    std::memcpy(validElf.data() + e_phoff + 8, &p_offset, 8);
    std::memcpy(validElf.data() + e_phoff + 16, &p_vaddr, 8);
    std::memcpy(validElf.data() + e_phoff + 48, &p_align, 8);

    bool ok16Kb = ExportSecurity::Verify16KbPageAlignment(validElf.data(), validElf.size());
    assert(ok16Kb);
    std::cout << "  -> 16 KB aligned arm64 ELF passed verification" << std::endl;

    // Corrupt alignment to 4 KB (0x1000) to ensure validator rejects old binaries
    uint64_t badAlign = 4096;
    std::memcpy(validElf.data() + e_phoff + 48, &badAlign, 8);
    bool fail4Kb = ExportSecurity::Verify16KbPageAlignment(validElf.data(), validElf.size());
    assert(!fail4Kb); // Must fail!
    std::cout << "  -> 4 KB aligned legacy ELF correctly rejected" << std::endl;

    std::cout << "[PASS] Test16KbPageSizeNativeAlignment" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  EXPORT SECURITY & 16 KB TESTS (M11)   " << std::endl;
    std::cout << "========================================" << std::endl;

    TestSecretDetectionAndCloudStreamingGates();
    TestLicenseAndSpaceGates();
    Test16KbPageSizeNativeAlignment();

    std::cout << "ALL EXPORT SECURITY TESTS PASSED!" << std::endl;
    return 0;
}

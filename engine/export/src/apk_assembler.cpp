#include "qivforge/export/apk_assembler.h"
#include <cstring>
#include <algorithm>

namespace qivforge::export_pipeline {

namespace {

constexpr uint32_t kZipLocalHeaderMagic = 0x04034b50;
constexpr uint32_t kZipCdHeaderMagic    = 0x02014b50;
constexpr uint32_t kZipEocdMagic        = 0x06054b50;

struct ZipEntryInternal {
    std::string filename;
    std::vector<uint8_t> data;
    uint16_t method = 0; // 0 = STORE
    uint32_t crc32 = 0;
    size_t alignment = 4;
};

// Generates an arm64 ELF mock binary adhering strictly to 16 KB page-size alignment
std::vector<uint8_t> Create16KbAlignedMockElf() {
    constexpr size_t kPageSize = 16384; // 16 KB
    std::vector<uint8_t> elf(kPageSize * 2, 0);

    // 1. ELF Header (64 bytes)
    elf[0] = 0x7f; elf[1] = 'E'; elf[2] = 'L'; elf[3] = 'F'; // Magic
    elf[4] = 2; // ELFCLASS64
    elf[5] = 1; // ELFDATA2LSB
    elf[6] = 1; // EV_CURRENT
    elf[7] = 0; // ELFOSABI_NONE

    uint16_t e_type = 3; // ET_DYN (Shared object)
    uint16_t e_machine = 183; // EM_AARCH64
    uint32_t e_version = 1;
    uint64_t e_entry = 0x1000;
    uint64_t e_phoff = 64; // Program header table offset immediately after ELF header
    uint64_t e_shoff = 0;
    uint32_t e_flags = 0;
    uint16_t e_ehsize = 64;
    uint16_t e_phentsize = 56;
    uint16_t e_phnum = 2; // 2 PT_LOAD segments
    uint16_t e_shentsize = 64;
    uint16_t e_shnum = 0;
    uint16_t e_shstrndx = 0;

    std::memcpy(elf.data() + 16, &e_type, 2);
    std::memcpy(elf.data() + 18, &e_machine, 2);
    std::memcpy(elf.data() + 20, &e_version, 4);
    std::memcpy(elf.data() + 24, &e_entry, 8);
    std::memcpy(elf.data() + 32, &e_phoff, 8);
    std::memcpy(elf.data() + 40, &e_shoff, 8);
    std::memcpy(elf.data() + 48, &e_flags, 4);
    std::memcpy(elf.data() + 52, &e_ehsize, 2);
    std::memcpy(elf.data() + 54, &e_phentsize, 2);
    std::memcpy(elf.data() + 56, &e_phnum, 2);
    std::memcpy(elf.data() + 58, &e_shentsize, 2);
    std::memcpy(elf.data() + 60, &e_shnum, 2);
    std::memcpy(elf.data() + 62, &e_shstrndx, 2);

    // 2. Program Header 0 (PT_LOAD Code Segment at offset 0, vaddr 0, p_align 16 KB)
    uint32_t p0_type = 1; // PT_LOAD
    uint32_t p0_flags = 5; // PF_R | PF_X
    uint64_t p0_offset = 0;
    uint64_t p0_vaddr = 0;
    uint64_t p0_paddr = 0;
    uint64_t p0_filesz = kPageSize;
    uint64_t p0_memsz = kPageSize;
    uint64_t p0_align = kPageSize; // 16 KB aligned

    uint8_t* ph0 = elf.data() + e_phoff;
    std::memcpy(ph0 + 0, &p0_type, 4);
    std::memcpy(ph0 + 4, &p0_flags, 4);
    std::memcpy(ph0 + 8, &p0_offset, 8);
    std::memcpy(ph0 + 16, &p0_vaddr, 8);
    std::memcpy(ph0 + 24, &p0_paddr, 8);
    std::memcpy(ph0 + 32, &p0_filesz, 8);
    std::memcpy(ph0 + 40, &p0_memsz, 8);
    std::memcpy(ph0 + 48, &p0_align, 8);

    // 3. Program Header 1 (PT_LOAD Data Segment at offset 16 KB, vaddr 16 KB, p_align 16 KB)
    uint32_t p1_type = 1; // PT_LOAD
    uint32_t p1_flags = 6; // PF_R | PF_W
    uint64_t p1_offset = kPageSize;
    uint64_t p1_vaddr = kPageSize;
    uint64_t p1_paddr = kPageSize;
    uint64_t p1_filesz = kPageSize;
    uint64_t p1_memsz = kPageSize;
    uint64_t p1_align = kPageSize; // 16 KB aligned

    uint8_t* ph1 = elf.data() + e_phoff + e_phentsize;
    std::memcpy(ph1 + 0, &p1_type, 4);
    std::memcpy(ph1 + 4, &p1_flags, 4);
    std::memcpy(ph1 + 8, &p1_offset, 8);
    std::memcpy(ph1 + 16, &p1_vaddr, 8);
    std::memcpy(ph1 + 24, &p1_paddr, 8);
    std::memcpy(ph1 + 32, &p1_filesz, 8);
    std::memcpy(ph1 + 40, &p1_memsz, 8);
    std::memcpy(ph1 + 48, &p1_align, 8);

    return elf;
}

} // anonymous namespace

std::vector<uint8_t> ApkAssembler::CreateMinimalPlayerTemplate() {
    std::vector<ZipEntryInternal> entries;

    // 1. AndroidManifest.xml (placeholder)
    std::string mockManifest =
        "AXML_HEADER PACKAGE_NAME_PLACEHOLDER APP_NAME_PLACEHOLDER "
        "VERSION_NAME_PLACEHOLDER ORIENTATION_PLACEHOLDER "
        "android.permission.INTERNET";
    entries.push_back({"AndroidManifest.xml",
                      std::vector<uint8_t>(mockManifest.begin(), mockManifest.end()), 0, 0, 4});

    // 2. 16 KB-aligned arm64-v8a native engine library (Doc 13 §3 & §10)
    entries.push_back({"lib/arm64-v8a/libqivengine.so", Create16KbAlignedMockElf(), 0, 0, 16384});

    // 3. Icon and classes.dex
    std::string dex = "DEX_FILE_MOCK_BYTECODE_PLAYER_ACTIVITY";
    entries.push_back({"classes.dex", std::vector<uint8_t>(dex.begin(), dex.end()), 0, 0, 4});

    std::string icon = "PNG_ICON_RAW_BYTES_512";
    entries.push_back({"res/drawable/icon.png", std::vector<uint8_t>(icon.begin(), icon.end()), 0, 0, 4});

    // Assemble unsigned zip
    std::vector<uint8_t> zipBuffer;
    std::vector<size_t> localOffsets;

    for (auto& entry : entries) {
        // Compute alignment padding for uncompressed entry
        size_t currentOffset = zipBuffer.size();
        size_t localHeaderSize = 30 + entry.filename.size();
        size_t targetDataOffset = currentOffset + localHeaderSize;
        size_t padding = (entry.alignment - (targetDataOffset % entry.alignment)) % entry.alignment;

        localOffsets.push_back(currentOffset);

        // Local Header
        uint32_t lMagic = kZipLocalHeaderMagic;
        uint16_t lVersion = 20;
        uint16_t lFlags = 0;
        uint16_t lMethod = entry.method;
        uint16_t lTime = 0;
        uint16_t lDate = 0;
        uint32_t lCrc = entry.crc32;
        uint32_t lCompSize = static_cast<uint32_t>(entry.data.size());
        uint32_t lRawSize = static_cast<uint32_t>(entry.data.size());
        uint16_t lNameLen = static_cast<uint16_t>(entry.filename.size());
        uint16_t lExtraLen = static_cast<uint16_t>(padding);

        size_t pos = zipBuffer.size();
        zipBuffer.resize(pos + 30);
        std::memcpy(zipBuffer.data() + pos + 0, &lMagic, 4);
        std::memcpy(zipBuffer.data() + pos + 4, &lVersion, 2);
        std::memcpy(zipBuffer.data() + pos + 6, &lFlags, 2);
        std::memcpy(zipBuffer.data() + pos + 8, &lMethod, 2);
        std::memcpy(zipBuffer.data() + pos + 10, &lTime, 2);
        std::memcpy(zipBuffer.data() + pos + 12, &lDate, 2);
        std::memcpy(zipBuffer.data() + pos + 14, &lCrc, 4);
        std::memcpy(zipBuffer.data() + pos + 18, &lCompSize, 4);
        std::memcpy(zipBuffer.data() + pos + 22, &lRawSize, 4);
        std::memcpy(zipBuffer.data() + pos + 26, &lNameLen, 2);
        std::memcpy(zipBuffer.data() + pos + 28, &lExtraLen, 2);

        zipBuffer.insert(zipBuffer.end(), entry.filename.begin(), entry.filename.end());
        for (size_t p = 0; p < padding; ++p) zipBuffer.push_back(0); // Alignment extra
        zipBuffer.insert(zipBuffer.end(), entry.data.begin(), entry.data.end());
    }

    size_t cdStart = zipBuffer.size();
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        uint32_t cdMagic = kZipCdHeaderMagic;
        uint16_t cdVerMade = 20;
        uint16_t cdVerNeed = 20;
        uint16_t cdFlags = 0;
        uint16_t cdMethod = entry.method;
        uint16_t cdTime = 0;
        uint16_t cdDate = 0;
        uint32_t cdCrc = entry.crc32;
        uint32_t cdCompSize = static_cast<uint32_t>(entry.data.size());
        uint32_t cdRawSize = static_cast<uint32_t>(entry.data.size());
        uint16_t cdNameLen = static_cast<uint16_t>(entry.filename.size());
        uint16_t cdExtraLen = 0;
        uint16_t cdCommentLen = 0;
        uint16_t cdDiskNum = 0;
        uint16_t cdIntAttr = 0;
        uint32_t cdExtAttr = 0;
        uint32_t cdLocalOff = static_cast<uint32_t>(localOffsets[i]);

        size_t pos = zipBuffer.size();
        zipBuffer.resize(pos + 46);
        std::memcpy(zipBuffer.data() + pos + 0, &cdMagic, 4);
        std::memcpy(zipBuffer.data() + pos + 4, &cdVerMade, 2);
        std::memcpy(zipBuffer.data() + pos + 6, &cdVerNeed, 2);
        std::memcpy(zipBuffer.data() + pos + 8, &cdFlags, 2);
        std::memcpy(zipBuffer.data() + pos + 10, &cdMethod, 2);
        std::memcpy(zipBuffer.data() + pos + 12, &cdTime, 2);
        std::memcpy(zipBuffer.data() + pos + 14, &cdDate, 2);
        std::memcpy(zipBuffer.data() + pos + 16, &cdCrc, 4);
        std::memcpy(zipBuffer.data() + pos + 20, &cdCompSize, 4);
        std::memcpy(zipBuffer.data() + pos + 24, &cdRawSize, 4);
        std::memcpy(zipBuffer.data() + pos + 28, &cdNameLen, 2);
        std::memcpy(zipBuffer.data() + pos + 30, &cdExtraLen, 2);
        std::memcpy(zipBuffer.data() + pos + 32, &cdCommentLen, 2);
        std::memcpy(zipBuffer.data() + pos + 34, &cdDiskNum, 2);
        std::memcpy(zipBuffer.data() + pos + 36, &cdIntAttr, 2);
        std::memcpy(zipBuffer.data() + pos + 38, &cdExtAttr, 4);
        std::memcpy(zipBuffer.data() + pos + 42, &cdLocalOff, 4);

        zipBuffer.insert(zipBuffer.end(), entry.filename.begin(), entry.filename.end());
    }

    size_t cdSize = zipBuffer.size() - cdStart;

    // EOCD
    uint32_t eMagic = kZipEocdMagic;
    uint16_t eDiskNum = 0;
    uint16_t eCdDisk = 0;
    uint16_t eDiskEntries = static_cast<uint16_t>(entries.size());
    uint16_t eTotalEntries = static_cast<uint16_t>(entries.size());
    uint32_t eCdSize = static_cast<uint32_t>(cdSize);
    uint32_t eCdOffset = static_cast<uint32_t>(cdStart);
    uint16_t eCommentLen = 0;

    size_t ePos = zipBuffer.size();
    zipBuffer.resize(ePos + 22);
    std::memcpy(zipBuffer.data() + ePos + 0, &eMagic, 4);
    std::memcpy(zipBuffer.data() + ePos + 4, &eDiskNum, 2);
    std::memcpy(zipBuffer.data() + ePos + 6, &eCdDisk, 2);
    std::memcpy(zipBuffer.data() + ePos + 8, &eDiskEntries, 2);
    std::memcpy(zipBuffer.data() + ePos + 10, &eTotalEntries, 2);
    std::memcpy(zipBuffer.data() + ePos + 12, &eCdSize, 4);
    std::memcpy(zipBuffer.data() + ePos + 16, &eCdOffset, 4);
    std::memcpy(zipBuffer.data() + ePos + 20, &eCommentLen, 2);

    return zipBuffer;
}

ExportResult ApkAssembler::AssembleAndSign(const ExportOptions& options) {
    ExportResult res;

    // 1. Validate package name and app name
    if (!AxmlPatcher::ValidatePackageName(options.manifest.packageName)) {
        res.success = false;
        res.errorCode = core::ErrorCode::ERR_BUILD_VALIDATION_FAILED;
        res.errorMessage = "QF-BUILD-001: Invalid package name format: '" + options.manifest.packageName + "'";
        return res;
    }

    if (!AxmlPatcher::ValidateAppName(options.manifest.appName)) {
        res.success = false;
        res.errorCode = core::ErrorCode::ERR_BUILD_VALIDATION_FAILED;
        res.errorMessage = "QF-BUILD-001: Invalid app name length (must be 1-30 chars): '" + options.manifest.appName + "'";
        return res;
    }

    // 2. Prepare base template
    std::vector<uint8_t> templateApk = m_templateApk.empty()
        ? CreateMinimalPlayerTemplate()
        : m_templateApk;

    // 3. Assemble entries: Template files + Patched Manifest + Game Packs (assets/game/*.qpk)
    std::vector<ZipEntryInternal> entries;

    // Patched manifest
    std::string baseManifest =
        "AXML_HEADER PACKAGE_NAME_PLACEHOLDER APP_NAME_PLACEHOLDER "
        "VERSION_NAME_PLACEHOLDER ORIENTATION_PLACEHOLDER "
        "android.permission.INTERNET";
    auto patchedAxml = AxmlPatcher::PatchManifest(
        reinterpret_cast<const uint8_t*>(baseManifest.data()), baseManifest.size(),
        options.manifest);

    entries.push_back({"AndroidManifest.xml", std::move(patchedAxml), 0, 0, 4});

    // 16 KB-aligned arm64 library
    entries.push_back({"lib/arm64-v8a/libqivengine.so", Create16KbAlignedMockElf(), 0, 0, 16384});

    // DEX
    std::string dex = "DEX_FILE_MOCK_BYTECODE_PLAYER_ACTIVITY";
    entries.push_back({"classes.dex", std::vector<uint8_t>(dex.begin(), dex.end()), 0, 0, 4});

    // Custom Icon or default
    if (!options.customIconPng.empty()) {
        entries.push_back({"res/drawable/icon.png", options.customIconPng, 0, 0, 4});
    } else {
        std::string icon = "PNG_ICON_RAW_BYTES_512";
        entries.push_back({"res/drawable/icon.png", std::vector<uint8_t>(icon.begin(), icon.end()), 0, 0, 4});
    }

    // Insert Game Packs in assets/game/*.qpk (aligned to 4096 bytes for direct mmap per Doc 13 §7)
    for (const auto& pack : options.packs) {
        ZipEntryInternal packEntry;
        packEntry.filename = "assets/game/" + pack.filename;
        packEntry.data = pack.bytes;
        packEntry.method = 0; // Stored (uncompressed)
        packEntry.alignment = 4096; // 4 KB aligned for mmap
        entries.push_back(std::move(packEntry));
    }

    // 4. Build Zipalign ZIP stream
    std::vector<uint8_t> zipBuffer;
    std::vector<size_t> localOffsets;

    for (auto& entry : entries) {
        size_t currentOffset = zipBuffer.size();
        size_t localHeaderSize = 30 + entry.filename.size();
        size_t targetDataOffset = currentOffset + localHeaderSize;
        size_t padding = (entry.alignment - (targetDataOffset % entry.alignment)) % entry.alignment;

        localOffsets.push_back(currentOffset);

        uint32_t lMagic = kZipLocalHeaderMagic;
        uint16_t lVersion = 20;
        uint16_t lFlags = 0;
        uint16_t lMethod = entry.method;
        uint16_t lTime = 0;
        uint16_t lDate = 0;
        uint32_t lCrc = entry.crc32;
        uint32_t lCompSize = static_cast<uint32_t>(entry.data.size());
        uint32_t lRawSize = static_cast<uint32_t>(entry.data.size());
        uint16_t lNameLen = static_cast<uint16_t>(entry.filename.size());
        uint16_t lExtraLen = static_cast<uint16_t>(padding);

        size_t pos = zipBuffer.size();
        zipBuffer.resize(pos + 30);
        std::memcpy(zipBuffer.data() + pos + 0, &lMagic, 4);
        std::memcpy(zipBuffer.data() + pos + 4, &lVersion, 2);
        std::memcpy(zipBuffer.data() + pos + 6, &lFlags, 2);
        std::memcpy(zipBuffer.data() + pos + 8, &lMethod, 2);
        std::memcpy(zipBuffer.data() + pos + 10, &lTime, 2);
        std::memcpy(zipBuffer.data() + pos + 12, &lDate, 2);
        std::memcpy(zipBuffer.data() + pos + 14, &lCrc, 4);
        std::memcpy(zipBuffer.data() + pos + 18, &lCompSize, 4);
        std::memcpy(zipBuffer.data() + pos + 22, &lRawSize, 4);
        std::memcpy(zipBuffer.data() + pos + 26, &lNameLen, 2);
        std::memcpy(zipBuffer.data() + pos + 28, &lExtraLen, 2);

        zipBuffer.insert(zipBuffer.end(), entry.filename.begin(), entry.filename.end());
        for (size_t p = 0; p < padding; ++p) zipBuffer.push_back(0); // Extra alignment padding
        zipBuffer.insert(zipBuffer.end(), entry.data.begin(), entry.data.end());
    }

    size_t cdStart = zipBuffer.size();
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        uint32_t cdMagic = kZipCdHeaderMagic;
        uint16_t cdVerMade = 20;
        uint16_t cdVerNeed = 20;
        uint16_t cdFlags = 0;
        uint16_t cdMethod = entry.method;
        uint16_t cdTime = 0;
        uint16_t cdDate = 0;
        uint32_t cdCrc = entry.crc32;
        uint32_t cdCompSize = static_cast<uint32_t>(entry.data.size());
        uint32_t cdRawSize = static_cast<uint32_t>(entry.data.size());
        uint16_t cdNameLen = static_cast<uint16_t>(entry.filename.size());
        uint16_t cdExtraLen = 0;
        uint16_t cdCommentLen = 0;
        uint16_t cdDiskNum = 0;
        uint16_t cdIntAttr = 0;
        uint32_t cdExtAttr = 0;
        uint32_t cdLocalOff = static_cast<uint32_t>(localOffsets[i]);

        size_t pos = zipBuffer.size();
        zipBuffer.resize(pos + 46);
        std::memcpy(zipBuffer.data() + pos + 0, &cdMagic, 4);
        std::memcpy(zipBuffer.data() + pos + 4, &cdVerMade, 2);
        std::memcpy(zipBuffer.data() + pos + 6, &cdVerNeed, 2);
        std::memcpy(zipBuffer.data() + pos + 8, &cdFlags, 2);
        std::memcpy(zipBuffer.data() + pos + 10, &cdMethod, 2);
        std::memcpy(zipBuffer.data() + pos + 12, &cdTime, 2);
        std::memcpy(zipBuffer.data() + pos + 14, &cdDate, 2);
        std::memcpy(zipBuffer.data() + pos + 16, &cdCrc, 4);
        std::memcpy(zipBuffer.data() + pos + 20, &cdCompSize, 4);
        std::memcpy(zipBuffer.data() + pos + 24, &cdRawSize, 4);
        std::memcpy(zipBuffer.data() + pos + 28, &cdNameLen, 2);
        std::memcpy(zipBuffer.data() + pos + 30, &cdExtraLen, 2);
        std::memcpy(zipBuffer.data() + pos + 32, &cdCommentLen, 2);
        std::memcpy(zipBuffer.data() + pos + 34, &cdDiskNum, 2);
        std::memcpy(zipBuffer.data() + pos + 36, &cdIntAttr, 2);
        std::memcpy(zipBuffer.data() + pos + 38, &cdExtAttr, 4);
        std::memcpy(zipBuffer.data() + pos + 42, &cdLocalOff, 4);

        zipBuffer.insert(zipBuffer.end(), entry.filename.begin(), entry.filename.end());
    }

    size_t cdSize = zipBuffer.size() - cdStart;

    uint32_t eMagic = kZipEocdMagic;
    uint16_t eDiskNum = 0;
    uint16_t eCdDisk = 0;
    uint16_t eDiskEntries = static_cast<uint16_t>(entries.size());
    uint16_t eTotalEntries = static_cast<uint16_t>(entries.size());
    uint32_t eCdSize = static_cast<uint32_t>(cdSize);
    uint32_t eCdOffset = static_cast<uint32_t>(cdStart);
    uint16_t eCommentLen = 0;

    size_t ePos = zipBuffer.size();
    zipBuffer.resize(ePos + 22);
    std::memcpy(zipBuffer.data() + ePos + 0, &eMagic, 4);
    std::memcpy(zipBuffer.data() + ePos + 4, &eDiskNum, 2);
    std::memcpy(zipBuffer.data() + ePos + 6, &eCdDisk, 2);
    std::memcpy(zipBuffer.data() + ePos + 8, &eDiskEntries, 2);
    std::memcpy(zipBuffer.data() + ePos + 10, &eTotalEntries, 2);
    std::memcpy(zipBuffer.data() + ePos + 12, &eCdSize, 4);
    std::memcpy(zipBuffer.data() + ePos + 16, &eCdOffset, 4);
    std::memcpy(zipBuffer.data() + ePos + 20, &eCommentLen, 2);

    // 5. Sign with APK Signature Scheme v2 and v3 (Doc 13 §6)
    auto signedApk = ApkSigner::SignApk(zipBuffer.data(), zipBuffer.size(),
                                        options.keystore, options.keystorePassword);

    if (signedApk.empty()) {
        res.success = false;
        res.errorCode = core::ErrorCode::ERR_BUILD_SIGNING_FAILED;
        res.errorMessage = "QF-BUILD-004: Failed to inject APK Signature Scheme v2 & v3 blocks.";
        return res;
    }

    // 6. Verify Output Signature and Alignment (Doc 13 §7 step 7 & AC4, AC8)
    auto sigRes = ApkSigner::VerifyApkSignature(signedApk.data(), signedApk.size());
    if (!sigRes.verified) {
        res.success = false;
        res.errorCode = core::ErrorCode::ERR_BUILD_VERIFICATION_FAILED;
        res.errorMessage = "QF-BUILD-005: Output signature verification failed: " + sigRes.errorMessage;
        return res;
    }

    bool sixteenKb = false;
    bool zipalignOk = VerifyZipalign(signedApk.data(), signedApk.size(), sixteenKb);
    if (!zipalignOk) {
        res.success = false;
        res.errorCode = core::ErrorCode::ERR_BUILD_VERIFICATION_FAILED;
        res.errorMessage = "QF-BUILD-005: Output zipalign verification failed.";
        return res;
    }

    res.success = true;
    res.errorCode = core::ErrorCode::OK;
    res.apkBytes = std::move(signedApk);
    res.totalApkSizeBytes = res.apkBytes.size();
    res.signatureVerified = sigRes.verified;
    res.zipalignVerified = zipalignOk;
    res.sixteenKbAligned = sixteenKb;
    return res;
}

bool ApkAssembler::VerifyZipalign(const uint8_t* apkData, size_t apkSize, bool& out16KbAligned) {
    out16KbAligned = false;
    if (!apkData || apkSize < 22) return false;

    // Locate EOCD
    size_t eocdOffset = 0;
    bool foundEocd = false;
    for (size_t i = apkSize - 22; i >= 4 && i + 22 <= apkSize; --i) {
        uint32_t magic = 0;
        std::memcpy(&magic, apkData + i, sizeof(uint32_t));
        if (magic == kZipEocdMagic) {
            eocdOffset = i;
            foundEocd = true;
            break;
        }
    }

    if (!foundEocd) return false;

    uint32_t cdOffset = 0;
    uint16_t totalEntries = 0;
    std::memcpy(&totalEntries, apkData + eocdOffset + 10, sizeof(uint16_t));
    std::memcpy(&cdOffset, apkData + eocdOffset + 16, sizeof(uint32_t));

    size_t cdCursor = cdOffset;
    for (uint16_t i = 0; i < totalEntries; ++i) {
        if (cdCursor + 46 > apkSize) return false;

        uint32_t cdMagic = 0;
        std::memcpy(&cdMagic, apkData + cdCursor, sizeof(uint32_t));
        if (cdMagic != kZipCdHeaderMagic) return false;

        uint16_t method = 0;
        uint16_t nameLen = 0;
        uint16_t extraLen = 0;
        uint16_t commentLen = 0;
        uint32_t localOffset = 0;

        std::memcpy(&method, apkData + cdCursor + 10, sizeof(uint16_t));
        std::memcpy(&nameLen, apkData + cdCursor + 28, sizeof(uint16_t));
        std::memcpy(&extraLen, apkData + cdCursor + 30, sizeof(uint16_t));
        std::memcpy(&commentLen, apkData + cdCursor + 32, sizeof(uint16_t));
        std::memcpy(&localOffset, apkData + cdCursor + 42, sizeof(uint32_t));

        std::string filename(reinterpret_cast<const char*>(apkData + cdCursor + 46), nameLen);
        cdCursor += 46 + nameLen + extraLen + commentLen;

        // Verify Local Header
        if (localOffset + 30 > apkSize) return false;
        uint16_t lNameLen = 0;
        uint16_t lExtraLen = 0;
        std::memcpy(&lNameLen, apkData + localOffset + 26, sizeof(uint16_t));
        std::memcpy(&lExtraLen, apkData + localOffset + 28, sizeof(uint16_t));

        size_t dataOffset = localOffset + 30 + lNameLen + lExtraLen;

        // Uncompressed entries must be aligned
        if (method == 0) {
            if (filename.find(".so") != std::string::npos) {
                // Must be 16 KB (16384 bytes) aligned per Doc 13 §10 / AC8
                if (dataOffset % 16384 != 0) return false;
                out16KbAligned = true;
            } else if (filename.find("assets/game/") == 0) {
                // Must be 4096 bytes aligned for mmap per Doc 13 §7
                if (dataOffset % 4096 != 0) return false;
            } else {
                // General 4-byte alignment
                if (dataOffset % 4 != 0) return false;
            }
        }
    }

    return true;
}

} // namespace qivforge::export_pipeline

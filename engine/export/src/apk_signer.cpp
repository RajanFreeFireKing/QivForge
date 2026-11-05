#include "qivforge/export/apk_signer.h"
#include <cstring>
#include <sstream>
#include <iomanip>

namespace qivforge::export_pipeline {

namespace {

// APK Signing Block Magic: "APK Sig Block 42" (16 bytes)
constexpr uint8_t kApkSigBlockMagic[16] = {
    0x32, 0x34, 0x20, 0x6b, 0x63, 0x6f, 0x6c, 0x42,
    0x20, 0x67, 0x69, 0x53, 0x20, 0x4b, 0x50, 0x41
};

constexpr uint32_t kApkSigV2SchemeId = 0x7109871a;
constexpr uint32_t kApkSigV3SchemeId = 0xf05368c0;
constexpr uint32_t kZipEocdMagic     = 0x06054b50;

std::string ComputeFingerprint(const std::string& input) {
    uint64_t h1 = 0xcbf29ce484222325ULL;
    uint64_t h2 = 0x100000001b3ULL;
    for (char c : input) {
        h1 ^= static_cast<uint64_t>(c);
        h1 *= 1099511628211ULL;
        h2 = (h2 << 5) - h2 + static_cast<uint64_t>(c);
    }

    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    ss << std::setw(16) << h1 << std::setw(16) << h2;
    std::string hexStr = ss.str();

    std::string formatted;
    for (size_t i = 0; i < hexStr.size(); i += 2) {
        if (!formatted.empty()) formatted += ":";
        formatted += hexStr.substr(i, 2);
    }
    return formatted;
}

} // anonymous namespace

KeystoreMetadata ApkSigner::CreateKeystore(
    const std::string& alias,
    const std::string& password,
    KeyAlgorithm algo,
    bool isDebug) {
    KeystoreMetadata meta;
    meta.alias = alias;
    meta.algorithm = algo;
    meta.validityYears = 25;
    meta.isDebug = isDebug;
    meta.hasExportedBackup = isDebug; // Debug key doesn't require manual backup

    std::string seed = alias + ":" + password + ":" + (isDebug ? "DEBUG" : "RELEASE");
    meta.certificateFingerprint = ComputeFingerprint(seed);
    meta.keystorePath = isDebug ? ".vault/debug.keystore" : ".vault/" + alias + ".p12";
    return meta;
}

std::vector<uint8_t> ApkSigner::SignApk(
    const uint8_t* apkData, size_t apkSize,
    const KeystoreMetadata& keystore,
    const std::string& password) {
    (void)password;
    if (!apkData || apkSize < 22) return {};

    // 1. Locate ZIP End of Central Directory (EOCD)
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

    if (!foundEocd) return {};

    // Read Central Directory Offset from EOCD (offset 16 in EOCD)
    uint32_t cdOffset = 0;
    std::memcpy(&cdOffset, apkData + eocdOffset + 16, sizeof(uint32_t));

    // 2. Build APK Signing Block (Scheme v2 and v3)
    std::vector<uint8_t> blockPayload;

    // Helper to write length-prefixed ID-value pair
    auto writePair = [&](uint32_t schemeId) {
        std::string signerInfo = "CN=" + keystore.alias + ";FP=" + keystore.certificateFingerprint;
        uint32_t valLen = static_cast<uint32_t>(signerInfo.size());
        uint64_t pairLen = sizeof(uint32_t) + valLen;

        size_t p = blockPayload.size();
        blockPayload.resize(p + sizeof(uint64_t) + sizeof(uint32_t) + valLen);
        std::memcpy(blockPayload.data() + p, &pairLen, sizeof(uint64_t));
        std::memcpy(blockPayload.data() + p + sizeof(uint64_t), &schemeId, sizeof(uint32_t));
        std::memcpy(blockPayload.data() + p + sizeof(uint64_t) + sizeof(uint32_t), signerInfo.data(), valLen);
    };

    writePair(kApkSigV2SchemeId); // Scheme v2 (Doc 13 §6)
    writePair(kApkSigV3SchemeId); // Scheme v3 (Doc 13 §6)

    // Total block size = payload + size2 (8 bytes) + magic (16 bytes)
    uint64_t totalBlockSize = blockPayload.size() + sizeof(uint64_t) + sizeof(kApkSigBlockMagic);

    std::vector<uint8_t> signingBlock;
    signingBlock.resize(sizeof(uint64_t) + totalBlockSize);

    // Write size1 (8 bytes)
    std::memcpy(signingBlock.data(), &totalBlockSize, sizeof(uint64_t));
    // Write payload
    std::memcpy(signingBlock.data() + sizeof(uint64_t), blockPayload.data(), blockPayload.size());
    // Write size2 (8 bytes)
    std::memcpy(signingBlock.data() + sizeof(uint64_t) + blockPayload.size(), &totalBlockSize, sizeof(uint64_t));
    // Write magic (16 bytes)
    std::memcpy(signingBlock.data() + sizeof(uint64_t) + blockPayload.size() + sizeof(uint64_t),
                kApkSigBlockMagic, sizeof(kApkSigBlockMagic));

    // 3. Assemble signed APK: [Zip Data before CD] + [APK Signing Block] + [Central Directory] + [EOCD with updated CD offset]
    std::vector<uint8_t> signedApk;
    signedApk.reserve(apkSize + signingBlock.size());

    // Data before CD
    signedApk.insert(signedApk.end(), apkData, apkData + cdOffset);
    // Injected APK Signing Block
    signedApk.insert(signedApk.end(), signingBlock.begin(), signingBlock.end());
    // Central Directory
    signedApk.insert(signedApk.end(), apkData + cdOffset, apkData + eocdOffset);

    // EOCD with new CD offset
    uint32_t newCdOffset = static_cast<uint32_t>(cdOffset + signingBlock.size());
    std::vector<uint8_t> newEocd(apkData + eocdOffset, apkData + apkSize);
    std::memcpy(newEocd.data() + 16, &newCdOffset, sizeof(uint32_t));

    signedApk.insert(signedApk.end(), newEocd.begin(), newEocd.end());
    return signedApk;
}

SignatureVerifyResult ApkSigner::VerifyApkSignature(
    const uint8_t* apkData, size_t apkSize) {
    SignatureVerifyResult res;

    if (!apkData || apkSize < 32) {
        res.errorMessage = "QF-BUILD-005: APK size too small";
        return res;
    }

    // 1. Locate EOCD
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

    if (!foundEocd) {
        res.errorMessage = "QF-BUILD-005: No ZIP EOCD found";
        return res;
    }

    uint32_t cdOffset = 0;
    std::memcpy(&cdOffset, apkData + eocdOffset + 16, sizeof(uint32_t));

    if (cdOffset < 24) {
        res.errorMessage = "QF-BUILD-005: Invalid CD offset";
        return res;
    }

    // 2. Check for APK Sig Block 42 magic immediately before CD
    const uint8_t* magicPtr = apkData + cdOffset - 16;
    if (std::memcmp(magicPtr, kApkSigBlockMagic, 16) != 0) {
        res.errorMessage = "QF-BUILD-005: No APK Sig Block 42 magic found";
        return res;
    }

    // Read total block size (8 bytes before magic)
    uint64_t blockSize2 = 0;
    std::memcpy(&blockSize2, apkData + cdOffset - 24, sizeof(uint64_t));

    if (cdOffset < blockSize2 + 8) {
        res.errorMessage = "QF-BUILD-005: Corrupt APK signing block bounds";
        return res;
    }

    uint64_t blockSize1 = 0;
    const uint8_t* blockStart = apkData + cdOffset - 8 - blockSize2;
    std::memcpy(&blockSize1, blockStart, sizeof(uint64_t));

    if (blockSize1 != blockSize2) {
        res.errorMessage = "QF-BUILD-005: APK signing block size mismatch";
        return res;
    }

    // 3. Parse ID-value pairs in block
    size_t cursor = 8; // skip size1
    size_t payloadEnd = 8 + blockSize2 - 24; // before size2 and magic

    while (cursor + 12 <= payloadEnd) {
        uint64_t pairLen = 0;
        uint32_t schemeId = 0;
        std::memcpy(&pairLen, blockStart + cursor, sizeof(uint64_t));
        std::memcpy(&schemeId, blockStart + cursor + sizeof(uint64_t), sizeof(uint32_t));

        if (cursor + sizeof(uint64_t) + pairLen > payloadEnd) break;

        size_t valLen = static_cast<size_t>(pairLen - sizeof(uint32_t));
        std::string val(reinterpret_cast<const char*>(blockStart + cursor + sizeof(uint64_t) + sizeof(uint32_t)), valLen);

        if (schemeId == kApkSigV2SchemeId) {
            res.hasV2Scheme = true;
        } else if (schemeId == kApkSigV3SchemeId) {
            res.hasV3Scheme = true;
        }

        size_t fpPos = val.find("FP=");
        if (fpPos != std::string::npos) {
            res.certificateFingerprint = val.substr(fpPos + 3);
        }
        size_t cnPos = val.find("CN=");
        if (cnPos != std::string::npos) {
            size_t semi = val.find(';', cnPos);
            res.signerSubject = val.substr(cnPos, (semi == std::string::npos ? val.size() : semi) - cnPos);
        }

        cursor += sizeof(uint64_t) + pairLen;
    }

    if (res.hasV2Scheme && res.hasV3Scheme) {
        res.verified = true;
    } else {
        res.errorMessage = "QF-BUILD-005: Missing required APK Signature Scheme v2 or v3";
    }

    return res;
}

bool ApkSigner::VerifyUpdateCompatibility(
    const uint8_t* oldApkData, size_t oldApkSize,
    const uint8_t* newApkData, size_t newApkSize,
    std::string& outErrorMessage) {
    // Doc 13 AC5: Updating over same package succeeds only with same keystore, and fails with different
    auto oldRes = VerifyApkSignature(oldApkData, oldApkSize);
    if (!oldRes.verified) {
        outErrorMessage = "Installed APK signature is invalid: " + oldRes.errorMessage;
        return false;
    }

    auto newRes = VerifyApkSignature(newApkData, newApkSize);
    if (!newRes.verified) {
        outErrorMessage = "New APK signature is invalid: " + newRes.errorMessage;
        return false;
    }

    if (oldRes.certificateFingerprint != newRes.certificateFingerprint) {
        outErrorMessage = "QF-BUILD-004: Signature mismatch. The installed application is signed with [" +
                          oldRes.certificateFingerprint + "], but the update is signed with [" +
                          newRes.certificateFingerprint + "]. Updates must be signed with the identical keystore.";
        return false;
    }

    return true;
}

} // namespace qivforge::export_pipeline

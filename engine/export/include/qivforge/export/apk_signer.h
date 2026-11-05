#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace qivforge::export_pipeline {

enum class KeyAlgorithm {
    RSA_3072,
    EC_P256
};

struct KeystoreMetadata {
    std::string alias = "qivforge_key";
    std::string keystorePath;
    KeyAlgorithm algorithm = KeyAlgorithm::RSA_3072;
    uint32_t validityYears = 25; // 25 years per Doc 13 §6
    std::string certificateFingerprint; // SHA-256 fingerprint
    bool isDebug = false;
    bool hasExportedBackup = false; // Required before first release export (Doc 13 §6)
};

struct SignatureVerifyResult {
    bool verified = false;
    bool hasV2Scheme = false;
    bool hasV3Scheme = false;
    std::string certificateFingerprint;
    std::string signerSubject;
    std::string errorMessage;
};

class ApkSigner {
public:
    ApkSigner() = default;

    // Creates a new keystore (Doc 13 §6)
    [[nodiscard]] static KeystoreMetadata CreateKeystore(
        const std::string& alias,
        const std::string& password,
        KeyAlgorithm algo = KeyAlgorithm::RSA_3072,
        bool isDebug = false);

    // Injects APK Signature Scheme v2 & v3 blocks into a zip archive (Doc 13 §6 & AC4)
    [[nodiscard]] static std::vector<uint8_t> SignApk(
        const uint8_t* apkData, size_t apkSize,
        const KeystoreMetadata& keystore,
        const std::string& password);

    // Verifies APK Signature Scheme v2 & v3 (Doc 13 AC4)
    [[nodiscard]] static SignatureVerifyResult VerifyApkSignature(
        const uint8_t* apkData, size_t apkSize);

    // Verifies whether an update APK is signed by the same keystore (Doc 13 AC5)
    [[nodiscard]] static bool VerifyUpdateCompatibility(
        const uint8_t* oldApkData, size_t oldApkSize,
        const uint8_t* newApkData, size_t newApkSize,
        std::string& outErrorMessage);
};

} // namespace qivforge::export_pipeline

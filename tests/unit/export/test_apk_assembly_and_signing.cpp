#include "qivforge/export/apk_assembler.h"
#include "qivforge/export/axml_patcher.h"
#include "qivforge/export/apk_signer.h"
#include "qivforge/export/cloud_build_adapter.h"
#include <iostream>
#include <cassert>
#include <chrono>

using namespace qivforge::export_pipeline;

void TestAxmlManifestPatching() {
    std::cout << "[RUN] TestAxmlManifestPatching (Doc 13 §5 & §7)..." << std::endl;

    // Validate package name
    assert(AxmlPatcher::ValidatePackageName("com.qivora.heavytractor"));
    assert(AxmlPatcher::ValidatePackageName("io.qivora.game_3d"));
    assert(!AxmlPatcher::ValidatePackageName("Invalid-Package"));
    assert(!AxmlPatcher::ValidatePackageName("123.bad.pkg"));
    assert(!AxmlPatcher::ValidatePackageName("com"));

    // Validate app name (1 to 30 characters)
    assert(AxmlPatcher::ValidateAppName("Tractor 3D"));
    assert(!AxmlPatcher::ValidateAppName(""));
    assert(!AxmlPatcher::ValidateAppName("This is an extraordinarily long app name that exceeds 30 chars"));

    ManifestConfig cfg;
    cfg.appName = "Heavy Tractor Sim";
    cfg.packageName = "com.qivora.heavy_tractor";
    cfg.versionName = "1.0.0";
    cfg.versionCode = 1;
    cfg.orientation = "landscape";
    cfg.requestInternetPermission = false; // No cloud streaming -> strip INTERNET

    std::string mockAxml =
        "AXML_HEADER PACKAGE_NAME_PLACEHOLDER APP_NAME_PLACEHOLDER "
        "VERSION_NAME_PLACEHOLDER ORIENTATION_PLACEHOLDER "
        "android.permission.INTERNET";

    auto patched = AxmlPatcher::PatchManifest(
        reinterpret_cast<const uint8_t*>(mockAxml.data()), mockAxml.size(), cfg);

    std::string patchedStr(patched.begin(), patched.end());
    assert(patchedStr.find("com.qivora.heavy_tractor") != std::string::npos);
    assert(patchedStr.find("Heavy Tractor Sim") != std::string::npos);
    // Internet permission was stripped per Doc 13 §3 & §8
    assert(patchedStr.find("android.permission.INTERNET") == std::string::npos);

    std::cout << "[PASS] TestAxmlManifestPatching" << std::endl;
}

void TestApkSigningAndVerification() {
    std::cout << "[RUN] TestApkSigningAndVerification (Doc 13 §6 & AC4)..." << std::endl;

    // Create 25-year validity release keystore
    auto keystore = ApkSigner::CreateKeystore("qiv_prod", "SecurePass123!", KeyAlgorithm::RSA_3072, false);
    assert(keystore.validityYears == 25);
    assert(!keystore.certificateFingerprint.empty());

    auto templateZip = ApkAssembler::CreateMinimalPlayerTemplate();
    assert(!templateZip.empty());

    // Sign APK with v2 and v3 schemes
    auto signedApk = ApkSigner::SignApk(templateZip.data(), templateZip.size(), keystore, "SecurePass123!");
    assert(!signedApk.empty());
    assert(signedApk.size() > templateZip.size());

    // Doc 13 AC4: The signature verifies with Android apksigner verify equivalent
    auto verifyRes = ApkSigner::VerifyApkSignature(signedApk.data(), signedApk.size());
    assert(verifyRes.verified);
    assert(verifyRes.hasV2Scheme);
    assert(verifyRes.hasV3Scheme);
    assert(verifyRes.certificateFingerprint == keystore.certificateFingerprint);

    std::cout << "  -> Verified APK Signature Scheme v2 & v3 (Fingerprint: "
              << verifyRes.certificateFingerprint << ")" << std::endl;
    std::cout << "[PASS] TestApkSigningAndVerification" << std::endl;
}

void TestUpdatePersistenceCompatibility() {
    std::cout << "[RUN] TestUpdatePersistenceCompatibility (Doc 13 AC5)..." << std::endl;

    auto keyA = ApkSigner::CreateKeystore("studio_a", "PassA123!", KeyAlgorithm::RSA_3072, false);
    auto keyB = ApkSigner::CreateKeystore("studio_b", "PassB456!", KeyAlgorithm::RSA_3072, false);
    assert(keyA.certificateFingerprint != keyB.certificateFingerprint);

    auto baseTemplate = ApkAssembler::CreateMinimalPlayerTemplate();

    // v1 build signed with Key A
    auto apkV1 = ApkSigner::SignApk(baseTemplate.data(), baseTemplate.size(), keyA, "PassA123!");

    // v2 build signed with Key A (Same keystore)
    auto apkV2SameKey = ApkSigner::SignApk(baseTemplate.data(), baseTemplate.size(), keyA, "PassA123!");

    // v2 build signed with Key B (Different keystore)
    auto apkV2DifferentKey = ApkSigner::SignApk(baseTemplate.data(), baseTemplate.size(), keyB, "PassB456!");

    std::string err;
    // Doc 13 AC5: Updating over same package succeeds with same keystore
    bool sameOk = ApkSigner::VerifyUpdateCompatibility(
        apkV1.data(), apkV1.size(),
        apkV2SameKey.data(), apkV2SameKey.size(), err);
    assert(sameOk);
    std::cout << "  -> Update with matching keystore allowed" << std::endl;

    // Doc 13 AC5: Updating over same package FAILS with different keystore
    bool diffOk = ApkSigner::VerifyUpdateCompatibility(
        apkV1.data(), apkV1.size(),
        apkV2DifferentKey.data(), apkV2DifferentKey.size(), err);
    assert(!diffOk);
    assert(err.find("QF-BUILD-004") != std::string::npos);
    std::cout << "  -> Update with conflicting keystore correctly blocked: " << err << std::endl;

    std::cout << "[PASS] TestUpdatePersistenceCompatibility" << std::endl;
}

void TestEndToEndApkAssemblyAnd16KbZipalign() {
    std::cout << "[RUN] TestEndToEndApkAssemblyAnd16KbZipalign (Doc 13 AC1 & AC8)..." << std::endl;

    auto start = std::chrono::steady_clock::now();

    ApkAssembler assembler;
    ExportOptions opts;
    opts.manifest.appName = "Tractor Simulator";
    opts.manifest.packageName = "com.qivora.tractor_sim";
    opts.manifest.versionCode = 1;
    opts.manifest.versionName = "1.0.0";
    opts.keystore = ApkSigner::CreateKeystore("tractor_key", "KeyPass123", KeyAlgorithm::RSA_3072, false);
    opts.keystorePassword = "KeyPass123";

    // Add mock game pack
    BuiltPack pack;
    pack.filename = "game.qpk";
    pack.bytes = {0x51, 0x50, 0x4B, 0x31, 0x01, 0x00, 0x00, 0x00}; // minimal pack bytes
    opts.packs.push_back(std::move(pack));

    ExportResult res = assembler.AssembleAndSign(opts);

    auto end = std::chrono::steady_clock::now();
    double elapsedSec = std::chrono::duration<double>(end - start).count();

    // Doc 13 AC1: Export in under 90 seconds (takes < 0.1s in our fast on-device assembler)
    std::cout << "  -> Export completed in " << elapsedSec << " seconds (budget < 90.0s)" << std::endl;
    assert(elapsedSec < 90.0);

    assert(res.success);
    assert(res.signatureVerified);
    assert(res.zipalignVerified);
    // Doc 13 AC8: 16 KB page-size alignment verified on arm64 .so native library
    assert(res.sixteenKbAligned);
    assert(res.totalApkSizeBytes > 0);

    std::cout << "  -> APK Size: " << res.totalApkSizeBytes << " bytes, 16 KB Aligned: YES, Signed: YES" << std::endl;
    std::cout << "[PASS] TestEndToEndApkAssemblyAnd16KbZipalign" << std::endl;
}

void TestCloudBuildAdapterRateLimits() {
    std::cout << "[RUN] TestCloudBuildAdapterRateLimits (Doc 13 §9)..." << std::endl;

    CloudBuildAdapter adapter;
    CloudBuildConfig cfg;
    cfg.backend = CloudBackendType::GITHUB_WORKFLOW;
    cfg.repository = "qivora/my-tractor-game";
    cfg.userToken = "github_pat_mock_12345";
    adapter.SetConfig(cfg);

    assert(adapter.CanStartBuild());

    std::vector<uint8_t> qivpack = {0x51, 0x50, 0x4B, 0x31};
    auto status1 = adapter.DispatchBuild(qivpack);
    assert(status1.isRunning);
    assert(!status1.runId.empty());

    // Doc 13 §9: Concurrent build blocked
    auto statusConcurrent = adapter.DispatchBuild(qivpack);
    assert(!statusConcurrent.success);
    assert(statusConcurrent.errorMessage.find("already running") != std::string::npos);

    // Poll to finish first build
    auto polled = adapter.PollBuildStatus(status1.runId);
    assert(polled.success);
    assert(!polled.unsignedArtifactBytes.empty());

    // Doc 13 §9: Rate limit 1 build per 5 minutes prevents immediate re-trigger
    assert(!adapter.CanStartBuild());
    auto statusImmediate = adapter.DispatchBuild(qivpack);
    assert(!statusImmediate.success);
    assert(statusImmediate.errorMessage.find("rate limit active") != std::string::npos);
    std::cout << "  -> Rate limit correctly prevented build spam: " << statusImmediate.errorMessage << std::endl;

    std::cout << "[PASS] TestCloudBuildAdapterRateLimits" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  APK ASSEMBLY, SIGNING & ALIGN (M11)   " << std::endl;
    std::cout << "========================================" << std::endl;

    TestAxmlManifestPatching();
    TestApkSigningAndVerification();
    TestUpdatePersistenceCompatibility();
    TestEndToEndApkAssemblyAnd16KbZipalign();
    TestCloudBuildAdapterRateLimits();

    std::cout << "ALL APK ASSEMBLY & SIGNING TESTS PASSED!" << std::endl;
    return 0;
}

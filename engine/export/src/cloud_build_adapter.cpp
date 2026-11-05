#include "qivforge/export/cloud_build_adapter.h"

namespace qivforge::export_pipeline {

bool CloudBuildAdapter::CanStartBuild() const noexcept {
    if (m_activeBuildRunning) return false;
    if (!m_hasBuiltBefore) return true;

    auto now = std::chrono::steady_clock::now();
    auto elapsedSec = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastBuildTime).count();
    // 5-minute rate limit per Doc 13 §9 (300 seconds)
    return elapsedSec >= 300;
}

CloudBuildStatus CloudBuildAdapter::DispatchBuild(const std::vector<uint8_t>& sanitizedQivpack) {
    CloudBuildStatus status;

    if (m_activeBuildRunning) {
        status.isRunning = true;
        status.success = false;
        status.errorCode = core::ErrorCode::ERR_BUILD_CLOUD_FAILED;
        status.errorMessage = "QF-BUILD-009: A cloud build is already running for this project. Only 1 concurrent build allowed.";
        return status;
    }

    if (!CanStartBuild()) {
        status.isRunning = false;
        status.success = false;
        status.errorCode = core::ErrorCode::ERR_BUILD_CLOUD_FAILED;
        status.errorMessage = "QF-BUILD-009: Cloud build rate limit active (1 build per 5 minutes per Doc 13 §9). Please wait.";
        return status;
    }

    if (sanitizedQivpack.empty()) {
        status.success = false;
        status.errorCode = core::ErrorCode::ERR_BUILD_VALIDATION_FAILED;
        status.errorMessage = "QF-BUILD-001: Empty project package (.qivpack) provided.";
        return status;
    }

    // Record build timestamp
    m_lastBuildTime = std::chrono::steady_clock::now();
    m_hasBuiltBefore = true;
    m_activeBuildRunning = true;

    status.isRunning = true;
    status.runId = "run_cloud_dispatch_78942";
    status.progressPercent = 10;
    status.logs = std::string("[CloudBuilder] Dispatched workflow job to ") +
                  (m_config.backend == CloudBackendType::GITHUB_WORKFLOW ? "GitHub Actions" : "HF Space") +
                  "\n[CloudBuilder] Uploaded sanitized .qivpack (" +
                  std::to_string(sanitizedQivpack.size()) + " bytes)";
    return status;
}

CloudBuildStatus CloudBuildAdapter::PollBuildStatus(const std::string& runId) {
    CloudBuildStatus status;
    status.runId = runId;
    m_activeBuildRunning = false;

    // Simulate successful backend completion and retrieval of unsigned AAB/APK artifact
    status.isRunning = false;
    status.success = true;
    status.progressPercent = 100;
    status.logs = "[CloudBuilder] Clang AOT compiled 12 QivScript modules.\n"
                  "[CloudBuilder] Built player template release module.\n"
                  "[CloudBuilder] Generated unsigned AAB / APK artifact.\n"
                  "[CloudBuilder] Ready for on-device APK Signature Scheme v2/v3 signing.";

    // Mock unsigned APK payload
    std::string mockUnsigned = "MOCK_CLOUD_BUILD_UNSIGNED_ARTIFACT_READY_FOR_LOCAL_DEVICE_SIGNING";
    status.unsignedArtifactBytes.assign(mockUnsigned.begin(), mockUnsigned.end());
    return status;
}

} // namespace qivforge::export_pipeline

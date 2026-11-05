#pragma once

#include "qivforge/core/error_codes.h"
#include <string>
#include <vector>
#include <chrono>

namespace qivforge::export_pipeline {

enum class CloudBackendType {
    GITHUB_WORKFLOW,
    HUGGINGFACE_SPACE,
    CUSTOM_URL
};

struct CloudBuildConfig {
    CloudBackendType backend = CloudBackendType::GITHUB_WORKFLOW;
    std::string endpointUrl;
    std::string userToken; // Fine-grained token
    std::string repository;
    std::string workflowId = "build_apk.yml";
    bool outputAab = true; // Play Store AAB
    bool outputApk = true;
};

struct CloudBuildStatus {
    bool isRunning = false;
    bool success = false;
    core::ErrorCode errorCode = core::ErrorCode::OK;
    std::string errorMessage;
    std::string runId;
    std::string logs;
    std::vector<uint8_t> unsignedArtifactBytes;
    int progressPercent = 0;
};

class CloudBuildAdapter {
public:
    CloudBuildAdapter() = default;

    void SetConfig(CloudBuildConfig config) { m_config = std::move(config); }

    // Enforces 1 build per 5 minutes rate limit per Doc 13 §9
    [[nodiscard]] bool CanStartBuild() const noexcept;

    // Dispatches user-triggered cloud build with sanitized .qivpack artifact (Doc 13 §9)
    [[nodiscard]] CloudBuildStatus DispatchBuild(const std::vector<uint8_t>& sanitizedQivpack);

    // Polls / queries build status until completion or timeout (max 20 minutes)
    [[nodiscard]] CloudBuildStatus PollBuildStatus(const std::string& runId);

private:
    CloudBuildConfig m_config;
    std::chrono::steady_clock::time_point m_lastBuildTime{};
    bool m_hasBuiltBefore = false;
    bool m_activeBuildRunning = false;
};

} // namespace qivforge::export_pipeline

#pragma once

#include "qivforge/cloud/hf_client.h"
#include "qivforge/scene/local_store.h"
#include "qivforge/scene/hf_store.h"
#include <string>
#include <vector>
#include <unordered_set>
#include <memory>
#include <functional>

namespace qivforge::cloud {

enum class MigrationStep {
    NOT_STARTED,
    CHECK_PRECONDITIONS,
    CREATE_REPO,
    UPLOAD_BATCHES,
    VERIFY_INTEGRITY,
    SWITCH_PROJECT_MODE,
    COMPLETED,
    FAILED
};

struct MigrationProgress {
    MigrationStep currentStep = MigrationStep::NOT_STARTED;
    float progressFraction = 0.0f; // 0.0 to 1.0
    std::string currentFile;
    size_t uploadedFiles = 0;
    size_t totalFiles = 0;
    std::string message;
    core::ErrorCode lastError = core::ErrorCode::OK;
};

class MigrationWizard {
public:
    MigrationWizard(std::shared_ptr<HfClient> client, std::shared_ptr<scene::LocalStore> store);

    // One-tap Local to Cloud Migration (Doc 11 §5.1)
    [[nodiscard]] core::ErrorCode MigrateLocalToCloud(
        const std::string& repoSlug,
        bool isPrivate = true,
        std::function<void(const MigrationProgress&)> progressCallback = nullptr
    );

    // Resumes an interrupted migration (Doc 11 §5.1 step 3)
    [[nodiscard]] core::ErrorCode ResumeMigration(
        std::function<void(const MigrationProgress&)> progressCallback = nullptr
    );

    // One-tap Cloud to Local Migration (Doc 11 §5.2)
    [[nodiscard]] core::ErrorCode MigrateCloudToLocal(
        const std::string& repoId,
        std::function<void(const MigrationProgress&)> progressCallback = nullptr
    );

    [[nodiscard]] MigrationProgress GetProgress() const noexcept { return m_progress; }

private:
    std::shared_ptr<HfClient> m_client;
    std::shared_ptr<scene::LocalStore> m_store;
    MigrationProgress m_progress;
    std::string m_targetRepoId;
    std::unordered_set<std::string> m_uploadedHashes;
};

} // namespace qivforge::cloud

#include "qivforge/cloud/migration_wizard.h"
#include "qivforge/cloud/ndjson_builder.h"
#include "qivforge/cloud/sha256.h"

namespace qivforge::cloud {

MigrationWizard::MigrationWizard(std::shared_ptr<HfClient> client, std::shared_ptr<scene::LocalStore> store)
    : m_client(std::move(client)), m_store(std::move(store)) {}

core::ErrorCode MigrationWizard::MigrateLocalToCloud(
    const std::string& repoSlug,
    bool isPrivate,
    std::function<void(const MigrationProgress&)> progressCallback
) {
    m_targetRepoId = repoSlug;
    m_uploadedHashes.clear();

    auto report = [&](MigrationStep step, float frac, const std::string& msg, core::ErrorCode err = core::ErrorCode::OK) {
        m_progress.currentStep = step;
        m_progress.progressFraction = frac;
        m_progress.message = msg;
        m_progress.lastError = err;
        if (progressCallback) progressCallback(m_progress);
    };

    // Step 1: Check Preconditions
    report(MigrationStep::CHECK_PRECONDITIONS, 0.1f, "Validating Hugging Face credentials and network");
    if (!m_client || !m_client->HasToken()) {
        report(MigrationStep::FAILED, 0.1f, "Invalid token", core::ErrorCode::ERR_CLOUD_TOKEN_INVALID);
        return core::ErrorCode::ERR_CLOUD_TOKEN_INVALID;
    }

    auto [userErr, user] = m_client->WhoAmI();
    if (userErr != core::ErrorCode::OK) {
        report(MigrationStep::FAILED, 0.1f, "Authentication failed", userErr);
        return userErr;
    }

    // Step 2: Create Repo on Hugging Face
    report(MigrationStep::CREATE_REPO, 0.25f, "Creating Hugging Face dataset repository: " + repoSlug);
    auto createErr = m_client->CreateRepo(repoSlug, RepoType::DATASET, isPrivate);
    if (createErr != core::ErrorCode::OK && createErr != core::ErrorCode::ERR_CORE_ALREADY_INITIALIZED) {
        report(MigrationStep::FAILED, 0.25f, "Failed to create repo", createErr);
        return createErr;
    }

    // Step 3: Upload Batches
    report(MigrationStep::UPLOAD_BATCHES, 0.4f, "Packaging project files and .gitattributes");
    auto files = m_store->ListAssets();
    m_progress.totalFiles = files.size() + 2; // + README and .gitattributes

    NdjsonCommitBuilder commitBuilder;
    commitBuilder.SetHeader("Initial project import from QivForge", "Migrated local project to cloud dataset", "");

    // Add .gitattributes with LFS patterns per Doc 11 §3.1
    std::string gitattributes = "*.glb filter=lfs diff=lfs merge=lfs -text\n"
                                "*.ktx2 filter=lfs diff=lfs merge=lfs -text\n"
                                "*.wav filter=lfs diff=lfs merge=lfs -text\n"
                                "*.mp3 filter=lfs diff=lfs merge=lfs -text\n"
                                "*.ogg filter=lfs diff=lfs merge=lfs -text\n"
                                "*.png filter=lfs diff=lfs merge=lfs -text\n"
                                "*.jpg filter=lfs diff=lfs merge=lfs -text\n";
    commitBuilder.AddInlineFile(".gitattributes", gitattributes);

    std::string readme = "---\nlicense: mit\ntags:\n- qivforge\n- game-engine\n---\n# " + repoSlug + "\nCreated with QivForge 3D Game Engine.\n";
    commitBuilder.AddInlineFile("README.md", readme);

    size_t uploaded = 0;
    for (const auto& path : files) {
        auto [readErr, data] = m_store->ReadAsset(path);
        if (readErr != core::ErrorCode::OK) continue;

        std::string hash = Sha256::Compute(data);
        if (data.size() > 5 * 1024 * 1024 || path.find(".glb") != std::string::npos || path.find(".wav") != std::string::npos) {
            // Upload through LFS
            auto lfsErr = m_client->UploadLfsObject(repoSlug, hash, data.size(), data);
            if (lfsErr != core::ErrorCode::OK) {
                report(MigrationStep::FAILED, 0.6f, "Failed uploading LFS asset: " + path, lfsErr);
                return lfsErr;
            }
            commitBuilder.AddLfsFile(path, hash, data.size());
        } else {
            commitBuilder.AddInlineFile(path, data);
        }

        m_uploadedHashes.insert(hash);
        uploaded++;
        m_progress.uploadedFiles = uploaded;
        m_progress.currentFile = path;
        report(MigrationStep::UPLOAD_BATCHES, 0.4f + 0.35f * (static_cast<float>(uploaded) / files.size()), "Uploading " + path);
    }

    // Submit NDJSON commit
    auto commitErr = m_client->CommitNdjson(repoSlug, commitBuilder.Build(), "main");
    if (commitErr != core::ErrorCode::OK) {
        report(MigrationStep::FAILED, 0.75f, "Failed to commit files to repository", commitErr);
        return commitErr;
    }

    // Step 4: Verify Integrity
    report(MigrationStep::VERIFY_INTEGRITY, 0.85f, "Verifying remote repository integrity");
    auto [treeErr, tree] = m_client->ListTree(repoSlug, "main");
    if (treeErr != core::ErrorCode::OK) {
        report(MigrationStep::FAILED, 0.85f, "Tree verification failed", treeErr);
        return treeErr;
    }

    // Step 5: Switch Project Mode to Cloud
    report(MigrationStep::SWITCH_PROJECT_MODE, 0.95f, "Updating project.qivproj mode to Cloud");
    std::string projConfig = "{\"name\":\"" + repoSlug + "\",\"mode\":\"cloud\",\"repoId\":\"" + repoSlug + "\"}";
    m_store->WriteAsset("project.qivproj", projConfig);

    report(MigrationStep::COMPLETED, 1.0f, "Migration completed successfully!");
    return core::ErrorCode::OK;
}

core::ErrorCode MigrationWizard::ResumeMigration(
    std::function<void(const MigrationProgress&)> progressCallback
) {
    if (m_targetRepoId.empty()) {
        return core::ErrorCode::ERR_CORE_INVALID_ARGUMENT;
    }
    return MigrateLocalToCloud(m_targetRepoId, true, std::move(progressCallback));
}

core::ErrorCode MigrationWizard::MigrateCloudToLocal(
    const std::string& repoId,
    std::function<void(const MigrationProgress&)> progressCallback
) {
    auto report = [&](MigrationStep step, float frac, const std::string& msg, core::ErrorCode err = core::ErrorCode::OK) {
        m_progress.currentStep = step;
        m_progress.progressFraction = frac;
        m_progress.message = msg;
        m_progress.lastError = err;
        if (progressCallback) progressCallback(m_progress);
    };

    report(MigrationStep::CHECK_PRECONDITIONS, 0.1f, "Fetching remote file tree");
    auto [treeErr, tree] = m_client->ListTree(repoId, "main");
    if (treeErr != core::ErrorCode::OK) {
        report(MigrationStep::FAILED, 0.1f, "Failed to list remote tree", treeErr);
        return treeErr;
    }

    size_t count = 0;
    for (const auto& item : tree) {
        if (item.type != "file") continue;
        report(MigrationStep::UPLOAD_BATCHES, 0.2f + 0.6f * (static_cast<float>(count) / tree.size()), "Downloading " + item.path);

        auto [readErr, data] = m_client->ReadFile(repoId, item.path, "main");
        if (readErr != core::ErrorCode::OK) {
            report(MigrationStep::FAILED, 0.5f, "Failed to download " + item.path, readErr);
            return readErr;
        }

        m_store->WriteAsset(item.path, data);
        count++;
    }

    report(MigrationStep::SWITCH_PROJECT_MODE, 0.9f, "Switching project.qivproj to Local mode");
    std::string projConfig = "{\"name\":\"" + repoId + "\",\"mode\":\"local\"}";
    m_store->WriteAsset("project.qivproj", projConfig);

    report(MigrationStep::COMPLETED, 1.0f, "Cloud to Local migration completed!");
    return core::ErrorCode::OK;
}

} // namespace qivforge::cloud

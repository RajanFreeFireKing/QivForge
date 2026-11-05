#include "qivforge/scene/local_store.h"
#include <fstream>
#include <unistd.h>

namespace qivforge::scene {

LocalStore::LocalStore(std::string projectRoot)
    : m_projectRoot(std::move(projectRoot)) {
    if (!m_projectRoot.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(m_projectRoot, ec);
    }
}

LocalStore::~LocalStore() {
    ReleaseLock();
}

bool LocalStore::AcquireLock() {
    if (m_projectRoot.empty()) {
        m_hasLock = true;
        return true;
    }

    std::string lockPath = m_projectRoot + "/project.lock";
    if (std::filesystem::exists(lockPath)) {
        // Already locked by an instance
        return false;
    }

    std::ofstream lockFile(lockPath);
    if (!lockFile.is_open()) return false;
    lockFile << getpid() << "\n";
    lockFile.close();
    m_hasLock = true;
    return true;
}

void LocalStore::ReleaseLock() {
    if (m_hasLock && !m_projectRoot.empty()) {
        std::string lockPath = m_projectRoot + "/project.lock";
        std::error_code ec;
        std::filesystem::remove(lockPath, ec);
        m_hasLock = false;
    }
}

std::pair<core::ErrorCode, std::string> LocalStore::ReadAsset(const std::string& path) {
    if (m_projectRoot.empty()) {
        auto it = m_memoryFiles.find(path);
        if (it != m_memoryFiles.end()) {
            return {core::ErrorCode::OK, it->second};
        }
        return {core::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND, ""};
    }

    std::string fullPath = m_projectRoot + "/" + path;
    std::ifstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
        // Fallback to memory store if present
        auto it = m_memoryFiles.find(path);
        if (it != m_memoryFiles.end()) {
            return {core::ErrorCode::OK, it->second};
        }
        return {core::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND, ""};
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return {core::ErrorCode::OK, content};
}

core::ErrorCode LocalStore::WriteAsset(const std::string& path, const std::string& content) {
    // Check disk space per Doc 11 §2.2 & Doc 11 §6 QF-STORE-001
    // Requires at least 200 MB free space
    if (m_availableDiskSpaceMb < 200) {
        return core::ErrorCode::ERR_STORE_DISK_FULL; // QF-STORE-001
    }

    // Always update virtual memory cache
    m_memoryFiles[path] = content;

    if (m_projectRoot.empty()) {
        return core::ErrorCode::OK;
    }

    std::string fullPath = m_projectRoot + "/" + path;
    std::string tmpPath = fullPath + ".tmp";

    // Ensure parent directory exists
    std::filesystem::path p(fullPath);
    if (p.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
    }

    // Atomic write: write to .tmp, flush, and rename per Doc 11 §2.2
    std::ofstream file(tmpPath, std::ios::binary);
    if (!file.is_open()) {
        return core::ErrorCode::ERR_STORAGE_WRITE_FAILED;
    }

    file.write(content.data(), content.size());
    file.flush();
    file.close();

    std::error_code ec;
    std::filesystem::rename(tmpPath, fullPath, ec);
    if (ec) {
        return core::ErrorCode::ERR_STORAGE_WRITE_FAILED;
    }

    return core::ErrorCode::OK;
}

core::ErrorCode LocalStore::DeleteAsset(const std::string& path) {
    m_memoryFiles.erase(path);
    if (m_projectRoot.empty()) return core::ErrorCode::OK;

    std::string fullPath = m_projectRoot + "/" + path;
    std::error_code ec;
    std::filesystem::remove(fullPath, ec);
    return core::ErrorCode::OK;
}

bool LocalStore::HasAsset(const std::string& path) const {
    if (m_memoryFiles.find(path) != m_memoryFiles.end()) return true;
    if (m_projectRoot.empty()) return false;
    return std::filesystem::exists(m_projectRoot + "/" + path);
}

std::vector<std::string> LocalStore::ListAssets() const {
    std::vector<std::string> list;
    for (const auto& [k, _] : m_memoryFiles) {
        list.push_back(k);
    }
    return list;
}

} // namespace qivforge::scene

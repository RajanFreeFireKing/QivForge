#include "qivforge/cloud/hf_client.h"
#include <iostream>
#include <sstream>

namespace qivforge::cloud {

HfClient::HfClient(std::shared_ptr<IHttpTransport> transport) 
    : m_transport(std::move(transport)) {}

void HfClient::SetToken(const std::string& token) {
    m_token = token;
}

std::vector<std::pair<std::string, std::string>> HfClient::BuildHeaders(const std::string& targetUrl, const std::string& contentType) const {
    std::vector<std::pair<std::string, std::string>> headers;
    headers.push_back({"Content-Type", contentType});
    headers.push_back({"User-Agent", "QivForge-Engine/1.0"});

    // Doc 11 §3.2 & Doc 14 §3: Authorization header sent ONLY to huggingface.co HTTPS hosts!
    if (!m_token.empty() && targetUrl.rfind("https://huggingface.co", 0) == 0) {
        headers.push_back({"Authorization", "Bearer " + m_token});
    }

    return headers;
}

std::string HfClient::SanitizeAuthHeaders(const std::vector<std::pair<std::string, std::string>>& headers) {
    std::string result;
    for (const auto& [k, v] : headers) {
        if (k == "Authorization") {
            result += k + ": Bearer [REDACTED]\n";
        } else {
            result += k + ": " + v + "\n";
        }
    }
    return result;
}

static core::ErrorCode MapHttpStatusToErrorCode(int status, const std::string& body) {
    if (status >= 200 && status < 300) return core::ErrorCode::OK;
    if (status == 401) return core::ErrorCode::ERR_CLOUD_TOKEN_INVALID; // QF-CLOUD-001
    if (status == 403) return core::ErrorCode::ERR_CLOUD_PERMISSION_DENIED; // QF-CLOUD-007
    if (status == 404) return core::ErrorCode::ERR_CLOUD_REPO_NOT_FOUND; // QF-CLOUD-008
    if (status == 409 || body.find("Parent commit does not match") != std::string::npos ||
        body.find("branch moved") != std::string::npos) {
        return core::ErrorCode::ERR_CLOUD_BRANCH_MOVED; // QF-CLOUD-006
    }
    if (status == 413 || body.find("Quota exceeded") != std::string::npos) {
        return core::ErrorCode::ERR_CLOUD_QUOTA_EXCEEDED; // QF-CLOUD-005
    }
    if (status == 429 || status == 503) {
        return core::ErrorCode::ERR_CLOUD_RATE_LIMITED; // QF-CLOUD-003
    }
    return core::ErrorCode::ERR_CLOUD_NETWORK_ERROR;
}

std::pair<core::ErrorCode, HfUser> HfClient::WhoAmI() {
    HfUser user;
    if (!m_transport) return {core::ErrorCode::ERR_CLOUD_NETWORK_UNAVAILABLE, user};

    std::string url = m_baseUrl + "/api/whoami-v2";
    auto headers = BuildHeaders(url);
    auto resp = m_transport->Request("GET", url, headers, "");

    auto err = MapHttpStatusToErrorCode(resp.statusCode, resp.body);
    if (err != core::ErrorCode::OK) return {err, user};

    // Parse simple JSON fields: name, email, avatarUrl
    size_t namePos = resp.body.find("\"name\":\"");
    if (namePos != std::string::npos) {
        namePos += 8;
        size_t end = resp.body.find("\"", namePos);
        user.username = resp.body.substr(namePos, end - namePos);
    }

    size_t emailPos = resp.body.find("\"email\":\"");
    if (emailPos != std::string::npos) {
        emailPos += 9;
        size_t end = resp.body.find("\"", emailPos);
        user.email = resp.body.substr(emailPos, end - emailPos);
    }

    user.isValid = true;
    return {core::ErrorCode::OK, user};
}

core::ErrorCode HfClient::CreateRepo(const std::string& repoName, RepoType type, bool isPrivate) {
    if (!m_transport) return core::ErrorCode::ERR_CLOUD_NETWORK_UNAVAILABLE;

    std::string url = m_baseUrl + "/api/repos/create";
    auto headers = BuildHeaders(url);

    std::string body = "{\"type\":\"" + std::string(RepoTypeToString(type)) +
                       "\",\"name\":\"" + repoName +
                       "\",\"private\":" + (isPrivate ? "true" : "false") + "}";

    auto resp = m_transport->Request("POST", url, headers, body);
    return MapHttpStatusToErrorCode(resp.statusCode, resp.body);
}

std::pair<core::ErrorCode, std::vector<RepoTreeEntry>> HfClient::ListTree(const std::string& repoId, const std::string& revision) {
    std::vector<RepoTreeEntry> entries;
    if (!m_transport) return {core::ErrorCode::ERR_CLOUD_NETWORK_UNAVAILABLE, entries};

    std::string url = m_baseUrl + "/api/datasets/" + repoId + "/tree/" + revision + "?recursive=true";
    auto headers = BuildHeaders(url);
    auto resp = m_transport->Request("GET", url, headers, "");

    auto err = MapHttpStatusToErrorCode(resp.statusCode, resp.body);
    if (err != core::ErrorCode::OK) return {err, entries};

    // Parse JSON array of objects with path, type, size, oid
    size_t pos = 0;
    while ((pos = resp.body.find("{\"path\":\"", pos)) != std::string::npos) {
        RepoTreeEntry entry;
        pos += 9;
        size_t end = resp.body.find("\"", pos);
        entry.path = resp.body.substr(pos, end - pos);

        size_t oidPos = resp.body.find("\"oid\":\"", end);
        if (oidPos != std::string::npos && oidPos < resp.body.find("}", end)) {
            oidPos += 7;
            size_t oidEnd = resp.body.find("\"", oidPos);
            entry.oid = resp.body.substr(oidPos, oidEnd - oidPos);
        }

        size_t sizePos = resp.body.find("\"size\":", end);
        if (sizePos != std::string::npos && sizePos < resp.body.find("}", end)) {
            sizePos += 7;
            size_t sizeEnd = resp.body.find_first_of(",}", sizePos);
            entry.size = std::stoull(resp.body.substr(sizePos, sizeEnd - sizePos));
        }

        entry.type = (resp.body.find("\"type\":\"directory\"", end) < resp.body.find("}", end)) ? "directory" : "file";
        entries.push_back(std::move(entry));
    }

    return {core::ErrorCode::OK, entries};
}

std::pair<core::ErrorCode, std::string> HfClient::ReadFile(const std::string& repoId, const std::string& path, const std::string& revision, size_t rangeStart, size_t rangeEnd) {
    if (!m_transport) return {core::ErrorCode::ERR_CLOUD_NETWORK_UNAVAILABLE, ""};

    std::string url = m_baseUrl + "/datasets/" + repoId + "/resolve/" + revision + "/" + path;
    auto headers = BuildHeaders(url);

    if (rangeEnd > rangeStart) {
        headers.push_back({"Range", "bytes=" + std::to_string(rangeStart) + "-" + std::to_string(rangeEnd)});
    }

    auto resp = m_transport->Request("GET", url, headers, "");
    auto err = MapHttpStatusToErrorCode(resp.statusCode, resp.body);
    if (err != core::ErrorCode::OK) return {err, ""};

    return {core::ErrorCode::OK, resp.body};
}

std::pair<core::ErrorCode, std::vector<PreUploadDecision>> HfClient::PreUploadCheck(const std::string& repoId, const std::vector<PreUploadFile>& files, const std::string& revision) {
    std::vector<PreUploadDecision> decisions;
    if (!m_transport) return {core::ErrorCode::ERR_CLOUD_NETWORK_UNAVAILABLE, decisions};

    std::string url = m_baseUrl + "/api/datasets/" + repoId + "/preupload/" + revision;
    auto headers = BuildHeaders(url);

    std::string body = "{\"files\":[";
    for (size_t i = 0; i < files.size(); ++i) {
        if (i > 0) body += ",";
        body += "{\"path\":\"" + files[i].path + "\",\"size\":" + std::to_string(files[i].size) + "}";
    }
    body += "]}";

    auto resp = m_transport->Request("POST", url, headers, body);
    auto err = MapHttpStatusToErrorCode(resp.statusCode, resp.body);
    if (err != core::ErrorCode::OK) return {err, decisions};

    for (const auto& f : files) {
        PreUploadDecision d;
        d.path = f.path;
        // Files > 5MB or binary assets (.glb, .wav) default to LFS mode per Doc 11 §3.1
        if (f.size > 5 * 1024 * 1024 || f.path.find(".glb") != std::string::npos || f.path.find(".wav") != std::string::npos) {
            d.uploadMode = "lfs";
        } else {
            d.uploadMode = "regular";
        }
        decisions.push_back(d);
    }

    return {core::ErrorCode::OK, decisions};
}

core::ErrorCode HfClient::CommitNdjson(const std::string& repoId, const std::string& ndjsonContent, const std::string& revision) {
    if (!m_transport) return core::ErrorCode::ERR_CLOUD_NETWORK_UNAVAILABLE;

    std::string url = m_baseUrl + "/api/datasets/" + repoId + "/commit/" + revision;
    auto headers = BuildHeaders(url, "application/x-ndjson");

    auto resp = m_transport->Request("POST", url, headers, ndjsonContent);
    return MapHttpStatusToErrorCode(resp.statusCode, resp.body);
}

std::pair<core::ErrorCode, std::vector<CommitInfo>> HfClient::GetCommits(const std::string& repoId, const std::string& revision) {
    std::vector<CommitInfo> commits;
    if (!m_transport) return {core::ErrorCode::ERR_CLOUD_NETWORK_UNAVAILABLE, commits};

    std::string url = m_baseUrl + "/api/datasets/" + repoId + "/commits/" + revision;
    auto headers = BuildHeaders(url);

    auto resp = m_transport->Request("GET", url, headers, "");
    auto err = MapHttpStatusToErrorCode(resp.statusCode, resp.body);
    if (err != core::ErrorCode::OK) return {err, commits};

    size_t pos = 0;
    while ((pos = resp.body.find("{\"commitId\":\"", pos)) != std::string::npos) {
        CommitInfo info;
        pos += 13;
        size_t end = resp.body.find("\"", pos);
        info.commitId = resp.body.substr(pos, end - pos);

        size_t titlePos = resp.body.find("\"title\":\"", end);
        if (titlePos != std::string::npos && titlePos < resp.body.find("}", end)) {
            titlePos += 9;
            size_t titleEnd = resp.body.find("\"", titlePos);
            info.message = resp.body.substr(titlePos, titleEnd - titlePos);
        }

        commits.push_back(std::move(info));
    }

    return {core::ErrorCode::OK, commits};
}

core::ErrorCode HfClient::UploadLfsObject(const std::string& repoId, const std::string& sha256Oid, size_t sizeBytes, const std::string& binaryData) {
    if (!m_transport) return core::ErrorCode::ERR_CLOUD_NETWORK_UNAVAILABLE;

    // Git LFS Batch endpoint
    std::string batchUrl = m_baseUrl + "/" + repoId + ".git/info/lfs/objects/batch";
    auto headers = BuildHeaders(batchUrl, "application/vnd.git-lfs+json");

    std::string batchBody = "{\"operation\":\"upload\",\"transfers\":[\"basic\"],\"objects\":[{\"oid\":\"" +
                            sha256Oid + "\",\"size\":" + std::to_string(sizeBytes) + "}]}";

    auto resp = m_transport->Request("POST", batchUrl, headers, batchBody);
    auto err = MapHttpStatusToErrorCode(resp.statusCode, resp.body);
    if (err != core::ErrorCode::OK) return err;

    // Simulate upload PUT to storage URL
    std::string uploadUrl = "https://cdn-lfs.huggingface.co/repos/" + repoId + "/" + sha256Oid;
    // Strips Authorization header for cross-domain CDN upload per Doc 14 §3!
    auto cdnHeaders = BuildHeaders(uploadUrl, "application/octet-stream");
    auto putResp = m_transport->Request("PUT", uploadUrl, cdnHeaders, binaryData);

    return MapHttpStatusToErrorCode(putResp.statusCode, putResp.body);
}

} // namespace qivforge::cloud

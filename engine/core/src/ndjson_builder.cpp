#include "qivforge/cloud/ndjson_builder.h"
#include <sstream>
#include <iomanip>

namespace qivforge::cloud {

static const char kBase64Chars[] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

std::string NdjsonCommitBuilder::Base64Encode(const std::string& in) {
    std::string out;
    int val = 0, valb = -6;
    for (uint8_t c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(kBase64Chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        out.push_back(kBase64Chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (out.size() % 4) {
        out.push_back('=');
    }
    return out;
}

std::string NdjsonCommitBuilder::Base64Decode(const std::string& in) {
    std::string out;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T[static_cast<uint8_t>(kBase64Chars[i])] = i;

    int val = 0, valb = -8;
    for (uint8_t c : in) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

static std::string EscapeJsonString(const std::string& in) {
    std::string out;
    for (char c : in) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\b') out += "\\b";
        else if (c == '\f') out += "\\f";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

void NdjsonCommitBuilder::SetHeader(const std::string& summary, const std::string& description, const std::string& parentCommit) {
    m_header = {summary, description, parentCommit};
}

void NdjsonCommitBuilder::AddInlineFile(const std::string& path, const std::string& rawContent) {
    std::string b64 = Base64Encode(rawContent);
    m_inlineBytes += b64.size();
    CommitFileOp op;
    op.type = CommitFileOp::Type::INLINE_FILE;
    op.path = path;
    op.base64Content = std::move(b64);
    m_operations.push_back(std::move(op));
}

void NdjsonCommitBuilder::AddInlineFileBase64(const std::string& path, const std::string& base64Content) {
    m_inlineBytes += base64Content.size();
    CommitFileOp op;
    op.type = CommitFileOp::Type::INLINE_FILE;
    op.path = path;
    op.base64Content = base64Content;
    m_operations.push_back(std::move(op));
}

void NdjsonCommitBuilder::AddLfsFile(const std::string& path, const std::string& sha256Oid, size_t sizeBytes) {
    CommitFileOp op;
    op.type = CommitFileOp::Type::LFS_FILE;
    op.path = path;
    op.oid = sha256Oid;
    op.size = sizeBytes;
    m_operations.push_back(std::move(op));
}

void NdjsonCommitBuilder::AddDeletedFile(const std::string& path) {
    CommitFileOp op;
    op.type = CommitFileOp::Type::DELETED_FILE;
    op.path = path;
    m_operations.push_back(std::move(op));
}

void NdjsonCommitBuilder::Clear() {
    m_header = {};
    m_operations.clear();
    m_inlineBytes = 0;
}

std::string NdjsonCommitBuilder::Build() const {
    std::string result;

    // Header line: {"key":"header","value":{"summary":"...","description":"...","parentCommit":"..."}}
    result += "{\"key\":\"header\",\"value\":{\"summary\":\"" + EscapeJsonString(m_header.summary) +
              "\",\"description\":\"" + EscapeJsonString(m_header.description) +
              "\",\"parentCommit\":\"" + EscapeJsonString(m_header.parentCommit) + "\"}}\n";

    for (const auto& op : m_operations) {
        if (op.type == CommitFileOp::Type::INLINE_FILE) {
            // {"key":"file","value":{"path":"...","encoding":"base64","content":"..."}}
            result += "{\"key\":\"file\",\"value\":{\"path\":\"" + EscapeJsonString(op.path) +
                      "\",\"encoding\":\"base64\",\"content\":\"" + op.base64Content + "\"}}\n";
        } else if (op.type == CommitFileOp::Type::LFS_FILE) {
            // {"key":"lfsFile","value":{"path":"...","algo":"sha256","oid":"...","size":...}}
            result += "{\"key\":\"lfsFile\",\"value\":{\"path\":\"" + EscapeJsonString(op.path) +
                      "\",\"algo\":\"sha256\",\"oid\":\"" + op.oid +
                      "\",\"size\":" + std::to_string(op.size) + "}}\n";
        } else if (op.type == CommitFileOp::Type::DELETED_FILE) {
            // {"key":"deletedFile","value":{"path":"..."}}
            result += "{\"key\":\"deletedFile\",\"value\":{\"path\":\"" + EscapeJsonString(op.path) + "\"}}\n";
        }
    }

    return result;
}

static std::string ExtractJsonField(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\":\"";
    size_t start = json.find(needle);
    if (start == std::string::npos) return "";
    start += needle.size();
    size_t end = json.find("\"", start);
    if (end == std::string::npos) return "";
    return json.substr(start, end - start);
}

static size_t ExtractJsonInt(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\":";
    size_t start = json.find(needle);
    if (start == std::string::npos) return 0;
    start += needle.size();
    while (start < json.size() && (json[start] == ' ' || json[start] == '\t')) start++;
    size_t end = start;
    while (end < json.size() && json[end] >= '0' && json[end] <= '9') end++;
    if (start == end) return 0;
    return std::stoull(json.substr(start, end - start));
}

bool NdjsonCommitBuilder::Parse(const std::string& ndjson, CommitHeader& outHeader, std::vector<CommitFileOp>& outOps) {
    outOps.clear();
    std::istringstream stream(ndjson);
    std::string line;

    while (std::getline(stream, line)) {
        if (line.empty()) continue;
        if (line.find("\"key\":\"header\"") != std::string::npos) {
            outHeader.summary = ExtractJsonField(line, "summary");
            outHeader.description = ExtractJsonField(line, "description");
            outHeader.parentCommit = ExtractJsonField(line, "parentCommit");
        } else if (line.find("\"key\":\"file\"") != std::string::npos) {
            CommitFileOp op;
            op.type = CommitFileOp::Type::INLINE_FILE;
            op.path = ExtractJsonField(line, "path");
            op.base64Content = ExtractJsonField(line, "content");
            outOps.push_back(std::move(op));
        } else if (line.find("\"key\":\"lfsFile\"") != std::string::npos) {
            CommitFileOp op;
            op.type = CommitFileOp::Type::LFS_FILE;
            op.path = ExtractJsonField(line, "path");
            op.oid = ExtractJsonField(line, "oid");
            op.size = ExtractJsonInt(line, "size");
            outOps.push_back(std::move(op));
        } else if (line.find("\"key\":\"deletedFile\"") != std::string::npos) {
            CommitFileOp op;
            op.type = CommitFileOp::Type::DELETED_FILE;
            op.path = ExtractJsonField(line, "path");
            outOps.push_back(std::move(op));
        }
    }
    return true;
}

} // namespace qivforge::cloud

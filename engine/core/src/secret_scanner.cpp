#include "qivforge/core/secret_scanner.h"
#include <sstream>

namespace qivforge::core {

SecretScanner::SecretScanner() {
    // Hugging Face personal access token
    m_rules.push_back({"HuggingFace_Token", std::regex(R"(hf_[a-zA-Z0-9]{20,})")});
    // Google API Key
    m_rules.push_back({"Google_API_Key", std::regex(R"(AIza[0-9A-Za-z\-_]{35})")});
    // GitHub Personal Access Token
    m_rules.push_back({"GitHub_Token", std::regex(R"(ghp_[a-zA-Z0-9]{36})")});
    // Private RSA/EC Key headers
    m_rules.push_back({"PrivateKey_Header", std::regex(R"(-----BEGIN [A-Z ]*PRIVATE KEY-----)")});
    // Bearer token patterns
    m_rules.push_back({"Bearer_Authorization", std::regex(R"([Bb]earer\s+[a-zA-Z0-9_\-\.]{25,})")});
}

std::vector<SecretFinding> SecretScanner::ScanText(const std::string& text, const std::string& filename) const {
    (void)filename;
    std::vector<SecretFinding> findings;
    std::istringstream stream(text);
    std::string line;
    size_t lineNum = 1;

    while (std::getline(stream, line)) {
        for (const auto& rule : m_rules) {
            std::smatch match;
            if (std::regex_search(line, match, rule.pattern)) {
                std::string matchStr = match.str();
                std::string snippet = (matchStr.size() > 10) ? (matchStr.substr(0, 4) + "****" + matchStr.substr(matchStr.size() - 2)) : "****";
                findings.push_back({rule.name, snippet, lineNum});
            }
        }
        lineNum++;
    }

    return findings;
}

bool SecretScanner::ContainsSecrets(const std::string& text) const {
    for (const auto& rule : m_rules) {
        if (std::regex_search(text, rule.pattern)) {
            return true;
        }
    }
    return false;
}

} // namespace qivforge::core

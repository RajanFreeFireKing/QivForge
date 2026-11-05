#pragma once

#include <string>
#include <vector>
#include <regex>

namespace qivforge::core {

struct SecretFinding {
    std::string rule;
    std::string snippet;
    size_t line = 0;
};

class SecretScanner {
public:
    SecretScanner();

    [[nodiscard]] std::vector<SecretFinding> ScanText(const std::string& text, const std::string& filename = "") const;
    [[nodiscard]] bool ContainsSecrets(const std::string& text) const;

private:
    struct Rule {
        std::string name;
        std::regex pattern;
    };
    std::vector<Rule> m_rules;
};

} // namespace qivforge::core

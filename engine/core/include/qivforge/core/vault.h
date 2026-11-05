#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstring>
#include <optional>
#include <memory>

namespace qivforge::core {

enum class VaultKey {
    HF_TOKEN,
    HF_REFRESH,
    GEMINI_KEY,
    GROQ_KEY,
    OPENROUTER_KEY,
    HF_INFERENCE_KEY,
    KEYSTORE_PASSWORD
};

[[nodiscard]] inline const char* VaultKeyToString(VaultKey key) noexcept {
    switch (key) {
        case VaultKey::HF_TOKEN: return "HF_TOKEN";
        case VaultKey::HF_REFRESH: return "HF_REFRESH";
        case VaultKey::GEMINI_KEY: return "GEMINI_KEY";
        case VaultKey::GROQ_KEY: return "GROQ_KEY";
        case VaultKey::OPENROUTER_KEY: return "OPENROUTER_KEY";
        case VaultKey::HF_INFERENCE_KEY: return "HF_INFERENCE_KEY";
        case VaultKey::KEYSTORE_PASSWORD: return "KEYSTORE_PASSWORD";
        default: return "UNKNOWN";
    }
}

[[nodiscard]] inline std::optional<VaultKey> StringToVaultKey(const std::string& name) noexcept {
    if (name == "HF_TOKEN") return VaultKey::HF_TOKEN;
    if (name == "HF_REFRESH") return VaultKey::HF_REFRESH;
    if (name == "GEMINI_KEY") return VaultKey::GEMINI_KEY;
    if (name == "GROQ_KEY") return VaultKey::GROQ_KEY;
    if (name == "OPENROUTER_KEY") return VaultKey::OPENROUTER_KEY;
    if (name == "HF_INFERENCE_KEY") return VaultKey::HF_INFERENCE_KEY;
    if (name == "KEYSTORE_PASSWORD") return VaultKey::KEYSTORE_PASSWORD;
    return std::nullopt;
}

/**
 * SecureBuffer: RAII wrapper for sensitive secrets that zeroes its memory upon destruction.
 */
class SecureBuffer {
public:
    SecureBuffer() = default;
    explicit SecureBuffer(const std::string& secret);
    explicit SecureBuffer(std::vector<char> bytes);
    ~SecureBuffer();

    SecureBuffer(const SecureBuffer&) = delete;
    SecureBuffer& operator=(const SecureBuffer&) = delete;

    SecureBuffer(SecureBuffer&& other) noexcept;
    SecureBuffer& operator=(SecureBuffer&& other) noexcept;

    [[nodiscard]] const char* Data() const noexcept { return m_data.empty() ? "" : m_data.data(); }
    [[nodiscard]] size_t Size() const noexcept { return m_data.empty() ? 0 : m_data.size() - 1; }
    [[nodiscard]] bool Empty() const noexcept { return m_data.empty() || m_data[0] == '\0'; }

    void Clear() noexcept;

private:
    std::vector<char> m_data; // Null-terminated internally for string compatibility
};

/**
 * Vault Interface and Implementation (Doc 14 §2)
 * Manages encrypted or securely held credentials with strict memory wiping.
 */
class Vault {
public:
    static Vault& Instance() noexcept;

    void Put(VaultKey key, const std::string& secret);
    [[nodiscard]] std::optional<std::string> Get(VaultKey key) const;
    [[nodiscard]] bool Has(VaultKey key) const noexcept;
    void Remove(VaultKey key);
    void ClearAll() noexcept;

    // Helper to redact secrets for logging or safe UI display (e.g., "hf_****...aB3x")
    [[nodiscard]] static std::string Redact(const std::string& secret) noexcept;

private:
    Vault() = default;
    ~Vault() { ClearAll(); }

    std::unordered_map<VaultKey, std::vector<uint8_t>> m_store;
    // Simple XOR key stream for in-process memory scrambling to prevent heap scanning
    std::vector<uint8_t> m_obfuscationMask = {0x5A, 0xA5, 0xC3, 0x3C, 0xF0, 0x0F, 0x96, 0x69};
};

} // namespace qivforge::core

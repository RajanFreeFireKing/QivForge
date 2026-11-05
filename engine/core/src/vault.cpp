#include "qivforge/core/vault.h"
#include <algorithm>

namespace qivforge::core {

SecureBuffer::SecureBuffer(const std::string& secret) {
    if (!secret.empty()) {
        m_data.resize(secret.size() + 1);
        std::memcpy(m_data.data(), secret.data(), secret.size());
        m_data[secret.size()] = '\0';
    }
}

SecureBuffer::SecureBuffer(std::vector<char> bytes) : m_data(std::move(bytes)) {}

SecureBuffer::~SecureBuffer() {
    Clear();
}

SecureBuffer::SecureBuffer(SecureBuffer&& other) noexcept : m_data(std::move(other.m_data)) {
    other.m_data.clear();
}

SecureBuffer& SecureBuffer::operator=(SecureBuffer&& other) noexcept {
    if (this != &other) {
        Clear();
        m_data = std::move(other.m_data);
        other.m_data.clear();
    }
    return *this;
}

void SecureBuffer::Clear() noexcept {
    if (!m_data.empty()) {
        // Securely wipe memory
        volatile char* p = m_data.data();
        size_t n = m_data.size();
        while (n--) {
            *p++ = '\0';
        }
        m_data.clear();
    }
}

Vault& Vault::Instance() noexcept {
    static Vault s_instance;
    return s_instance;
}

void Vault::Put(VaultKey key, const std::string& secret) {
    if (secret.empty()) {
        Remove(key);
        return;
    }

    std::vector<uint8_t> obfuscated(secret.size());
    for (size_t i = 0; i < secret.size(); ++i) {
        obfuscated[i] = static_cast<uint8_t>(secret[i]) ^ m_obfuscationMask[i % m_obfuscationMask.size()];
    }
    m_store[key] = std::move(obfuscated);
}

std::optional<std::string> Vault::Get(VaultKey key) const {
    auto it = m_store.find(key);
    if (it == m_store.end()) {
        return std::nullopt;
    }

    std::string result;
    result.resize(it->second.size());
    for (size_t i = 0; i < it->second.size(); ++i) {
        result[i] = static_cast<char>(it->second[i] ^ m_obfuscationMask[i % m_obfuscationMask.size()]);
    }
    return result;
}

bool Vault::Has(VaultKey key) const noexcept {
    return m_store.find(key) != m_store.end();
}

void Vault::Remove(VaultKey key) {
    auto it = m_store.find(key);
    if (it != m_store.end()) {
        volatile uint8_t* p = it->second.data();
        size_t n = it->second.size();
        while (n--) {
            *p++ = 0;
        }
        m_store.erase(it);
    }
}

void Vault::ClearAll() noexcept {
    for (auto& pair : m_store) {
        volatile uint8_t* p = pair.second.data();
        size_t n = pair.second.size();
        while (n--) {
            *p++ = 0;
        }
    }
    m_store.clear();
}

std::string Vault::Redact(const std::string& secret) noexcept {
    if (secret.empty()) return "<empty>";
    if (secret.size() <= 8) return "********";

    // Show first 3 and last 4 characters, redact the middle
    std::string prefix = secret.substr(0, 3);
    std::string suffix = secret.substr(secret.size() - 4);
    return prefix + "****..." + suffix;
}

} // namespace qivforge::core

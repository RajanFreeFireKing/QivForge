#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace qivforge::cloud {

class Sha256 {
public:
    Sha256();
    void Update(const uint8_t* data, size_t length);
    void Update(const std::string& data);
    [[nodiscard]] std::string Final();

    // Convenient one-shot computation
    [[nodiscard]] static std::string Compute(const std::string& input);
    [[nodiscard]] static std::string Compute(const uint8_t* data, size_t length);

private:
    void Transform(const uint8_t block[64]);

    uint32_t m_state[8];
    uint64_t m_count;
    uint8_t m_buffer[64];
};

} // namespace qivforge::cloud

#pragma once

#include "qivforge/core/error_codes.h"
#include <string>
#include <vector>
#include <mutex>

namespace qivforge::core {

enum class CommandType : uint32_t {
    NONE = 0,
    CREATE_ACTOR = 1,
    DESTROY_ACTOR = 2,
    SET_TRANSFORM = 3,
    PLAY = 4,
    PAUSE = 5,
    STOP = 6,
    LOAD_SCENE = 7
};

struct EngineCommand {
    CommandType type = CommandType::NONE;
    uint32_t targetId = 0;
    float paramFloats[8] = {0.0f};
    char paramStr[64] = {0};
};

class CommandQueue {
public:
    void Enqueue(const EngineCommand& cmd) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push_back(cmd);
    }

    std::vector<EngineCommand> Drain() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<EngineCommand> drained;
        drained.swap(m_queue);
        return drained;
    }

    [[nodiscard]] bool IsEmpty() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

private:
    std::mutex m_mutex;
    std::vector<EngineCommand> m_queue;
};

} // namespace qivforge::core

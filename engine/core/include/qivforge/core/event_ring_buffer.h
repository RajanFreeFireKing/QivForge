#pragma once

#include <cstdint>
#include <atomic>
#include <array>
#include <cstring>

namespace qivforge::core {

enum class EventType : uint32_t {
    NONE = 0,
    STATS_UPDATED = 1,
    SURFACE_STATE_CHANGED = 2,
    HIERARCHY_CHANGED = 3,
    SELECTION_CHANGED = 4,
    PLAY_STATE_CHANGED = 5,
    SCRIPT_ERROR = 6
};

struct StatsPayload {
    float fps;
    float frameMs;
    uint32_t drawCalls;
    uint32_t triangleCount;
};

struct SurfacePayload {
    int32_t width;
    int32_t height;
    bool hasSurface;
};

struct ActorPayload {
    uint32_t actorId;
    char name[32];
};

struct ScriptErrorPayload {
    int32_t line;
    char file[32];
    char message[64];
};

struct EngineEvent {
    EventType type = EventType::NONE;
    uint64_t timestamp = 0;
    union {
        StatsPayload stats;
        SurfacePayload surface;
        ActorPayload actor;
        ScriptErrorPayload scriptError;
        uint32_t rawData[32];
    } payload;
};

/**
 * Lock-free single-producer single-consumer ring buffer per Doc 02 §6
 * Zero dynamic memory allocations during runtime.
 */
class EventRingBuffer {
public:
    static constexpr size_t CAPACITY = 1024;
    static constexpr size_t MASK = CAPACITY - 1;

    EventRingBuffer() noexcept : m_head(0), m_tail(0) {
        m_buffer.fill(EngineEvent{});
    }

    bool Push(const EngineEvent& event) noexcept {
        const size_t currentTail = m_tail.load(std::memory_order_relaxed);
        const size_t currentHead = m_head.load(std::memory_order_acquire);

        if ((currentTail - currentHead) >= CAPACITY) {
            // Buffer full, drop event to prevent blocking engine main thread
            return false;
        }

        m_buffer[currentTail & MASK] = event;
        m_tail.store(currentTail + 1, std::memory_order_release);
        return true;
    }

    bool Pop(EngineEvent& outEvent) noexcept {
        const size_t currentHead = m_head.load(std::memory_order_relaxed);
        const size_t currentTail = m_tail.load(std::memory_order_acquire);

        if (currentHead == currentTail) {
            // Buffer empty
            return false;
        }

        outEvent = m_buffer[currentHead & MASK];
        m_head.store(currentHead + 1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] size_t Size() const noexcept {
        const size_t head = m_head.load(std::memory_order_relaxed);
        const size_t tail = m_tail.load(std::memory_order_relaxed);
        return (tail >= head) ? (tail - head) : 0;
    }

    [[nodiscard]] bool IsEmpty() const noexcept {
        return m_head.load(std::memory_order_relaxed) == m_tail.load(std::memory_order_relaxed);
    }

    void Clear() noexcept {
        m_head.store(0, std::memory_order_relaxed);
        m_tail.store(0, std::memory_order_relaxed);
    }

private:
    alignas(64) std::atomic<size_t> m_head;
    alignas(64) std::atomic<size_t> m_tail;
    std::array<EngineEvent, CAPACITY> m_buffer;
};

} // namespace qivforge::core

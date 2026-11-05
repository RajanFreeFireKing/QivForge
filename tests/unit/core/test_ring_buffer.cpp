#include "qivforge/core/event_ring_buffer.h"
#include <cassert>
#include <iostream>

using namespace qivforge::core;

void TestRingBufferPushPop() {
    EventRingBuffer buffer;
    assert(buffer.IsEmpty());
    assert(buffer.Size() == 0);

    // Push stats event
    EngineEvent ev1;
    ev1.type = EventType::STATS_UPDATED;
    ev1.payload.stats.fps = 60.0f;
    ev1.payload.stats.frameMs = 16.6f;
    ev1.payload.stats.drawCalls = 1;
    ev1.payload.stats.triangleCount = 12;

    assert(buffer.Push(ev1));
    assert(!buffer.IsEmpty());
    assert(buffer.Size() == 1);

    // Pop stats event
    EngineEvent popped;
    assert(buffer.Pop(popped));
    assert(buffer.IsEmpty());
    assert(buffer.Size() == 0);

    assert(popped.type == EventType::STATS_UPDATED);
    assert(popped.payload.stats.fps == 60.0f);
    assert(popped.payload.stats.drawCalls == 1);
    assert(popped.payload.stats.triangleCount == 12);

    std::cout << "[PASS] TestRingBufferPushPop" << std::endl;
}

void TestRingBufferCapacityAndOverflow() {
    EventRingBuffer buffer;

    // Fill to capacity
    for (size_t i = 0; i < EventRingBuffer::CAPACITY; ++i) {
        EngineEvent ev;
        ev.type = EventType::STATS_UPDATED;
        ev.payload.stats.fps = static_cast<float>(i);
        assert(buffer.Push(ev));
    }

    assert(buffer.Size() == EventRingBuffer::CAPACITY);

    // Next push should fail gracefully without crashing
    EngineEvent overflowEv;
    overflowEv.type = EventType::STATS_UPDATED;
    assert(!buffer.Push(overflowEv));

    // Drain and verify order
    for (size_t i = 0; i < EventRingBuffer::CAPACITY; ++i) {
        EngineEvent out;
        assert(buffer.Pop(out));
        assert(out.payload.stats.fps == static_cast<float>(i));
    }

    assert(buffer.IsEmpty());
    std::cout << "[PASS] TestRingBufferCapacityAndOverflow" << std::endl;
}

int main() {
    std::cout << "Running Lock-Free Event Ring Buffer Tests..." << std::endl;
    TestRingBufferPushPop();
    TestRingBufferCapacityAndOverflow();
    std::cout << "All Event Ring Buffer Tests Passed Successfully!" << std::endl;
    return 0;
}

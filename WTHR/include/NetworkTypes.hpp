#pragma once

#include <vector>
#include <cstdint>
#include <queue>
#include <mutex>
#include <memory>

// ============================================================================
// 1. SHARED NETWORK TYPES
// ============================================================================

// A simple connection identifier (e.g., 0 for server, unique ID for clients)
using ConnectionID = size_t;

// The raw network message data
struct Packet {
    uint32_t typeID;             // Opcode / Message identifier (e.g., 1 = Input, 2 = UI)
    std::vector<uint8_t> bytes;  // Raw payload
};

// Wraps a packet so the engine knows exactly who sent it or who it belongs to
struct OwnedPacket {
    ConnectionID senderID;       // Who sent this?
    Packet packet;               // The actual data
};

// ============================================================================
// 2. THREAD-SAFE QUEUE (The Bridge)
// ============================================================================
template<typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;
    ~ThreadSafeQueue() = default;

    // Disallow copying to prevent accidental thread issues
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    void Push(const T& value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(value);
    }

    bool Pop(T& outValue) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty()) return false;

        outValue = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    bool Empty() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::queue<T> empty;
        std::swap(m_queue, empty);
    }

private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
};

// Tests/NetworkTypesTests.cpp
#include <catch2/catch_test_macros.hpp>
#include "NetworkTypes.hpp"
#include <thread>
#include <vector>

TEST_CASE("ThreadSafeQueue Operations", "[network][queue]")
{
    ThreadSafeQueue<OwnedPacket> queue;

    SECTION("Push and Pop order is FIFO (First-In, First-Out)")
    {
        OwnedPacket p1{ 1, { 101, { 0x01, 0x02 } } };
        OwnedPacket p2{ 2, { 102, { 0x03, 0x04 } } };

        queue.Push(p1);
        queue.Push(p2);

        OwnedPacket popped;

        REQUIRE(queue.Pop(popped));
        REQUIRE(popped.senderID == 1);
        REQUIRE(popped.packet.typeID == 101);

        REQUIRE(queue.Pop(popped));
        REQUIRE(popped.senderID == 2);
        REQUIRE(popped.packet.typeID == 102);

        // Queue should now be empty
        REQUIRE_FALSE(queue.Pop(popped));
        REQUIRE(queue.Empty());
    }

    SECTION("Clear empties the queue")
    {
        queue.Push({ 1, { 1, { 0xFF } } });
        queue.Push({ 2, { 2, { 0xEE } } });

        REQUIRE_FALSE(queue.Empty());

        queue.Clear();

        REQUIRE(queue.Empty());

        OwnedPacket dummy;
        REQUIRE_FALSE(queue.Pop(dummy));
    }
}

TEST_CASE("ThreadSafeQueue Multi-Threaded Stress Test", "[network][queue][concurrency]")
{
    ThreadSafeQueue<uint32_t> queue;
    constexpr int itemsPerThread = 1000;
    constexpr int numProducers = 4;

    SECTION("Concurrent pushes from multiple threads do not corrupt data")
    {
        std::vector<std::thread> producers;
        producers.reserve(numProducers);

        // Spawn multiple producer threads pushing items simultaneously
        for (int i = 0; i < numProducers; ++i) {
            producers.emplace_back([&queue, i]() {
                for (int j = 0; j < itemsPerThread; ++j) {
                    queue.Push(static_cast<uint32_t>(i * itemsPerThread + j));
                }
                });
        }

        // Wait for all producer threads to complete
        for (auto& t : producers) {
            t.join();
        }

        // Verify total count
        int count = 0;
        uint32_t val;
        while (queue.Pop(val)) {
            count++;
        }

        REQUIRE(count == (numProducers * itemsPerThread));
        REQUIRE(queue.Empty());
    }
}
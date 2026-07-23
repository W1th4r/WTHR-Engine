#include <catch2/catch_test_macros.hpp>
#include "NetworkServer.hpp"

#define ASIO_STANDALONE
#include <asio.hpp>
#include <thread>
#include <chrono>
#include <vector>
#include <string>

using namespace std::chrono_literals;

// Lightweight inline test client to interact with NetworkServer
class TestClient {
public:
    TestClient() : m_socket(m_io) {}

    bool Connect(const std::string& host, uint16_t port) {
        std::error_code ec;
        asio::ip::tcp::resolver resolver(m_io);
        auto endpoints = resolver.resolve(host, std::to_string(port), ec);
        if (ec) return false;

        asio::connect(m_socket, endpoints, ec);
        return !ec;
    }

    void Send(uint32_t typeID, const std::string& payload) {
        uint32_t length = static_cast<uint32_t>(payload.size());
        std::array<uint32_t, 2> header{ typeID, length };

        std::vector<asio::const_buffer> buffers;
        buffers.push_back(asio::buffer(header));
        if (length > 0) {
            buffers.push_back(asio::buffer(payload.data(), payload.size()));
        }

        asio::write(m_socket, buffers);
    }

    bool Read(uint32_t& outType, std::string& outPayload, std::chrono::milliseconds timeout = 500ms) {
        m_socket.non_blocking(true);
        auto start = std::chrono::steady_clock::now();

        std::array<uint32_t, 2> header{};
        std::error_code ec;

        // Poll for header
        while (std::chrono::steady_clock::now() - start < timeout) {
            asio::read(m_socket, asio::buffer(header), ec);
            if (!ec) break;
            std::this_thread::sleep_for(5ms);
        }
        if (ec) return false;

        outType = header[0];
        uint32_t length = header[1];

        outPayload.resize(length);
        if (length > 0) {
            asio::read(m_socket, asio::buffer(outPayload.data(), length), ec);
        }

        m_socket.non_blocking(false);
        return true;
    }

    void Disconnect() {
        std::error_code ec;
        m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
        m_socket.close(ec);
    }

    ~TestClient() {
        Disconnect();
    }

private:
    asio::io_context m_io;
    asio::ip::tcp::socket m_socket;
};

// ============================================================================
// CATCH2 TEST SUITE
// ============================================================================

TEST_CASE("NetworkServer Core Functionality", "[network][server]") {
    constexpr uint16_t TEST_PORT = 27016;

    SECTION("Server starts and stops cleanly") {
        NetworkServer server;
        REQUIRE(server.Start(nullptr, TEST_PORT));
        REQUIRE(server.GetActiveClientIDs().empty());

        server.Stop();
    }

    SECTION("Client connection & OnConnect dummy packet dispatch") {
        NetworkServer server;
        REQUIRE(server.Start("127.0.0.1", TEST_PORT));

        TestClient client;
        REQUIRE(client.Connect("127.0.0.1", TEST_PORT));

        std::this_thread::sleep_for(50ms); // Allow accept pump to process

        auto activeClients = server.GetActiveClientIDs();
        REQUIRE(activeClients.size() == 1);

        ConnectionID clientID = activeClients[0];

        // Verify the "OnConnect" event packet (typeID 0) pushed to incoming queue
        ConnectionID eventClientID = 0;
        uint32_t eventTypeID = 999;
        std::string payload;

        bool popped = server.ReceiveFrom(eventClientID, eventTypeID, payload);
        REQUIRE(popped);
        CHECK(eventClientID == clientID);
        CHECK(eventTypeID == 0); // Connect opcode defined in server

        server.Stop();
    }

    SECTION("ReceiveFrom reads inbound client messages correctly") {
        NetworkServer server;
        REQUIRE(server.Start("127.0.0.1", TEST_PORT));

        TestClient client;
        REQUIRE(client.Connect("127.0.0.1", TEST_PORT));
        std::this_thread::sleep_for(50ms);

        // Clear the OnConnect event
        ConnectionID dummyID; uint32_t dummyType; std::string dummyPayload;
        server.ReceiveFrom(dummyID, dummyType, dummyPayload);

        // Send payload from test client
        const uint32_t sentType = 500;
        const std::string sentData = "Client to Server Test Message";
        client.Send(sentType, sentData);

        std::this_thread::sleep_for(50ms);

        ConnectionID recvClientID = 0;
        uint32_t recvType = 0;
        std::string recvData;

        bool success = server.ReceiveFrom(recvClientID, recvType, recvData);
        REQUIRE(success);
        CHECK(recvType == sentType);
        CHECK(recvData == sentData);

        server.Stop();
    }

    SECTION("SendToClient delivers targeted messages") {
        NetworkServer server;
        REQUIRE(server.Start("127.0.0.1", TEST_PORT));

        TestClient client;
        REQUIRE(client.Connect("127.0.0.1", TEST_PORT));
        std::this_thread::sleep_for(50ms);

        auto clients = server.GetActiveClientIDs();
        REQUIRE(clients.size() == 1);
        ConnectionID targetID = clients[0];

        const uint32_t targetType = 777;
        const std::string targetPayload = "Private message for client";

        server.SendToClient(targetID, targetType, targetPayload);

        uint32_t recvType = 0;
        std::string recvData;
        bool readOk = client.Read(recvType, recvData);

        REQUIRE(readOk);
        CHECK(recvType == targetType);
        CHECK(recvData == targetPayload);

        server.Stop();
    }

    SECTION("Broadcast delivers to all clients and respects ignoreClientID") {
        NetworkServer server;
        REQUIRE(server.Start("127.0.0.1", TEST_PORT));

        TestClient client1, client2;
        REQUIRE(client1.Connect("127.0.0.1", TEST_PORT));
        REQUIRE(client2.Connect("127.0.0.1", TEST_PORT));
        std::this_thread::sleep_for(50ms);

        auto clients = server.GetActiveClientIDs();
        REQUIRE(clients.size() == 2);

        ConnectionID id1 = clients[0];

        // Broadcast ignoring client1
        const uint32_t bcastType = 888;
        const std::string bcastData = "Broadcast notice";
        server.Broadcast(bcastType, bcastData, id1);

        uint32_t c1Type = 0, c2Type = 0;
        std::string c1Data, c2Data;

        // Client 1 should NOT receive anything
        bool c1Recv = client1.Read(c1Type, c1Data, 100ms);
        CHECK_FALSE(c1Recv);

        // Client 2 SHOULD receive the broadcast
        bool c2Recv = client2.Read(c2Type, c2Data, 500ms);
        REQUIRE(c2Recv);
        CHECK(c2Type == bcastType);
        CHECK(c2Data == bcastData);

        server.Stop();
    }

    SECTION("DisconnectClient forcibly kicks a client") {
        NetworkServer server;
        REQUIRE(server.Start("127.0.0.1", TEST_PORT));

        TestClient client;
        REQUIRE(client.Connect("127.0.0.1", TEST_PORT));
        std::this_thread::sleep_for(50ms);

        auto clients = server.GetActiveClientIDs();
        REQUIRE(clients.size() == 1);
        ConnectionID targetID = clients[0];

        server.DisconnectClient(targetID);
        std::this_thread::sleep_for(50ms);

        CHECK(server.GetActiveClientIDs().empty());

        server.Stop();
    }
}
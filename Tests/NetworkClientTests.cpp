#include <catch2/catch_test_macros.hpp>
#include "NetworkClient.hpp"
#include <asio.hpp>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

// Lightweight local mock server for isolated testing
class MockServer {
public:
    MockServer(uint16_t port)
        : m_acceptor(m_io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
        m_socket(m_io) {
    }

    void Run() {
        m_thread = std::thread([this]() {
            m_acceptor.accept(m_socket);
            });
    }

    void Send(uint32_t typeID, const std::string& payload) {
        uint32_t length = static_cast<uint32_t>(payload.size());

        std::vector<asio::const_buffer> buffers;
        buffers.push_back(asio::buffer(&typeID, sizeof(typeID)));
        buffers.push_back(asio::buffer(&length, sizeof(length)));
        if (length > 0) {
            buffers.push_back(asio::buffer(payload.data(), payload.size()));
        }

        asio::write(m_socket, buffers);
    }

    bool Read(uint32_t& outType, std::string& outPayload) {
        std::array<uint32_t, 2> header{};
        std::error_code ec;

        asio::read(m_socket, asio::buffer(header), ec);
        if (ec) return false;

        outType = header[0];
        uint32_t length = header[1];

        outPayload.resize(length);
        if (length > 0) {
            asio::read(m_socket, asio::buffer(outPayload.data(), length), ec);
        }
        return !ec;
    }

    ~MockServer() {
        std::error_code ec;
        m_socket.close(ec);
        m_acceptor.close(ec);
        m_io.stop();
        if (m_thread.joinable()) m_thread.join();
    }

private:
    asio::io_context m_io;
    asio::ip::tcp::acceptor m_acceptor;
    asio::ip::tcp::socket m_socket;
    std::thread m_thread;
};

TEST_CASE("NetworkClient Lifecycle and Messaging", "[network][client]") {
    constexpr uint16_t TEST_PORT = 27015;

    SECTION("Client connects and disconnects cleanly") {
        MockServer server(TEST_PORT);
        server.Run();

        NetworkClient client;
        REQUIRE_FALSE(client.IsConnected());

        bool connected = client.Start("127.0.0.1", TEST_PORT);
        REQUIRE(connected);
        REQUIRE(client.IsConnected());

        client.Stop();
        REQUIRE_FALSE(client.IsConnected());
    }

    SECTION("Client sends packet to server successfully") {
        MockServer server(TEST_PORT);
        server.Run();

        NetworkClient client;
        REQUIRE(client.Start("127.0.0.1", TEST_PORT));

        const uint32_t sendType = 1001;
        const std::string sendData = "Ping from WTHR Client";

        client.Send(sendType, sendData);

        // Allow background thread to process send loop
        std::this_thread::sleep_for(50ms);

        uint32_t recvType = 0;
        std::string recvData;
        bool readSuccess = server.Read(recvType, recvData);

        REQUIRE(readSuccess);
        CHECK(recvType == sendType);
        CHECK(recvData == sendData);

        client.Stop();
    }

    SECTION("Client receives packet from server successfully") {
        MockServer server(TEST_PORT);
        server.Run();

        NetworkClient client;
        REQUIRE(client.Start("127.0.0.1", TEST_PORT));

        const uint32_t respType = 2002;
        const std::string respData = "Pong from Mock Server";

        server.Send(respType, respData);

        // Allow background thread to process read loop
        std::this_thread::sleep_for(50ms);

        uint32_t clientRecvType = 0;
        std::string clientRecvData;
        bool recvSuccess = client.Receive(clientRecvType, clientRecvData);

        REQUIRE(recvSuccess);
        CHECK(clientRecvType == respType);
        CHECK(clientRecvData == respData);

        client.Stop();
    }
}
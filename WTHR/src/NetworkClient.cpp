#include "NetworkClient.hpp"
#include <asio.hpp>
#include <iostream>
#include <array>
#include <vector>

// Define the Asio structure exclusively inside the client implementation file
struct NetworkClientImpl {
    asio::io_context ioContext;
    asio::ip::tcp::socket socket;
    std::optional<asio::executor_work_guard<asio::io_context::executor_type>> workGuard;

    NetworkClientImpl() : socket(ioContext) {}
};

NetworkClient::NetworkClient() : NetworkManager(){
    // Allocate the client-specific Asio implementation pointer
    m_clientImpl = std::make_unique<NetworkClientImpl>();
}

NetworkClient::~NetworkClient() {
    Stop();
}

bool NetworkClient::Start(const char* host, int port) {
    if (m_isConnected) return false;
    try {
        asio::ip::tcp::resolver resolver(m_clientImpl->ioContext);
        auto endpoints = resolver.resolve(host, std::to_string(port));

        asio::connect(m_clientImpl->socket, endpoints);

        m_isConnected = true;
        m_isRunning = true;

        // 1. Prime the reading engine
        ReadPacket();

        // 2. Prime the writing engine (Check if there's anything to send)
        WritePackets();

        // 3. Fire up the background thread to handle BOTH pipelines simultaneously
        m_workerThread = std::thread(&NetworkClient::WorkerThread, this);

        std::cout << "[Client] Successfully connected to " << host << ":" << port << "\n";
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[Client] Connection failed: " << e.what() << "\n";
        Stop();
        return false;
    }
}

void NetworkClient::Stop() {
    if (!m_isRunning) return;
    m_isRunning = false;
    m_isConnected = false;

    if (m_clientImpl->workGuard) {
        m_clientImpl->workGuard.reset();
    }

    asio::post(m_clientImpl->ioContext, [this]() {
        if (m_clientImpl->socket.is_open()) {
            std::error_code ec;
            m_clientImpl->socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
            m_clientImpl->socket.close(ec);
        }

        m_clientImpl->ioContext.stop();
        });

    if (m_workerThread.joinable()) {
        m_workerThread.detach();
    }

    NetworkManager::Stop();
    std::cout << "[Client] Network connection stopped clean.\n";
}

void NetworkClient::Send(uint32_t typeID, std::string_view payload) {
    if (!m_isConnected) return;

    OwnedPacket owned;
    owned.senderID = 0; // Client always assumes 0 (outgoing to server)
    owned.packet.typeID = typeID;

    // Copy the raw string bytes directly into the packet's byte vector
    owned.packet.bytes.assign(payload.begin(), payload.end());

    m_outgoingQueue.Push(owned);
}
bool NetworkClient::Receive(uint32_t& outTypeID, std::string& outPayload) {
    OwnedPacket owned;
    if (m_incomingQueue.Pop(owned)) {
        outTypeID = owned.packet.typeID;

        // Reconstruct the byte vector back into a readable string payload
        outPayload.assign(owned.packet.bytes.begin(), owned.packet.bytes.end());
        return true;
    }
    return false;
}

bool NetworkClient::IsConnected() const {
    return m_isConnected;
}

void NetworkClient::WorkerThread() {
    // Keep Asio loop humming until context is explicitly stopped
    m_clientImpl->workGuard.emplace(asio::make_work_guard(m_clientImpl->ioContext));
    m_clientImpl->ioContext.run();
}

void NetworkClient::ReadPacket() {
    if (!m_isConnected) return;

    // Use a shared buffer to read the header: TypeID (4B) + Length (4B)
    auto headerBuffer = std::make_shared<std::array<uint32_t, 2>>();

    asio::async_read(m_clientImpl->socket, asio::buffer(*headerBuffer),
        [this, headerBuffer](std::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                uint32_t typeID = (*headerBuffer)[0];
                uint32_t payloadLength = (*headerBuffer)[1];

                // Prep memory payload allocation
                auto bodyBuffer = std::make_shared<std::vector<uint8_t>>(payloadLength);

                // Read full payload size frame
                asio::async_read(m_clientImpl->socket, asio::buffer(*bodyBuffer),
                    [this, typeID, bodyBuffer](std::error_code ec, std::size_t /*length*/) {
                        if (!ec) {
                            OwnedPacket owned;
                            owned.senderID = 0; // In client context, 0 denotes incoming from central server
                            owned.packet.typeID = typeID;
                            owned.packet.bytes = std::move(*bodyBuffer);

                            // Shove it safely into the Game Thread queue
                            m_incomingQueue.Push(owned);

                            // Recurse to handle incoming pipeline flow
                            ReadPacket();
                        }
                        else {
                            std::cerr << "[Client] Error reading message frame body.\n";
                            Stop();
                        }
                    });
            }
            else {
                if (m_isRunning) {
                    std::cerr << "[Client] Lost connection to server or read error occurred.\n";
                    Stop();
                }
            }
        });
}

void NetworkClient::WritePackets() {
    if (!m_isConnected || !m_isRunning) return;

    OwnedPacket outgoingItem;

    // Check if the thread-safe queue has data
    if (m_outgoingQueue.Pop(outgoingItem)) {
        // We use shared_ptrs so the data stays alive during the async operation
        auto typeID = std::make_shared<uint32_t>(outgoingItem.packet.typeID);
        auto payloadLength = std::make_shared<uint32_t>(static_cast<uint32_t>(outgoingItem.packet.bytes.size()));
        auto bodyBuffer = std::make_shared<std::vector<uint8_t>>(std::move(outgoingItem.packet.bytes));

        // Gather buffers linearly
        std::vector<asio::const_buffer> buffers;
        buffers.push_back(asio::buffer(typeID.get(), sizeof(uint32_t)));
        buffers.push_back(asio::buffer(payloadLength.get(), sizeof(uint32_t)));
        if (*payloadLength > 0) {
            buffers.push_back(asio::buffer(bodyBuffer->data(), bodyBuffer->size()));
        }

        // Send asynchronously on our single background thread
        asio::async_write(m_clientImpl->socket, buffers,
            [this, typeID, payloadLength, bodyBuffer](std::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    // Packet sent successfully! Immediately check for the next one.
                    WritePackets();
                }
                else {
                    std::cerr << "[Client] Async write error: " << ec.message() << "\n";
                    Stop();
                }
            });
    }
    else {
        // Queue was empty. Instead of sleeping the thread, tell Asio to call us back in 1ms.
        // This keeps the background thread 100% responsive to incoming data!
        auto timer = std::make_shared<asio::steady_timer>(m_clientImpl->ioContext, std::chrono::milliseconds(1));
        timer->async_wait([this, timer](std::error_code ec) {
            if (!ec) {
                WritePackets();
            }
            });
    }
}
// core/src/NetworkServer.cpp
#include "NetworkServer.hpp"

#define ASIO_STANDALONE
#include <asio.hpp>

#include <iostream>
#include <thread>
#include <array>
#include <vector>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <optional>

// ============================================================================
// 1. OUTGOING MESSAGE BUFFER
// ============================================================================
// Holds the header and payload safely in memory while ASIO processes the write
struct OutgoingMessage {
    std::array<uint32_t, 2> header;
    std::vector<uint8_t> body;
};

// ============================================================================
// 2. INTERNAL CLIENT SESSION (Hidden entirely inside the implementation unit)
// ============================================================================
class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
    ClientSession(asio::ip::tcp::socket socket, ConnectionID id, NetworkServer& server, ThreadSafeQueue<OwnedPacket>& incomingQueue)
        : m_socket(std::move(socket)), m_id(id), m_server(server), m_incomingQueue(incomingQueue) {
    }

    void StartReading() {
        ReadHeader();
    }

    void Send(uint32_t typeID, std::string_view payload) {
        auto msg = std::make_shared<OutgoingMessage>();
        msg->header[0] = typeID;
        msg->header[1] = static_cast<uint32_t>(payload.size());
        msg->body.assign(payload.begin(), payload.end());

        // asio::post bounces this work onto the ASIO background thread.
        // This makes m_outQueue completely thread-safe without needing mutexes.
        asio::post(m_socket.get_executor(), [self = shared_from_this(), this, msg]() {
            bool writeInProgress = !m_outQueue.empty();
            m_outQueue.push(msg);

            // If a write wasn't already happening, kick off the chain
            if (!writeInProgress) {
                WriteImpl();
            }
            });
    }

    void Disconnect() {
        asio::post(m_socket.get_executor(), [self = shared_from_this(), this]() {
            if (m_socket.is_open()) {
                std::error_code ec;
                m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
                m_socket.close(ec);

                // Notify the server so it can remove us from the active sessions map
                m_server.OnClientDisconnect(m_id);
            }
            });
    }

private:
    void WriteImpl() {
        auto msg = m_outQueue.front();

        std::vector<asio::const_buffer> buffers{
            asio::buffer(msg->header.data(), msg->header.size() * sizeof(uint32_t))
        };
        if (!msg->body.empty()) {
            buffers.push_back(asio::buffer(msg->body.data(), msg->body.size()));
        }

        asio::async_write(m_socket, buffers,
            [self = shared_from_this(), this](std::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    m_outQueue.pop(); // Remove finished message
                    if (!m_outQueue.empty()) {
                        WriteImpl(); // Queue not empty, write the next one
                    }
                }
                else {
                    std::cerr << "[Server] Failed to write to Client " << m_id << ": " << ec.message() << "\n";
                    Disconnect();
                }
            });
    }

    void ReadHeader() {
        auto self = shared_from_this();
        auto header = std::make_shared<std::array<uint32_t, 2>>();

        asio::async_read(m_socket, asio::buffer(*header),
            [this, self, header](std::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    uint32_t typeID = (*header)[0];
                    uint32_t length = (*header)[1];

                    if (length > 0) {
                        ReadBody(typeID, length);
                    }
                    else {
                        // Empty message payload
                        PushIncoming(typeID, {});
                        ReadHeader(); // Loop back to read the next header
                    }
                }
                else {
                    Disconnect();
                }
            });
    }

    void ReadBody(uint32_t typeID, uint32_t length) {
        auto self = shared_from_this();
        auto body = std::make_shared<std::vector<uint8_t>>(length);

        asio::async_read(m_socket, asio::buffer(*body),
            [this, self, typeID, body](std::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    PushIncoming(typeID, std::move(*body));
                    ReadHeader(); // Loop back to read the next header
                }
                else {
                    Disconnect();
                }
            });
    }

    void PushIncoming(uint32_t typeID, std::vector<uint8_t> bytes) {
        OwnedPacket op;
        op.senderID = m_id;
        op.packet.typeID = typeID;
        op.packet.bytes = std::move(bytes);

        m_incomingQueue.Push(op);
    }

    asio::ip::tcp::socket m_socket;
    ConnectionID m_id;
    NetworkServer& m_server;
    ThreadSafeQueue<OwnedPacket>& m_incomingQueue; // Assumed inherited from NetworkManager

    std::queue<std::shared_ptr<OutgoingMessage>> m_outQueue;
};

// ============================================================================
// 3. SERVER PIMPL STORAGE STRUCTURE
// ============================================================================
struct NetworkServer::ServerImpl {
    asio::io_context context;
    std::optional<asio::ip::tcp::acceptor> acceptor;
    std::thread backgroundThread;
    std::unordered_map<ConnectionID, std::shared_ptr<ClientSession>> sessions;
    std::mutex sessionsMutex;
};

// ============================================================================
// 4. NETWORK SERVER PUBLIC METHODS
// ============================================================================
NetworkServer::NetworkServer() : m_serverImpl(std::make_unique<ServerImpl>()) {
    m_isRunning = false;
}

NetworkServer::~NetworkServer() {
    Stop();
}

bool NetworkServer::Start(const char* host, int port) {
    if (m_isRunning) return false;
    try {
        asio::ip::tcp::endpoint endpoint;
        if (host == nullptr || std::strlen(host) == 0) {
            endpoint = asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port);
        }
        else {
            endpoint = asio::ip::tcp::endpoint(asio::ip::make_address(host), port);
        }

        m_serverImpl->acceptor.emplace(m_serverImpl->context, endpoint);
        m_isRunning = true;

        // Kick off the asynchronous continuous accept pump
        StartAccepting();

        // 1 dedicated background thread runs all connection accepts, reads, and writes
        m_serverImpl->backgroundThread = std::thread([this]() {
            auto workGuard = asio::make_work_guard(m_serverImpl->context);
            m_serverImpl->context.run();
            });

        std::cout << "[Server] Listening on port " << port << "...\n";
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[Server] Failed to start: " << e.what() << "\n";
        Stop();
        return false;
    }
}

void NetworkServer::Stop() {
    if (!m_isRunning) return;
    m_isRunning = false;

    // 1. First, gracefully extract all sessions to avoid recursive mutex deadlocks
    std::vector<std::shared_ptr<ClientSession>> sessionsToSever;
    {
        std::lock_guard<std::mutex> lock(m_serverImpl->sessionsMutex);
        for (auto& [id, session] : m_serverImpl->sessions) {
            sessionsToSever.push_back(session);
        }
        m_serverImpl->sessions.clear();
    }

    // 2. Sever connections outside the lock
    for (auto& session : sessionsToSever) {
        session->Disconnect();
    }

    // 3. Stop accepting new connections
    if (m_serverImpl->acceptor && m_serverImpl->acceptor->is_open()) {
        m_serverImpl->acceptor->close();
    }

    // 4. Kill the context and join the thread
    m_serverImpl->context.stop();
    if (m_serverImpl->backgroundThread.joinable()) {
        m_serverImpl->backgroundThread.join();
    }

    std::cout << "[Server] Stopped successfully.\n";
}
std::vector<ConnectionID> NetworkServer::GetActiveClientIDs() const {
    std::vector<ConnectionID> clientIDs;

    // We must lock the mutex because the background thread 
    // might be adding/removing sessions simultaneously.
    std::lock_guard<std::mutex> lock(m_serverImpl->sessionsMutex);

    for (const auto& [id, session] : m_serverImpl->sessions) {
        clientIDs.push_back(id);
    }

    return clientIDs;
}
void NetworkServer::StartAccepting() {
    m_serverImpl->acceptor->async_accept(
        [this](std::error_code ec, asio::ip::tcp::socket socket) {
            if (!ec) {
                ConnectionID newID = ++m_idCounter;
                std::cout << "[Server] New client connected! Assigned ID: " << newID << "\n";

                auto newSession = std::make_shared<ClientSession>(
                    std::move(socket), newID, *this, m_incomingQueue
                );

                {
                    std::lock_guard<std::mutex> lock(m_serverImpl->sessionsMutex);
                    m_serverImpl->sessions[newID] = newSession;
                }

                // NEW: Push a "Client Connected" dummy packet to notify the game engine instantly
                OwnedPacket connectEvent;
                connectEvent.senderID = newID;
                connectEvent.packet.typeID = 0; // Use 0 (or a specific opcode) to represent "OnConnect"
                connectEvent.packet.bytes = {};
                m_incomingQueue.Push(connectEvent);

                newSession->StartReading();
            }

            if (m_isRunning && m_serverImpl->acceptor->is_open()) {
                StartAccepting();
            }
        });
}
// --- MANDATORY INTERFACE CORES ---
void NetworkServer::Send(uint32_t typeID, std::string_view payload) {
    Broadcast(typeID, payload, 0);
}

bool NetworkServer::Receive(uint32_t& outTypeID, std::string& outPayload) {
    ConnectionID dummyID;
    return ReceiveFrom(dummyID, outTypeID, outPayload);
}

// --- ACTUAL GAME PIPELINES ---
bool NetworkServer::ReceiveFrom(ConnectionID& outClientID, uint32_t& outTypeID, std::string& outPayload) {
    OwnedPacket owned;
    if (m_incomingQueue.Pop(owned)) {
        outClientID = owned.senderID;
        outTypeID = owned.packet.typeID;
        outPayload.assign(owned.packet.bytes.begin(), owned.packet.bytes.end());
		spdlog::info("[Server] Received packet from Client {}: TypeID {}, Payload Size {}", outClientID, outTypeID, owned.packet.bytes.size());
        return true;
    }
    return false;
}

void NetworkServer::SendToClient(ConnectionID clientID, uint32_t typeID, std::string_view payload) {
    std::lock_guard<std::mutex> lock(m_serverImpl->sessionsMutex);
    auto it = m_serverImpl->sessions.find(clientID);
    if (it != m_serverImpl->sessions.end()) {
        it->second->Send(typeID, payload);
    }
}

void NetworkServer::Broadcast(uint32_t typeID, std::string_view payload, ConnectionID ignoreClientID) {
    std::lock_guard<std::mutex> lock(m_serverImpl->sessionsMutex);
    for (auto& [id, session] : m_serverImpl->sessions) {
        if (id != ignoreClientID) {
            session->Send(typeID, payload);
        }
    }
}

// Internal lifecycle hook called by standard sessions on socket death
void NetworkServer::OnClientDisconnect(ConnectionID clientID) {
    std::lock_guard<std::mutex> lock(m_serverImpl->sessionsMutex);
    m_serverImpl->sessions.erase(clientID);
    std::cout << "[Server] Client disconnected: " << clientID << "\n";
}

void NetworkServer::DisconnectClient(ConnectionID clientID) {
    std::shared_ptr<ClientSession> sessionToKill;

    // 1. Find the session and remove it from the map while under the lock
    {
        std::lock_guard<std::mutex> lock(m_serverImpl->sessionsMutex);
        auto it = m_serverImpl->sessions.find(clientID);
        if (it != m_serverImpl->sessions.end()) {
            sessionToKill = it->second; // Keep it alive momentarily
            m_serverImpl->sessions.erase(it);
        }
    }

    // 2. If we found it, trigger the socket closure outside the lock
    if (sessionToKill) {
        sessionToKill->Disconnect();
        std::cout << "[Server] Client has been kicked: " << clientID << "\n";
    }
}
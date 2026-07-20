#pragma once

#include "NetworkManager.hpp"
#include <memory>
#include <string_view>
#include <string>

class NetworkServer : public NetworkManager {
public:
    NetworkServer();
    virtual ~NetworkServer() override;

    virtual bool Start(const char* host, int port) override;
    virtual void Stop() override;

    // --- MANDATORY NETWORKMANAGER OVERRIDES ---
    // We must override these so the class can be instantiated.
    // We map 'Send' to mean 'Broadcast to everyone'.
    void Send(uint32_t typeID, std::string_view payload) override;
    // We map 'Receive' to pull the next packet, ignoring who sent it (good for global events).
    bool Receive(uint32_t& outTypeID, std::string& outPayload) override;


    // --- SERVER-SPECIFIC PUBLIC API (Use these in Game/Lua!) ---
    void SendToClient(ConnectionID clientID, uint32_t typeID, std::string_view payload);
    void Broadcast(uint32_t typeID, std::string_view payload, ConnectionID ignoreClientID = 0);

    void OnClientDisconnect(ConnectionID clientID);

    void DisconnectClient(ConnectionID clientID);
    // Server-specific pop that preserves WHO sent the data
    bool ReceiveFrom(ConnectionID& outClientID, uint32_t& outTypeID, std::string& outPayload);

    bool IsRunning() const { return m_isRunning; }
    std::vector<ConnectionID> GetActiveClientIDs() const;
private:
    void StartAccepting();

    struct ServerImpl;
    std::unique_ptr<ServerImpl> m_serverImpl;
    uint32_t m_idCounter = 0;


    bool m_isRunning = false;
};
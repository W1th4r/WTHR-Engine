#pragma once

#include <NetworkManager.hpp>
#include <thread>
#include <atomic>
#include <memory>

struct NetworkClientImpl; // Forward declared implementation container

class NetworkClient : public NetworkManager {
public:
    NetworkClient();
    ~NetworkClient() override;

    bool Start(const char* host, int port) override;
    void Stop() override;

    void Send(uint32_t typeID, std::string_view payload) override;
    bool Receive(uint32_t& outTypeID, std::string& outPayload)override;

    bool IsConnected() const;

private:
    void WorkerThread();
    void ReadPacket();
    void WritePackets();

private:
    std::thread m_workerThread;
    std::atomic<bool> m_isConnected{ false };
    std::atomic<bool> m_isRunning{ false };
    // Client specific private implementation detail container
    std::unique_ptr<NetworkClientImpl> m_clientImpl;
};
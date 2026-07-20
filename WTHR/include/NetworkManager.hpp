#pragma once
#include <memory>
#include <NetworkTypes.hpp>

class NetworkManager {
public:
    NetworkManager();
    virtual ~NetworkManager();

    // Lifecycle
    virtual bool Start(const char* host, int port) = 0;
    virtual void Stop();

    // ==========================================
    // THE GAME / UI THREAD INTERFACE
    // ==========================================

    virtual void Send(uint32_t typeID,const std::string_view payload) = 0;
	virtual bool Receive(uint32_t& typeID, std::string& payload) = 0;

protected:
    ThreadSafeQueue<OwnedPacket> m_incomingQueue;
    ThreadSafeQueue<OwnedPacket> m_outgoingQueue;
};
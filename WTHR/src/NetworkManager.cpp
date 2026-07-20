#include "NetworkManager.hpp"
#include <asio.hpp> 


//struct NetworkManager::Impl {
//    asio::io_context io_context;
//    // Any other Asio-specific sockets, contexts, or background workers go here!
//};

NetworkManager::NetworkManager() {
    //: pimpl(std::make_unique<Impl>()) {
}

NetworkManager::~NetworkManager() {
    Stop();
}

void NetworkManager::Stop() {
    // Clear out the mailboxes so no stale data leaks
    m_incomingQueue.Clear();
    m_outgoingQueue.Clear();
}

// These functions now act as the "Gatekeepers" for your queues.
// Your Game Loop calls these instead of interacting with the queues directly.


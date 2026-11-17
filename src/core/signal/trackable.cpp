#include "core/signal/trackable.hpp"

namespace northern::core {

Trackable::~Trackable() {
    disconnectAllTracked();
}

Trackable::Trackable(const Trackable&) noexcept {
    // Copy construction does not copy connections to this new instance
}

Trackable& Trackable::operator=(const Trackable&) noexcept {
    // Copy assignment does not copy connections
    return *this;
}

Trackable::Trackable(Trackable&& other) noexcept {
    std::lock_guard<std::mutex> lock(other.mutex_);
    connections_ = std::move(other.connections_);
}

Trackable& Trackable::operator=(Trackable&& other) noexcept {
    if (this != &other) {
        disconnectAllTracked();
        std::scoped_lock lock(mutex_, other.mutex_);
        connections_ = std::move(other.connections_);
    }
    return *this;
}

void Trackable::disconnectAllTracked() noexcept {
    std::vector<Connection> toDisconnect;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        toDisconnect.swap(connections_);
    }
    for (auto& conn : toDisconnect) {
        conn.disconnect();
    }
}

void Trackable::registerConnection(Connection conn) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Remove stale connections while registering to keep list compact
    std::erase_if(connections_, [](const Connection& c) { return !c.isConnected(); });
    connections_.push_back(std::move(conn));
}

void Trackable::unregisterConnection(ConnectionId id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::erase_if(connections_, [id](const Connection& c) { return c.id() == id; });
}

} // namespace northern::core

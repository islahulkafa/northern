#pragma once

#include "core/signal/connection.hpp"
#include <mutex>
#include <vector>

namespace northern::core {

namespace detail {
class TrackableAccessor;
}

class Trackable {
public:
    Trackable() = default;
    ~Trackable();

    Trackable(const Trackable&) noexcept;
    Trackable& operator=(const Trackable&) noexcept;

    Trackable(Trackable&& other) noexcept;
    Trackable& operator=(Trackable&& other) noexcept;

    void disconnectAllTracked() noexcept;

private:
    friend class detail::TrackableAccessor;

    void registerConnection(Connection conn);
    void unregisterConnection(ConnectionId id);

    mutable std::mutex mutex_;
    std::vector<Connection> connections_;
};

namespace detail {

class TrackableAccessor {
public:
    static void registerConnection(Trackable* trackable, Connection conn) {
        if (trackable) {
            trackable->registerConnection(std::move(conn));
        }
    }

    static void unregisterConnection(Trackable* trackable, ConnectionId id) {
        if (trackable) {
            trackable->unregisterConnection(id);
        }
    }
};

} // namespace detail

} // namespace northern::core

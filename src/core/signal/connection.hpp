#pragma once

#include <cstdint>
#include <memory>
#include <utility>

namespace northern::core {

using ConnectionId = uint64_t;

namespace detail {

class IConnectionState {
public:
    virtual ~IConnectionState() = default;
    virtual void disconnect() noexcept = 0;
    [[nodiscard]] virtual bool isConnected() const noexcept = 0;
    [[nodiscard]] virtual ConnectionId id() const noexcept = 0;
};

} // namespace detail

class Connection {
public:
    Connection() noexcept = default;
    explicit Connection(std::weak_ptr<detail::IConnectionState> state) noexcept
        : state_(std::move(state)) {}

    void disconnect() noexcept {
        if (auto s = state_.lock()) {
            s->disconnect();
        }
    }

    [[nodiscard]] bool isConnected() const noexcept {
        if (auto s = state_.lock()) {
            return s->isConnected();
        }
        return false;
    }

    [[nodiscard]] explicit operator bool() const noexcept { return isConnected(); }

    [[nodiscard]] ConnectionId id() const noexcept {
        if (auto s = state_.lock()) {
            return s->id();
        }
        return 0;
    }

    bool operator==(const Connection& other) const noexcept { return id() == other.id(); }

    bool operator!=(const Connection& other) const noexcept { return !(*this == other); }

private:
    std::weak_ptr<detail::IConnectionState> state_;
};

class ScopedConnection {
public:
    ScopedConnection() noexcept = default;
    explicit ScopedConnection(Connection connection) noexcept
        : connection_(std::move(connection)) {}

    ~ScopedConnection() { disconnect(); }

    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection& operator=(const ScopedConnection&) = delete;

    ScopedConnection(ScopedConnection&& other) noexcept
        : connection_(std::exchange(other.connection_, Connection{})) {}

    ScopedConnection& operator=(ScopedConnection&& other) noexcept {
        if (this != &other) {
            disconnect();
            connection_ = std::exchange(other.connection_, Connection{});
        }
        return *this;
    }

    ScopedConnection& operator=(Connection connection) noexcept {
        if (connection_ != connection) {
            disconnect();
            connection_ = std::move(connection);
        }
        return *this;
    }

    void disconnect() noexcept { connection_.disconnect(); }

    [[nodiscard]] bool isConnected() const noexcept { return connection_.isConnected(); }

    [[nodiscard]] explicit operator bool() const noexcept { return isConnected(); }

    [[nodiscard]] Connection release() noexcept { return std::exchange(connection_, Connection{}); }

    [[nodiscard]] const Connection& connection() const noexcept { return connection_; }

private:
    Connection connection_;
};

} // namespace northern::core

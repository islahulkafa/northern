#pragma once

#include "core/signal/connection.hpp"
#include "core/signal/trackable.hpp"
#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>
#include <vector>

namespace northern::core {

namespace detail {
ConnectionId generateNextConnectionId() noexcept;
}

template <typename... Args>
class Signal {
private:
    struct Slot;
    using SlotList = std::vector<Slot>;

    class ConnectionState final : public detail::IConnectionState {
    public:
        ConnectionState(ConnectionId id, Signal* signal) noexcept
            : id_(id), signal_(signal), connected_(true) {}

        void disconnect() noexcept override {
            bool expected = true;
            if (connected_.compare_exchange_strong(expected, false)) {
                if (signal_) {
                    signal_->disconnect(id_);
                }
            }
        }

        [[nodiscard]] bool isConnected() const noexcept override {
            return connected_.load(std::memory_order_acquire);
        }

        [[nodiscard]] ConnectionId id() const noexcept override { return id_; }

        void detachSignal() noexcept {
            signal_ = nullptr;
            connected_.store(false, std::memory_order_release);
        }

    private:
        ConnectionId id_{0};
        Signal* signal_{nullptr};
        std::atomic<bool> connected_{false};
    };

    struct Slot {
        ConnectionId id{0};
        std::shared_ptr<ConnectionState> state;
        std::function<void(Args...)> callback;
    };

public:
    Signal() : slots_(std::make_shared<const SlotList>()) {}

    ~Signal() { disconnectAll(); }

    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;

    Signal(Signal&& other) noexcept {
        std::lock_guard<std::mutex> lock(other.mutex_);
        slots_ = std::move(other.slots_);
        other.slots_ = std::make_shared<const SlotList>();
    }

    Signal& operator=(Signal&& other) noexcept {
        if (this != &other) {
            disconnectAll();
            std::scoped_lock lock(mutex_, other.mutex_);
            slots_ = std::move(other.slots_);
            other.slots_ = std::make_shared<const SlotList>();
        }
        return *this;
    }

    template <typename F>
        requires std::is_invocable_v<F, Args...> && (!std::is_same_v<std::decay_t<F>, Signal>)
    Connection connect(F&& callable) {
        ConnectionId id = detail::generateNextConnectionId();
        auto state = std::make_shared<ConnectionState>(id, this);

        Slot newSlot{id, state, std::function<void(Args...)>(std::forward<F>(callable))};

        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto newList = std::make_shared<SlotList>(*slots_);
            newList->push_back(std::move(newSlot));
            slots_ = std::move(newList);
        }

        return Connection(state);
    }

    template <typename Receiver, typename Method>
        requires std::is_invocable_v<Method, Receiver*, Args...>
    Connection connect(Receiver* receiver, Method method) {
        auto conn = connect([receiver, method](Args... args) {
            std::invoke(method, receiver, std::forward<Args>(args)...);
        });

        if constexpr (std::is_base_of_v<Trackable, Receiver>) {
            detail::TrackableAccessor::registerConnection(receiver, conn);
        }

        return conn;
    }

    template <typename Receiver, typename Method>
        requires std::is_invocable_v<Method, Receiver*, Args...>
    Connection connect(std::weak_ptr<Receiver> weakReceiver, Method method) {
        return connect([weakReceiver, method](Args... args) {
            if (auto r = weakReceiver.lock()) {
                std::invoke(method, r.get(), std::forward<Args>(args)...);
            }
        });
    }

    void emit(Args... args) const {
        std::shared_ptr<const SlotList> snapshot;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            snapshot = slots_;
        }

        for (const auto& slot : *snapshot) {
            if (slot.state && slot.state->isConnected()) {
                slot.callback(args...);
            }
        }
    }

    void operator()(Args... args) const { emit(std::forward<Args>(args)...); }

    void disconnect(ConnectionId id) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (slots_->empty()) {
            return;
        }

        auto it = std::find_if(slots_->begin(), slots_->end(),
                               [id](const Slot& s) { return s.id == id; });

        if (it != slots_->end()) {
            auto newList = std::make_shared<SlotList>();
            newList->reserve(slots_->size() - 1);
            for (const auto& s : *slots_) {
                if (s.id != id) {
                    newList->push_back(s);
                }
            }
            slots_ = std::move(newList);
        }
    }

    void disconnectAll() noexcept {
        std::shared_ptr<const SlotList> oldSlots;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            oldSlots = std::move(slots_);
            slots_ = std::make_shared<const SlotList>();
        }

        if (oldSlots) {
            for (const auto& slot : *oldSlots) {
                if (slot.state) {
                    slot.state->detachSignal();
                }
            }
        }
    }

    [[nodiscard]] size_t slotCount() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return slots_->size();
    }

    [[nodiscard]] bool hasSlots() const noexcept { return slotCount() > 0; }

private:
    mutable std::mutex mutex_;
    std::shared_ptr<const SlotList> slots_;
};

} // namespace northern::core

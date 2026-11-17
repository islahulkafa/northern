#include "core/signal/signal.hpp"

namespace northern::core::detail {

ConnectionId generateNextConnectionId() noexcept {
    static std::atomic<ConnectionId> s_counter{1};
    return s_counter.fetch_add(1, std::memory_order_relaxed);
}

} // namespace northern::core::detail

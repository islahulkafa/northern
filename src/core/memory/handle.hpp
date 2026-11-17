#pragma once

#include <compare>
#include <cstdint>
#include <functional>

namespace northern::core {

template <typename Tag>
struct Handle {
    uint32_t index{0};
    uint32_t generation{0};

    constexpr Handle() noexcept = default;
    constexpr Handle(uint32_t idx, uint32_t gen) noexcept : index(idx), generation(gen) {}

    [[nodiscard]] constexpr bool isValid() const noexcept { return generation != 0; }

    [[nodiscard]] explicit constexpr operator bool() const noexcept { return isValid(); }

    constexpr auto operator<=>(const Handle&) const noexcept = default;

    static constexpr Handle invalid() noexcept { return Handle{0, 0}; }
};

} // namespace northern::core

template <typename Tag>
struct std::hash<northern::core::Handle<Tag>> {
    std::size_t operator()(const northern::core::Handle<Tag>& h) const noexcept {
        const uint64_t combined = (static_cast<uint64_t>(h.generation) << 32) | h.index;
        return std::hash<uint64_t>{}(combined);
    }
};

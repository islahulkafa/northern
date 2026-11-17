#pragma once

#include "core/error/error.hpp"
#include "core/error/result.hpp"
#include <string>
#include <string_view>

namespace northern::platform {

class Clipboard {
public:
    static core::Result<void, core::Error> setText(std::string_view text);
    static core::Result<std::string, core::Error> getText();
    [[nodiscard]] static bool hasText() noexcept;
};

} // namespace northern::platform

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace northern::core {

enum class ErrorCode : uint32_t {
    Ok = 0,
    InvalidArgument,
    InvalidState,
    NotFound,
    AlreadyExists,
    OutOfMemory,
    OutOfBounds,
    NotSupported,
    Timeout,
    IOError,
    ParseError,
    PlatformError,
    GraphicsError,
    InternalError
};

const char* errorCodeToString(ErrorCode code) noexcept;

struct Error {
    ErrorCode code = ErrorCode::Ok;
    std::string message;
    std::string_view file;
    uint32_t line = 0;
    std::shared_ptr<Error> cause;

    Error() = default;

    Error(ErrorCode c, std::string msg, std::string_view f = "", uint32_t l = 0,
          std::shared_ptr<Error> nested = nullptr)
        : code(c), message(std::move(msg)), file(f), line(l), cause(std::move(nested)) {}

    [[nodiscard]] std::string toString() const;
    [[nodiscard]] explicit operator bool() const noexcept { return code != ErrorCode::Ok; }
};

#define NORTHERN_MAKE_ERROR(code, message)                                                         \
    ::northern::core::Error(code, (message), __FILE__, static_cast<uint32_t>(__LINE__))

#define NORTHERN_MAKE_ERROR_WITH_CAUSE(code, message, causePtr)                                    \
    ::northern::core::Error(code, (message), __FILE__, static_cast<uint32_t>(__LINE__), (causePtr))

} // namespace northern::core

namespace nrth {
using namespace northern::core;
}

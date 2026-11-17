#include "core/error/error.hpp"
#include <sstream>

namespace northern::core {

const char* errorCodeToString(ErrorCode code) noexcept {
    switch (code) {
    case ErrorCode::Ok:
        return "Ok";
    case ErrorCode::InvalidArgument:
        return "InvalidArgument";
    case ErrorCode::InvalidState:
        return "InvalidState";
    case ErrorCode::NotFound:
        return "NotFound";
    case ErrorCode::AlreadyExists:
        return "AlreadyExists";
    case ErrorCode::OutOfMemory:
        return "OutOfMemory";
    case ErrorCode::OutOfBounds:
        return "OutOfBounds";
    case ErrorCode::NotSupported:
        return "NotSupported";
    case ErrorCode::Timeout:
        return "Timeout";
    case ErrorCode::IOError:
        return "IOError";
    case ErrorCode::ParseError:
        return "ParseError";
    case ErrorCode::PlatformError:
        return "PlatformError";
    case ErrorCode::GraphicsError:
        return "GraphicsError";
    case ErrorCode::InternalError:
        return "InternalError";
    }
    return "UnknownError";
}

std::string Error::toString() const {
    std::ostringstream ss;
    ss << "[" << errorCodeToString(code) << "]";
    if (!file.empty()) {
        ss << " " << file << ":" << line;
    }
    if (!message.empty()) {
        ss << ": " << message;
    }
    if (cause) {
        ss << "\n  Caused by: " << cause->toString();
    }
    return ss.str();
}

} // namespace northern::core

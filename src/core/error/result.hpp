#pragma once

#include "core/error/error.hpp"
#include <concepts>
#include <functional>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

namespace northern::core {

template <typename T, typename E = Error>
class Result {
public:
    using ValueType = T;
    using ErrorType = E;

    Result(const T& val) : storage_(val) {}
    Result(T&& val) : storage_(std::move(val)) {}

    Result(const E& err) : storage_(err) {}
    Result(E&& err) : storage_(std::move(err)) {}

    template <typename U>
        requires std::is_constructible_v<T, U> && (!std::is_same_v<std::decay_t<U>, Result>)
    Result(U&& val) : storage_(std::in_place_type<T>, std::forward<U>(val)) {}

    [[nodiscard]] bool isOk() const noexcept { return std::holds_alternative<T>(storage_); }

    [[nodiscard]] bool isErr() const noexcept { return std::holds_alternative<E>(storage_); }

    [[nodiscard]] explicit operator bool() const noexcept { return isOk(); }

    [[nodiscard]] T& value() & {
        if (isErr()) {
            throw std::runtime_error("Attempted to access value of an Err Result");
        }
        return std::get<T>(storage_);
    }

    [[nodiscard]] const T& value() const& {
        if (isErr()) {
            throw std::runtime_error("Attempted to access value of an Err Result");
        }
        return std::get<T>(storage_);
    }

    [[nodiscard]] T&& value() && {
        if (isErr()) {
            throw std::runtime_error("Attempted to access value of an Err Result");
        }
        return std::get<T>(std::move(storage_));
    }

    [[nodiscard]] E& error() & {
        if (isOk()) {
            throw std::runtime_error("Attempted to access error of an Ok Result");
        }
        return std::get<E>(storage_);
    }

    [[nodiscard]] const E& error() const& {
        if (isOk()) {
            throw std::runtime_error("Attempted to access error of an Ok Result");
        }
        return std::get<E>(storage_);
    }

    [[nodiscard]] E&& error() && {
        if (isOk()) {
            throw std::runtime_error("Attempted to access error of an Ok Result");
        }
        return std::get<E>(std::move(storage_));
    }

    template <typename U>
    [[nodiscard]] T valueOr(U&& defaultValue) const& {
        if (isOk()) {
            return std::get<T>(storage_);
        }
        return static_cast<T>(std::forward<U>(defaultValue));
    }

    template <typename U>
    [[nodiscard]] T valueOr(U&& defaultValue) && {
        if (isOk()) {
            return std::get<T>(std::move(storage_));
        }
        return static_cast<T>(std::forward<U>(defaultValue));
    }

    template <typename F>
    auto map(F&& func) const& -> Result<std::invoke_result_t<F, const T&>, E> {
        using ReturnType = std::invoke_result_t<F, const T&>;
        if (isOk()) {
            return Result<ReturnType, E>(std::invoke(std::forward<F>(func), std::get<T>(storage_)));
        }
        return Result<ReturnType, E>(std::get<E>(storage_));
    }

    template <typename F>
    auto map(F&& func) && -> Result<std::invoke_result_t<F, T&&>, E> {
        using ReturnType = std::invoke_result_t<F, T&&>;
        if (isOk()) {
            return Result<ReturnType, E>(
                std::invoke(std::forward<F>(func), std::get<T>(std::move(storage_))));
        }
        return Result<ReturnType, E>(std::get<E>(std::move(storage_)));
    }

    template <typename F>
    auto flatMap(F&& func) const& {
        using ResultU = std::invoke_result_t<F, const T&>;
        if (isOk()) {
            return std::invoke(std::forward<F>(func), std::get<T>(storage_));
        }
        return ResultU(std::get<E>(storage_));
    }

    template <typename F>
    auto flatMap(F&& func) && {
        using ResultU = std::invoke_result_t<F, T&&>;
        if (isOk()) {
            return std::invoke(std::forward<F>(func), std::get<T>(std::move(storage_)));
        }
        return ResultU(std::get<E>(std::move(storage_)));
    }

    template <typename F>
    Result<T, std::invoke_result_t<F, const E&>> mapError(F&& func) const& {
        using NewError = std::invoke_result_t<F, const E&>;
        if (isErr()) {
            return Result<T, NewError>(std::invoke(std::forward<F>(func), std::get<E>(storage_)));
        }
        return Result<T, NewError>(std::get<T>(storage_));
    }

private:
    std::variant<T, E> storage_;
};

template <typename E>
class Result<void, E> {
public:
    using ValueType = void;
    using ErrorType = E;

    Result() : error_(std::nullopt) {}

    Result(const E& err) : error_(err) {}
    Result(E&& err) : error_(std::move(err)) {}

    [[nodiscard]] bool isOk() const noexcept { return !error_.has_value(); }

    [[nodiscard]] bool isErr() const noexcept { return error_.has_value(); }

    [[nodiscard]] explicit operator bool() const noexcept { return isOk(); }

    void value() const {
        if (isErr()) {
            throw std::runtime_error("Attempted to access value of an Err Result<void>");
        }
    }

    [[nodiscard]] E& error() & {
        if (isOk()) {
            throw std::runtime_error("Attempted to access error of an Ok Result<void>");
        }
        return *error_;
    }

    [[nodiscard]] const E& error() const& {
        if (isOk()) {
            throw std::runtime_error("Attempted to access error of an Ok Result<void>");
        }
        return *error_;
    }

    [[nodiscard]] E&& error() && {
        if (isOk()) {
            throw std::runtime_error("Attempted to access error of an Ok Result<void>");
        }
        return std::move(*error_);
    }

    static Result<void, E> ok() noexcept { return Result<void, E>(); }

private:
    std::optional<E> error_;
};

} // namespace northern::core

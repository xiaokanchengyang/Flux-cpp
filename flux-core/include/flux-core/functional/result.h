#pragma once

#include <expected>
#include <string>
#include <functional>
#include <optional>
#include <variant>

namespace Flux::Functional {

/**
 * @brief Modern C++23 Result type using std::expected
 * Provides functional error handling without exceptions
 */
template<typename T>
using Result = std::expected<T, std::string>;

/**
 * @brief Void result for operations that don't return values
 */
using VoidResult = Result<void>;

/**
 * @brief Error type for more detailed error information
 */
struct Error {
    std::string message;
    std::string code;
    std::optional<std::string> details;
    
    Error(std::string msg, std::string error_code = "", std::optional<std::string> detail = std::nullopt)
        : message(std::move(msg)), code(std::move(error_code)), details(std::move(detail)) {}
};

/**
 * @brief Enhanced Result type with structured error information
 */
template<typename T>
using DetailedResult = std::expected<T, Error>;

/**
 * @brief Functional utilities for Result types
 */
namespace ResultUtils {

/**
 * @brief Create a successful result
 */
template<typename T>
constexpr auto Ok(T&& value) -> Result<std::decay_t<T>> {
    return Result<std::decay_t<T>>{std::forward<T>(value)};
}

/**
 * @brief Create an error result
 */
template<typename T>
constexpr auto Err(std::string message) -> Result<T> {
    return std::unexpected(std::move(message));
}

/**
 * @brief Create a detailed error result
 */
template<typename T>
constexpr auto DetailedErr(std::string message, std::string code = "", std::optional<std::string> details = std::nullopt) -> DetailedResult<T> {
    return std::unexpected(Error{std::move(message), std::move(code), std::move(details)});
}

/**
 * @brief Map function for Result types (functor)
 */
template<typename T, typename F>
constexpr auto map(const Result<T>& result, F&& func) -> Result<std::invoke_result_t<F, T>> {
    if (result.has_value()) {
        return Ok(std::invoke(std::forward<F>(func), result.value()));
    }
    return std::unexpected(result.error());
}

/**
 * @brief FlatMap function for Result types (monad)
 */
template<typename T, typename F>
constexpr auto flatMap(const Result<T>& result, F&& func) -> std::invoke_result_t<F, T> {
    if (result.has_value()) {
        return std::invoke(std::forward<F>(func), result.value());
    }
    using ReturnType = std::invoke_result_t<F, T>;
    return ReturnType{std::unexpected(result.error())};
}

/**
 * @brief Apply function if result is successful, otherwise return error
 */
template<typename T, typename F>
constexpr auto andThen(const Result<T>& result, F&& func) -> std::invoke_result_t<F, T> {
    return flatMap(result, std::forward<F>(func));
}

/**
 * @brief Provide default value if result is error
 */
template<typename T>
constexpr auto orElse(const Result<T>& result, T&& defaultValue) -> T {
    return result.value_or(std::forward<T>(defaultValue));
}

/**
 * @brief Chain multiple operations that return Results
 */
template<typename T, typename... Funcs>
constexpr auto chain(Result<T> initial, Funcs&&... funcs) {
    return (flatMap(initial, std::forward<Funcs>(funcs)), ...);
}

} // namespace ResultUtils

} // namespace Flux::Functional

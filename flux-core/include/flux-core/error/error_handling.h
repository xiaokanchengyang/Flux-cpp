#pragma once

#include "../functional/result.h"
#include <source_location>
#include <stacktrace>
#include <format>
#include <chrono>

namespace Flux::Error {

/**
 * @brief Error severity levels
 */
enum class Severity {
    Info,
    Warning,
    Error,
    Critical
};

/**
 * @brief Error categories for better error classification
 */
enum class Category {
    FileSystem,
    Archive,
    Network,
    Memory,
    Configuration,
    User,
    System
};

/**
 * @brief Comprehensive error information
 */
struct ErrorInfo {
    std::string message;
    Category category;
    Severity severity;
    std::string errorCode;
    std::source_location location;
    std::chrono::system_clock::time_point timestamp;
    std::optional<std::stacktrace> stackTrace;
    std::unordered_map<std::string, std::string> context;
    
    ErrorInfo(std::string msg, 
              Category cat = Category::System,
              Severity sev = Severity::Error,
              std::string code = "",
              std::source_location loc = std::source_location::current())
        : message(std::move(msg))
        , category(cat)
        , severity(sev)
        , errorCode(std::move(code))
        , location(loc)
        , timestamp(std::chrono::system_clock::now()) {
        
        // Capture stack trace for critical errors
        if (severity == Severity::Critical) {
            stackTrace = std::stacktrace::current();
        }
    }
    
    /**
     * @brief Add context information to error
     */
    ErrorInfo& addContext(const std::string& key, const std::string& value) {
        context[key] = value;
        return *this;
    }
    
    /**
     * @brief Format error for display
     */
    std::string format() const {
        return std::format("[{}] {}: {} ({}:{})", 
                          severityToString(severity),
                          categoryToString(category),
                          message,
                          location.file_name(),
                          location.line());
    }

private:
    static std::string severityToString(Severity sev) {
        switch (sev) {
            case Severity::Info: return "INFO";
            case Severity::Warning: return "WARN";
            case Severity::Error: return "ERROR";
            case Severity::Critical: return "CRITICAL";
        }
        return "UNKNOWN";
    }
    
    static std::string categoryToString(Category cat) {
        switch (cat) {
            case Category::FileSystem: return "FS";
            case Category::Archive: return "ARCHIVE";
            case Category::Network: return "NET";
            case Category::Memory: return "MEM";
            case Category::Configuration: return "CONFIG";
            case Category::User: return "USER";
            case Category::System: return "SYS";
        }
        return "UNKNOWN";
    }
};

/**
 * @brief Enhanced Result type with ErrorInfo
 */
template<typename T>
using ErrorResult = std::expected<T, ErrorInfo>;

/**
 * @brief Error handling utilities
 */
namespace ErrorUtils {

/**
 * @brief Create error result with context
 */
template<typename T>
constexpr auto makeError(std::string message, 
                        Category category = Category::System,
                        Severity severity = Severity::Error,
                        std::string code = "",
                        std::source_location location = std::source_location::current()) -> ErrorResult<T> {
    return std::unexpected(ErrorInfo{std::move(message), category, severity, std::move(code), location});
}

/**
 * @brief Create filesystem error
 */
template<typename T>
constexpr auto fsError(std::string message, 
                      std::string path = "",
                      std::source_location location = std::source_location::current()) -> ErrorResult<T> {
    auto error = ErrorInfo{std::move(message), Category::FileSystem, Severity::Error, "FS_ERROR", location};
    if (!path.empty()) {
        error.addContext("path", path);
    }
    return std::unexpected(std::move(error));
}

/**
 * @brief Create archive error
 */
template<typename T>
constexpr auto archiveError(std::string message,
                           std::string archivePath = "",
                           std::source_location location = std::source_location::current()) -> ErrorResult<T> {
    auto error = ErrorInfo{std::move(message), Category::Archive, Severity::Error, "ARCHIVE_ERROR", location};
    if (!archivePath.empty()) {
        error.addContext("archive", archivePath);
    }
    return std::unexpected(std::move(error));
}

/**
 * @brief Convert simple Result to ErrorResult
 */
template<typename T>
constexpr auto fromSimpleResult(const Functional::Result<T>& result,
                               Category category = Category::System,
                               std::source_location location = std::source_location::current()) -> ErrorResult<T> {
    if (result.has_value()) {
        return result.value();
    }
    return makeError<T>(result.error(), category, Severity::Error, "", location);
}

/**
 * @brief Try-catch wrapper that converts exceptions to ErrorResult
 */
template<typename F>
auto tryExecute(F&& func, 
               std::source_location location = std::source_location::current()) -> ErrorResult<std::invoke_result_t<F>> {
    try {
        if constexpr (std::is_void_v<std::invoke_result_t<F>>) {
            std::invoke(std::forward<F>(func));
            return {};
        } else {
            return std::invoke(std::forward<F>(func));
        }
    } catch (const std::filesystem::filesystem_error& e) {
        return fsError<std::invoke_result_t<F>>(e.what(), e.path1().string(), location);
    } catch (const std::bad_alloc& e) {
        return makeError<std::invoke_result_t<F>>(e.what(), Category::Memory, Severity::Critical, "OUT_OF_MEMORY", location);
    } catch (const std::exception& e) {
        return makeError<std::invoke_result_t<F>>(e.what(), Category::System, Severity::Error, "EXCEPTION", location);
    } catch (...) {
        return makeError<std::invoke_result_t<F>>("Unknown exception occurred", Category::System, Severity::Critical, "UNKNOWN_EXCEPTION", location);
    }
}

/**
 * @brief Chain operations with error propagation
 */
template<typename T, typename F>
constexpr auto andThen(const ErrorResult<T>& result, F&& func) -> std::invoke_result_t<F, T> {
    if (result.has_value()) {
        return std::invoke(std::forward<F>(func), result.value());
    }
    using ReturnType = std::invoke_result_t<F, T>;
    return ReturnType{std::unexpected(result.error())};
}

/**
 * @brief Map function for ErrorResult
 */
template<typename T, typename F>
constexpr auto map(const ErrorResult<T>& result, F&& func) -> ErrorResult<std::invoke_result_t<F, T>> {
    if (result.has_value()) {
        return std::invoke(std::forward<F>(func), result.value());
    }
    return std::unexpected(result.error());
}

/**
 * @brief Provide default value on error
 */
template<typename T>
constexpr auto orElse(const ErrorResult<T>& result, T&& defaultValue) -> T {
    return result.value_or(std::forward<T>(defaultValue));
}

/**
 * @brief Log error and return default value
 */
template<typename T>
T logAndDefault(const ErrorResult<T>& result, T&& defaultValue, const std::string& context = "") {
    if (!result.has_value()) {
        // Log error (implementation depends on logging framework)
        // spdlog::error("{}: {}", context, result.error().format());
    }
    return result.value_or(std::forward<T>(defaultValue));
}

} // namespace ErrorUtils

} // namespace Flux::Error

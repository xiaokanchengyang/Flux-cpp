#pragma once

/**
 * Compatibility header for C++23 features
 * Provides fallbacks for compilers that don't fully support C++23
 */

#include <version>

// Check if std::expected is available
#if __cpp_lib_expected >= 202202L
    #include <expected>
    namespace Flux {
        template<typename T, typename E>
        using expected = std::expected<T, E>;
        
        template<typename E>
        using unexpected = std::unexpected<E>;
    }
#else
    // Fallback implementation for compilers without std::expected
    #include <variant>
    #include <stdexcept>
    
    namespace Flux {
        template<typename E>
        class unexpected {
        public:
            constexpr explicit unexpected(const E& e) : error_(e) {}
            constexpr explicit unexpected(E&& e) : error_(std::move(e)) {}
            
            constexpr const E& error() const& { return error_; }
            constexpr E& error() & { return error_; }
            constexpr const E&& error() const&& { return std::move(error_); }
            constexpr E&& error() && { return std::move(error_); }
            
        private:
            E error_;
        };
        
        template<typename T, typename E>
        class expected {
        public:
            constexpr expected() = default;
            constexpr expected(const T& value) : storage_(value) {}
            constexpr expected(T&& value) : storage_(std::move(value)) {}
            constexpr expected(const unexpected<E>& unexp) : storage_(unexp.error()) {}
            constexpr expected(unexpected<E>&& unexp) : storage_(std::move(unexp.error())) {}
            
            constexpr bool has_value() const noexcept {
                return std::holds_alternative<T>(storage_);
            }
            
            constexpr explicit operator bool() const noexcept {
                return has_value();
            }
            
            constexpr const T& value() const& {
                if (!has_value()) {
                    throw std::runtime_error("Expected has no value");
                }
                return std::get<T>(storage_);
            }
            
            constexpr T& value() & {
                if (!has_value()) {
                    throw std::runtime_error("Expected has no value");
                }
                return std::get<T>(storage_);
            }
            
            constexpr const T&& value() const&& {
                if (!has_value()) {
                    throw std::runtime_error("Expected has no value");
                }
                return std::move(std::get<T>(storage_));
            }
            
            constexpr T&& value() && {
                if (!has_value()) {
                    throw std::runtime_error("Expected has no value");
                }
                return std::move(std::get<T>(storage_));
            }
            
            constexpr const E& error() const& {
                return std::get<E>(storage_);
            }
            
            constexpr E& error() & {
                return std::get<E>(storage_);
            }
            
            constexpr const E&& error() const&& {
                return std::move(std::get<E>(storage_));
            }
            
            constexpr E&& error() && {
                return std::move(std::get<E>(storage_));
            }
            
            constexpr const T* operator->() const {
                return &value();
            }
            
            constexpr T* operator->() {
                return &value();
            }
            
            constexpr const T& operator*() const& {
                return value();
            }
            
            constexpr T& operator*() & {
                return value();
            }
            
            constexpr const T&& operator*() const&& {
                return std::move(value());
            }
            
            constexpr T&& operator*() && {
                return std::move(value());
            }
            
        private:
            std::variant<T, E> storage_;
        };
        
        // Specialization for void
        template<typename E>
        class expected<void, E> {
        public:
            constexpr expected() = default;
            constexpr expected(const unexpected<E>& unexp) : error_(unexp.error()), has_error_(true) {}
            constexpr expected(unexpected<E>&& unexp) : error_(std::move(unexp.error())), has_error_(true) {}
            
            constexpr bool has_value() const noexcept {
                return !has_error_;
            }
            
            constexpr explicit operator bool() const noexcept {
                return has_value();
            }
            
            constexpr void value() const {
                if (!has_value()) {
                    throw std::runtime_error("Expected has no value");
                }
            }
            
            constexpr const E& error() const& {
                return error_;
            }
            
            constexpr E& error() & {
                return error_;
            }
            
            constexpr const E&& error() const&& {
                return std::move(error_);
            }
            
            constexpr E&& error() && {
                return std::move(error_);
            }
            
        private:
            E error_;
            bool has_error_{false};
        };
    }
#endif

// Convenience macros for creating expected values
#define FLUX_EXPECTED_SUCCESS() Flux::expected<void, std::string>{}
#define FLUX_EXPECTED_ERROR(msg) Flux::expected<void, std::string>{Flux::unexpected<std::string>{msg}}



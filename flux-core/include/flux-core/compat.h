#pragma once

// Compatibility layer for C++23 features
#include <version>

// Check for std::expected support
#if __cpp_lib_expected >= 202202L
    #include <expected>
    namespace Flux {
        template<typename T, typename E>
        using expected = std::expected<T, E>;
        
        template<typename E>
        using unexpected = std::unexpected<E>;
    }
#else
    // Fallback implementation for older compilers
    #include <variant>
    #include <stdexcept>
    
    namespace Flux {
        template<typename E>
        class unexpected {
        public:
            explicit unexpected(E&& error) : error_(std::move(error)) {}
            explicit unexpected(const E& error) : error_(error) {}
            
            const E& error() const& { return error_; }
            E& error() & { return error_; }
            E&& error() && { return std::move(error_); }
            
        private:
            E error_;
        };
        
        template<typename T, typename E>
        class expected {
        public:
            expected(T&& value) : data_(std::move(value)) {}
            expected(const T& value) : data_(value) {}
            expected(unexpected<E>&& err) : data_(std::move(err.error())) {}
            expected(const unexpected<E>& err) : data_(err.error()) {}
            
            bool has_value() const { return std::holds_alternative<T>(data_); }
            operator bool() const { return has_value(); }
            
            const T& value() const& { 
                if (!has_value()) throw std::runtime_error("Expected has no value");
                return std::get<T>(data_); 
            }
            
            T& value() & { 
                if (!has_value()) throw std::runtime_error("Expected has no value");
                return std::get<T>(data_); 
            }
            
            T&& value() && { 
                if (!has_value()) throw std::runtime_error("Expected has no value");
                return std::get<T>(std::move(data_)); 
            }
            
            const E& error() const& { 
                if (has_value()) throw std::runtime_error("Expected has value, not error");
                return std::get<E>(data_); 
            }
            
        private:
            std::variant<T, E> data_;
        };
    }
#endif

// Check for std::ranges::fold_left support
#if __cpp_lib_ranges_fold >= 202207L
    #include <algorithm>
    namespace Flux {
        using std::ranges::fold_left;
    }
#else
    // Fallback implementation
    #include <numeric>
    namespace Flux {
        template<typename Range, typename T, typename BinaryOp>
        constexpr auto fold_left(Range&& range, T init, BinaryOp op) {
            return std::accumulate(std::begin(range), std::end(range), init, op);
        }
    }
#endif

// Check for std::format support
#if __cpp_lib_format >= 201907L
    #include <format>
    namespace Flux {
        using std::format;
    }
#else
    // Fallback to fmt library or simple string concatenation
    #include <sstream>
    #include <string>
    
    namespace Flux {
        template<typename... Args>
        std::string format(const std::string& fmt, Args&&... args) {
            // Simple fallback - in production, use fmt library
            std::ostringstream oss;
            oss << fmt;
            return oss.str();
        }
    }
#endif
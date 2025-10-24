#pragma once

#include "result.h"
#include <algorithm>
#include <ranges>
#include <functional>
#include <vector>
#include <unordered_map>
#include <string>

namespace Flux::Functional {

/**
 * @brief Operation type enumeration for functional dispatch
 */
enum class OperationType {
    Extract,
    Pack,
    Convert,
    List,
    Verify,
    Unknown
};

/**
 * @brief Operation context containing input parameters
 */
struct OperationContext {
    std::vector<std::filesystem::path> inputs;
    std::filesystem::path output;
    std::unordered_map<std::string, std::string> options;
    
    template<typename T>
    std::optional<T> getOption(const std::string& key) const;
};

/**
 * @brief Function type for operation handlers
 */
using OperationHandler = std::function<Result<void>(const OperationContext&)>;

/**
 * @brief Operation registry for functional dispatch
 */
class OperationRegistry {
public:
    /**
     * @brief Register an operation handler
     */
    void registerOperation(OperationType type, OperationHandler handler) {
        handlers_[type] = std::move(handler);
    }
    
    /**
     * @brief Execute operation using functional dispatch
     */
    Result<void> execute(OperationType type, const OperationContext& context) const {
        auto it = handlers_.find(type);
        if (it == handlers_.end()) {
            return ResultUtils::Err<void>("Unknown operation type");
        }
        return it->second(context);
    }
    
    /**
     * @brief Check if operation is supported
     */
    bool isSupported(OperationType type) const {
        return handlers_.contains(type);
    }

private:
    std::unordered_map<OperationType, OperationHandler> handlers_;
};

/**
 * @brief Functional utilities for operations
 */
namespace OperationUtils {

/**
 * @brief Detect operation type from inputs using functional approach
 */
inline Result<OperationType> detectOperation(const std::vector<std::filesystem::path>& inputs, 
                                           const std::filesystem::path& output) {
    using namespace std::ranges;
    
    // Count different input types using ranges
    auto archiveCount = count_if(inputs, [](const auto& path) {
        return std::filesystem::is_regular_file(path) && isArchiveFile(path);
    });
    
    auto directoryCount = count_if(inputs, [](const auto& path) {
        return std::filesystem::is_directory(path);
    });
    
    auto regularFileCount = inputs.size() - archiveCount - directoryCount;
    
    // Functional decision making using pattern matching
    return std::visit([&](auto&&... args) -> Result<OperationType> {
        // Only archives -> Extract
        if (archiveCount > 0 && directoryCount == 0 && regularFileCount == 0) {
            return ResultUtils::Ok(OperationType::Extract);
        }
        // Only files/directories -> Pack
        if (archiveCount == 0 && (directoryCount > 0 || regularFileCount > 0)) {
            return ResultUtils::Ok(OperationType::Pack);
        }
        // Archives with archive output -> Convert
        if (archiveCount > 0 && !output.empty() && isArchiveFile(output)) {
            return ResultUtils::Ok(OperationType::Convert);
        }
        // Single archive without output -> List or Extract
        if (archiveCount == 1 && output.empty()) {
            return ResultUtils::Ok(OperationType::List);
        }
        
        return ResultUtils::Err<OperationType>("Cannot determine operation type");
    }, std::make_tuple());
}

/**
 * @brief Validate inputs using functional composition
 */
inline Result<void> validateInputs(const std::vector<std::filesystem::path>& inputs) {
    using namespace std::ranges;
    
    if (inputs.empty()) {
        return ResultUtils::Err<void>("No inputs provided");
    }
    
    // Check if all inputs exist using ranges algorithms
    auto nonExistentInputs = inputs | views::filter([](const auto& path) {
        return !std::filesystem::exists(path);
    });
    
    if (!nonExistentInputs.empty()) {
        auto firstNonExistent = *nonExistentInputs.begin();
        return ResultUtils::Err<void>("Input does not exist: " + firstNonExistent.string());
    }
    
    return ResultUtils::Ok();
}

/**
 * @brief Create operation context with validation
 */
inline Result<OperationContext> createContext(std::vector<std::filesystem::path> inputs,
                                            std::filesystem::path output,
                                            std::unordered_map<std::string, std::string> options = {}) {
    return validateInputs(inputs)
        .and_then([&](auto) -> Result<OperationContext> {
            return ResultUtils::Ok(OperationContext{
                .inputs = std::move(inputs),
                .output = std::move(output),
                .options = std::move(options)
            });
        });
}

/**
 * @brief Execute operation with full functional pipeline
 */
inline Result<void> executeOperation(const std::vector<std::filesystem::path>& inputs,
                                   const std::filesystem::path& output,
                                   const OperationRegistry& registry,
                                   const std::unordered_map<std::string, std::string>& options = {}) {
    return createContext(inputs, output, options)
        .and_then([&](const OperationContext& context) {
            return detectOperation(context.inputs, context.output)
                .and_then([&](OperationType type) {
                    return registry.execute(type, context);
                });
        });
}

} // namespace OperationUtils

/**
 * @brief Helper function to check if file is an archive
 */
inline bool isArchiveFile(const std::filesystem::path& path) {
    static const std::unordered_set<std::string> archiveExtensions = {
        ".zip", ".tar", ".gz", ".xz", ".bz2", ".7z", ".rar"
    };
    
    auto extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    return archiveExtensions.contains(extension) ||
           (path.extension() == ".tar" && archiveExtensions.contains(path.stem().extension().string()));
}

} // namespace Flux::Functional

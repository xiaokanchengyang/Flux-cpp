#pragma once

#include "operation_detector.h"
#include <flux-core/functional/operations.h>
#include <flux-core/error/error_handling.h>
#include <unordered_map>
#include <functional>

namespace FluxCLI::Handlers {

/**
 * @brief Configuration for smart command execution
 */
struct SmartExecutionConfig {
    std::vector<std::filesystem::path> inputs;
    std::filesystem::path output;
    std::unordered_map<std::string, std::string> options;
    bool dryRun = false;
    bool verbose = false;
    bool quiet = false;
    
    /**
     * @brief Validate configuration
     */
    Flux::Error::ErrorResult<void> validate() const;
};

/**
 * @brief Result of smart execution
 */
struct SmartExecutionResult {
    Flux::Functional::OperationType detectedOperation;
    std::string operationDescription;
    std::vector<std::string> detectedFormats;
    bool success;
    std::string message;
    std::chrono::milliseconds executionTime;
};

/**
 * @brief Smart command executor using functional programming patterns
 */
class SmartExecutor {
public:
    using ExecutionResult = Flux::Error::ErrorResult<SmartExecutionResult>;
    using OperationStrategy = std::function<Flux::Error::ErrorResult<void>(const SmartExecutionConfig&)>;
    
    /**
     * @brief Constructor with operation registry
     */
    explicit SmartExecutor();
    
    /**
     * @brief Execute smart command with functional pipeline
     */
    ExecutionResult execute(const SmartExecutionConfig& config);
    
    /**
     * @brief Register custom operation strategy
     */
    void registerStrategy(Flux::Functional::OperationType operation, OperationStrategy strategy);

private:
    std::unordered_map<Flux::Functional::OperationType, OperationStrategy> strategies_;
    
    /**
     * @brief Initialize default strategies
     */
    void initializeDefaultStrategies();
    
    /**
     * @brief Analyze inputs and detect operation
     */
    Flux::Error::ErrorResult<Flux::Functional::OperationType> analyzeAndDetect(const SmartExecutionConfig& config);
    
    /**
     * @brief Execute detected operation
     */
    Flux::Error::ErrorResult<void> executeOperation(Flux::Functional::OperationType operation, 
                                                   const SmartExecutionConfig& config);
    
    /**
     * @brief Handle dry run mode
     */
    Flux::Error::ErrorResult<void> handleDryRun(Flux::Functional::OperationType operation,
                                               const SmartExecutionConfig& config);
    
    /**
     * @brief Log operation details
     */
    void logOperationDetails(Flux::Functional::OperationType operation,
                           const SmartExecutionConfig& config,
                           const std::vector<std::string>& detectedFormats);
};

/**
 * @brief Functional utilities for smart execution
 */
namespace SmartUtils {

/**
 * @brief Create execution pipeline
 */
template<typename... Steps>
auto createPipeline(Steps&&... steps) {
    return [steps...](const auto& input) {
        return (steps(input) && ...);
    };
}

/**
 * @brief Measure execution time
 */
template<typename F>
auto measureTime(F&& func) -> std::pair<std::invoke_result_t<F>, std::chrono::milliseconds> {
    auto start = std::chrono::high_resolution_clock::now();
    
    if constexpr (std::is_void_v<std::invoke_result_t<F>>) {
        std::invoke(std::forward<F>(func));
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        return {void{}, duration};
    } else {
        auto result = std::invoke(std::forward<F>(func));
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        return {std::move(result), duration};
    }
}

/**
 * @brief Conditional execution based on configuration
 */
template<typename F>
auto executeIf(bool condition, F&& func) -> Flux::Error::ErrorResult<void> {
    if (condition) {
        return Flux::Error::ErrorUtils::tryExecute(std::forward<F>(func));
    }
    return {};
}

/**
 * @brief Log with different levels based on configuration
 */
void logMessage(const std::string& message, 
               const SmartExecutionConfig& config,
               bool isError = false);

} // namespace SmartUtils

} // namespace FluxCLI::Handlers

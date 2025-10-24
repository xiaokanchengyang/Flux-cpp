#include "smart_executor.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <algorithm>

namespace FluxCLI::Handlers {

using namespace Flux::Functional;
using namespace Flux::Error;

auto SmartExecutionConfig::validate() const -> ErrorResult<void> {
    if (inputs.empty()) {
        return ErrorUtils::makeError<void>("No inputs provided", Category::User);
    }
    
    // Validate all inputs exist
    for (const auto& input : inputs) {
        if (!std::filesystem::exists(input)) {
            return ErrorUtils::fsError<void>("Input does not exist", input.string());
        }
    }
    
    return {};
}

SmartExecutor::SmartExecutor() {
    initializeDefaultStrategies();
}

auto SmartExecutor::execute(const SmartExecutionConfig& config) -> ExecutionResult {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Functional pipeline: validate -> detect -> execute
    return config.validate()
        .and_then([&](auto) { return analyzeAndDetect(config); })
        .and_then([&](OperationType operation) -> ErrorResult<SmartExecutionResult> {
            
            // Log operation details if not quiet
            if (!config.quiet) {
                auto analysis = OperationDetector::analyzeInputs(config.inputs);
                logOperationDetails(operation, config, analysis.detectedFormats);
            }
            
            // Handle dry run
            if (config.dryRun) {
                return handleDryRun(operation, config)
                    .transform([&](auto) {
                        auto endTime = std::chrono::high_resolution_clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
                        
                        return SmartExecutionResult{
                            .detectedOperation = operation,
                            .operationDescription = OperationDetector::getOperationDescription(operation),
                            .detectedFormats = {},
                            .success = true,
                            .message = "Dry run completed - no files were modified",
                            .executionTime = duration
                        };
                    });
            }
            
            // Execute actual operation
            return executeOperation(operation, config)
                .transform([&](auto) {
                    auto endTime = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
                    
                    return SmartExecutionResult{
                        .detectedOperation = operation,
                        .operationDescription = OperationDetector::getOperationDescription(operation),
                        .detectedFormats = {},
                        .success = true,
                        .message = "Operation completed successfully",
                        .executionTime = duration
                    };
                });
        });
}

void SmartExecutor::registerStrategy(OperationType operation, OperationStrategy strategy) {
    strategies_[operation] = std::move(strategy);
}

void SmartExecutor::initializeDefaultStrategies() {
    // Register default strategies using lambdas
    strategies_[OperationType::Extract] = [](const SmartExecutionConfig& config) -> ErrorResult<void> {
        return ExecutionStrategies::executeExtract(config.inputs, config.output, config.options);
    };
    
    strategies_[OperationType::Pack] = [](const SmartExecutionConfig& config) -> ErrorResult<void> {
        return ExecutionStrategies::executePack(config.inputs, config.output, config.options);
    };
    
    strategies_[OperationType::Convert] = [](const SmartExecutionConfig& config) -> ErrorResult<void> {
        return ExecutionStrategies::executeConvert(config.inputs, config.output, config.options);
    };
    
    strategies_[OperationType::List] = [](const SmartExecutionConfig& config) -> ErrorResult<void> {
        return ExecutionStrategies::executeList(config.inputs, config.options);
    };
}

auto SmartExecutor::analyzeAndDetect(const SmartExecutionConfig& config) -> ErrorResult<OperationType> {
    SmartUtils::logMessage("🔍 Analyzing inputs to determine best operation...", config);
    
    return OperationDetector::detect(config.inputs, config.output)
        .and_then([&](OperationType operation) -> ErrorResult<OperationType> {
            return OperationDetector::validate(operation, config.inputs, config.output)
                .transform([operation](auto) { return operation; });
        });
}

auto SmartExecutor::executeOperation(OperationType operation, const SmartExecutionConfig& config) -> ErrorResult<void> {
    auto it = strategies_.find(operation);
    if (it == strategies_.end()) {
        return ErrorUtils::makeError<void>("No strategy registered for operation", Category::System);
    }
    
    return it->second(config);
}

auto SmartExecutor::handleDryRun(OperationType operation, const SmartExecutionConfig& config) -> ErrorResult<void> {
    SmartUtils::logMessage("🔍 Dry run mode - showing what would be executed:", config);
    SmartUtils::logMessage("Operation: " + OperationDetector::getOperationDescription(operation), config);
    
    if (!config.output.empty()) {
        SmartUtils::logMessage("Output: " + config.output.string(), config);
    } else {
        auto recommendedOutput = OperationDetector::getRecommendedOutput(operation, config.inputs);
        if (!recommendedOutput.empty()) {
            SmartUtils::logMessage("Recommended output: " + recommendedOutput.string(), config);
        }
    }
    
    SmartUtils::logMessage("Inputs:", config);
    for (const auto& input : config.inputs) {
        SmartUtils::logMessage("  - " + input.string(), config);
    }
    
    return {};
}

void SmartExecutor::logOperationDetails(OperationType operation,
                                      const SmartExecutionConfig& config,
                                      const std::vector<std::string>& detectedFormats) {
    SmartUtils::logMessage("🎯 Detected operation: " + OperationDetector::getOperationDescription(operation), config);
    
    if (!detectedFormats.empty()) {
        std::string formatsStr = std::accumulate(
            detectedFormats.begin(), detectedFormats.end(), std::string{},
            [](const std::string& a, const std::string& b) {
                return a.empty() ? b : a + ", " + b;
            }
        );
        SmartUtils::logMessage("📋 Detected formats: " + formatsStr, config);
    }
}

namespace SmartUtils {

void logMessage(const std::string& message, const SmartExecutionConfig& config, bool isError) {
    if (config.quiet && !isError) {
        return;
    }
    
    if (isError) {
        spdlog::error(message);
    } else {
        spdlog::info(message);
    }
}

} // namespace SmartUtils

} // namespace FluxCLI::Handlers

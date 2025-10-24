#pragma once

#include <flux-core/functional/operations.h>
#include <flux-core/error/error_handling.h>
#include <filesystem>
#include <vector>
#include <unordered_map>
#include <string>

namespace FluxCLI::Handlers {

/**
 * @brief Smart operation detection using functional programming
 */
class OperationDetector {
public:
    using DetectionResult = Flux::Error::ErrorResult<Flux::Functional::OperationType>;
    using ValidationResult = Flux::Error::ErrorResult<void>;
    
    /**
     * @brief Detect operation type from inputs using functional approach
     */
    static DetectionResult detect(const std::vector<std::filesystem::path>& inputs,
                                 const std::filesystem::path& output = {});
    
    /**
     * @brief Validate inputs for detected operation
     */
    static ValidationResult validate(Flux::Functional::OperationType operation,
                                   const std::vector<std::filesystem::path>& inputs,
                                   const std::filesystem::path& output = {});
    
    /**
     * @brief Get operation description
     */
    static std::string getOperationDescription(Flux::Functional::OperationType operation);
    
    /**
     * @brief Get recommended output path for operation
     */
    static std::filesystem::path getRecommendedOutput(Flux::Functional::OperationType operation,
                                                     const std::vector<std::filesystem::path>& inputs);

private:
    /**
     * @brief Analyze input types using ranges
     */
    struct InputAnalysis {
        size_t archiveCount;
        size_t directoryCount;
        size_t regularFileCount;
        std::vector<std::string> detectedFormats;
    };
    
    static InputAnalysis analyzeInputs(const std::vector<std::filesystem::path>& inputs);
    
    /**
     * @brief Decision rules for operation detection
     */
    static DetectionResult applyDetectionRules(const InputAnalysis& analysis,
                                             const std::filesystem::path& output);
};

/**
 * @brief Operation execution strategies
 */
namespace ExecutionStrategies {

/**
 * @brief Strategy for extract operations
 */
Flux::Error::ErrorResult<void> executeExtract(const std::vector<std::filesystem::path>& inputs,
                                             const std::filesystem::path& output,
                                             const std::unordered_map<std::string, std::string>& options);

/**
 * @brief Strategy for pack operations
 */
Flux::Error::ErrorResult<void> executePack(const std::vector<std::filesystem::path>& inputs,
                                          const std::filesystem::path& output,
                                          const std::unordered_map<std::string, std::string>& options);

/**
 * @brief Strategy for convert operations
 */
Flux::Error::ErrorResult<void> executeConvert(const std::vector<std::filesystem::path>& inputs,
                                             const std::filesystem::path& output,
                                             const std::unordered_map<std::string, std::string>& options);

/**
 * @brief Strategy for list operations
 */
Flux::Error::ErrorResult<void> executeList(const std::vector<std::filesystem::path>& inputs,
                                          const std::unordered_map<std::string, std::string>& options);

} // namespace ExecutionStrategies

} // namespace FluxCLI::Handlers

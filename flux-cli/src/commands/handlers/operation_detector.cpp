#include "operation_detector.h"
#include <flux-core/utils/format_detector.h>
#include <algorithm>
#include <ranges>
#include <unordered_set>

namespace FluxCLI::Handlers {

using namespace Flux::Functional;
using namespace Flux::Error;

auto OperationDetector::detect(const std::vector<std::filesystem::path>& inputs,
                              const std::filesystem::path& output) -> DetectionResult {
    if (inputs.empty()) {
        return ErrorUtils::makeError<OperationType>("No inputs provided", Category::User);
    }
    
    // Analyze inputs using functional approach
    auto analysis = analyzeInputs(inputs);
    
    // Apply detection rules
    return applyDetectionRules(analysis, output);
}

auto OperationDetector::validate(OperationType operation,
                                const std::vector<std::filesystem::path>& inputs,
                                const std::filesystem::path& output) -> ValidationResult {
    using namespace std::ranges;
    
    // Check if all inputs exist
    auto nonExistentInputs = inputs | views::filter([](const auto& path) {
        return !std::filesystem::exists(path);
    });
    
    if (!nonExistentInputs.empty()) {
        auto firstNonExistent = *nonExistentInputs.begin();
        return ErrorUtils::fsError<void>("Input does not exist", firstNonExistent.string());
    }
    
    // Operation-specific validation
    switch (operation) {
        case OperationType::Extract: {
            // All inputs should be archives
            auto nonArchiveInputs = inputs | views::filter([](const auto& path) {
                return !isArchiveFile(path);
            });
            
            if (!nonArchiveInputs.empty()) {
                auto firstNonArchive = *nonArchiveInputs.begin();
                return ErrorUtils::archiveError<void>("Input is not an archive", firstNonArchive.string());
            }
            break;
        }
        
        case OperationType::Pack: {
            // At least one input should be a file or directory
            bool hasValidInput = any_of(inputs, [](const auto& path) {
                return std::filesystem::is_regular_file(path) || std::filesystem::is_directory(path);
            });
            
            if (!hasValidInput) {
                return ErrorUtils::makeError<void>("No valid files or directories to pack", Category::User);
            }
            break;
        }
        
        case OperationType::Convert: {
            // Should have archive inputs and archive output
            if (inputs.empty() || output.empty()) {
                return ErrorUtils::makeError<void>("Convert operation requires inputs and output", Category::User);
            }
            
            if (!isArchiveFile(output)) {
                return ErrorUtils::makeError<void>("Output must be an archive for convert operation", Category::User);
            }
            break;
        }
        
        case OperationType::List: {
            // Should have exactly one archive input
            if (inputs.size() != 1) {
                return ErrorUtils::makeError<void>("List operation requires exactly one archive", Category::User);
            }
            
            if (!isArchiveFile(inputs[0])) {
                return ErrorUtils::archiveError<void>("Input must be an archive for list operation", inputs[0].string());
            }
            break;
        }
        
        default:
            return ErrorUtils::makeError<void>("Unknown operation type", Category::System);
    }
    
    return {};
}

std::string OperationDetector::getOperationDescription(OperationType operation) {
    static const std::unordered_map<OperationType, std::string> descriptions = {
        {OperationType::Extract, "Extract archives to directory"},
        {OperationType::Pack, "Create archive from files/directories"},
        {OperationType::Convert, "Convert archive format"},
        {OperationType::List, "List archive contents"},
        {OperationType::Verify, "Verify archive integrity"},
        {OperationType::Unknown, "Unknown operation"}
    };
    
    auto it = descriptions.find(operation);
    return it != descriptions.end() ? it->second : "Unknown operation";
}

std::filesystem::path OperationDetector::getRecommendedOutput(OperationType operation,
                                                            const std::vector<std::filesystem::path>& inputs) {
    switch (operation) {
        case OperationType::Extract:
            return std::filesystem::current_path();
            
        case OperationType::Pack: {
            if (!inputs.empty()) {
                auto baseName = inputs[0].filename().stem();
                return std::filesystem::current_path() / (baseName.string() + ".zip");
            }
            return std::filesystem::current_path() / "archive.zip";
        }
        
        case OperationType::Convert: {
            if (!inputs.empty()) {
                auto baseName = inputs[0].filename().stem();
                return std::filesystem::current_path() / (baseName.string() + "_converted.zip");
            }
            return std::filesystem::current_path() / "converted.zip";
        }
        
        default:
            return {};
    }
}

auto OperationDetector::analyzeInputs(const std::vector<std::filesystem::path>& inputs) -> InputAnalysis {
    using namespace std::ranges;
    
    InputAnalysis analysis{};
    
    // Count different input types using ranges
    analysis.archiveCount = count_if(inputs, [](const auto& path) {
        return std::filesystem::is_regular_file(path) && isArchiveFile(path);
    });
    
    analysis.directoryCount = count_if(inputs, [](const auto& path) {
        return std::filesystem::is_directory(path);
    });
    
    analysis.regularFileCount = inputs.size() - analysis.archiveCount - analysis.directoryCount;
    
    // Detect formats for archive files
    for (const auto& input : inputs | views::filter([](const auto& path) { return isArchiveFile(path); })) {
        // This would use the actual format detector
        // auto format = FluxCore::Utils::FormatDetector::detect(input);
        // analysis.detectedFormats.push_back(format);
        
        // Placeholder implementation
        auto extension = input.extension().string();
        if (!extension.empty()) {
            analysis.detectedFormats.push_back(extension.substr(1)); // Remove the dot
        }
    }
    
    return analysis;
}

auto OperationDetector::applyDetectionRules(const InputAnalysis& analysis,
                                          const std::filesystem::path& output) -> DetectionResult {
    // Rule 1: Only archives -> Extract
    if (analysis.archiveCount > 0 && analysis.directoryCount == 0 && analysis.regularFileCount == 0) {
        return OperationType::Extract;
    }
    
    // Rule 2: Only files/directories -> Pack
    if (analysis.archiveCount == 0 && (analysis.directoryCount > 0 || analysis.regularFileCount > 0)) {
        return OperationType::Pack;
    }
    
    // Rule 3: Archives with archive output -> Convert
    if (analysis.archiveCount > 0 && !output.empty() && isArchiveFile(output)) {
        return OperationType::Convert;
    }
    
    // Rule 4: Single archive without output -> List
    if (analysis.archiveCount == 1 && output.empty() && 
        analysis.directoryCount == 0 && analysis.regularFileCount == 0) {
        return OperationType::List;
    }
    
    // Rule 5: Mixed inputs -> Unknown (requires user clarification)
    if (analysis.archiveCount > 0 && (analysis.directoryCount > 0 || analysis.regularFileCount > 0)) {
        return ErrorUtils::makeError<OperationType>(
            "Mixed input types detected. Please specify operation explicitly.", 
            Category::User
        );
    }
    
    return ErrorUtils::makeError<OperationType>("Cannot determine operation type", Category::User);
}

} // namespace FluxCLI::Handlers

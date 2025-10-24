#include "smart_command.h"
#include "handlers/smart_executor.h"
#include "../utils/format_utils.h"
#include "../utils/progress_bar.h"
#include <flux-core/functional/operations.h>
#include <flux-core/error/error_handling.h>
#include <spdlog/spdlog.h>
#include <iostream>
#include <algorithm>
#include <fstream>

namespace FluxCLI::Commands {

using namespace Flux::Error;
using namespace FluxCLI::Handlers;

void SmartConfig::validate() {
    // Convert to modern SmartExecutionConfig and validate
    SmartExecutionConfig config{
        .inputs = inputs,
        .output = output,
        .options = {
            {"password", password},
            {"recursive", recursive ? "true" : "false"},
            {"overwrite", overwrite ? "true" : "false"}
        },
        .dryRun = dry_run,
        .verbose = verbose,
        .quiet = quiet
    };
    
    auto result = config.validate();
    if (!result.has_value()) {
        throw std::invalid_argument(result.error().message);
    }
}

void setupSmartCommand(CLI::App* app, bool& verbose, bool& quiet) {
    static SmartConfig config;
    
    // Input files/directories (positional arguments)
    std::vector<std::string> input_strings;
    app->add_option("inputs", input_strings, "Input files or directories")
       ->required();
    
    // Optional output path
    std::string output_string;
    app->add_option("-o,--output", output_string, "Output path (auto-detected if not specified)");
    
    // Password
    app->add_option("-p,--password", config.password, "Archive password");
    
    // Recursive processing
    app->add_flag("-r,--recursive", config.recursive, "Process directories recursively");
    
    // Overwrite existing files
    app->add_flag("--overwrite", config.overwrite, "Overwrite existing files");
    
    // Dry run mode
    app->add_flag("--dry-run", config.dry_run, "Show what would be done without executing");
    
    // Command callback
    app->callback([&config, &input_strings, &output_string, &verbose, &quiet]() {
        config.verbose = verbose;
        config.quiet = quiet;
        
        if (!output_string.empty()) {
            config.output = output_string;
        }
        
        // Convert input strings to paths
        for (const auto& input_str : input_strings) {
            config.inputs.emplace_back(input_str);
        }
        
        try {
            config.validate();
            int result = executeSmartCommand(config);
            std::exit(result);
        } catch (const std::exception& e) {
            spdlog::error("Configuration error: {}", e.what());
            std::exit(1);
        }
    });
}

int executeSmartCommand(const SmartConfig& config) {
    // Convert to modern execution config
    SmartExecutionConfig execConfig{
        .inputs = config.inputs,
        .output = config.output,
        .options = {
            {"password", config.password},
            {"recursive", config.recursive ? "true" : "false"},
            {"overwrite", config.overwrite ? "true" : "false"}
        },
        .dryRun = config.dry_run,
        .verbose = config.verbose,
        .quiet = config.quiet
    };
    
    // Use modern functional executor
    SmartExecutor executor;
    auto result = executor.execute(execConfig);
    
    if (!result.has_value()) {
        const auto& error = result.error();
        spdlog::error("❌ Smart operation failed: {}", error.format());
        
        // Add context information if available
        for (const auto& [key, value] : error.context) {
            spdlog::error("   {}: {}", key, value);
        }
        
        return 1;
    }
    
    const auto& execResult = result.value();
    
    if (execResult.success) {
        spdlog::info("✅ {}", execResult.message);
        if (config.verbose) {
            spdlog::info("Execution time: {}ms", execResult.executionTime.count());
        }
        return 0;
    } else {
        spdlog::error("❌ {}", execResult.message);
        return 1;
    }
}

SmartOperation detectOperation(const SmartConfig& config) {
    size_t archive_count = 0;
    size_t directory_count = 0;
    size_t regular_file_count = 0;
    
    // Analyze input types
    for (const auto& input : config.inputs) {
        if (std::filesystem::is_directory(input)) {
            directory_count++;
        } else if (std::filesystem::is_regular_file(input)) {
            if (isArchiveFile(input)) {
                archive_count++;
            } else {
                regular_file_count++;
            }
        }
    }
    
    // Decision logic
    if (archive_count > 0 && directory_count == 0 && regular_file_count == 0) {
        // Only archives -> extract
        return SmartOperation::EXTRACT;
    } else if (archive_count == 0 && (directory_count > 0 || regular_file_count > 0)) {
        // Only directories/files -> pack
        return SmartOperation::PACK;
    } else if (archive_count > 0 && !config.output.empty() && isArchiveFile(config.output)) {
        // Archives with archive output -> convert
        return SmartOperation::CONVERT;
    } else if (archive_count == 1 && config.output.empty()) {
        // Single archive without output -> could be extract or list
        return SmartOperation::EXTRACT;  // Default to extract
    }
    
    return SmartOperation::UNKNOWN;
}

SmartResult analyzeAndSuggest(const SmartConfig& config) {
    SmartResult result;
    result.operation = detectOperation(config);
    
    // Collect detected formats
    for (const auto& input : config.inputs) {
        if (isArchiveFile(input)) {
            std::string format = detectArchiveFormat(input);
            if (!format.empty()) {
                result.detected_formats.push_back(format);
            }
        }
    }
    
    // Remove duplicates
    std::sort(result.detected_formats.begin(), result.detected_formats.end());
    result.detected_formats.erase(
        std::unique(result.detected_formats.begin(), result.detected_formats.end()),
        result.detected_formats.end());
    
    // Generate operation description
    switch (result.operation) {
        case SmartOperation::EXTRACT:
            result.operation_description = "Extract archive(s)";
            if (config.inputs.size() == 1) {
                result.operation_description += " to " + 
                    (config.output.empty() ? "current directory" : config.output.string());
            } else {
                result.operation_description += fmt::format(" ({} archives)", config.inputs.size());
            }
            break;
            
        case SmartOperation::PACK:
            result.operation_description = "Create archive from input(s)";
            if (!config.output.empty()) {
                result.operation_description += " -> " + config.output.string();
            }
            break;
            
        case SmartOperation::CONVERT:
            result.operation_description = "Convert archive format(s)";
            if (!config.output.empty()) {
                result.operation_description += " -> " + detectArchiveFormat(config.output);
            }
            break;
            
        case SmartOperation::LIST:
            result.operation_description = "List archive contents";
            break;
            
        case SmartOperation::TEST:
            result.operation_description = "Test archive integrity";
            break;
            
        case SmartOperation::UNKNOWN:
            result.operation_description = "Cannot determine operation";
            break;
    }
    
    result.success = (result.operation != SmartOperation::UNKNOWN);
    return result;
}

bool executeDetectedOperation(const SmartConfig& config, SmartOperation operation) {
    try {
        switch (operation) {
            case SmartOperation::EXTRACT: {
                for (const auto& input : config.inputs) {
                    std::filesystem::path output_dir = config.output;
                    if (output_dir.empty()) {
                        output_dir = std::filesystem::current_path();
                    }
                    
                    // For multiple archives, create subdirectories
                    if (config.inputs.size() > 1) {
                        output_dir = output_dir / input.stem();
                    }
                    
                    bool success = smartExtract(input, output_dir, config.password, config.overwrite);
                    if (!success) {
                        return false;
                    }
                }
                return true;
            }
            
            case SmartOperation::PACK: {
                std::filesystem::path output_path = config.output;
                if (output_path.empty()) {
                    output_path = getRecommendedOutput(config, operation);
                }
                
                return smartPack(config.inputs, output_path, config.password);
            }
            
            case SmartOperation::CONVERT: {
                // For conversion, we need to extract and repack
                for (const auto& input : config.inputs) {
                    // Create temporary extraction directory
                    auto temp_dir = std::filesystem::temp_directory_path() / 
                                   ("flux_smart_" + std::to_string(std::hash<std::string>{}(input.string())));
                    
                    // Extract
                    bool extract_success = smartExtract(input, temp_dir, config.password, true);
                    if (!extract_success) {
                        return false;
                    }
                    
                    // Determine output path
                    std::filesystem::path output_path = config.output;
                    if (output_path.empty()) {
                        output_path = input.parent_path() / (input.stem().string() + "_converted");
                        // Add appropriate extension based on target format
                        output_path += ".tar.zst";  // Default format
                    }
                    
                    // Repack
                    bool pack_success = smartPack({temp_dir}, output_path, config.password);
                    
                    // Clean up
                    if (std::filesystem::exists(temp_dir)) {
                        std::filesystem::remove_all(temp_dir);
                    }
                    
                    if (!pack_success) {
                        return false;
                    }
                }
                return true;
            }
            
            default:
                spdlog::error("Operation not yet implemented: {}", static_cast<int>(operation));
                return false;
        }
    } catch (const std::exception& e) {
        spdlog::error("Failed to execute operation: {}", e.what());
        return false;
    }
}

bool isArchiveFile(const std::filesystem::path& path) {
    if (!std::filesystem::is_regular_file(path)) {
        return false;
    }
    
    std::string filename = path.filename().string();
    std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
    
    // Check common archive extensions
    std::vector<std::string> archive_extensions = {
        ".zip", ".7z", ".tar", ".tar.gz", ".tgz", ".tar.xz", ".txz", 
        ".tar.zst", ".tar.bz2", ".tbz2", ".rar", ".gz", ".xz", ".zst"
    };
    
    for (const auto& ext : archive_extensions) {
        if (filename.ends_with(ext)) {
            return true;
        }
    }
    
    return false;
}

std::string detectArchiveFormat(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        return "";
    }
    
    try {
        // Try to open with Flux to detect format
        auto archive = Flux::Archive::open(path.string());
        return archive->getFormatName();
    } catch (const std::exception&) {
        // Fallback to extension-based detection
        std::string filename = path.filename().string();
        std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
        
        if (filename.ends_with(".zip")) return "ZIP";
        if (filename.ends_with(".7z")) return "7-Zip";
        if (filename.ends_with(".tar.gz") || filename.ends_with(".tgz")) return "TAR+GZIP";
        if (filename.ends_with(".tar.xz") || filename.ends_with(".txz")) return "TAR+XZ";
        if (filename.ends_with(".tar.zst")) return "TAR+ZSTD";
        if (filename.ends_with(".tar.bz2") || filename.ends_with(".tbz2")) return "TAR+BZIP2";
        if (filename.ends_with(".tar")) return "TAR";
        if (filename.ends_with(".rar")) return "RAR";
        if (filename.ends_with(".gz")) return "GZIP";
        if (filename.ends_with(".xz")) return "XZ";
        if (filename.ends_with(".zst")) return "ZSTD";
        
        return "Unknown";
    }
}

std::filesystem::path getRecommendedOutput(const SmartConfig& config, SmartOperation operation) {
    switch (operation) {
        case SmartOperation::PACK: {
            if (config.inputs.size() == 1) {
                auto input = config.inputs[0];
                if (std::filesystem::is_directory(input)) {
                    return input.string() + ".tar.zst";
                } else {
                    return input.parent_path() / (input.stem().string() + ".tar.zst");
                }
            } else {
                return "archive.tar.zst";
            }
        }
        
        case SmartOperation::CONVERT: {
            if (!config.inputs.empty()) {
                auto input = config.inputs[0];
                return input.parent_path() / (input.stem().string() + "_converted.tar.zst");
            }
            return "converted.tar.zst";
        }
        
        default:
            return std::filesystem::current_path();
    }
}

bool confirmOperation(const SmartResult& result, const SmartConfig& config) {
    std::cout << "\n";
    spdlog::info("📋 Operation Summary:");
    spdlog::info("  Operation: {}", result.operation_description);
    spdlog::info("  Inputs: {}", config.inputs.size());
    
    for (size_t i = 0; i < config.inputs.size(); ++i) {
        spdlog::info("    {}: {}", i + 1, config.inputs[i].string());
    }
    
    if (!config.output.empty()) {
        spdlog::info("  Output: {}", config.output.string());
    }
    
    if (!result.detected_formats.empty()) {
        spdlog::info("  Formats: {}", fmt::join(result.detected_formats, ", "));
    }
    
    if (config.dry_run) {
        spdlog::info("  Mode: DRY RUN (no files will be modified)");
        return true;
    }
    
    std::cout << "\nProceed with this operation? [Y/n]: ";
    std::string response;
    std::getline(std::cin, response);
    
    return response.empty() || response == "y" || response == "Y" || response == "yes" || response == "Yes";
}

} // namespace FluxCLI::Commands


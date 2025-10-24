#include "auto_command.h"
#include "extract_command.h"
#include "pack_command.h"
#include "../utils/format_utils.h"
#include "../utils/progress_bar.h"
#include <flux-core/extractor.h>
#include <flux-core/packer.h>
#include <flux-core/exceptions.h>
#include <spdlog/spdlog.h>
#include <iostream>
#include <algorithm>

namespace FluxCLI::Commands {

void AutoConfig::validate() {
    if (inputs.empty()) {
        throw std::invalid_argument("At least one input file or directory must be specified");
    }
    
    // Check if all inputs exist
    for (const auto& input : inputs) {
        if (!std::filesystem::exists(input)) {
            throw Flux::FileNotFoundException(input.string());
        }
    }
    
    // Validate operation
    if (!operation.empty() && operation != "auto" && operation != "extract" && operation != "pack") {
        throw std::invalid_argument("Invalid operation: " + operation);
    }
}

std::string AutoConfig::detectOperation() const {
    if (!operation.empty() && operation != "auto") {
        return operation;
    }
    
    // Check if all inputs are archive files
    bool all_archives = true;
    bool any_archives = false;
    
    for (const auto& input : inputs) {
        if (std::filesystem::is_regular_file(input)) {
            try {
                Utils::FormatUtils::detectFormatFromContent(input);
                any_archives = true;
            } catch (const Flux::UnsupportedFormatException&) {
                try {
                    Utils::FormatUtils::detectFormatFromExtension(input);
                    any_archives = true;
                } catch (const Flux::UnsupportedFormatException&) {
                    all_archives = false;
                }
            }
        } else {
            all_archives = false;
        }
    }
    
    if (all_archives) {
        return "extract";
    } else if (any_archives) {
        throw std::invalid_argument("Mixed input types: cannot auto-detect operation");
    } else {
        return "pack";
    }
}

void setupAutoCommand(CLI::App* app, bool& verbose, bool& quiet) {
    static AutoConfig config;
    
    // Input files/directories (positional arguments)
    std::vector<std::string> input_strings;
    app->add_option("inputs", input_strings, "Input files or directories")
       ->required();
    
    // Output directory for extraction
    std::string output_dir_string;
    app->add_option("-o,--output", output_dir_string, 
                    "Output directory for extraction or output file for packing");
    
    // Operation type
    app->add_option("--operation", config.operation, "Operation type")
       ->check(CLI::IsMember({"auto", "extract", "pack"}));
    
    // Password
    app->add_option("-p,--password", config.password, "Archive password");
    
    // Recursive processing
    app->add_flag("-r,--recursive", config.recursive, "Process directories recursively");
    
    // Overwrite existing files
    app->add_flag("--overwrite", config.overwrite, "Overwrite existing files");
    
    // Preserve directory structure
    app->add_flag("--no-preserve", [&config](size_t) { config.preserve_structure = false; },
                  "Do not preserve directory structure");
    
    // Command callback
    app->callback([&config, &input_strings, &output_dir_string, &verbose, &quiet]() {
        config.verbose = verbose;
        config.quiet = quiet;
        
        // Convert input strings to paths
        for (const auto& input_str : input_strings) {
            config.inputs.emplace_back(input_str);
        }
        
        // Set output path
        if (!output_dir_string.empty()) {
            config.output_dir = output_dir_string;
            config.output_file = output_dir_string;
        }
        
        try {
            config.validate();
            int result = executeAutoCommand(config);
            std::exit(result);
        } catch (const std::exception& e) {
            spdlog::error("Configuration error: {}", e.what());
            std::exit(1);
        }
    });
}

int executeAutoCommand(const AutoConfig& config) {
    try {
        spdlog::info("Starting auto operation");
        
        // Detect operation type
        std::string operation = config.detectOperation();
        spdlog::info("Detected operation: {}", operation);
        
        if (operation == "extract") {
            return executeAutoExtract(config);
        } else if (operation == "pack") {
            return executeAutoPack(config);
        } else {
            spdlog::error("Unknown operation: {}", operation);
            return 1;
        }
        
    } catch (const std::exception& e) {
        spdlog::error("Auto operation failed: {}", e.what());
        return 1;
    }
}

int executeAutoExtract(const AutoConfig& config) {
    int total_success = 0;
    int total_failed = 0;
    
    for (const auto& archive_path : config.inputs) {
        spdlog::info("Processing archive: {}", archive_path.string());
        
        // Determine output directory for this archive
        std::filesystem::path output_dir = config.output_dir;
        if (output_dir.empty()) {
            output_dir = archive_path.parent_path();
        }
        
        // Create subdirectory based on archive name if multiple archives
        if (config.inputs.size() > 1) {
            std::string archive_name = archive_path.stem().string();
            output_dir = output_dir / archive_name;
        }
        
        try {
            if (smartExtract(archive_path, output_dir, config.password, config.overwrite)) {
                total_success++;
                spdlog::info("Successfully extracted: {}", archive_path.string());
            } else {
                total_failed++;
                spdlog::error("Failed to extract: {}", archive_path.string());
            }
        } catch (const std::exception& e) {
            total_failed++;
            spdlog::error("Error extracting {}: {}", archive_path.string(), e.what());
        }
    }
    
    spdlog::info("Extraction completed: {} successful, {} failed", total_success, total_failed);
    return total_failed > 0 ? 1 : 0;
}

int executeAutoPack(const AutoConfig& config) {
    std::filesystem::path output_file = config.output_file;
    
    // If no output file specified, create default name
    if (output_file.empty()) {
        std::string default_name = "archive";
        if (config.inputs.size() == 1) {
            default_name = config.inputs[0].stem().string();
        }
        output_file = std::filesystem::current_path() / (default_name + ".tar.zst");
    }
    
    try {
        if (smartPack(config.inputs, output_file, config.password)) {
            spdlog::info("Successfully created archive: {}", output_file.string());
            return 0;
        } else {
            spdlog::error("Failed to create archive: {}", output_file.string());
            return 1;
        }
    } catch (const std::exception& e) {
        spdlog::error("Error creating archive: {}", e.what());
        return 1;
    }
}

bool smartExtract(const std::filesystem::path& archive_path,
                 const std::filesystem::path& output_dir,
                 const std::string& password,
                 bool overwrite) {
    try {
        // Detect archive format
        Flux::ArchiveFormat format;
        try {
            format = Utils::FormatUtils::detectFormatFromContent(archive_path);
        } catch (const Flux::UnsupportedFormatException&) {
            format = Utils::FormatUtils::detectFormatFromExtension(archive_path);
        }
        
        spdlog::debug("Detected format: {}", Flux::formatToString(format));
        
        // Create output directory if it doesn't exist
        if (!std::filesystem::exists(output_dir)) {
            std::filesystem::create_directories(output_dir);
        }
        
        // Create extractor
        auto extractor = Flux::createExtractor(format);
        
        // Setup extraction options
        Flux::ExtractOptions options;
        options.overwrite_mode = overwrite ? Flux::OverwriteMode::OVERWRITE : Flux::OverwriteMode::SKIP;
        options.password = password;
        options.preserve_permissions = true;
        options.preserve_timestamps = true;
        options.hoist_single_folder = true; // Smart directory hoisting
        
        // Create progress callback
        Utils::ProgressBarManager progress_manager(false);
        progress_manager.start("Extracting " + archive_path.filename().string());
        
        auto progress_callback = progress_manager.createProgressCallback();
        
        // Extract archive
        extractor->extract(archive_path, output_dir, options, progress_callback);
        
        progress_manager.finish(true, "Extraction completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Extraction failed: {}", e.what());
        return false;
    }
}

bool smartPack(const std::vector<std::filesystem::path>& inputs,
              const std::filesystem::path& output_file,
              const std::string& password) {
    try {
        // Detect format from output file extension
        Flux::ArchiveFormat format;
        try {
            format = Utils::FormatUtils::detectFormatFromExtension(output_file);
        } catch (const Flux::UnsupportedFormatException&) {
            // Default to TAR+ZSTD for best compression and performance
            format = Flux::ArchiveFormat::TAR_ZSTD;
            spdlog::info("Using default format: TAR+ZSTD");
        }
        
        spdlog::debug("Using format: {}", Flux::formatToString(format));
        
        // Create output directory if needed
        auto output_dir = output_file.parent_path();
        if (!output_dir.empty() && !std::filesystem::exists(output_dir)) {
            std::filesystem::create_directories(output_dir);
        }
        
        // Create packer
        auto packer = Flux::createPacker(format);
        
        // Setup packing options
        Flux::PackOptions options;
        options.compression_level = -1; // Use default compression level
        options.num_threads = 0; // Auto-detect thread count
        options.password = password;
        options.preserve_permissions = true;
        options.preserve_timestamps = true;
        
        // Smart compression strategy based on input types
        options.compression_strategy = Flux::CompressionStrategy::AUTO;
        
        // Create progress callback
        Utils::ProgressBarManager progress_manager(false);
        progress_manager.start("Creating " + output_file.filename().string());
        
        auto progress_callback = progress_manager.createProgressCallback();
        
        // Pack files
        packer->pack(inputs, output_file, options, progress_callback);
        
        progress_manager.finish(true, "Archive created successfully");
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Packing failed: {}", e.what());
        return false;
    }
}

} // namespace FluxCLI::Commands


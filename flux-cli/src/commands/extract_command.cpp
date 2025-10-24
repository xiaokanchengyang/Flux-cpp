#include "extract_command.h"
#include "../utils/format_utils.h"
#include "../utils/progress_bar.h"
#include <flux-core/extractor.h>
#include <flux-core/exceptions.h>
#include <spdlog/spdlog.h>
#include <iostream>

namespace FluxCLI::Commands {

void ExtractConfig::validate() {
    // Validate archive file
    if (archive.empty()) {
        throw std::invalid_argument("Archive file must be specified");
    }
    
    if (!std::filesystem::exists(archive)) {
        throw Flux::FileNotFoundException(archive.string());
    }
    
    if (!std::filesystem::is_regular_file(archive)) {
        throw std::invalid_argument("Specified path is not a file: " + archive.string());
    }
    
    // Validate output directory
    if (output_dir.empty()) {
        output_dir = std::filesystem::current_path();
    }
    
    // Validate strip_components
    if (strip_components < 0) {
        throw std::invalid_argument("strip-components cannot be negative");
    }
}

Flux::ExtractOptions ExtractConfig::toFluxOptions() const {
        Flux::ExtractOptions options;
    options.overwrite_mode = overwrite_mode;
    options.hoist_single_folder = hoist_single_folder;
    options.preserve_permissions = preserve_permissions;
    options.preserve_timestamps = preserve_timestamps;
        options.password = password;
    options.include_patterns = include_patterns;
    options.exclude_patterns = exclude_patterns;
    
    return options;
}

void setupExtractCommand(CLI::App* app, bool& verbose, bool& quiet) {
    static ExtractConfig config;
    
    // Archive file (positional argument)
    std::string archive_string;
    app->add_option("archive", archive_string, "Archive file to extract")
       ->required()
       ->check(CLI::ExistingFile);
    
    // Output directory (optional, defaults to current directory)
    std::string output_string;
    app->add_option("-o,--output", output_string, "Output directory (default: current directory)");
    
    // Overwrite mode
    std::string overwrite_mode_str = "skip";
    app->add_option("--overwrite", overwrite_mode_str, "Overwrite mode")
       ->check(CLI::IsMember({"skip", "overwrite", "prompt"}));
    
    // Smart directory hoisting
    app->add_flag("--hoist", config.hoist_single_folder, 
                  "Smart directory hoisting: if archive contains only one root directory, hoist its contents");
    
    // Strip directory levels
    app->add_option("--strip-components", config.strip_components, 
                    "Strip specified number of parent directories when extracting")
       ->check(CLI::NonNegativeNumber);
    
    // Password
    app->add_option("-p,--password", config.password, "Archive password (leave empty to prompt)");
    
    // Include/exclude patterns
    app->add_option("--include", config.include_patterns, "Only extract files matching patterns");
    app->add_option("--exclude", config.exclude_patterns, "Exclude files matching patterns");
    
    // Permissions and timestamps
    app->add_flag("--no-permissions", [&config](size_t) { config.preserve_permissions = false; },
                  "Do not preserve file permissions");
    app->add_flag("--no-timestamps", [&config](size_t) { config.preserve_timestamps = false; },
                  "Do not preserve file timestamps");
    
    // Command callback
    app->callback([&config, &archive_string, &output_string, &overwrite_mode_str, &verbose, &quiet]() {
        config.archive = archive_string;
        
        if (!output_string.empty()) {
            config.output_dir = output_string;
        }
        
        // Parse overwrite mode
        if (overwrite_mode_str == "skip") {
            config.overwrite_mode = Flux::OverwriteMode::SKIP;
        } else if (overwrite_mode_str == "overwrite") {
            config.overwrite_mode = Flux::OverwriteMode::OVERWRITE;
        } else if (overwrite_mode_str == "prompt") {
            config.overwrite_mode = Flux::OverwriteMode::PROMPT;
        }
        
        config.verbose = verbose;
        config.quiet = quiet;
        
        // Validate configuration
        config.validate();
        
        // Execute command
        int exit_code = executeExtractCommand(config);
        if (exit_code != 0) {
            std::exit(exit_code);
        }
    });
}

int executeExtractCommand(const ExtractConfig& config) {
    try {
        spdlog::info("Starting extract operation");
        spdlog::debug("Archive file: {}", config.archive.string());
        spdlog::debug("Output directory: {}", config.output_dir.string());
        
        // Detect archive format
        Flux::ArchiveFormat format;
        try {
            format = Utils::FormatUtils::detectFormatFromContent(config.archive);
        } catch (const Flux::UnsupportedFormatException&) {
            // If unable to detect from content, try detecting from extension
            format = Utils::FormatUtils::detectFormatFromExtension(config.archive);
        }
        
        spdlog::debug("Detected format: {}", Flux::formatToString(format));
        
        // Validate output directory
        if (!validateOutputDirectory(config.output_dir)) {
            throw std::invalid_argument("Output directory is invalid or cannot be created");
        }
        
        // Create extractor
        auto extractor = Flux::createExtractor(format);
        auto options = config.toFluxOptions();
        
        // If smart directory hoisting is enabled but user hasn't explicitly set it, auto-detect
        if (!config.hoist_single_folder && shouldHoistDirectory(config.archive)) {
            spdlog::info("Detected single root directory, enabling smart directory hoisting");
            options.hoist_single_folder = true;
        }
        
        // Create progress bar manager
        Utils::ProgressBarManager progress_manager(config.quiet);
        
        // Get archive info to estimate size
        size_t estimated_size = 0;
        try {
            // Get archive info for progress reporting
            auto info_result = extractor->getArchiveInfo(config.archive, config.password);
            if (info_result.has_value()) {
                auto info = info_result.value();
                spdlog::info("Archive contains {} files ({} bytes compressed, {} bytes uncompressed)", 
                           info.file_count, info.compressed_size, info.uncompressed_size);
            }
            // auto archive_info = extractor->getArchiveInfo(config.archive);
            // estimated_size = archive_info.uncompressed_size;
            // If unable to get info, use archive file size as estimate
            estimated_size = std::filesystem::file_size(config.archive);
        } catch (...) {
            // If unable to get info, use archive file size as estimate
            estimated_size = std::filesystem::file_size(config.archive);
        }
        
        progress_manager.start("Extracting", estimated_size);
        
        // Execute extraction
        auto result = extractor->extract(
            config.archive,
            config.output_dir,
            options,
            progress_manager.createProgressCallback(),
            [](std::string_view error, bool fatal) {
                if (fatal) {
                    spdlog::error("Fatal error: {}", error);
                } else {
                    spdlog::warn("Warning: {}", error);
                }
            }
        );
        
        // Complete progress bar
        if (result.success) {
            std::string summary = "Extracted " + std::to_string(result.files_extracted) + " files, " +
                                Utils::FormatUtils::formatFileSize(result.total_size);
            
            if (!result.skipped_files.empty()) {
                summary += " (skipped " + std::to_string(result.skipped_files.size()) + " files)";
            }
            
            progress_manager.finish(true, summary);
            
            if (!config.quiet) {
                spdlog::info("✅ Extraction completed!");
                spdlog::info("📁 Output directory: {}", config.output_dir.string());
                spdlog::info("📊 Statistics:");
                spdlog::info("   • Extracted files: {}", result.files_extracted);
                spdlog::info("   • Total size: {}", Utils::FormatUtils::formatFileSize(result.total_size));
                spdlog::info("   • Duration: {}", Utils::FormatUtils::formatDuration(result.duration.count()));
            
            if (!result.skipped_files.empty()) {
                    spdlog::info("   • Skipped files: {}", result.skipped_files.size());
                    if (config.verbose) {
                    for (const auto& skipped : result.skipped_files) {
                            spdlog::info("     - {}", skipped);
                        }
                    }
                }
            }
            
            return 0;
        } else {
            progress_manager.finish(false, result.error_message);
            spdlog::error("Extraction failed: {}", result.error_message);
            return 1;
        }

    } catch (const Flux::FileNotFoundException& e) {
        spdlog::error("File not found: {}", e.what());
        return 2;
    } catch (const Flux::PermissionDeniedException& e) {
        spdlog::error("Permission denied: {}", e.what());
        return 3;
    } catch (const Flux::CorruptedArchiveException& e) {
        spdlog::error("Archive file corrupted: {}", e.what());
        return 4;
    } catch (const Flux::UnsupportedFormatException& e) {
        spdlog::error("Unsupported format: {}", e.what());
        return 5;
    } catch (const Flux::InvalidPasswordException& e) {
        spdlog::error("Incorrect password: {}", e.what());
        return 6;
    } catch (const std::exception& e) {
        spdlog::error("Error: {}", e.what());
        return 1;
    }
    
    return 0;
}

bool shouldHoistDirectory(const std::filesystem::path& archive_path) {
    try {
        // Need to implement logic to check if archive contains only one root directory
        // Since this requires accessing archive contents, we return false for now
        // In actual implementation, should use Flux::listArchiveContents() or similar function
        
        // Pseudocode:
        // auto entries = Flux::listArchiveContents(archive_path);
        // if (entries.empty()) return false;
        // 
        // std::string root_dir;
        // for (const auto& entry : entries) {
        //     auto parts = splitPath(entry.path);
        //     if (parts.empty()) continue;
        //     
        //     if (root_dir.empty()) {
        //         root_dir = parts[0];
        //     } else if (root_dir != parts[0]) {
        //         return false; // Multiple root directories
        //     }
        // }
        // return !root_dir.empty();
        
        return false;
    } catch (...) {
        return false;
    }
}

std::vector<std::string> previewExtraction(const ExtractConfig& config) {
    std::vector<std::string> file_list;
    
    try {
        // Need to implement preview functionality
        // Use Flux::listArchiveContents() to get file list
        
        // Pseudocode:
        // auto entries = Flux::listArchiveContents(config.archive);
        // for (const auto& entry : entries) {
        //     // Apply include/exclude filters
        //     if (matchesPatterns(entry.path, config.include_patterns, config.exclude_patterns)) {
        //         file_list.push_back(entry.path);
        //     }
        // }
        
    } catch (const std::exception& e) {
        spdlog::error("Cannot preview archive contents: {}", e.what());
    }
    
    return file_list;
}

bool validateOutputDirectory(const std::filesystem::path& output_dir, bool create_if_missing) {
    std::error_code ec;
    
    if (std::filesystem::exists(output_dir, ec)) {
        if (!std::filesystem::is_directory(output_dir, ec)) {
            spdlog::error("Output path exists but is not a directory: {}", output_dir.string());
            return false;
        }
        
        // Check write permissions
        auto temp_file = output_dir / ".flux_write_test";
        std::ofstream test_stream(temp_file);
        if (!test_stream.is_open()) {
            spdlog::error("Output directory has no write permissions: {}", output_dir.string());
            return false;
        }
        test_stream.close();
        std::filesystem::remove(temp_file, ec);
        
        return true;
    } else if (create_if_missing) {
        // Try to create directory
        if (std::filesystem::create_directories(output_dir, ec)) {
            spdlog::debug("Created output directory: {}", output_dir.string());
            return true;
        } else {
            spdlog::error("Cannot create output directory {}: {}", output_dir.string(), ec.message());
            return false;
        }
    } else {
        spdlog::error("Output directory does not exist: {}", output_dir.string());
        return false;
    }
}

} // namespace FluxCLI::Commands
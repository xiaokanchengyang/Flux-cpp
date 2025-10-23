#include "pack_command.h"
#include "../utils/format_utils.h"
#include "../utils/progress_bar.h"
#include <flux-core/packer.h>
#include <flux-core/exceptions.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <set>
#include <regex>

namespace FluxCLI::Commands {

void PackConfig::validate() {
    // Validate input files
    if (inputs.empty()) {
        throw std::invalid_argument("At least one input file or directory must be specified");
    }
    
    // Validate output file
    if (output.empty()) {
        throw std::invalid_argument("Output archive file must be specified (-o/--output)");
    }
    
    // Validate format
    if (!format_str.empty()) {
        try {
            format = Utils::FormatUtils::parseFormatString(format_str);
        } catch (const Flux::UnsupportedFormatException& e) {
            throw std::invalid_argument("Unsupported format: " + format_str);
        }
    } else {
        // Infer format from output file extension
        try {
            format = Utils::FormatUtils::detectFormatFromExtension(output);
        } catch (const Flux::UnsupportedFormatException& e) {
            throw std::invalid_argument("Cannot infer format from file extension, please specify explicitly using -f/--format");
        }
    }
    
    // Validate compression level
    if (compression_level != -1) {
        if (!Utils::FormatUtils::isCompressionLevelValid(format, compression_level)) {
            auto [min_level, max_level, default_level] = Utils::FormatUtils::getCompressionLevelRange(format);
            throw std::invalid_argument("Compression level " + std::to_string(compression_level) + 
                                      " is invalid, valid range: " + std::to_string(min_level) + 
                                      "-" + std::to_string(max_level));
        }
    }
    
    // Validate thread count
    if (num_threads < 0) {
        throw std::invalid_argument("Thread count cannot be negative");
    }
    
    // Validate compression strategy
    if (strategy != "auto" && strategy != "store" && strategy != "compress") {
        throw std::invalid_argument("Invalid compression strategy: " + strategy + " (supported: auto, store, compress)");
    }
}

Flux::PackOptions PackConfig::toFluxOptions() const {
        Flux::PackOptions options;
    options.format = format;
    
    if (compression_level != -1) {
        options.compression_level = compression_level;
    } else {
        // Use default compression level
        auto [min_level, max_level, default_level] = Utils::FormatUtils::getCompressionLevelRange(format);
        options.compression_level = default_level;
    }
    
    options.num_threads = num_threads;
    options.preserve_permissions = preserve_permissions;
    options.preserve_timestamps = preserve_timestamps;
        options.password = password;

    return options;
}

void setupPackCommand(CLI::App* app, bool& verbose, bool& quiet) {
    static PackConfig config;
    
    // Input files/directories (positional arguments)
    std::vector<std::string> input_strings;
    app->add_option("inputs", input_strings, "Input files or directories")
       ->required()
       ->check(CLI::ExistingPath);
    
    // Output file (required)
    std::string output_string;
    app->add_option("-o,--output", output_string, "Output archive file path")
       ->required();
    
    // Format (optional, inferred from extension)
    app->add_option("-f,--format", config.format_str, "Archive format")
       ->check(CLI::IsMember(Utils::FormatUtils::getSupportedFormatStrings()));
    
    // Compression level
    app->add_option("-l,--level", config.compression_level, "Compression level (0-9 or format-specific range)")
       ->check(CLI::Range(0, 22)); // Zstd supports up to 22
    
    // Thread count
    app->add_option("-t,--threads", config.num_threads, "Number of concurrent threads (0=auto-detect)")
       ->check(CLI::NonNegativeNumber);
    
    // Exclude patterns
    app->add_option("--exclude", config.exclude_patterns, "Exclude file patterns (supports glob)");
    
    // Compression strategy
    app->add_option("--strategy", config.strategy, "Compression strategy")
       ->check(CLI::IsMember({"auto", "store", "compress"}));
    
    // Password protection
    app->add_option("-p,--password", config.password, "Password protection (leave empty to prompt)");
    
    // Permissions and timestamps
    app->add_flag("--no-permissions", [&config](size_t) { config.preserve_permissions = false; },
                  "Do not preserve file permissions");
    app->add_flag("--no-timestamps", [&config](size_t) { config.preserve_timestamps = false; },
                  "Do not preserve file timestamps");
    
    // Command callback
    app->callback([&config, &input_strings, &output_string, &verbose, &quiet]() {
        // Convert input paths
        config.inputs.clear();
        for (const auto& input_str : input_strings) {
            config.inputs.emplace_back(input_str);
        }
        
        config.output = output_string;
        config.verbose = verbose;
        config.quiet = quiet;
        
        // Validate configuration
        config.validate();
        
        // Execute command
        int exit_code = executePackCommand(config);
        if (exit_code != 0) {
            std::exit(exit_code);
        }
    });
}

int executePackCommand(const PackConfig& config) {
    try {
        spdlog::info("Starting pack operation");
        spdlog::debug("Output file: {}", config.output.string());
        spdlog::debug("Format: {}", Flux::formatToString(config.format));
        spdlog::debug("Input file count: {}", config.inputs.size());
        
        // Validate input file existence
        for (const auto& input : config.inputs) {
            if (!std::filesystem::exists(input)) {
                throw Flux::FileNotFoundException(input.string());
            }
        }
        
        // Validate output path
        if (!validateOutputPath(config.output, config.inputs)) {
            throw std::invalid_argument("Output path is invalid or conflicts with input paths");
        }
        
        // Create packer
        auto packer = Flux::createPacker(config.format);
        auto options = config.toFluxOptions();
        
        // Create progress bar manager
        Utils::ProgressBarManager progress_manager(config.quiet);
        
        // Estimate total size
        size_t total_size = 0;
        for (const auto& input : config.inputs) {
            std::error_code ec;
            if (std::filesystem::is_regular_file(input, ec)) {
                total_size += std::filesystem::file_size(input, ec);
            } else if (std::filesystem::is_directory(input, ec)) {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(input, ec)) {
                    if (entry.is_regular_file(ec)) {
                        total_size += entry.file_size(ec);
                    }
                }
            }
        }
        
        progress_manager.start("Packing", total_size);
        
        // Execute packing
        auto result = packer->pack(
            config.inputs,
            config.output,
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
            std::string summary = "Compressed " + std::to_string(result.files_processed) + " files, " +
                                Utils::FormatUtils::formatFileSize(result.total_uncompressed_size) + " → " +
                                Utils::FormatUtils::formatFileSize(result.total_compressed_size) + " (" +
                                Utils::FormatUtils::formatCompressionRatio(result.total_uncompressed_size, result.total_compressed_size) + ")";
            
            progress_manager.finish(true, summary);
            
            if (!config.quiet) {
                spdlog::info("✅ Packing completed!");
                spdlog::info("📁 Output file: {}", config.output.string());
                spdlog::info("📊 Statistics:");
                spdlog::info("   • File count: {}", result.files_processed);
                spdlog::info("   • Original size: {}", Utils::FormatUtils::formatFileSize(result.total_uncompressed_size));
                spdlog::info("   • Compressed size: {}", Utils::FormatUtils::formatFileSize(result.total_compressed_size));
                spdlog::info("   • Compression ratio: {}", Utils::FormatUtils::formatCompressionRatio(result.total_uncompressed_size, result.total_compressed_size));
                spdlog::info("   • Duration: {}", Utils::FormatUtils::formatDuration(result.duration.count()));
            }
            
            return 0;
        } else {
            progress_manager.finish(false, result.error_message);
            spdlog::error("Packing failed: {}", result.error_message);
            return 1;
        }

    } catch (const Flux::FileNotFoundException& e) {
        spdlog::error("File not found: {}", e.what());
        return 2;
    } catch (const Flux::PermissionDeniedException& e) {
        spdlog::error("Permission denied: {}", e.what());
        return 3;
    } catch (const Flux::UnsupportedFormatException& e) {
        spdlog::error("Unsupported format: {}", e.what());
        return 5;
    } catch (const std::exception& e) {
        spdlog::error("Error: {}", e.what());
        return 1;
    }
    
    return 0;
}

bool shouldCompressFile(const std::filesystem::path& file_path) {
    std::string ext = file_path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    // Already compressed formats, no need to compress again
    static const std::set<std::string> compressed_extensions = {
        ".zip", ".7z", ".rar", ".gz", ".bz2", ".xz", ".zst",
        ".jpg", ".jpeg", ".png", ".gif", ".webp", ".avif",
        ".mp3", ".aac", ".ogg", ".flac", ".m4a",
        ".mp4", ".avi", ".mkv", ".webm", ".mov",
        ".pdf", ".docx", ".xlsx", ".pptx"
    };
    
    return compressed_extensions.find(ext) == compressed_extensions.end();
}

std::vector<std::filesystem::path> expandInputPatterns(const std::vector<std::string>& patterns) {
    std::vector<std::filesystem::path> result;
    
    for (const auto& pattern : patterns) {
        // Simple glob expansion implementation
        // In real projects, more complex glob libraries might be needed
        if (pattern.find('*') != std::string::npos || pattern.find('?') != std::string::npos) {
            // Should implement glob expansion here
            // For now, directly add pattern as path
            result.emplace_back(pattern);
        } else {
            result.emplace_back(pattern);
        }
    }
    
    return result;
}

bool validateOutputPath(const std::filesystem::path& output_path, 
                       const std::vector<std::filesystem::path>& inputs) {
    // Check if output path conflicts with input paths
    std::filesystem::path abs_output = std::filesystem::absolute(output_path);
    
    for (const auto& input : inputs) {
        std::filesystem::path abs_input = std::filesystem::absolute(input);
        
        // Check if output file is inside input directory
        if (std::filesystem::is_directory(abs_input)) {
            auto rel_path = std::filesystem::relative(abs_output, abs_input);
            if (!rel_path.empty() && rel_path.string().find("..") != 0) {
                spdlog::warn("Output file {} is inside input directory {}, this may cause recursive inclusion", 
                           abs_output.string(), abs_input.string());
                return false;
            }
        }
        
        // Check if output file is the same as input file
        if (abs_output == abs_input) {
            spdlog::error("Output file cannot be the same as input file");
            return false;
        }
    }
    
    // Check if output directory exists, create if it doesn't
    auto output_dir = output_path.parent_path();
    if (!output_dir.empty() && !std::filesystem::exists(output_dir)) {
        std::error_code ec;
        std::filesystem::create_directories(output_dir, ec);
        if (ec) {
            spdlog::error("Cannot create output directory {}: {}", output_dir.string(), ec.message());
            return false;
        }
    }
    
    return true;
}

} // namespace FluxCLI::Commands
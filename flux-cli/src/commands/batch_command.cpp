#include "batch_command.h"
#include "auto_command.h"
#include "../utils/format_utils.h"
#include "../utils/progress_bar.h"
#include <flux-core/extractor.h>
#include <flux-core/packer.h>
#include <flux-core/exceptions.h>
#include <spdlog/spdlog.h>
#include <iostream>
#include <algorithm>
#include <thread>
#include <mutex>
#include <regex>
#include <iomanip>

namespace FluxCLI::Commands {

void BatchConfig::validate() {
    if (inputs.empty()) {
        throw std::invalid_argument("At least one input must be specified");
    }
    
    if (operation.empty()) {
        throw std::invalid_argument("Operation must be specified");
    }
    
    if (operation != "extract" && operation != "pack" && operation != "convert") {
        throw std::invalid_argument("Invalid operation: " + operation);
    }
    
    if (operation == "convert" && target_format.empty()) {
        throw std::invalid_argument("Target format must be specified for convert operation");
    }
    
    if (max_parallel < 1 || max_parallel > 32) {
        throw std::invalid_argument("Max parallel operations must be between 1 and 32");
    }
}

void setupBatchCommand(CLI::App* app, bool& verbose, bool& quiet) {
    static BatchConfig config;
    
    // Input files/directories (positional arguments)
    std::vector<std::string> input_strings;
    app->add_option("inputs", input_strings, "Input files or directories")
       ->required();
    
    // Operation type
    app->add_option("operation", config.operation, "Batch operation")
       ->required()
       ->check(CLI::IsMember({"extract", "pack", "convert"}));
    
    // Output directory
    std::string output_dir_string;
    app->add_option("-o,--output", output_dir_string, "Output directory")
       ->required();
    
    // Target format for conversion
    app->add_option("--format", config.target_format, "Target format for conversion")
       ->check(CLI::IsMember(Utils::FormatUtils::getSupportedFormatStrings()));
    
    // Password
    app->add_option("-p,--password", config.password, "Archive password");
    
    // Parallel processing
    app->add_option("-j,--parallel", config.max_parallel, "Maximum parallel operations")
       ->check(CLI::Range(1, 32));
    
    // Recursive processing
    app->add_flag("-r,--recursive", config.recursive, "Process directories recursively");
    
    // Overwrite existing files
    app->add_flag("--overwrite", config.overwrite, "Overwrite existing files");
    
    // Continue on error
    app->add_flag("--stop-on-error", [&config](size_t) { config.continue_on_error = false; },
                  "Stop processing on first error");
    
    // Include/exclude patterns
    app->add_option("--include", config.include_patterns, "Include file patterns");
    app->add_option("--exclude", config.exclude_patterns, "Exclude file patterns");
    
    // Command callback
    app->callback([&config, &input_strings, &output_dir_string, &verbose, &quiet]() {
        config.verbose = verbose;
        config.quiet = quiet;
        config.output_dir = output_dir_string;
        
        // Convert input strings to paths
        for (const auto& input_str : input_strings) {
            config.inputs.emplace_back(input_str);
        }
        
        try {
            config.validate();
            int result = executeBatchCommand(config);
            std::exit(result);
        } catch (const std::exception& e) {
            spdlog::error("Configuration error: {}", e.what());
            std::exit(1);
        }
    });
}

int executeBatchCommand(const BatchConfig& config) {
    try {
        spdlog::info("Starting batch {} operation", config.operation);
        spdlog::info("Max parallel operations: {}", config.max_parallel);
        
        std::vector<BatchResult> results;
        
        if (config.operation == "extract") {
            results = batchExtract(config);
        } else if (config.operation == "pack") {
            results = batchPack(config);
        } else if (config.operation == "convert") {
            results = batchConvert(config);
        }
        
        printBatchSummary(results);
        
        // Count failures
        int failures = std::count_if(results.begin(), results.end(),
                                   [](const BatchResult& r) { return !r.success; });
        
        return failures > 0 ? 1 : 0;
        
    } catch (const std::exception& e) {
        spdlog::error("Batch operation failed: {}", e.what());
        return 1;
    }
}

std::vector<BatchResult> batchExtract(const BatchConfig& config) {
    std::vector<std::filesystem::path> archive_files;
    
    // Collect all archive files
    for (const auto& input : config.inputs) {
        if (std::filesystem::is_regular_file(input)) {
            archive_files.push_back(input);
        } else if (std::filesystem::is_directory(input)) {
            auto found_files = findArchiveFiles(input, config.recursive, 
                                              config.include_patterns, config.exclude_patterns);
            archive_files.insert(archive_files.end(), found_files.begin(), found_files.end());
        }
    }
    
    spdlog::info("Found {} archive files to extract", archive_files.size());
    
    std::vector<BatchResult> results(archive_files.size());
    std::mutex results_mutex;
    std::atomic<size_t> completed{0};
    std::atomic<bool> should_stop{false};
    
    // Create thread pool
    std::vector<std::future<void>> futures;
    size_t files_per_thread = std::max(size_t(1), archive_files.size() / config.max_parallel);
    
    for (size_t i = 0; i < archive_files.size(); i += files_per_thread) {
        size_t end_idx = std::min(i + files_per_thread, archive_files.size());
        
        futures.emplace_back(std::async(std::launch::async, [&, i, end_idx]() {
            for (size_t idx = i; idx < end_idx && !should_stop.load(); ++idx) {
                const auto& archive_path = archive_files[idx];
                
                auto start_time = std::chrono::steady_clock::now();
                
                // Determine output directory
                std::filesystem::path output_dir = config.output_dir;
                if (archive_files.size() > 1) {
                    std::string archive_name = archive_path.stem().string();
                    output_dir = output_dir / archive_name;
                }
                
                BatchResult result;
                result.input_path = archive_path;
                result.output_path = output_dir;
                
                try {
                    result.success = smartExtract(archive_path, output_dir, 
                                                config.password, config.overwrite);
                    
                    if (result.success) {
                        // Calculate processed bytes
                        result.processed_bytes = std::filesystem::file_size(archive_path);
                    }
                    
                } catch (const std::exception& e) {
                    result.success = false;
                    result.error_message = e.what();
                    
                    if (!config.continue_on_error) {
                        should_stop.store(true);
                    }
                }
                
                auto end_time = std::chrono::steady_clock::now();
                result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    end_time - start_time);
                
                {
                    std::lock_guard<std::mutex> lock(results_mutex);
                    results[idx] = std::move(result);
                }
                
                completed.fetch_add(1);
                
                if (!config.quiet) {
                    spdlog::info("Progress: {}/{} completed", completed.load(), archive_files.size());
                }
            }
        }));
    }
    
    // Wait for all threads to complete
    for (auto& future : futures) {
        future.wait();
    }
    
    return results;
}

std::vector<BatchResult> batchPack(const BatchConfig& config) {
    std::vector<BatchResult> results;
    
    // For pack operation, each input becomes one archive
    for (const auto& input : config.inputs) {
        BatchResult result;
        result.input_path = input;
        
        auto start_time = std::chrono::steady_clock::now();
        
        try {
            // Generate output filename
            std::string output_name = input.filename().string();
            if (config.target_format.empty()) {
                output_name += ".tar.zst";
            } else {
                output_name += Utils::FormatUtils::getDefaultExtension(
                    Utils::FormatUtils::parseFormatString(config.target_format));
            }
            
            result.output_path = config.output_dir / output_name;
            
            result.success = smartPack({input}, result.output_path, config.password);
            
            if (result.success) {
                result.processed_bytes = std::filesystem::file_size(result.output_path);
            }
            
        } catch (const std::exception& e) {
            result.success = false;
            result.error_message = e.what();
        }
        
        auto end_time = std::chrono::steady_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);
        
        results.push_back(std::move(result));
        
        if (!result.success && !config.continue_on_error) {
            break;
        }
    }
    
    return results;
}

std::vector<BatchResult> batchConvert(const BatchConfig& config) {
    std::vector<std::filesystem::path> archive_files;
    
    // Collect all archive files
    for (const auto& input : config.inputs) {
        if (std::filesystem::is_regular_file(input)) {
            archive_files.push_back(input);
        } else if (std::filesystem::is_directory(input)) {
            auto found_files = findArchiveFiles(input, config.recursive,
                                              config.include_patterns, config.exclude_patterns);
            archive_files.insert(archive_files.end(), found_files.begin(), found_files.end());
        }
    }
    
    std::vector<BatchResult> results;
    
    for (const auto& archive_path : archive_files) {
        BatchResult result;
        result.input_path = archive_path;
        
        auto start_time = std::chrono::steady_clock::now();
        
        try {
            // Create temporary extraction directory
            auto temp_dir = std::filesystem::temp_directory_path() / 
                           ("flux_convert_" + std::to_string(std::hash<std::string>{}(archive_path.string())));
            
            // Extract to temporary directory
            bool extract_success = smartExtract(archive_path, temp_dir, config.password, true);
            
            if (extract_success) {
                // Generate output filename with new format
                std::string output_name = archive_path.stem().string();
                output_name += Utils::FormatUtils::getDefaultExtension(
                    Utils::FormatUtils::parseFormatString(config.target_format));
                
                result.output_path = config.output_dir / output_name;
                
                // Pack with new format
                result.success = smartPack({temp_dir}, result.output_path, config.password);
                
                if (result.success) {
                    result.processed_bytes = std::filesystem::file_size(result.output_path);
                }
            }
            
            // Clean up temporary directory
            if (std::filesystem::exists(temp_dir)) {
                std::filesystem::remove_all(temp_dir);
            }
            
        } catch (const std::exception& e) {
            result.success = false;
            result.error_message = e.what();
        }
        
        auto end_time = std::chrono::steady_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);
        
        results.push_back(std::move(result));
        
        if (!result.success && !config.continue_on_error) {
            break;
        }
    }
    
    return results;
}

std::vector<std::filesystem::path> findArchiveFiles(
    const std::filesystem::path& directory,
    bool recursive,
    const std::vector<std::string>& include_patterns,
    const std::vector<std::string>& exclude_patterns) {
    
    std::vector<std::filesystem::path> archive_files;
    
    // Supported archive extensions
    std::vector<std::string> archive_extensions = {
        ".zip", ".7z", ".tar", ".tar.gz", ".tgz", ".tar.xz", ".txz", ".tar.zst"
    };
    
    auto iterator = recursive ? 
        std::filesystem::recursive_directory_iterator(directory) :
        std::filesystem::directory_iterator(directory);
    
    for (const auto& entry : iterator) {
        if (!entry.is_regular_file()) {
            continue;
        }
        
        const auto& path = entry.path();
        std::string filename = path.filename().string();
        std::string extension = path.extension().string();
        
        // Check if it's an archive file
        bool is_archive = false;
        for (const auto& ext : archive_extensions) {
            if (filename.ends_with(ext)) {
                is_archive = true;
                break;
            }
        }
        
        if (!is_archive) {
            continue;
        }
        
        // Apply include patterns
        if (!include_patterns.empty()) {
            bool matches_include = false;
            for (const auto& pattern : include_patterns) {
                std::regex regex_pattern(pattern);
                if (std::regex_match(filename, regex_pattern)) {
                    matches_include = true;
                    break;
                }
            }
            if (!matches_include) {
                continue;
            }
        }
        
        // Apply exclude patterns
        bool matches_exclude = false;
        for (const auto& pattern : exclude_patterns) {
            std::regex regex_pattern(pattern);
            if (std::regex_match(filename, regex_pattern)) {
                matches_exclude = true;
                break;
            }
        }
        if (matches_exclude) {
            continue;
        }
        
        archive_files.push_back(path);
    }
    
    return archive_files;
}

void printBatchSummary(const std::vector<BatchResult>& results) {
    if (results.empty()) {
        spdlog::info("No operations performed");
        return;
    }
    
    size_t successful = std::count_if(results.begin(), results.end(),
                                    [](const BatchResult& r) { return r.success; });
    size_t failed = results.size() - successful;
    
    size_t total_bytes = 0;
    std::chrono::milliseconds total_duration{0};
    
    for (const auto& result : results) {
        total_bytes += result.processed_bytes;
        total_duration += result.duration;
    }
    
    spdlog::info("Batch Operation Summary:");
    spdlog::info("  Total operations: {}", results.size());
    spdlog::info("  Successful: {}", successful);
    spdlog::info("  Failed: {}", failed);
    spdlog::info("  Total processed: {}", Utils::FormatUtils::formatFileSize(total_bytes));
    spdlog::info("  Total time: {}", Utils::FormatUtils::formatDuration(total_duration.count()));
    
    if (total_duration.count() > 0) {
        double throughput = static_cast<double>(total_bytes) / (total_duration.count() / 1000.0);
        spdlog::info("  Average throughput: {}/s", Utils::FormatUtils::formatFileSize(static_cast<size_t>(throughput)));
    }
    
    // Show failed operations
    if (failed > 0) {
        spdlog::error("Failed operations:");
        for (const auto& result : results) {
            if (!result.success) {
                spdlog::error("  {}: {}", result.input_path.string(), result.error_message);
            }
        }
    }
}

} // namespace FluxCLI::Commands


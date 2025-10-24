#pragma once

#include <CLI/CLI.hpp>
#include <flux-core/archive.h>
#include <filesystem>
#include <vector>
#include <string>
#include <future>

namespace FluxCLI::Commands {
    /**
     * Batch operation configuration
     */
    struct BatchConfig {
        std::vector<std::filesystem::path> inputs;    // Input files/directories
        std::filesystem::path output_dir;             // Output directory
        std::string operation;                        // Operation: extract, pack, convert
        std::string target_format;                    // Target format for conversion
        std::string password;                         // Password for protected archives
        int max_parallel = 4;                         // Maximum parallel operations
        bool recursive = false;                       // Process directories recursively
        bool overwrite = false;                       // Overwrite existing files
        bool continue_on_error = true;                // Continue processing on errors
        bool verbose = false;                         // Verbose mode
        bool quiet = false;                           // Quiet mode
        
        // File patterns for filtering
        std::vector<std::string> include_patterns;
        std::vector<std::string> exclude_patterns;
        
        void validate();
    };
    
    /**
     * Batch operation result
     */
    struct BatchResult {
        std::filesystem::path input_path;
        std::filesystem::path output_path;
        bool success = false;
        std::string error_message;
        size_t processed_bytes = 0;
        std::chrono::milliseconds duration{0};
    };
    
    /**
     * Setup batch command
     */
    void setupBatchCommand(CLI::App* app, bool& verbose, bool& quiet);
    
    /**
     * Execute batch command
     */
    int executeBatchCommand(const BatchConfig& config);
    
    /**
     * Batch extract multiple archives
     */
    std::vector<BatchResult> batchExtract(const BatchConfig& config);
    
    /**
     * Batch pack multiple directories/files
     */
    std::vector<BatchResult> batchPack(const BatchConfig& config);
    
    /**
     * Batch convert archives between formats
     */
    std::vector<BatchResult> batchConvert(const BatchConfig& config);
    
    /**
     * Find all archive files in directory (recursive)
     */
    std::vector<std::filesystem::path> findArchiveFiles(
        const std::filesystem::path& directory,
        bool recursive = false,
        const std::vector<std::string>& include_patterns = {},
        const std::vector<std::string>& exclude_patterns = {}
    );
    
    /**
     * Print batch operation summary
     */
    void printBatchSummary(const std::vector<BatchResult>& results);
}


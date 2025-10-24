#pragma once

#include <CLI/CLI.hpp>
#include <filesystem>
#include <string>
#include <vector>

namespace FluxCLI::Commands {
    /**
     * Smart command configuration
     */
    struct SmartConfig {
        std::vector<std::filesystem::path> inputs;    // Input files/paths
        std::filesystem::path output;                 // Output path (optional)
        std::string password;                         // Password for protected archives
        bool recursive = false;                       // Process directories recursively
        bool overwrite = false;                       // Overwrite existing files
        bool verbose = false;                         // Verbose mode
        bool quiet = false;                           // Quiet mode
        bool dry_run = false;                         // Show what would be done without doing it
        
        void validate();
    };
    
    /**
     * Detected operation type
     */
    enum class SmartOperation {
        EXTRACT,        // Extract archive(s)
        PACK,           // Create archive from files/directories
        CONVERT,        // Convert between archive formats
        LIST,           // List archive contents
        TEST,           // Test archive integrity
        UNKNOWN         // Cannot determine operation
    };
    
    /**
     * Smart operation result
     */
    struct SmartResult {
        SmartOperation operation;
        std::string operation_description;
        std::vector<std::string> detected_formats;
        bool success = false;
        std::string message;
    };
    
    /**
     * Setup smart command
     */
    void setupSmartCommand(CLI::App* app, bool& verbose, bool& quiet);
    
    /**
     * Execute smart command with auto-detection
     */
    int executeSmartCommand(const SmartConfig& config);
    
    /**
     * Detect the most appropriate operation based on inputs
     */
    SmartOperation detectOperation(const SmartConfig& config);
    
    /**
     * Analyze input and suggest the best operation
     */
    SmartResult analyzeAndSuggest(const SmartConfig& config);
    
    /**
     * Execute detected operation
     */
    bool executeDetectedOperation(const SmartConfig& config, SmartOperation operation);
    
    /**
     * Check if path is an archive file
     */
    bool isArchiveFile(const std::filesystem::path& path);
    
    /**
     * Detect archive format from file
     */
    std::string detectArchiveFormat(const std::filesystem::path& path);
    
    /**
     * Get recommended output path for operation
     */
    std::filesystem::path getRecommendedOutput(const SmartConfig& config, SmartOperation operation);
    
    /**
     * Print operation analysis and ask for confirmation
     */
    bool confirmOperation(const SmartResult& result, const SmartConfig& config);
}


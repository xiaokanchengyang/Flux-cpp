#pragma once

#include <CLI/CLI.hpp>
#include <flux-core/archive.h>
#include <filesystem>
#include <vector>
#include <string>

namespace FluxCLI::Commands {
    /**
     * Universal auto command configuration
     * Automatically detects operation type and format
     */
    struct AutoConfig {
        std::vector<std::filesystem::path> inputs;    // Input files/directories
        std::filesystem::path output_dir;             // Output directory for extraction
        std::filesystem::path output_file;            // Output file for packing
        std::string operation;                        // Operation type: auto, extract, pack
        std::string password;                         // Password for protected archives
        bool recursive = false;                       // Process directories recursively
        bool overwrite = false;                       // Overwrite existing files
        bool preserve_structure = true;               // Preserve directory structure
        bool verbose = false;                         // Verbose mode
        bool quiet = false;                           // Quiet mode
        
        // Validation
        void validate();
        
        // Auto-detect operation based on inputs
        std::string detectOperation() const;
    };
    
    /**
     * Setup auto command
     * @param app CLI application
     * @param verbose Global verbose flag
     * @param quiet Global quiet flag
     */
    void setupAutoCommand(CLI::App* app, bool& verbose, bool& quiet);
    
    /**
     * Execute auto command
     * @param config Auto command configuration
     * @return Exit code
     */
    int executeAutoCommand(const AutoConfig& config);
    
    /**
     * Smart extract: extract any archive format to specified directory
     * @param archive_path Archive file path
     * @param output_dir Output directory
     * @param password Optional password
     * @param overwrite Whether to overwrite existing files
     * @return Success status
     */
    bool smartExtract(const std::filesystem::path& archive_path,
                     const std::filesystem::path& output_dir,
                     const std::string& password = "",
                     bool overwrite = false);
    
    /**
     * Smart pack: automatically choose best format and settings
     * @param inputs Input files/directories
     * @param output_file Output archive file
     * @param password Optional password
     * @return Success status
     */
    bool smartPack(const std::vector<std::filesystem::path>& inputs,
                  const std::filesystem::path& output_file,
                  const std::string& password = "");
}


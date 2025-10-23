#pragma once
#include <string>
#include <vector>
#include <filesystem>

namespace Flux {
    /**
     * Supported archive formats
     */
    enum class ArchiveFormat { 
        ZIP,        // ZIP format
        TAR_ZSTD,   // TAR + Zstandard compression
        TAR_GZ,     // TAR + Gzip compression
        TAR_XZ,     // TAR + XZ compression
        SEVEN_ZIP   // 7-Zip format
    };

    /**
     * File overwrite mode
     */
    enum class OverwriteMode { 
        SKIP,       // Skip existing files
        OVERWRITE,  // Overwrite existing files
        PROMPT      // Prompt user for choice
    };

    /**
     * Compression options configuration
     */
    struct PackOptions {
        ArchiveFormat format = ArchiveFormat::TAR_ZSTD;  // Default format
        int compression_level = 3;                        // Compression level (0-9)
        int num_threads = 0;                             // Thread count (0 = auto)
        bool preserve_permissions = true;                 // Preserve file permissions
        bool preserve_timestamps = true;                  // Preserve timestamps
        std::string password;                            // Password protection (optional)
        
        // Validate compression level
        bool isCompressionLevelValid() const {
            return compression_level >= 0 && compression_level <= 9;
        }
    };

    /**
     * Extraction options configuration
     */
    struct ExtractOptions {
        OverwriteMode overwrite_mode = OverwriteMode::SKIP;  // Overwrite mode
        bool hoist_single_folder = false;                   // Hoist single folder
        bool preserve_permissions = true;                    // Preserve file permissions
        bool preserve_timestamps = true;                     // Preserve timestamps
        std::string password;                               // Password (if required)
        std::vector<std::string> include_patterns;          // Include patterns
        std::vector<std::string> exclude_patterns;          // Exclude patterns
    };

    /**
     * Archive file information
     */
    struct ArchiveInfo {
        std::filesystem::path path;          // Archive file path
        ArchiveFormat format;                // Format
        size_t compressed_size;              // Compressed size
        size_t uncompressed_size;            // Original size
        size_t file_count;                   // File count
        bool is_encrypted;                   // Whether encrypted
        std::string creation_time;           // Creation time
    };

    /**
     * Archive entry information
     */
    struct ArchiveEntry {
        std::string name;                    // File name
        std::filesystem::path path;          // Relative path
        size_t compressed_size;              // Compressed size
        size_t uncompressed_size;            // Original size
        bool is_directory;                   // Whether is directory
        std::string modification_time;       // Modification time
        uint32_t permissions;                // File permissions
    };
}


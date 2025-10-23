#pragma once

/**
 * Flux-Core - High-performance cross-platform archive library
 * 
 * This is the core library of the Flux archive tool, providing complete compression and decompression functionality.
 * Supports multiple formats: ZIP, TAR+ZSTD, TAR+GZ, TAR+XZ, 7-Zip
 * 
 * Key features:
 * - Multi-threaded compression/decompression
 * - Progress callback support
 * - Password protection
 * - Integrity verification
 * - Partial extraction
 * - Cross-platform compatibility
 */

#include <map>
#include <string>

// Core data structures and enums
#include "archive.h"

// Exception handling
#include "exceptions.h"

// Packer interface
#include "packer.h"

// Extractor interface
#include "extractor.h"

namespace Flux {
    /**
     * Library version information
     */
    struct Version {
        static constexpr int MAJOR = 1;
        static constexpr int MINOR = 0;
        static constexpr int PATCH = 0;
        
        static std::string toString() {
            return std::to_string(MAJOR) + "." + 
                   std::to_string(MINOR) + "." + 
                   std::to_string(PATCH);
        }
    };

    /**
     * Initialize the Flux library
     * This function must be called before using other library features
     */
    void initialize();

    /**
     * Cleanup Flux library resources
     * Call this function before program exit to clean up resources
     */
    void cleanup();

    /**
     * Get library version string
     * @return Version string (e.g., "1.0.0")
     */
    std::string getVersion();

    /**
     * Get supported format information
     * @return Mapping of format names and descriptions
     */
    std::map<ArchiveFormat, std::string> getSupportedFormatsInfo();
}


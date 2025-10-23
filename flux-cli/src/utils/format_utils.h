#pragma once

#include <flux-core/archive.h>
#include <string>
#include <filesystem>
#include <vector>

namespace FluxCLI::Utils {
    /**
     * Format detection and conversion utilities
     */
    class FormatUtils {
    public:
        /**
         * Infer archive format from file extension
         * @param filename File name or path
         * @return Inferred format, throws exception if unable to infer
         */
        static Flux::ArchiveFormat detectFormatFromExtension(const std::filesystem::path& filename);
        
        /**
         * Detect archive format from file content
         * @param filepath File path
         * @return Detected format, throws exception if unable to detect
         */
        static Flux::ArchiveFormat detectFormatFromContent(const std::filesystem::path& filepath);
        
        /**
         * Convert format string to enum
         * @param format_str Format string (e.g., "zip", "tar.gz")
         * @return Corresponding format enum
         */
        static Flux::ArchiveFormat parseFormatString(const std::string& format_str);
        
        /**
         * Get default file extension for format
         * @param format Archive format
         * @return Default extension (e.g., ".zip", ".tar.gz")
         */
        static std::string getDefaultExtension(Flux::ArchiveFormat format);
        
        /**
         * Get list of all supported format strings
         * @return List of format strings
         */
        static std::vector<std::string> getSupportedFormatStrings();
        
        /**
         * Validate if compression level is valid for specified format
         * @param format Archive format
         * @param level Compression level
         * @return Whether valid
         */
        static bool isCompressionLevelValid(Flux::ArchiveFormat format, int level);
        
        /**
         * Get recommended compression level range for format
         * @param format Archive format
         * @return {min level, max level, default level}
         */
        static std::tuple<int, int, int> getCompressionLevelRange(Flux::ArchiveFormat format);
        
        /**
         * Format file size to human-readable format
         * @param bytes Number of bytes
         * @param binary Whether to use binary units (1024) instead of decimal units (1000)
         * @return Formatted string
         */
        static std::string formatFileSize(size_t bytes, bool binary = true);
        
        /**
         * Format duration
         * @param milliseconds Number of milliseconds
         * @return Formatted string
         */
        static std::string formatDuration(size_t milliseconds);
        
        /**
         * Format compression ratio
         * @param original_size Original size
         * @param compressed_size Compressed size
         * @return Formatted compression ratio string (e.g., "65.2%")
         */
        static std::string formatCompressionRatio(size_t original_size, size_t compressed_size);
    };
    
    /**
     * File path utilities
     */
    class PathUtils {
    public:
        /**
         * Normalize path separators (use forward slashes consistently)
         * @param path Input path
         * @return Normalized path
         */
        static std::string normalizePath(const std::string& path);
        
        /**
         * Check if path is absolute
         * @param path Path to check
         * @return Whether path is absolute
         */
        static bool isAbsolutePath(const std::string& path);
        
        /**
         * Get relative path relative to base directory
         * @param path Target path
         * @param base Base directory
         * @return Relative path
         */
        static std::filesystem::path getRelativePath(
            const std::filesystem::path& path, 
            const std::filesystem::path& base
        );
        
        /**
         * Safely join path components (prevent path traversal attacks)
         * @param base Base path
         * @param component Component to add
         * @return Safely joined path
         */
        static std::filesystem::path safeJoinPath(
            const std::filesystem::path& base,
            const std::string& component
        );
    };
}


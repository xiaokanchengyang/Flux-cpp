#pragma once

#include <array>
#include <string_view>
#include <chrono>

namespace Flux::Constants {
    // Version information
    inline constexpr int VERSION_MAJOR = 1;
    inline constexpr int VERSION_MINOR = 0;
    inline constexpr int VERSION_PATCH = 0;
    inline constexpr std::string_view VERSION_STRING = "1.0.0";

    // Default compression settings
    inline constexpr int DEFAULT_COMPRESSION_LEVEL = 3;
    inline constexpr int MIN_COMPRESSION_LEVEL = 0;
    inline constexpr int MAX_COMPRESSION_LEVEL = 9;
    inline constexpr int DEFAULT_THREAD_COUNT = 0; // Auto-detect

    // Buffer sizes (in bytes)
    inline constexpr size_t DEFAULT_BUFFER_SIZE = 64 * 1024;      // 64KB
    inline constexpr size_t LARGE_BUFFER_SIZE = 1024 * 1024;      // 1MB
    inline constexpr size_t MAX_BUFFER_SIZE = 16 * 1024 * 1024;   // 16MB

    // File size thresholds
    inline constexpr size_t SMALL_FILE_THRESHOLD = 1024 * 1024;   // 1MB
    inline constexpr size_t LARGE_FILE_THRESHOLD = 100 * 1024 * 1024; // 100MB

    // Progress reporting
    inline constexpr float PROGRESS_UPDATE_INTERVAL = 0.01f;      // 1% intervals
    inline constexpr auto PROGRESS_UPDATE_TIME = std::chrono::milliseconds(100);

    // Archive format extensions
    inline constexpr std::array<std::string_view, 5> ARCHIVE_EXTENSIONS = {
        ".zip", ".tar.zst", ".tar.gz", ".tar.xz", ".7z"
    };

    // Compression ratio estimates (for size estimation)
    namespace CompressionRatios {
        inline constexpr double ZIP = 0.6;
        inline constexpr double TAR_ZSTD = 0.4;
        inline constexpr double TAR_GZ = 0.5;
        inline constexpr double TAR_XZ = 0.3;
        inline constexpr double SEVEN_ZIP = 0.35;
        inline constexpr double DEFAULT = 0.5;
    }

    // Error messages
    namespace ErrorMessages {
        inline constexpr std::string_view NO_INPUT_FILES = "No input files specified";
        inline constexpr std::string_view FILE_NOT_FOUND = "File or directory not found";
        inline constexpr std::string_view ACCESS_DENIED = "Cannot access file";
        inline constexpr std::string_view UNSUPPORTED_FORMAT = "Unsupported archive format";
        inline constexpr std::string_view INVALID_COMPRESSION_LEVEL = "Invalid compression level";
        inline constexpr std::string_view OPERATION_CANCELLED = "Operation was cancelled";
        inline constexpr std::string_view INSUFFICIENT_SPACE = "Insufficient disk space";
    }

    // File patterns
    namespace Patterns {
        inline constexpr std::array<std::string_view, 8> COMMON_EXCLUDE_PATTERNS = {
            "*.tmp", "*.temp", "*.log", "*.bak", "*.swp", "~*", ".DS_Store", "Thumbs.db"
        };
        
        inline constexpr std::array<std::string_view, 6> BACKUP_PATTERNS = {
            "*.bak", "*.backup", "*.old", "*~", "*.orig", "*.save"
        };
    }

    // Resource paths (can be customized per platform)
    namespace Paths {
        inline constexpr std::string_view CONFIG_DIR = ".flux";
        inline constexpr std::string_view TEMP_DIR_PREFIX = "flux_temp_";
        inline constexpr std::string_view LOG_FILE = "flux.log";
        inline constexpr std::string_view CONFIG_FILE = "flux.conf";
    }

    // Performance tuning
    namespace Performance {
        inline constexpr size_t MIN_PARALLEL_SIZE = 10 * 1024 * 1024;  // 10MB
        inline constexpr int MAX_WORKER_THREADS = 16;
        inline constexpr int IO_QUEUE_DEPTH = 32;
        inline constexpr size_t MEMORY_LIMIT_MB = 512;  // 512MB default memory limit
    }
}

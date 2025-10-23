#pragma once
#include "archive.h"
#include "compat.h"
#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <ranges>
#include <algorithm>
#include <span>
#include <filesystem>
#include <chrono>
#include <array>

namespace Flux {
    /**
     * Progress callback function type
     * Parameters: (current file name, progress 0.0-1.0, processed bytes, total bytes)
     */
    using ProgressCallback = std::function<void(std::string_view, float, size_t, size_t)>;

    /**
     * Error callback function type  
     * Parameters: (error message, is fatal error)
     */
    using ErrorCallback = std::function<void(std::string_view, bool)>;

    /**
     * Packing result structure
     */
    struct PackResult {
        bool success{false};                          // Success status
        std::string error_message{};                  // Error message if failed
        size_t files_processed{0};                    // Number of files processed
        size_t total_compressed_size{0};              // Total compressed size
        size_t total_uncompressed_size{0};            // Total original size  
        double compression_ratio{0.0};                // Compression ratio
        std::chrono::milliseconds duration{0};        // Processing duration
        
        // Modern C++20 spaceship operator for comparison
        auto operator<=>(const PackResult&) const = default;
    };

    /**
     * Abstract packer interface using modern C++ features
     */
    class Packer {
    public:
        virtual ~Packer() = default;

        /**
         * Pack files and folders into an archive
         * @param inputs Input file/folder paths
         * @param output Output archive path
         * @param options Packing options
         * @param on_progress Progress callback (optional)
         * @param on_error Error callback (optional)
         * @return Packing result
         */
        virtual PackResult pack(
            std::span<const std::filesystem::path> inputs,
            const std::filesystem::path& output,
            const PackOptions& options,
            const ProgressCallback& on_progress = nullptr,
            const ErrorCallback& on_error = nullptr
        ) = 0;

        /**
         * Validate that input files exist and are readable
         * @param inputs Input paths
         * @return Validation result using std::expected (C++23)
         */
        virtual Flux::expected<void, std::string> validateInputs(
            std::span<const std::filesystem::path> inputs
        ) const;

        /**
         * Estimate compressed size (rough estimation)
         * @param inputs Input paths
         * @param format Archive format
         * @return Estimated compressed size in bytes
         */
        virtual std::optional<size_t> estimateCompressedSize(
            std::span<const std::filesystem::path> inputs,
            ArchiveFormat format
        ) const;

        /**
         * Cancel current packing operation
         */
        virtual void cancel() = 0;

        /**
         * Check if the specified format is supported
         * @param format Archive format
         * @return Whether the format is supported
         */
        virtual bool supportsFormat(ArchiveFormat format) const = 0;
    };

    /**
     * Create a packer instance suitable for the specified format
     * @param format Archive format
     * @return Smart pointer to packer instance
     */
    [[nodiscard]] std::unique_ptr<Packer> createPacker(ArchiveFormat format);

    /**
     * Get list of all supported formats
     * @return List of supported formats
     */
    [[nodiscard]] constexpr std::array<ArchiveFormat, 5> getSupportedFormats() noexcept {
        return {
            ArchiveFormat::ZIP,
            ArchiveFormat::TAR_ZSTD,
            ArchiveFormat::TAR_GZ,
            ArchiveFormat::TAR_XZ,
            ArchiveFormat::SEVEN_ZIP
        };
    }

    /**
     * Convert format enum to string using functional approach
     * @param format Format enum
     * @return Format string
     */
    [[nodiscard]] constexpr std::string_view formatToString(ArchiveFormat format) noexcept {
        using enum ArchiveFormat;
        switch (format) {
            case ZIP: return "zip";
            case TAR_ZSTD: return "tar.zst";
            case TAR_GZ: return "tar.gz";
            case TAR_XZ: return "tar.xz";
            case SEVEN_ZIP: return "7z";
            default: return "unknown";
        }
    }

    /**
     * Convert string to format enum with error handling
     * @param format_str Format string
     * @return Format enum wrapped in expected (C++23)
     */
    [[nodiscard]] Flux::expected<ArchiveFormat, std::string> stringToFormat(std::string_view format_str) noexcept;
}


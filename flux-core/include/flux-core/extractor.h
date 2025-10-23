#pragma once
#include "archive.h"
#include "compat.h"
#include "packer.h"  // For ProgressCallback and ErrorCallback
#include <functional>
#include <memory>
#include <chrono>
#include <optional>
#include <string_view>
#include <span>

namespace Flux {
    /**
     * Extraction result structure
     */
    struct ExtractResult {
        bool success{false};                          // Success status
        std::string error_message{};                  // Error message if failed
        size_t files_extracted{0};                    // Number of files extracted
        size_t total_size{0};                         // Total extracted size
        std::chrono::milliseconds duration{0};        // Processing duration
        std::vector<std::string> skipped_files{};     // List of skipped files
        
        // Modern C++20 spaceship operator for comparison
        auto operator<=>(const ExtractResult&) const = default;
    };

    /**
     * Abstract extractor interface using modern C++ features
     */
    class Extractor {
    public:
        virtual ~Extractor() = default;

        /**
         * Extract archive file
         * @param archive_path Archive file path
         * @param output_dir Output directory
         * @param options Extraction options
         * @param on_progress Progress callback (optional)
         * @param on_error Error callback (optional)
         * @return Extraction result
         */
        virtual ExtractResult extract(
            const std::filesystem::path& archive_path,
            const std::filesystem::path& output_dir,
            const ExtractOptions& options,
            const ProgressCallback& on_progress = nullptr,
            const ErrorCallback& on_error = nullptr
        ) = 0;

        /**
         * Partial extraction - extract only specified files
         * @param archive_path Archive file path
         * @param output_dir Output directory
         * @param file_patterns File patterns to extract
         * @param options Extraction options
         * @param on_progress Progress callback (optional)
         * @param on_error Error callback (optional)
         * @return Extraction result
         */
        virtual ExtractResult extractPartial(
            const std::filesystem::path& archive_path,
            const std::filesystem::path& output_dir,
            std::span<const std::string> file_patterns,
            const ExtractOptions& options,
            const ProgressCallback& on_progress = nullptr,
            const ErrorCallback& on_error = nullptr
        ) = 0;

        /**
         * List archive contents
         * @param archive_path Archive file path
         * @param password Password (if required)
         * @return Archive entry list wrapped in expected
         */
        virtual Flux::expected<std::vector<ArchiveEntry>, std::string> listContents(
            const std::filesystem::path& archive_path,
            std::string_view password = ""
        ) = 0;

        /**
         * Get archive file information
         * @param archive_path Archive file path
         * @param password Password (if required)
         * @return Archive information wrapped in expected
         */
        virtual Flux::expected<ArchiveInfo, std::string> getArchiveInfo(
            const std::filesystem::path& archive_path,
            std::string_view password = ""
        ) = 0;

        /**
         * Verify archive file integrity
         * @param archive_path Archive file path
         * @param password Password (if required)
         * @return Verification result using expected
         */
        virtual Flux::expected<void, std::string> verifyIntegrity(
            const std::filesystem::path& archive_path,
            std::string_view password = ""
        ) = 0;

        /**
         * Detect archive file format
         * @param archive_path Archive file path
         * @return Detected format wrapped in expected
         */
        virtual Flux::expected<ArchiveFormat, std::string> detectFormat(
            const std::filesystem::path& archive_path
        ) = 0;

        /**
         * Cancel current extraction operation
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
     * Create extractor instance suitable for the specified format
     * @param format Archive format
     * @return Smart pointer to extractor instance
     */
    [[nodiscard]] std::unique_ptr<Extractor> createExtractor(ArchiveFormat format);

    /**
     * Auto-detect format and create extractor
     * @param archive_path Archive file path
     * @return Smart pointer to extractor instance wrapped in expected
     */
    [[nodiscard]] Flux::expected<std::unique_ptr<Extractor>, std::string> createExtractorAuto(
        const std::filesystem::path& archive_path
    ) noexcept;
}


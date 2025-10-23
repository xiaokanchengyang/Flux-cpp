#include "flux-core/packer.h"
#include "flux-core/exceptions.h"
#include "flux-core/constants.h"
#include <filesystem>
#include <iostream>
#include <ranges>
#include <algorithm>
#include <numeric>
#include "flux-core/compat.h"
#include <cctype>
#include <span>
#include <string>
#include <vector>

// Forward declarations for format implementation classes
namespace Flux::Formats {
    class ZipPacker;
    class TarPacker;
    class SevenZipPacker;
}

// Forward declarations for format implementation classes
namespace Flux::Formats {
    std::unique_ptr<Packer> createZipPacker();
    std::unique_ptr<Packer> createTarPacker();
    std::unique_ptr<Packer> createSevenZipPacker();
}

namespace Flux {
    // Default implementation of Packer base class using modern C++23 features
    Flux::expected<void, std::string> Packer::validateInputs(
        std::span<const std::filesystem::path> inputs) const {
        
        if (inputs.empty()) {
            return Flux::unexpected<std::string>{std::string{Constants::ErrorMessages::NO_INPUT_FILES}};
        }

        // Use ranges and algorithms for functional programming approach
        auto invalid_file = std::ranges::find_if(inputs, [](const auto& input) {
            return !std::filesystem::exists(input);
        });

        if (invalid_file != inputs.end()) {
            return Flux::unexpected<std::string>{Flux::format("{}: {}", 
                                             Constants::ErrorMessages::FILE_NOT_FOUND,
                                             invalid_file->string())};
        }

        // Check read permissions using ranges
        auto inaccessible_file = std::ranges::find_if(inputs, [](const auto& input) {
            std::error_code ec;
            [[maybe_unused]] auto status = std::filesystem::status(input, ec);
            return static_cast<bool>(ec);
        });

        if (inaccessible_file != inputs.end()) {
            std::error_code ec;
            [[maybe_unused]] auto status = std::filesystem::status(*inaccessible_file, ec);
            return Flux::unexpected<std::string>{std::format("{}: {} ({})", 
                                             Constants::ErrorMessages::ACCESS_DENIED,
                                             inaccessible_file->string(), 
                                             ec.message())};
        }

        return {};
    }

    std::optional<size_t> Packer::estimateCompressedSize(
        std::span<const std::filesystem::path> inputs,
        ArchiveFormat format) const {
        
        // Calculate total size using functional programming approach
        auto calculate_file_size = [](const std::filesystem::path& path) -> size_t {
            std::error_code ec;
            if (std::filesystem::is_regular_file(path, ec) && !ec) {
                return std::filesystem::file_size(path, ec);
            } else if (std::filesystem::is_directory(path, ec) && !ec) {
                return Flux::fold_left(
                    std::filesystem::recursive_directory_iterator(path, ec)
                    | std::views::filter([](const auto& entry) {
                        std::error_code ec;
                        return entry.is_regular_file(ec);
                    })
                    | std::views::transform([](const auto& entry) {
            std::error_code ec;
                        return entry.file_size(ec);
                    }),
                    size_t{0},
                    std::plus<size_t>{}
                );
            }
            return 0;
        };

        try {
            auto total_size = std::ranges::fold_left(
                inputs | std::views::transform(calculate_file_size),
                size_t{0},
                std::plus<size_t>{}
            );

            // Estimate compression ratio based on format using constants
            constexpr auto get_compression_ratio = [](ArchiveFormat fmt) constexpr -> double {
                using enum ArchiveFormat;
                switch (fmt) {
                    case ZIP: return Constants::CompressionRatios::ZIP;
                    case TAR_ZSTD: return Constants::CompressionRatios::TAR_ZSTD;
                    case TAR_GZ: return Constants::CompressionRatios::TAR_GZ;
                    case TAR_XZ: return Constants::CompressionRatios::TAR_XZ;
                    case SEVEN_ZIP: return Constants::CompressionRatios::SEVEN_ZIP;
                    default: return Constants::CompressionRatios::DEFAULT;
                }
            };

            return static_cast<size_t>(total_size * get_compression_ratio(format));
        } catch (const std::filesystem::filesystem_error&) {
            return std::nullopt;
        }
    }

    // Factory function implementation
    std::unique_ptr<Packer> createPacker(ArchiveFormat format) {
        switch (format) {
            case ArchiveFormat::ZIP:
                return Formats::createZipPacker();
            case ArchiveFormat::TAR_GZ:
            case ArchiveFormat::TAR_XZ:
            case ArchiveFormat::TAR_ZSTD:
                return Formats::createTarPacker();
            case ArchiveFormat::SEVEN_ZIP:
                return Formats::createSevenZipPacker();
            default:
                throw UnsupportedFormatException(Flux::format("Unsupported format: {}", 
                                                            formatToString(format)));
        }
    }

    // String to format conversion with error handling using C++23 features
    Flux::expected<ArchiveFormat, std::string> stringToFormat(std::string_view format_str) noexcept {
        using enum ArchiveFormat;
        
        // Convert to lowercase for case-insensitive comparison
        std::string lower_str;
        lower_str.reserve(format_str.size());
        std::ranges::transform(format_str, std::back_inserter(lower_str), 
                             [](char c) { return std::tolower(c); });

        // Use constexpr map for efficient lookup with comprehensive format support
        static constexpr std::array format_mappings = {
            std::pair{"zip", ZIP},
            std::pair{"tar.zst", TAR_ZSTD},
            std::pair{"tar.zstd", TAR_ZSTD},
            std::pair{"zst", TAR_ZSTD},
            std::pair{"zstd", TAR_ZSTD},
            std::pair{"tar.gz", TAR_GZ},
            std::pair{"tgz", TAR_GZ},
            std::pair{"gz", TAR_GZ},
            std::pair{"gzip", TAR_GZ},
            std::pair{"tar.xz", TAR_XZ},
            std::pair{"txz", TAR_XZ},
            std::pair{"xz", TAR_XZ},
            std::pair{"7z", SEVEN_ZIP},
            std::pair{"7zip", SEVEN_ZIP}
        };

        auto it = std::ranges::find_if(format_mappings, 
            [&lower_str](const auto& mapping) {
                return mapping.first == lower_str;
            });

        if (it != format_mappings.end()) {
            return it->second;
        }
        
        return Flux::unexpected<std::string>{std::format("{}: '{}'",
                                     Constants::ErrorMessages::UNSUPPORTED_FORMAT,
                                     format_str)};
    }
}


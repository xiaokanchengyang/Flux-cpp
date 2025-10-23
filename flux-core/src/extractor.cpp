#include "flux-core/extractor.h"
#include "flux-core/exceptions.h"
#include "flux-core/constants.h"
#include "flux-core/packer.h"  // For formatToString function
#include <filesystem>
#include <ranges>
#include <algorithm>
#include <format>

// Forward declarations for format implementation classes
namespace Flux::Formats {
    std::unique_ptr<Extractor> createZipExtractor();
    std::unique_ptr<Extractor> createTarExtractor();
    std::unique_ptr<Extractor> createSevenZipExtractor();
}

// Note: Format implementations should be linked separately, not included as .cpp files
// This is a temporary solution until proper library structure is implemented

namespace Flux {
    // Factory function implementation
    std::unique_ptr<Extractor> createExtractor(ArchiveFormat format) {
        switch (format) {
            case ArchiveFormat::ZIP:
                return Formats::createZipExtractor();
            case ArchiveFormat::TAR_GZ:
            case ArchiveFormat::TAR_XZ:
            case ArchiveFormat::TAR_ZSTD:
                return Formats::createTarExtractor();
            case ArchiveFormat::SEVEN_ZIP:
                return Formats::createSevenZipExtractor();
            default:
                throw UnsupportedFormatException(std::format("Unsupported format: {}", 
                                                            formatToString(format)));
        }
    }

    Flux::expected<std::unique_ptr<Extractor>, std::string> createExtractorAuto(
        const std::filesystem::path& archive_path) noexcept {
        
        if (!std::filesystem::exists(archive_path)) {
            return Flux::unexpected<std::string>{std::format("{}: {}", 
                                             Constants::ErrorMessages::FILE_NOT_FOUND,
                                             archive_path.string())};
        }

        // Detect format based on file extension using functional approach
        const std::string filename = archive_path.filename().string();
        
        // Use constexpr mapping for format detection
        constexpr std::array extension_mappings = {
            std::pair{".zip", ArchiveFormat::ZIP},
            std::pair{".7z", ArchiveFormat::SEVEN_ZIP},
            std::pair{".tar.gz", ArchiveFormat::TAR_GZ},
            std::pair{".tgz", ArchiveFormat::TAR_GZ},
            std::pair{".tar.xz", ArchiveFormat::TAR_XZ},
            std::pair{".txz", ArchiveFormat::TAR_XZ},
            std::pair{".tar.zst", ArchiveFormat::TAR_ZSTD},
            std::pair{".tar.zstd", ArchiveFormat::TAR_ZSTD}
        };

        auto detected_format = std::ranges::find_if(extension_mappings,
            [&filename](const auto& mapping) {
                return filename.ends_with(mapping.first);
            });

        if (detected_format == extension_mappings.end()) {
            // TODO: Implement file header detection logic
            return Flux::unexpected<std::string>{std::format("{}: {}", 
                                             Constants::ErrorMessages::UNSUPPORTED_FORMAT,
                                             filename)};
        }

        try {
            return createExtractor(detected_format->second);
        } catch (const std::exception& e) {
            return Flux::unexpected<std::string>{e.what()};
        }
    }
}


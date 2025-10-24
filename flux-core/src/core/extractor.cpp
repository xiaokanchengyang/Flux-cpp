#include "flux-core/extractor.h"
#include "flux-core/exceptions.h"
#include "flux-core/constants.h"
#include "flux-core/packer.h"  // For formatToString function
#include <filesystem>
#include <ranges>
#include <algorithm>
#include <format>
#include <fstream>
#include <cstring>

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
            // Try file header detection as fallback
            std::ifstream file(archive_path, std::ios::binary);
            if (file.is_open()) {
                char header[8];
                file.read(header, sizeof(header));
                file.close();
                
                // Check ZIP signature
                if (header[0] == 'P' && header[1] == 'K' && 
                    (header[2] == 0x03 || header[2] == 0x05 || header[2] == 0x07)) {
                    return ArchiveFormat::ZIP;
                }
                
                // Check TAR signature (ustar magic at offset 257, but we check first few bytes for common patterns)
                if (std::memcmp(header, "\x1f\x8b", 2) == 0) { // GZIP magic
                    return ArchiveFormat::TAR_GZ;
                }
                if (std::memcmp(header, "\xfd\x37\x7a\x58\x5a\x00", 6) == 0) { // XZ magic
                    return ArchiveFormat::TAR_XZ;
                }
                if (std::memcmp(header, "\x28\xb5\x2f\xfd", 4) == 0) { // ZSTD magic
                    return ArchiveFormat::TAR_ZSTD;
                }
            }
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


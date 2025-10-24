#include "flux-core/archive.h"
#include "flux-core/exceptions.h"
#include <fstream>
#include <vector>
#include <algorithm>
#include <ranges>
#include <array>

namespace Flux {
    namespace FormatDetector {
        /**
         * File format signature structure
         */
        struct FormatSignature {
            std::vector<uint8_t> signature;
            size_t offset;
            ArchiveFormat format;
            std::string description;
        };

        // Predefined format signatures
        static const std::vector<FormatSignature> SIGNATURES = {
            // ZIP format
            {{0x50, 0x4B, 0x03, 0x04}, 0, ArchiveFormat::ZIP, "ZIP Local File Header"},
            {{0x50, 0x4B, 0x05, 0x06}, 0, ArchiveFormat::ZIP, "ZIP End of Central Directory"},
            {{0x50, 0x4B, 0x07, 0x08}, 0, ArchiveFormat::ZIP, "ZIP Data Descriptor"},
            
            // 7-Zip format
            {{0x37, 0x7A, 0xBC, 0xAF, 0x27, 0x1C}, 0, ArchiveFormat::SEVEN_ZIP, "7-Zip"},
            
            // GZIP format (for TAR.GZ)
            {{0x1F, 0x8B}, 0, ArchiveFormat::TAR_GZ, "GZIP"},
            
            // XZ format (for TAR.XZ)
            {{0xFD, 0x37, 0x7A, 0x58, 0x5A, 0x00}, 0, ArchiveFormat::TAR_XZ, "XZ"},
            
            // ZSTD format (for TAR.ZSTD)
            {{0x28, 0xB5, 0x2F, 0xFD}, 0, ArchiveFormat::TAR_ZSTD, "ZSTD"},
        };

        /**
         * Check if file matches specified signature
         */
        bool matchesSignature(const std::vector<uint8_t>& data, 
                             const FormatSignature& signature) {
            if (data.size() < signature.offset + signature.signature.size()) {
                return false;
            }

            return std::equal(signature.signature.begin(), 
                            signature.signature.end(),
                            data.begin() + signature.offset);
        }

        /**
         * Detect archive format by analyzing file content
         */
        ArchiveFormat detectByContent(const std::filesystem::path& file_path) {
            std::ifstream file(file_path, std::ios::binary);
            if (!file.is_open()) {
                throw FileNotFoundException(file_path.string());
            }

            // Read file header data for format detection
            constexpr size_t BUFFER_SIZE = 512;
            std::vector<uint8_t> buffer(BUFFER_SIZE, 0);
            
            file.read(reinterpret_cast<char*>(buffer.data()), BUFFER_SIZE);
            size_t bytes_read = static_cast<size_t>(file.gcount());
            
            if (bytes_read < 4) {
                throw CorruptedArchiveException("File too small: " + file_path.string());
            }

            // Check all known format signatures
            for (const auto& signature : SIGNATURES) {
                if (matchesSignature(buffer, signature)) {
                    return signature.format;
                }
            }

            // Special handling: check for uncompressed TAR files
            // TAR files have ustar identifier at offset 257
            if (bytes_read >= 262) {
                std::string ustar_check(reinterpret_cast<char*>(buffer.data() + 257), 5);
                if (ustar_check == "ustar") {
                    // This is an uncompressed TAR file, but we don't support it directly
                    // Recommend users to use compressed TAR formats
                    throw UnsupportedFormatException("Uncompressed TAR format not supported. Use TAR.GZ, TAR.XZ, or TAR.ZSTD instead.");
                }
            }

            throw UnsupportedFormatException("Unknown archive format: " + file_path.string());
        }

        /**
         * Detect archive format by file extension
         */
        ArchiveFormat detectByExtension(const std::filesystem::path& file_path) {
            auto filename = file_path.filename().string();
            auto extension = file_path.extension().string();
            
            // Convert to lowercase using ranges (C++20)
            std::ranges::transform(filename, filename.begin(), ::tolower);
            std::ranges::transform(extension, extension.begin(), ::tolower);

            // Use constexpr map for extension mapping
            constexpr auto extensionMap = std::array{
                std::pair{".tar.gz", ArchiveFormat::TAR_GZ},
                std::pair{".tgz", ArchiveFormat::TAR_GZ},
                std::pair{".tar.xz", ArchiveFormat::TAR_XZ},
                std::pair{".txz", ArchiveFormat::TAR_XZ},
                std::pair{".tar.zst", ArchiveFormat::TAR_ZSTD},
                std::pair{".tar.zstd", ArchiveFormat::TAR_ZSTD},
                std::pair{".zip", ArchiveFormat::ZIP},
                std::pair{".7z", ArchiveFormat::SEVEN_ZIP}
            };
            
            // Check compound extensions first
            for (const auto& [ext, format] : extensionMap) {
                if (filename.ends_with(ext)) {
                    return format;
                }
            }

            throw UnsupportedFormatException("Cannot determine format from extension: " + filename);
        }

        /**
         * Comprehensive format detection (prioritize content detection)
         */
        ArchiveFormat detectFormat(const std::filesystem::path& file_path) {
            if (!std::filesystem::exists(file_path)) {
                throw FileNotFoundException(file_path.string());
            }

            try {
                // First try content-based detection
                return detectByContent(file_path);
            } catch (const UnsupportedFormatException&) {
                // If content detection fails, try extension-based detection
                try {
                    return detectByExtension(file_path);
                } catch (const UnsupportedFormatException&) {
                    // Both methods failed
                    throw UnsupportedFormatException("Cannot determine archive format for: " + file_path.string());
                }
            }
        }

        /**
         * Get detailed information about the archive format
         */
        std::string getFormatDescription(ArchiveFormat format) {
            switch (format) {
                case ArchiveFormat::ZIP:
                    return "ZIP Archive - Universal compression format with wide compatibility";
                case ArchiveFormat::TAR_GZ:
                    return "TAR+GZIP - Traditional Unix compression format";
                case ArchiveFormat::TAR_XZ:
                    return "TAR+XZ - High compression ratio format";
                case ArchiveFormat::TAR_ZSTD:
                    return "TAR+ZSTD - High performance compression format (recommended)";
                case ArchiveFormat::SEVEN_ZIP:
                    return "7-Zip - Professional high compression ratio format";
                default:
                    return "Unknown format";
            }
        }
    }
}


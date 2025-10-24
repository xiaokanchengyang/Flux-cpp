#include "flux-core/archive.h"
#include <fstream>
#include <vector>

namespace Flux {
    namespace Utils {
        /**
         * Detect file format by magic number (file header)
         */
        ArchiveFormat detectFormatByMagic(const std::filesystem::path& file_path) {
            std::ifstream file(file_path, std::ios::binary);
            if (!file.is_open()) {
                throw std::runtime_error("Cannot open file: " + file_path.string());
            }

            // Read first 16 bytes for format detection
            std::vector<uint8_t> header(16, 0);
            file.read(reinterpret_cast<char*>(header.data()), header.size());
            
            if (file.gcount() < 4) {
                throw std::runtime_error("File too small to determine format");
            }

            // ZIP format detection (PK signature)
            if (header[0] == 0x50 && header[1] == 0x4B) {
                return ArchiveFormat::ZIP;
            }

            // 7-Zip format detection
            if (header[0] == 0x37 && header[1] == 0x7A && 
                header[2] == 0xBC && header[3] == 0xAF && 
                header[4] == 0x27 && header[5] == 0x1C) {
                return ArchiveFormat::SEVEN_ZIP;
            }

            // TAR format detection (via ustar identifier at offset 257)
            file.seekg(257);
            std::vector<char> ustar(6);
            file.read(ustar.data(), 5);
            ustar[5] = '\0';
            
            if (std::string(ustar.data()) == "ustar") {
                // This is a TAR file, but need to check compression type
                // Since compressed TAR files have magic numbers at the beginning, we need to recheck
                file.seekg(0);
                file.read(reinterpret_cast<char*>(header.data()), 16);
                
                // GZIP magic number
                if (header[0] == 0x1F && header[1] == 0x8B) {
                    return ArchiveFormat::TAR_GZ;
                }
                
                // XZ magic number
                if (header[0] == 0xFD && header[1] == 0x37 && 
                    header[2] == 0x7A && header[3] == 0x58 && 
                    header[4] == 0x5A && header[5] == 0x00) {
                    return ArchiveFormat::TAR_XZ;
                }
                
                // ZSTD magic number
                if (header[0] == 0x28 && header[1] == 0xB5 && 
                    header[2] == 0x2F && header[3] == 0xFD) {
                    return ArchiveFormat::TAR_ZSTD;
                }
            }

            throw std::runtime_error("Unknown or unsupported archive format");
        }

        /**
         * Get MIME type for the given archive format
         */
        std::string getMimeType(ArchiveFormat format) {
            switch (format) {
                case ArchiveFormat::ZIP:
                    return "application/zip";
                case ArchiveFormat::TAR_GZ:
                    return "application/gzip";
                case ArchiveFormat::TAR_XZ:
                    return "application/x-xz";
                case ArchiveFormat::TAR_ZSTD:
                    return "application/zstd";
                case ArchiveFormat::SEVEN_ZIP:
                    return "application/x-7z-compressed";
                default:
                    return "application/octet-stream";
            }
        }

        /**
         * Get recommended file extension for the given archive format
         */
        std::string getRecommendedExtension(ArchiveFormat format) {
            switch (format) {
                case ArchiveFormat::ZIP:
                    return ".zip";
                case ArchiveFormat::TAR_GZ:
                    return ".tar.gz";
                case ArchiveFormat::TAR_XZ:
                    return ".tar.xz";
                case ArchiveFormat::TAR_ZSTD:
                    return ".tar.zst";
                case ArchiveFormat::SEVEN_ZIP:
                    return ".7z";
                default:
                    return ".archive";
            }
        }

        /**
         * Validate if compression level is valid for the given format
         */
        bool isCompressionLevelValid(ArchiveFormat format, int level) {
            // Use constexpr map for format-specific max levels
            constexpr auto getMaxLevel = [](ArchiveFormat fmt) constexpr -> int {
                switch (fmt) {
                    case ArchiveFormat::ZIP:
                    case ArchiveFormat::TAR_GZ:
                    case ArchiveFormat::TAR_XZ:
                    case ArchiveFormat::SEVEN_ZIP:
                        return 9;
                    case ArchiveFormat::TAR_ZSTD:
                        return 22; // ZSTD supports higher compression levels
                    default:
                        return -1; // Invalid format
                }
            };
            
            const auto maxLevel = getMaxLevel(format);
            return maxLevel >= 0 && level >= 0 && level <= maxLevel;
        }
    }
}


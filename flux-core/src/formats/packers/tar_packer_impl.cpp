#include "flux-core/packer.h"
#include "flux-core/exceptions.h"
#include "flux-core/constants.h"
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace Flux {
    namespace Formats {
        /**
         * TAR format packer implementation with basic TAR structure
         * Note: This is a basic implementation without compression libraries
         * For production use, integrate with libarchive or similar
         */
        class TarPackerImpl : public Packer {
        private:
            bool m_cancelled = false;
            ArchiveFormat m_format = ArchiveFormat::TAR_GZ;

            // TAR header structure (POSIX.1-1988 format)
            struct TarHeader {
                char name[100];      // File name
                char mode[8];        // File mode (octal)
                char uid[8];         // Owner user ID (octal)
                char gid[8];         // Owner group ID (octal)
                char size[12];       // File size (octal)
                char mtime[12];      // Modification time (octal)
                char checksum[8];    // Header checksum (octal)
                char typeflag;       // File type
                char linkname[100];  // Link name
                char magic[6];       // Magic number "ustar"
                char version[2];     // Version "00"
                char uname[32];      // Owner user name
                char gname[32];      // Owner group name
                char devmajor[8];    // Device major number
                char devminor[8];    // Device minor number
                char prefix[155];    // Path prefix
                char padding[12];    // Padding to 512 bytes
            };

            static_assert(sizeof(TarHeader) == 512, "TAR header must be exactly 512 bytes");

        public:
            explicit TarPackerImpl(ArchiveFormat format = ArchiveFormat::TAR_GZ) : m_format(format) {}

            PackResult pack(
                std::span<const std::filesystem::path> inputs,
                const std::filesystem::path& output,
                const PackOptions& options,
                const ProgressCallback& on_progress = nullptr,
                const ErrorCallback& on_error = nullptr) override {
                
                auto start_time = std::chrono::high_resolution_clock::now();
                PackResult result;
                result.success = false;
                result.files_processed = 0;
                result.total_compressed_size = 0;
                result.total_uncompressed_size = 0;
                result.compression_ratio = 0.0;

                try {
                    // Validate inputs
                    auto validation_result = validateInputs(inputs);
                    if (!validation_result.has_value()) {
                        result.error_message = validation_result.error();
                        return result;
                    }

                    // Create output directory if needed
                    std::filesystem::create_directories(output.parent_path());

                    spdlog::info("Creating TAR archive: {} (format: {})", 
                               output.string(), formatToString(m_format));

                    // Collect all files to pack
                    std::vector<std::filesystem::path> all_files;
                    size_t total_files = 0;
                    
                    for (const auto& input : inputs) {
                        if (std::filesystem::is_directory(input)) {
                            for (const auto& entry : std::filesystem::recursive_directory_iterator(input)) {
                                if (entry.is_regular_file()) {
                                    all_files.push_back(entry.path());
                                    total_files++;
                                }
                            }
                        } else if (std::filesystem::is_regular_file(input)) {
                            all_files.push_back(input);
                            total_files++;
                        }
                    }

                    spdlog::info("Found {} files to pack", total_files);

                    // Open output file
                    std::ofstream tar_file(output, std::ios::binary);
                    if (!tar_file.is_open()) {
                        result.error_message = fmt::format("Cannot create TAR file: {}", output.string());
                        return result;
                    }

                    // Pack each file
                    size_t processed_files = 0;
                    for (const auto& file_path : all_files) {
                        if (m_cancelled) {
                            break;
                        }

                        if (on_progress) {
                            float progress = static_cast<float>(processed_files) / static_cast<float>(total_files);
                            on_progress(fmt::format("Packing: {}", file_path.filename().string()), 
                                      progress, processed_files, total_files);
                        }

                        try {
                            if (!packFileToTar(tar_file, file_path, inputs)) {
                                spdlog::warn("Failed to pack file: {}", file_path.string());
                                if (on_error) {
                                    on_error(fmt::format("Failed to pack file: {}", file_path.string()), false);
                                }
                                continue;
                            }

                            result.files_processed++;
                            result.total_uncompressed_size += std::filesystem::file_size(file_path);
                            processed_files++;

                        } catch (const std::exception& e) {
                            spdlog::warn("Error packing file {}: {}", file_path.string(), e.what());
                            if (on_error) {
                                on_error(fmt::format("Error packing file {}: {}", file_path.string(), e.what()), false);
                            }
                        }
                    }

                    // Write TAR end-of-archive marker (two 512-byte blocks of zeros)
                    std::vector<char> zero_block(512, 0);
                    tar_file.write(zero_block.data(), 512);
                    tar_file.write(zero_block.data(), 512);

                    tar_file.close();

                    if (m_cancelled) {
                        result.error_message = "Packing cancelled by user";
                        spdlog::info("TAR packing cancelled");
                    } else {
                        result.success = true;
                        
                        // Calculate compressed size and compression ratio
                        result.total_compressed_size = std::filesystem::file_size(output);
                        if (result.total_uncompressed_size > 0) {
                            result.compression_ratio = static_cast<double>(result.total_compressed_size) / 
                                                     static_cast<double>(result.total_uncompressed_size);
                        }

                        spdlog::info("Successfully packed {} files into TAR archive", result.files_processed);
                        spdlog::info("TAR compression ratio: {:.2f}% ({} -> {} bytes)", 
                                   result.compression_ratio * 100.0, 
                                   result.total_uncompressed_size, 
                                   result.total_compressed_size);

                        // Note: This creates uncompressed TAR files
                        // For production, integrate compression libraries
                        if (m_format != ArchiveFormat::TAR_GZ) {
                            result.error_message = fmt::format("Compression format {} requires additional libraries (not yet integrated)", 
                                                             formatToString(m_format));
                        }
                    }

                } catch (const std::exception& e) {
                    result.error_message = fmt::format("TAR packing failed: {}", e.what());
                    spdlog::error("TAR packing error: {}", e.what());
                }

                auto end_time = std::chrono::high_resolution_clock::now();
                result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
                
                return result;
            }

            void cancel() override {
                m_cancelled = true;
                spdlog::info("TAR packing cancellation requested");
            }

            bool supportsFormat(ArchiveFormat format) const override {
                return format == ArchiveFormat::TAR_GZ || 
                       format == ArchiveFormat::TAR_XZ || 
                       format == ArchiveFormat::TAR_ZSTD;
            }

        private:
            bool packFileToTar(std::ofstream& tar_file, 
                             const std::filesystem::path& file_path,
                             std::span<const std::filesystem::path> inputs) {
                try {
                    // Determine archive path (relative to input root)
                    std::string archive_path = file_path.filename().string();
                    
                    // Find the input root for this file
                    for (const auto& input : inputs) {
                        if (file_path.string().starts_with(input.string())) {
                            auto relative_path = std::filesystem::relative(file_path, input.parent_path());
                            archive_path = relative_path.string();
                            // Convert Windows path separators to forward slashes
                            std::replace(archive_path.begin(), archive_path.end(), '\\', '/');
                            break;
                        }
                    }

                    // Truncate path if too long for TAR header
                    if (archive_path.length() >= 100) {
                        archive_path = archive_path.substr(0, 99);
                    }

                    // Create TAR header
                    TarHeader header = {};
                    
                    // File name
                    std::strncpy(header.name, archive_path.c_str(), sizeof(header.name) - 1);
                    
                    // File mode (644 for regular files)
                    std::snprintf(header.mode, sizeof(header.mode), "%07o", 0644);
                    
                    // UID/GID (0 for root)
                    std::snprintf(header.uid, sizeof(header.uid), "%07o", 0);
                    std::snprintf(header.gid, sizeof(header.gid), "%07o", 0);
                    
                    // File size
                    auto file_size = std::filesystem::file_size(file_path);
                    std::snprintf(header.size, sizeof(header.size), "%011lo", file_size);
                    
                    // Modification time
                    auto ftime = std::filesystem::last_write_time(file_path);
                    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
                    auto time_t_val = std::chrono::system_clock::to_time_t(sctp);
                    std::snprintf(header.mtime, sizeof(header.mtime), "%011lo", time_t_val);
                    
                    // File type (regular file)
                    header.typeflag = '0';
                    
                    // Magic and version
                    std::strcpy(header.magic, "ustar");
                    std::strcpy(header.version, "00");
                    
                    // User and group names
                    std::strcpy(header.uname, "root");
                    std::strcpy(header.gname, "root");
                    
                    // Calculate checksum
                    calculateChecksum(header);
                    
                    // Write header
                    tar_file.write(reinterpret_cast<const char*>(&header), sizeof(header));
                    
                    // Write file content
                    std::ifstream input_file(file_path, std::ios::binary);
                    if (!input_file.is_open()) {
                        return false;
                    }
                    
                    // Copy file content
                    constexpr size_t buffer_size = 8192;
                    std::vector<char> buffer(buffer_size);
                    size_t bytes_written = 0;
                    
                    while (input_file.read(buffer.data(), buffer_size) || input_file.gcount() > 0) {
                        auto bytes_read = input_file.gcount();
                        tar_file.write(buffer.data(), bytes_read);
                        bytes_written += bytes_read;
                    }
                    
                    // Pad to 512-byte boundary
                    size_t padding = (512 - (bytes_written % 512)) % 512;
                    if (padding > 0) {
                        std::vector<char> pad(padding, 0);
                        tar_file.write(pad.data(), padding);
                    }
                    
                    spdlog::debug("Added file to TAR: {} -> {} ({} bytes)", 
                                file_path.string(), archive_path, file_size);
                    
                    return true;
                    
                } catch (const std::exception& e) {
                    spdlog::error("Error packing file to TAR: {}", e.what());
                    return false;
                }
            }

            void calculateChecksum(TarHeader& header) {
                // Clear checksum field and fill with spaces
                std::memset(header.checksum, ' ', sizeof(header.checksum));
                
                // Calculate checksum as sum of all header bytes
                unsigned int checksum = 0;
                const unsigned char* bytes = reinterpret_cast<const unsigned char*>(&header);
                for (size_t i = 0; i < sizeof(header); ++i) {
                    checksum += bytes[i];
                }
                
                // Write checksum in octal format
                std::snprintf(header.checksum, sizeof(header.checksum), "%06o", checksum);
            }
        };

        // Factory function to create TAR packer
        std::unique_ptr<Packer> createTarPacker() {
            return std::make_unique<TarPackerImpl>();
        }
    }
}

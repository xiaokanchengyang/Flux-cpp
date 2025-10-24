#include "flux-core/extractor.h"
#include "flux-core/exceptions.h"
#include <zip.h>
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <fstream>
#include <chrono>
#include <filesystem>

namespace Flux {
    namespace Formats {
        /**
         * Real ZIP format extractor implementation using libzip
         */
        class ZipExtractorImpl : public Extractor {
        private:
            bool m_cancelled = false;

        public:
            ExtractResult extract(
                const std::filesystem::path& archive_path,
                const std::filesystem::path& output_dir,
                const ExtractOptions& options,
                const ProgressCallback& on_progress,
                const ErrorCallback& on_error) override {
                
                auto start_time = std::chrono::high_resolution_clock::now();
                ExtractResult result;
                result.success = false;
                result.files_extracted = 0;
                result.total_size = 0;

                int error_code = 0;
                zip_t* archive = zip_open(archive_path.string().c_str(), ZIP_RDONLY, &error_code);
                
                if (!archive) {
                    zip_error_t error;
                    zip_error_init_with_code(&error, error_code);
                    result.error_message = fmt::format("Cannot open ZIP archive: {}", zip_error_strerror(&error));
                    zip_error_fini(&error);
                    spdlog::error("Failed to open ZIP archive: {}", result.error_message);
                    return result;
                }

                try {
                    // Create output directory
                    std::filesystem::create_directories(output_dir);
                    
                    // Get number of entries
                    zip_int64_t num_entries = zip_get_num_entries(archive, 0);
                    if (num_entries < 0) {
                        result.error_message = "Cannot get number of entries in ZIP archive";
                        zip_close(archive);
                        return result;
                    }

                    spdlog::info("Extracting {} entries from ZIP archive: {}", num_entries, archive_path.string());

                    // Extract each entry
                    for (zip_int64_t i = 0; i < num_entries && !m_cancelled; ++i) {
                        if (on_progress) {
                            float progress = static_cast<float>(i) / static_cast<float>(num_entries);
                            on_progress(fmt::format("Extracting entry {}/{}", i + 1, num_entries), 
                                      progress, i, num_entries);
                        }

                        // Get entry info
                        zip_stat_t stat;
                        if (zip_stat_index(archive, i, 0, &stat) != 0) {
                            spdlog::warn("Cannot get info for entry {}", i);
                            continue;
                        }

                        std::filesystem::path entry_path = output_dir / stat.name;
                        
                        // Check if it's a directory
                        if (stat.name[strlen(stat.name) - 1] == '/') {
                            // Create directory
                            std::filesystem::create_directories(entry_path);
                            spdlog::debug("Created directory: {}", entry_path.string());
                            continue;
                        }

                        // Create parent directories
                        std::filesystem::create_directories(entry_path.parent_path());

                        // Open file in archive
                        zip_file_t* file = zip_fopen_index(archive, i, 0);
                        if (!file) {
                            spdlog::warn("Cannot open file in archive: {}", stat.name);
                            continue;
                        }

                        // Create output file
                        std::ofstream output_file(entry_path, std::ios::binary);
                        if (!output_file.is_open()) {
                            spdlog::warn("Cannot create output file: {}", entry_path.string());
                            zip_fclose(file);
                            continue;
                        }

                        // Extract file content
                        const size_t buffer_size = 8192;
                        char buffer[buffer_size];
                        zip_int64_t bytes_read;
                        
                        while ((bytes_read = zip_fread(file, buffer, buffer_size)) > 0) {
                            output_file.write(buffer, bytes_read);
                            result.total_size += bytes_read;
                        }

                        output_file.close();
                        zip_fclose(file);

                        // Set file modification time if available
                        if (stat.valid & ZIP_STAT_MTIME) {
                            std::filesystem::file_time_type ftime = 
                                std::filesystem::file_time_type::clock::from_time_t(stat.mtime);
                            std::filesystem::last_write_time(entry_path, ftime);
                        }

                        result.files_extracted++;
                        spdlog::debug("Extracted file: {} ({} bytes)", stat.name, stat.size);
                    }

                    if (m_cancelled) {
                        result.error_message = "Extraction cancelled by user";
                        spdlog::info("ZIP extraction cancelled");
                    } else {
                        result.success = true;
                        spdlog::info("Successfully extracted {} files from ZIP archive", result.files_extracted);
                    }

                } catch (const std::exception& e) {
                    result.error_message = fmt::format("ZIP extraction failed: {}", e.what());
                    spdlog::error("ZIP extraction error: {}", e.what());
                }

                zip_close(archive);
                
                auto end_time = std::chrono::high_resolution_clock::now();
                result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
                
                return result;
            }

            ExtractResult extractPartial(
                const std::filesystem::path& archive_path,
                const std::filesystem::path& output_dir,
                std::span<const std::string> file_patterns,
                const ExtractOptions& options,
                const ProgressCallback& on_progress = nullptr,
                const ErrorCallback& on_error = nullptr) override {
                
                ExtractResult result;
                result.success = false;
                
                int error_code = 0;
                zip_t* archive = zip_open(archive_path.string().c_str(), ZIP_RDONLY, &error_code);
                
                if (!archive) {
                    zip_error_t error;
                    zip_error_init_with_code(&error, error_code);
                    result.error_message = fmt::format("Cannot open ZIP archive: {}", zip_error_strerror(&error));
                    zip_error_fini(&error);
                    return result;
                }

                try {
                    std::filesystem::create_directories(output_dir);
                    
                    zip_int64_t num_entries = zip_get_num_entries(archive, 0);
                    size_t matched_files = 0;

                    for (zip_int64_t i = 0; i < num_entries && !m_cancelled; ++i) {
                        zip_stat_t stat;
                        if (zip_stat_index(archive, i, 0, &stat) != 0) {
                            continue;
                        }

                        // Check if file matches any pattern
                        bool matches = false;
                        for (const auto& pattern : file_patterns) {
                            // Simple pattern matching (can be enhanced with regex)
                            if (std::string(stat.name).find(pattern) != std::string::npos) {
                                matches = true;
                                break;
                            }
                        }

                        if (!matches) {
                            continue;
                        }

                        matched_files++;
                        
                        if (on_progress) {
                            on_progress(fmt::format("Extracting: {}", stat.name), 
                                      static_cast<float>(matched_files) / file_patterns.size(), 
                                      matched_files, file_patterns.size());
                        }

                        // Extract the matching file (similar to full extraction)
                        std::filesystem::path entry_path = output_dir / stat.name;
                        std::filesystem::create_directories(entry_path.parent_path());

                        zip_file_t* file = zip_fopen_index(archive, i, 0);
                        if (!file) continue;

                        std::ofstream output_file(entry_path, std::ios::binary);
                        if (!output_file.is_open()) {
                            zip_fclose(file);
                            continue;
                        }

                        const size_t buffer_size = 8192;
                        char buffer[buffer_size];
                        zip_int64_t bytes_read;
                        
                        while ((bytes_read = zip_fread(file, buffer, buffer_size)) > 0) {
                            output_file.write(buffer, bytes_read);
                            result.total_size += bytes_read;
                        }

                        output_file.close();
                        zip_fclose(file);
                        result.files_extracted++;
                    }

                    result.success = true;
                    spdlog::info("Partially extracted {} files from ZIP archive", result.files_extracted);

                } catch (const std::exception& e) {
                    result.error_message = fmt::format("Partial ZIP extraction failed: {}", e.what());
                    spdlog::error("Partial ZIP extraction error: {}", e.what());
                }

                zip_close(archive);
                return result;
            }

            Flux::expected<std::vector<ArchiveEntry>, std::string> listContents(
                const std::filesystem::path& archive_path,
                std::string_view password = "") override {
                
                std::vector<ArchiveEntry> entries;
                
                int error_code = 0;
                zip_t* archive = zip_open(archive_path.string().c_str(), ZIP_RDONLY, &error_code);
                
                if (!archive) {
                    zip_error_t error;
                    zip_error_init_with_code(&error, error_code);
                    return Flux::unexpected<std::string>(fmt::format("Cannot open ZIP archive: {}", zip_error_strerror(&error)));
                }

                try {
                    zip_int64_t num_entries = zip_get_num_entries(archive, 0);
                    entries.reserve(num_entries);

                    for (zip_int64_t i = 0; i < num_entries; ++i) {
                        zip_stat_t stat;
                        if (zip_stat_index(archive, i, 0, &stat) != 0) {
                            continue;
                        }

                        ArchiveEntry entry;
                        entry.name = std::filesystem::path(stat.name).filename().string();
                        entry.path = stat.name;
                        entry.is_directory = (stat.name[strlen(stat.name) - 1] == '/');
                        entry.compressed_size = stat.comp_size;
                        entry.uncompressed_size = stat.size;
                        
                        if (stat.valid & ZIP_STAT_MTIME) {
                            entry.modification_time = std::to_string(stat.mtime);
                        }
                        
                        if (stat.valid & ZIP_STAT_CRC) {
                            entry.crc32 = stat.crc;
                        }

                        entries.push_back(entry);
                    }

                    spdlog::debug("Listed {} entries from ZIP archive", entries.size());

                } catch (const std::exception& e) {
                    zip_close(archive);
                    return Flux::unexpected<std::string>(fmt::format("Cannot list ZIP contents: {}", e.what()));
                }

                zip_close(archive);
                return entries;
            }

            Flux::expected<ArchiveInfo, std::string> getArchiveInfo(
                const std::filesystem::path& archive_path,
                std::string_view password = "") override {
                
                ArchiveInfo info;
                info.path = archive_path;
                info.format = ArchiveFormat::ZIP;
                info.compressed_size = std::filesystem::file_size(archive_path);
                
                int error_code = 0;
                zip_t* archive = zip_open(archive_path.string().c_str(), ZIP_RDONLY, &error_code);
                
                if (!archive) {
                    zip_error_t error;
                    zip_error_init_with_code(&error, error_code);
                    return Flux::unexpected<std::string>(fmt::format("Cannot open ZIP archive: {}", zip_error_strerror(&error)));
                }

                try {
                    zip_int64_t num_entries = zip_get_num_entries(archive, 0);
                    info.file_count = num_entries;
                    info.uncompressed_size = 0;

                    // Calculate total uncompressed size
                    for (zip_int64_t i = 0; i < num_entries; ++i) {
                        zip_stat_t stat;
                        if (zip_stat_index(archive, i, 0, &stat) == 0) {
                            info.uncompressed_size += stat.size;
                        }
                    }

                    // Check if archive is encrypted (simplified check)
                    info.is_encrypted = false;
                    for (zip_int64_t i = 0; i < num_entries && !info.is_encrypted; ++i) {
                        zip_stat_t stat;
                        if (zip_stat_index(archive, i, 0, &stat) == 0) {
                            if (stat.encryption_method != ZIP_EM_NONE) {
                                info.is_encrypted = true;
                            }
                        }
                    }

                    info.creation_time = "Unknown"; // ZIP doesn't store archive creation time

                } catch (const std::exception& e) {
                    zip_close(archive);
                    return Flux::unexpected<std::string>(fmt::format("Cannot get ZIP archive info: {}", e.what()));
                }

                zip_close(archive);
                return info;
            }

            Flux::expected<void, std::string> verifyIntegrity(
                const std::filesystem::path& archive_path,
                std::string_view password = "") override {
                
                int error_code = 0;
                zip_t* archive = zip_open(archive_path.string().c_str(), ZIP_RDONLY, &error_code);
                
                if (!archive) {
                    zip_error_t error;
                    zip_error_init_with_code(&error, error_code);
                    return Flux::unexpected<std::string>(fmt::format("Cannot open ZIP archive: {}", zip_error_strerror(&error)));
                }

                try {
                    zip_int64_t num_entries = zip_get_num_entries(archive, 0);
                    
                    // Test each entry by trying to read it
                    for (zip_int64_t i = 0; i < num_entries; ++i) {
                        zip_stat_t stat;
                        if (zip_stat_index(archive, i, 0, &stat) != 0) {
                            zip_close(archive);
                            return Flux::unexpected<std::string>(fmt::format("Cannot get info for entry {}", i));
                        }

                        // Skip directories
                        if (stat.name[strlen(stat.name) - 1] == '/') {
                            continue;
                        }

                        zip_file_t* file = zip_fopen_index(archive, i, 0);
                        if (!file) {
                            zip_close(archive);
                            return Flux::unexpected<std::string>(fmt::format("Cannot open file in archive: {}", stat.name));
                        }

                        // Try to read the entire file to verify integrity
                        const size_t buffer_size = 8192;
                        char buffer[buffer_size];
                        zip_int64_t total_read = 0;
                        zip_int64_t bytes_read;
                        
                        while ((bytes_read = zip_fread(file, buffer, buffer_size)) > 0) {
                            total_read += bytes_read;
                        }

                        zip_fclose(file);

                        if (total_read != static_cast<zip_int64_t>(stat.size)) {
                            zip_close(archive);
                            return Flux::unexpected<std::string>(fmt::format("Size mismatch for file: {}", stat.name));
                        }
                    }

                    zip_close(archive);
                    return {};

                } catch (const std::exception& e) {
                    zip_close(archive);
                    return Flux::unexpected<std::string>(fmt::format("Integrity verification failed: {}", e.what()));
                }
            }

            Flux::expected<ArchiveFormat, std::string> detectFormat(
                const std::filesystem::path& archive_path) override {
                
                return ArchiveFormat::ZIP;
            }

            void cancel() override {
                m_cancelled = true;
                spdlog::info("ZIP extraction cancellation requested");
            }

            bool supportsFormat(ArchiveFormat format) const override {
                return format == ArchiveFormat::ZIP;
            }
        };

        // Factory function to create ZIP extractor
        std::unique_ptr<Extractor> createZipExtractor() {
            return std::make_unique<ZipExtractorImpl>();
        }
    }
}

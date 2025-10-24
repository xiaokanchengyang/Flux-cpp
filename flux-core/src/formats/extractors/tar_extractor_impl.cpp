#include "flux-core/extractor.h"
#include "flux-core/exceptions.h"
#include <archive.h>
#include <archive_entry.h>
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <fstream>
#include <chrono>
#include <filesystem>

namespace Flux {
    namespace Formats {
        /**
         * TAR format extractor implementation using libarchive
         * Supports TAR, TAR.GZ, TAR.XZ, TAR.ZSTD formats
         */
        class TarExtractorImpl : public Extractor {
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

                struct archive* a = archive_read_new();
                struct archive* ext = archive_write_disk_new();
                
                // Enable all supported formats and filters
                archive_read_support_format_all(a);
                archive_read_support_filter_all(a);
                
                // Set extraction flags
                int flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS;
                if (options.preserve_permissions) {
                    flags |= ARCHIVE_EXTRACT_OWNER;
                }
                archive_write_disk_set_options(ext, flags);
                archive_write_disk_set_standard_lookup(ext);

                int r = archive_read_open_filename(a, archive_path.string().c_str(), 10240);
                if (r != ARCHIVE_OK) {
                    result.error_message = fmt::format("Cannot open TAR archive: {}", archive_error_string(a));
                    archive_read_free(a);
                    archive_write_free(ext);
                    spdlog::error("Failed to open TAR archive: {}", result.error_message);
                    return result;
                }

                try {
                    // Create output directory
                    std::filesystem::create_directories(output_dir);
                    
                    struct archive_entry* entry;
                    size_t total_entries = 0;
                    size_t processed_entries = 0;

                    spdlog::info("Extracting TAR archive: {}", archive_path.string());

                    // First pass: count entries for progress reporting
                    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
                        total_entries++;
                        archive_read_data_skip(a);
                    }
                    
                    // Reset archive for actual extraction
                    archive_read_free(a);
                    a = archive_read_new();
                    archive_read_support_format_all(a);
                    archive_read_support_filter_all(a);
                    archive_read_open_filename(a, archive_path.string().c_str(), 10240);

                    // Extract each entry
                    while (archive_read_next_header(a, &entry) == ARCHIVE_OK && !m_cancelled) {
                        if (on_progress) {
                            float progress = static_cast<float>(processed_entries) / static_cast<float>(total_entries);
                            on_progress(fmt::format("Extracting: {}", archive_entry_pathname(entry)), 
                                      progress, processed_entries, total_entries);
                        }

                        // Construct full output path
                        std::filesystem::path entry_path = output_dir / archive_entry_pathname(entry);
                        archive_entry_set_pathname(entry, entry_path.string().c_str());

                        r = archive_write_header(ext, entry);
                        if (r < ARCHIVE_OK) {
                            spdlog::warn("Warning writing header: {}", archive_error_string(ext));
                        } else if (archive_entry_size(entry) > 0) {
                            // Extract file data
                            const void* buff;
                            size_t size;
                            la_int64_t offset;

                            while ((r = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK) {
                                r = archive_write_data_block(ext, buff, size, offset);
                                if (r < ARCHIVE_OK) {
                                    spdlog::warn("Warning writing data: {}", archive_error_string(ext));
                                    break;
                                }
                                result.total_size += size;
                            }
                        }

                        r = archive_write_finish_entry(ext);
                        if (r < ARCHIVE_OK) {
                            spdlog::warn("Warning finishing entry: {}", archive_error_string(ext));
                        }

                        result.files_extracted++;
                        processed_entries++;
                        spdlog::debug("Extracted: {}", archive_entry_pathname(entry));
                    }

                    if (m_cancelled) {
                        result.error_message = "Extraction cancelled by user";
                        spdlog::info("TAR extraction cancelled");
                    } else {
                        result.success = true;
                        spdlog::info("Successfully extracted {} files from TAR archive", result.files_extracted);
                    }

                } catch (const std::exception& e) {
                    result.error_message = fmt::format("TAR extraction failed: {}", e.what());
                    spdlog::error("TAR extraction error: {}", e.what());
                }

                archive_read_close(a);
                archive_read_free(a);
                archive_write_close(ext);
                archive_write_free(ext);
                
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
                result.files_extracted = 0;
                result.total_size = 0;

                struct archive* a = archive_read_new();
                struct archive* ext = archive_write_disk_new();
                
                archive_read_support_format_all(a);
                archive_read_support_filter_all(a);
                
                int flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM;
                if (options.preserve_permissions) {
                    flags |= ARCHIVE_EXTRACT_OWNER;
                }
                archive_write_disk_set_options(ext, flags);
                archive_write_disk_set_standard_lookup(ext);

                int r = archive_read_open_filename(a, archive_path.string().c_str(), 10240);
                if (r != ARCHIVE_OK) {
                    result.error_message = fmt::format("Cannot open TAR archive: {}", archive_error_string(a));
                    archive_read_free(a);
                    archive_write_free(ext);
                    return result;
                }

                try {
                    std::filesystem::create_directories(output_dir);
                    
                    struct archive_entry* entry;
                    size_t matched_files = 0;

                    while (archive_read_next_header(a, &entry) == ARCHIVE_OK && !m_cancelled) {
                        const char* pathname = archive_entry_pathname(entry);
                        
                        // Check if file matches any pattern
                        bool matches = false;
                        for (const auto& pattern : file_patterns) {
                            if (std::string(pathname).find(pattern) != std::string::npos) {
                                matches = true;
                                break;
                            }
                        }

                        if (!matches) {
                            archive_read_data_skip(a);
                            continue;
                        }

                        matched_files++;
                        
                        if (on_progress) {
                            on_progress(fmt::format("Extracting: {}", pathname), 
                                      static_cast<float>(matched_files) / file_patterns.size(), 
                                      matched_files, file_patterns.size());
                        }

                        // Extract the matching file
                        std::filesystem::path entry_path = output_dir / pathname;
                        archive_entry_set_pathname(entry, entry_path.string().c_str());

                        r = archive_write_header(ext, entry);
                        if (r >= ARCHIVE_OK && archive_entry_size(entry) > 0) {
                            const void* buff;
                            size_t size;
                            la_int64_t offset;

                            while ((r = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK) {
                                archive_write_data_block(ext, buff, size, offset);
                                result.total_size += size;
                            }
                        }
                        
                        archive_write_finish_entry(ext);
                        result.files_extracted++;
                    }

                    result.success = true;
                    spdlog::info("Partially extracted {} files from TAR archive", result.files_extracted);

                } catch (const std::exception& e) {
                    result.error_message = fmt::format("Partial TAR extraction failed: {}", e.what());
                    spdlog::error("Partial TAR extraction error: {}", e.what());
                }

                archive_read_close(a);
                archive_read_free(a);
                archive_write_close(ext);
                archive_write_free(ext);
                
                return result;
            }

            Flux::expected<std::vector<ArchiveEntry>, std::string> listContents(
                const std::filesystem::path& archive_path,
                std::string_view password = "") override {
                
                std::vector<ArchiveEntry> entries;
                
                struct archive* a = archive_read_new();
                archive_read_support_format_all(a);
                archive_read_support_filter_all(a);
                
                int r = archive_read_open_filename(a, archive_path.string().c_str(), 10240);
                if (r != ARCHIVE_OK) {
                    archive_read_free(a);
                    return Flux::unexpected<std::string>(fmt::format("Cannot open TAR archive: {}", archive_error_string(a)));
                }

                try {
                    struct archive_entry* entry;
                    
                    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
                        ArchiveEntry archiveEntry;
                        
                        const char* pathname = archive_entry_pathname(entry);
                        archiveEntry.name = std::filesystem::path(pathname).filename().string();
                        archiveEntry.path = pathname;
                        archiveEntry.is_directory = (archive_entry_filetype(entry) == AE_IFDIR);
                        archiveEntry.uncompressed_size = archive_entry_size(entry);
                        archiveEntry.compressed_size = archiveEntry.uncompressed_size; // TAR doesn't compress individual files
                        
                        // Get modification time
                        time_t mtime = archive_entry_mtime(entry);
                        archiveEntry.modification_time = std::to_string(mtime);
                        
                        // Get permissions
                        archiveEntry.permissions = archive_entry_perm(entry);
                        
                        entries.push_back(archiveEntry);
                        
                        // Skip file data
                        archive_read_data_skip(a);
                    }

                    spdlog::debug("Listed {} entries from TAR archive", entries.size());

                } catch (const std::exception& e) {
                    archive_read_close(a);
                    archive_read_free(a);
                    return Flux::unexpected<std::string>(fmt::format("Cannot list TAR contents: {}", e.what()));
                }

                archive_read_close(a);
                archive_read_free(a);
                return entries;
            }

            Flux::expected<ArchiveInfo, std::string> getArchiveInfo(
                const std::filesystem::path& archive_path,
                std::string_view password = "") override {
                
                ArchiveInfo info;
                info.path = archive_path;
                info.compressed_size = std::filesystem::file_size(archive_path);
                info.is_encrypted = false; // TAR archives are not typically encrypted
                info.creation_time = "Unknown"; // TAR doesn't store archive creation time
                
                // Detect format from filename
                auto format_result = detectFormat(archive_path);
                if (format_result.has_value()) {
                    info.format = format_result.value();
                } else {
                    return Flux::unexpected<std::string>("Cannot detect TAR format");
                }
                
                struct archive* a = archive_read_new();
                archive_read_support_format_all(a);
                archive_read_support_filter_all(a);
                
                int r = archive_read_open_filename(a, archive_path.string().c_str(), 10240);
                if (r != ARCHIVE_OK) {
                    archive_read_free(a);
                    return Flux::unexpected<std::string>(fmt::format("Cannot open TAR archive: {}", archive_error_string(a)));
                }

                try {
                    struct archive_entry* entry;
                    info.file_count = 0;
                    info.uncompressed_size = 0;
                    
                    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
                        info.file_count++;
                        info.uncompressed_size += archive_entry_size(entry);
                        archive_read_data_skip(a);
                    }

                } catch (const std::exception& e) {
                    archive_read_close(a);
                    archive_read_free(a);
                    return Flux::unexpected<std::string>(fmt::format("Cannot get TAR archive info: {}", e.what()));
                }

                archive_read_close(a);
                archive_read_free(a);
                return info;
            }

            Flux::expected<void, std::string> verifyIntegrity(
                const std::filesystem::path& archive_path,
                std::string_view password = "") override {
                
                struct archive* a = archive_read_new();
                archive_read_support_format_all(a);
                archive_read_support_filter_all(a);
                
                int r = archive_read_open_filename(a, archive_path.string().c_str(), 10240);
                if (r != ARCHIVE_OK) {
                    archive_read_free(a);
                    return Flux::unexpected<std::string>(fmt::format("Cannot open TAR archive: {}", archive_error_string(a)));
                }

                try {
                    struct archive_entry* entry;
                    
                    // Test each entry by trying to read its header and data
                    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
                        // Try to read all data to verify integrity
                        const void* buff;
                        size_t size;
                        la_int64_t offset;
                        
                        while ((r = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK) {
                            // Just read the data, don't need to do anything with it
                        }
                        
                        if (r != ARCHIVE_EOF) {
                            archive_read_close(a);
                            archive_read_free(a);
                            return Flux::unexpected<std::string>(fmt::format("Data corruption in entry: {}", archive_entry_pathname(entry)));
                        }
                    }

                    archive_read_close(a);
                    archive_read_free(a);
                    return {};

                } catch (const std::exception& e) {
                    archive_read_close(a);
                    archive_read_free(a);
                    return Flux::unexpected<std::string>(fmt::format("Integrity verification failed: {}", e.what()));
                }
            }

            Flux::expected<ArchiveFormat, std::string> detectFormat(
                const std::filesystem::path& archive_path) override {
                
                std::string filename = archive_path.filename().string();
                if (filename.ends_with(".tar.gz") || filename.ends_with(".tgz")) {
                    return ArchiveFormat::TAR_GZ;
                }
                if (filename.ends_with(".tar.xz") || filename.ends_with(".txz")) {
                    return ArchiveFormat::TAR_XZ;
                }
                if (filename.ends_with(".tar.zst") || filename.ends_with(".tar.zstd")) {
                    return ArchiveFormat::TAR_ZSTD;
                }
                return Flux::unexpected<std::string>{"Cannot detect TAR format from filename"};
            }

            void cancel() override {
                m_cancelled = true;
            }

            bool supportsFormat(ArchiveFormat format) const override {
                return format == ArchiveFormat::TAR_GZ || 
                       format == ArchiveFormat::TAR_XZ || 
                       format == ArchiveFormat::TAR_ZSTD;
            }
        };

        // Factory function to create TAR extractor
        std::unique_ptr<Extractor> createTarExtractor() {
            return std::make_unique<TarExtractorImpl>();
        }
    }
}

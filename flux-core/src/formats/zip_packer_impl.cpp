#include "flux-core/packer.h"
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
         * Real ZIP format packer implementation using libzip
         */
        class ZipPackerImpl : public Packer {
        private:
            bool m_cancelled = false;

        public:
            PackResult pack(
                std::span<const std::filesystem::path> inputs,
                const std::filesystem::path& output,
                const PackOptions& options,
                const ProgressCallback& on_progress = nullptr,
                const ErrorCallback& on_error = nullptr) override {
                
                auto start_time = std::chrono::high_resolution_clock::now();
                PackResult result;
                result.success = false;
                result.files_packed = 0;
                result.total_size = 0;

                // Create output directory if needed
                std::filesystem::create_directories(output.parent_path());

                int error_code = 0;
                zip_t* archive = zip_open(output.string().c_str(), ZIP_CREATE | ZIP_TRUNCATE, &error_code);
                
                if (!archive) {
                    zip_error_t error;
                    zip_error_init_with_code(&error, error_code);
                    result.error_message = fmt::format("Cannot create ZIP archive: {}", zip_error_strerror(&error));
                    zip_error_fini(&error);
                    spdlog::error("Failed to create ZIP archive: {}", result.error_message);
                    return result;
                }

                try {
                    // Set compression level
                    int compression_level = ZIP_CM_DEFAULT;
                    if (options.compression_level >= 0 && options.compression_level <= 9) {
                        compression_level = options.compression_level;
                    }

                    spdlog::info("Creating ZIP archive: {} with compression level {}", 
                               output.string(), compression_level);

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

                            // Create zip source from file
                            zip_source_t* source = zip_source_file(archive, file_path.string().c_str(), 0, 0);
                            if (!source) {
                                spdlog::warn("Cannot create source for file: {}", file_path.string());
                                continue;
                            }

                            // Add file to archive
                            zip_int64_t index = zip_file_add(archive, archive_path.c_str(), source, ZIP_FL_OVERWRITE);
                            if (index < 0) {
                                spdlog::warn("Cannot add file to archive: {}", archive_path);
                                zip_source_free(source);
                                continue;
                            }

                            // Set compression method and level
                            if (zip_set_file_compression(archive, index, ZIP_CM_DEFLATE, compression_level) < 0) {
                                spdlog::warn("Cannot set compression for file: {}", archive_path);
                            }

                            // Update statistics
                            result.files_packed++;
                            result.total_size += std::filesystem::file_size(file_path);
                            processed_files++;

                            spdlog::debug("Added file to ZIP: {} -> {}", file_path.string(), archive_path);

                        } catch (const std::exception& e) {
                            spdlog::warn("Error packing file {}: {}", file_path.string(), e.what());
                            if (on_error) {
                                on_error(fmt::format("Error packing file {}: {}", file_path.string(), e.what()));
                            }
                        }
                    }

                    // Add directories if needed
                    std::set<std::string> added_dirs;
                    for (const auto& input : inputs) {
                        if (std::filesystem::is_directory(input)) {
                            addDirectoryStructure(archive, input, input.parent_path(), added_dirs);
                        }
                    }

                    if (m_cancelled) {
                        result.error_message = "Packing cancelled by user";
                        spdlog::info("ZIP packing cancelled");
                    } else {
                        result.success = true;
                        spdlog::info("Successfully packed {} files into ZIP archive", result.files_packed);
                    }

                } catch (const std::exception& e) {
                    result.error_message = fmt::format("ZIP packing failed: {}", e.what());
                    spdlog::error("ZIP packing error: {}", e.what());
                }

                // Close archive
                if (zip_close(archive) < 0) {
                    result.error_message = fmt::format("Cannot close ZIP archive: {}", zip_strerror(archive));
                    result.success = false;
                    spdlog::error("Failed to close ZIP archive: {}", result.error_message);
                }
                
                auto end_time = std::chrono::high_resolution_clock::now();
                result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
                
                return result;
            }

            void cancel() override {
                m_cancelled = true;
                spdlog::info("ZIP packing cancellation requested");
            }

            bool supportsFormat(ArchiveFormat format) const override {
                return format == ArchiveFormat::ZIP;
            }

        private:
            void addDirectoryStructure(zip_t* archive, 
                                     const std::filesystem::path& dir_path,
                                     const std::filesystem::path& base_path,
                                     std::set<std::string>& added_dirs) {
                try {
                    for (const auto& entry : std::filesystem::recursive_directory_iterator(dir_path)) {
                        if (entry.is_directory()) {
                            auto relative_path = std::filesystem::relative(entry.path(), base_path);
                            std::string archive_path = relative_path.string();
                            
                            // Convert Windows path separators to forward slashes and add trailing slash
                            std::replace(archive_path.begin(), archive_path.end(), '\\', '/');
                            if (!archive_path.empty() && archive_path.back() != '/') {
                                archive_path += '/';
                            }

                            // Check if directory already added
                            if (added_dirs.find(archive_path) != added_dirs.end()) {
                                continue;
                            }

                            // Add directory entry
                            zip_int64_t index = zip_dir_add(archive, archive_path.c_str(), ZIP_FL_ENC_UTF_8);
                            if (index >= 0) {
                                added_dirs.insert(archive_path);
                                spdlog::debug("Added directory to ZIP: {}", archive_path);
                            }
                        }
                    }
                } catch (const std::exception& e) {
                    spdlog::warn("Error adding directory structure: {}", e.what());
                }
            }
        };

        // Factory function to create ZIP packer
        std::unique_ptr<Packer> createZipPacker() {
            return std::make_unique<ZipPackerImpl>();
        }
    }
}

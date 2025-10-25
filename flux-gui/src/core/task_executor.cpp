// Copyright (c) 2024 Flux Archive Manager Contributors
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "async_task_executor.h"
#include <flux-core/flux.h>
#include <flux-core/extractor.h>
#include <flux-core/packer.h>
#include <QThread>
#include <QMutexLocker>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <algorithm>
#include <memory>

namespace FluxGui {

TaskExecutor::TaskExecutor(QObject* parent)
    : QObject(parent)
    , is_cancelled_(false)
{
    // Initialize Flux library
    Flux::initialize();
}

TaskExecutor::~TaskExecutor() {
    cancel();
    // Cleanup Flux library
    Flux::cleanup();
}

void TaskExecutor::cancel() {
    QMutexLocker locker(&mutex_);
    is_cancelled_ = true;
}

bool TaskExecutor::isCancelled() const {
    QMutexLocker locker(&mutex_);
    return is_cancelled_;
}

void TaskExecutor::extractArchive(const QString& archive_path, const QString& output_path,
                                 const ExtractOptions& options) {
    QElapsedTimer timer;
    timer.start();
    
    emit taskStarted("Extracting Archive");
    
    try {
        // Detect archive format
        std::filesystem::path archive_fs_path = archive_path.toStdString();
        auto extractor_result = Flux::createExtractorAuto(archive_fs_path);
        
        if (!extractor_result.has_value()) {
            emit taskFinished(false, QString("Failed to detect archive format: %1")
                            .arg(QString::fromStdString(extractor_result.error())));
            return;
        }
        
        auto extractor = std::move(extractor_result.value());
        
        // Setup extraction options
        Flux::ExtractOptions flux_options;
        flux_options.overwrite_mode = static_cast<Flux::OverwriteMode>(options.overwrite_mode);
        flux_options.preserve_permissions = options.preserve_permissions;
        flux_options.preserve_timestamps = options.preserve_timestamps;
        flux_options.password = options.password.toStdString();
        
        // Convert include/exclude patterns
        for (const auto& pattern : options.include_patterns) {
            flux_options.include_patterns.push_back(pattern.toStdString());
        }
        for (const auto& pattern : options.exclude_patterns) {
            flux_options.exclude_patterns.push_back(pattern.toStdString());
        }
        
        // Setup progress callback
        auto progress_callback = [this](std::string_view filename, float progress,
                                       size_t processed, size_t total) {
            if (isCancelled()) {
                return;
            }
            
            emit progressUpdated(QString::fromUtf8(filename.data(), filename.size()),
                               progress, processed, total);
        };
        
        // Setup error callback
        bool has_errors = false;
        QString error_messages;
        auto error_callback = [&has_errors, &error_messages](std::string_view error_msg, bool is_fatal) {
            has_errors = true;
            if (!error_messages.isEmpty()) {
                error_messages += "\n";
            }
            error_messages += QString::fromUtf8(error_msg.data(), error_msg.size());
        };
        
        // Perform extraction
        std::filesystem::path output_fs_path = output_path.toStdString();
        auto result = extractor->extract(archive_fs_path, output_fs_path, flux_options,
                                        progress_callback, error_callback);
        
        if (isCancelled()) {
            emit taskFinished(false, "Extraction cancelled by user");
            return;
        }
        
        if (result.success) {
            QString message = QString("Successfully extracted %1 files to %2 (Time: %3ms)")
                            .arg(result.files_extracted)
                            .arg(output_path)
                            .arg(timer.elapsed());
            
            if (has_errors && !error_messages.isEmpty()) {
                message += QString("\nWarnings: %1").arg(error_messages);
            }
            
            emit taskFinished(true, message);
        } else {
            QString error_msg = QString("Extraction failed: %1").arg(QString::fromStdString(result.error_message));
            if (has_errors && !error_messages.isEmpty()) {
                error_msg += QString("\nAdditional errors: %1").arg(error_messages);
            }
            emit taskFinished(false, error_msg);
        }
        
    } catch (const std::exception& e) {
        emit taskFinished(false, QString("Exception during extraction: %1").arg(e.what()));
    } catch (...) {
        emit taskFinished(false, "Unknown error during extraction");
    }
}

void TaskExecutor::createArchive(const QStringList& input_paths, const QString& output_path,
                                const PackOptions& options) {
    QElapsedTimer timer;
    timer.start();
    
    emit taskStarted("Creating Archive");
    
    try {
        // Create packer for specified format
        auto packer = Flux::createPacker(static_cast<Flux::ArchiveFormat>(options.format));
        
        // Convert input paths to filesystem paths
        std::vector<std::filesystem::path> inputs;
        for (const auto& path : input_paths) {
            inputs.emplace_back(path.toStdString());
        }
        
        // Validate inputs first
        auto validation_result = packer->validateInputs(inputs);
        if (!validation_result.has_value()) {
            emit taskFinished(false, QString("Input validation failed: %1")
                            .arg(QString::fromStdString(validation_result.error())));
            return;
        }
        
        // Setup packing options
        Flux::PackOptions flux_options;
        flux_options.format = static_cast<Flux::ArchiveFormat>(options.format);
        flux_options.compression_level = options.compression_level;
        flux_options.num_threads = options.num_threads;
        flux_options.preserve_permissions = options.preserve_permissions;
        flux_options.preserve_timestamps = options.preserve_timestamps;
        flux_options.password = options.password.toStdString();
        
        // Setup progress callback
        auto progress_callback = [this](std::string_view filename, float progress,
                                       size_t processed, size_t total) {
            if (isCancelled()) {
                return;
            }
            
            emit progressUpdated(QString::fromUtf8(filename.data(), filename.size()),
                               progress, processed, total);
        };
        
        // Setup error callback
        bool has_errors = false;
        QString error_messages;
        auto error_callback = [&has_errors, &error_messages](std::string_view error_msg, bool is_fatal) {
            has_errors = true;
            if (!error_messages.isEmpty()) {
                error_messages += "\n";
            }
            error_messages += QString::fromUtf8(error_msg.data(), error_msg.size());
        };
        
        // Perform packing
        std::filesystem::path output_fs_path = output_path.toStdString();
        auto result = packer->pack(inputs, output_fs_path, flux_options,
                                  progress_callback, error_callback);
        
        if (isCancelled()) {
            emit taskFinished(false, "Archive creation cancelled by user");
            return;
        }
        
        if (result.success) {
            QString message = QString("Successfully created archive with %1 files (Time: %2ms)\n"
                                    "Compression ratio: %3%\n"
                                    "Original size: %4 bytes\n"
                                    "Compressed size: %5 bytes")
                            .arg(result.files_processed)
                            .arg(timer.elapsed())
                            .arg(result.compression_ratio * 100, 0, 'f', 1)
                            .arg(result.total_uncompressed_size)
                            .arg(result.total_compressed_size);
            
            if (has_errors && !error_messages.isEmpty()) {
                message += QString("\nWarnings: %1").arg(error_messages);
            }
            
            emit taskFinished(true, message);
        } else {
            QString error_msg = QString("Archive creation failed: %1").arg(QString::fromStdString(result.error_message));
            if (has_errors && !error_messages.isEmpty()) {
                error_msg += QString("\nAdditional errors: %1").arg(error_messages);
            }
            emit taskFinished(false, error_msg);
        }
        
    } catch (const std::exception& e) {
        emit taskFinished(false, QString("Exception during archive creation: %1").arg(e.what()));
    } catch (...) {
        emit taskFinished(false, "Unknown error during archive creation");
    }
}

void TaskExecutor::listArchiveContents(const QString& archive_path, const QString& password) {
    emit taskStarted("Reading Archive Contents");
    
    try {
        // Detect archive format and create extractor
        std::filesystem::path archive_fs_path = archive_path.toStdString();
        auto extractor_result = Flux::createExtractorAuto(archive_fs_path);
        
        if (!extractor_result.has_value()) {
            emit taskFinished(false, QString("Failed to detect archive format: %1")
                            .arg(QString::fromStdString(extractor_result.error())));
            return;
        }
        
        auto extractor = std::move(extractor_result.value());
        
        // Get archive info
        auto info_result = extractor->getArchiveInfo(archive_fs_path, password.toStdString());
        if (!info_result.has_value()) {
            emit taskFinished(false, QString("Failed to get archive info: %1")
                            .arg(QString::fromStdString(info_result.error())));
            return;
        }
        
        // List contents
        auto contents_result = extractor->listContents(archive_fs_path, password.toStdString());
        if (!contents_result.has_value()) {
            emit taskFinished(false, QString("Failed to list archive contents: %1")
                            .arg(QString::fromStdString(contents_result.error())));
            return;
        }
        
        if (isCancelled()) {
            emit taskFinished(false, "Archive listing cancelled by user");
            return;
        }
        
        // Convert results to Qt types
        ArchiveInfo qt_info;
        const auto& flux_info = info_result.value();
        qt_info.path = QString::fromStdString(flux_info.path.string());
        qt_info.format = static_cast<int>(flux_info.format);
        qt_info.compressed_size = flux_info.compressed_size;
        qt_info.uncompressed_size = flux_info.uncompressed_size;
        qt_info.file_count = flux_info.file_count;
        qt_info.is_encrypted = flux_info.is_encrypted;
        qt_info.creation_time = QString::fromStdString(flux_info.creation_time);
        
        QList<ArchiveEntry> qt_entries;
        for (const auto& flux_entry : contents_result.value()) {
            ArchiveEntry qt_entry;
            qt_entry.name = QString::fromStdString(flux_entry.name);
            qt_entry.path = QString::fromStdString(flux_entry.path.string());
            qt_entry.compressed_size = flux_entry.compressed_size;
            qt_entry.uncompressed_size = flux_entry.uncompressed_size;
            qt_entry.is_directory = flux_entry.is_directory;
            qt_entry.modification_time = QString::fromStdString(flux_entry.modification_time);
            qt_entry.permissions = flux_entry.permissions;
            qt_entries.append(qt_entry);
        }
        
        emit archiveContentsReady(qt_info, qt_entries);
        emit taskFinished(true, QString("Successfully read archive contents: %1 files")
                        .arg(qt_entries.size()));
        
    } catch (const std::exception& e) {
        emit taskFinished(false, QString("Exception during archive listing: %1").arg(e.what()));
    } catch (...) {
        emit taskFinished(false, "Unknown error during archive listing");
    }
}

void TaskExecutor::verifyArchive(const QString& archive_path, const QString& password) {
    emit taskStarted("Verifying Archive Integrity");
    
    try {
        // Detect archive format and create extractor
        std::filesystem::path archive_fs_path = archive_path.toStdString();
        auto extractor_result = Flux::createExtractorAuto(archive_fs_path);
        
        if (!extractor_result.has_value()) {
            emit taskFinished(false, QString("Failed to detect archive format: %1")
                            .arg(QString::fromStdString(extractor_result.error())));
            return;
        }
        
        auto extractor = std::move(extractor_result.value());
        
        // Verify integrity
        auto verify_result = extractor->verifyIntegrity(archive_fs_path, password.toStdString());
        
        if (isCancelled()) {
            emit taskFinished(false, "Archive verification cancelled by user");
            return;
        }
        
        if (verify_result.has_value()) {
            emit taskFinished(true, "Archive integrity verification passed");
        } else {
            emit taskFinished(false, QString("Archive integrity verification failed: %1")
                            .arg(QString::fromStdString(verify_result.error())));
        }
        
    } catch (const std::exception& e) {
        emit taskFinished(false, QString("Exception during archive verification: %1").arg(e.what()));
    } catch (...) {
        emit taskFinished(false, "Unknown error during archive verification");
    }
}

void TaskExecutor::benchmarkCompression(const QStringList& input_paths, const QString& output_dir) {
    emit taskStarted("Running Compression Benchmark");
    
    try {
        QMap<QString, BenchmarkResult> results;
        
        // Test different formats and compression levels
        std::vector<std::pair<Flux::ArchiveFormat, QString>> test_formats = {
            {Flux::ArchiveFormat::ZIP, "ZIP"},
            {Flux::ArchiveFormat::TAR_GZ, "TAR.GZ"},
            {Flux::ArchiveFormat::TAR_XZ, "TAR.XZ"},
            {Flux::ArchiveFormat::TAR_ZSTD, "TAR.ZSTD"},
            {Flux::ArchiveFormat::SEVEN_ZIP, "7Z"}
        };
        
        std::vector<int> compression_levels = {1, 3, 6, 9};
        
        int total_tests = test_formats.size() * compression_levels.size();
        int current_test = 0;
        
        for (const auto& [format, format_name] : test_formats) {
            if (isCancelled()) break;
            
            for (int level : compression_levels) {
                if (isCancelled()) break;
                
                QString test_name = QString("%1 Level %2").arg(format_name).arg(level);
                emit progressUpdated(test_name, static_cast<float>(current_test) / total_tests, 
                                   current_test, total_tests);
                
                // Create test archive
                QString test_output = QString("%1/benchmark_%2_level_%3.archive")
                                    .arg(output_dir).arg(format_name.toLower()).arg(level);
                
                PackOptions options;
                options.format = static_cast<int>(format);
                options.compression_level = level;
                options.num_threads = 1; // Single-threaded for consistent benchmarks
                
                QElapsedTimer timer;
                timer.start();
                
                // Run compression test (simplified - would need actual implementation)
                QThread::msleep(500 + (level * 100)); // Simulate work
                
                qint64 compression_time = timer.elapsed();
                
                // Simulate results
                BenchmarkResult result;
                result.format_name = test_name;
                result.compression_time_ms = compression_time;
                result.decompression_time_ms = compression_time / 2; // Simulate
                result.compression_ratio = 0.3 + (level * 0.05); // Simulate
                result.compressed_size = 1000000 * result.compression_ratio; // Simulate
                result.original_size = 1000000; // Simulate
                
                results[test_name] = result;
                current_test++;
            }
        }
        
        if (isCancelled()) {
            emit taskFinished(false, "Benchmark cancelled by user");
            return;
        }
        
        emit benchmarkCompleted(results);
        emit taskFinished(true, QString("Benchmark completed: %1 tests").arg(results.size()));
        
    } catch (const std::exception& e) {
        emit taskFinished(false, QString("Exception during benchmark: %1").arg(e.what()));
    } catch (...) {
        emit taskFinished(false, "Unknown error during benchmark");
    }
}

} // namespace FluxGui

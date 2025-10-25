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

#pragma once

#include <QObject>
#include <QStringList>
#include <QMutex>
#include <QList>
#include <QMap>
#include <memory>

namespace FluxGui {

// Forward declarations
struct ArchiveInfo;
struct ArchiveEntry;
struct BenchmarkResult;

/**
 * @brief Extract options for GUI operations
 */
struct ExtractOptions {
    int overwrite_mode = 0;  // 0=Skip, 1=Overwrite, 2=Prompt
    bool preserve_permissions = true;
    bool preserve_timestamps = true;
    QString password;
    QStringList include_patterns;
    QStringList exclude_patterns;
};

/**
 * @brief Pack options for GUI operations
 */
struct PackOptions {
    int format = 1;  // Archive format (maps to Flux::ArchiveFormat)
    int compression_level = 3;
    int num_threads = 0;  // 0 = auto
    bool preserve_permissions = true;
    bool preserve_timestamps = true;
    QString password;
};

/**
 * @brief Archive information structure
 */
struct ArchiveInfo {
    QString path;
    int format;
    quint64 compressed_size;
    quint64 uncompressed_size;
    quint64 file_count;
    bool is_encrypted;
    QString creation_time;
};

/**
 * @brief Archive entry information
 */
struct ArchiveEntry {
    QString name;
    QString path;
    quint64 compressed_size;
    quint64 uncompressed_size;
    bool is_directory;
    QString modification_time;
    quint32 permissions;
};

/**
 * @brief Benchmark result structure
 */
struct BenchmarkResult {
    QString format_name;
    qint64 compression_time_ms;
    qint64 decompression_time_ms;
    double compression_ratio;
    quint64 compressed_size;
    quint64 original_size;
};

/**
 * @brief Asynchronous task executor for archive operations
 * 
 * This class provides a thread-safe interface for executing
 * archive operations in the background without blocking the GUI.
 */
class TaskExecutor : public QObject {
    Q_OBJECT

public:
    explicit TaskExecutor(QObject* parent = nullptr);
    ~TaskExecutor() override;

    // Delete copy and move operations
    TaskExecutor(const TaskExecutor&) = delete;
    TaskExecutor& operator=(const TaskExecutor&) = delete;
    TaskExecutor(TaskExecutor&&) = delete;
    TaskExecutor& operator=(TaskExecutor&&) = delete;

    /**
     * @brief Cancel the current operation
     */
    void cancel();

    /**
     * @brief Check if an operation is currently cancelled
     */
    bool isCancelled() const;

public slots:
    /**
     * @brief Extract an archive to the specified directory
     * @param archive_path Path to the archive file
     * @param output_path Output directory path
     * @param options Extraction options
     */
    void extractArchive(const QString& archive_path, const QString& output_path,
                       const ExtractOptions& options = ExtractOptions{});

    /**
     * @brief Create an archive from input files/directories
     * @param input_paths List of input file/directory paths
     * @param output_path Output archive path
     * @param options Packing options
     */
    void createArchive(const QStringList& input_paths, const QString& output_path,
                      const PackOptions& options = PackOptions{});

    /**
     * @brief List the contents of an archive
     * @param archive_path Path to the archive file
     * @param password Password for encrypted archives (optional)
     */
    void listArchiveContents(const QString& archive_path, const QString& password = QString{});

    /**
     * @brief Verify the integrity of an archive
     * @param archive_path Path to the archive file
     * @param password Password for encrypted archives (optional)
     */
    void verifyArchive(const QString& archive_path, const QString& password = QString{});

    /**
     * @brief Run compression benchmark on input files
     * @param input_paths List of input file paths
     * @param output_dir Directory to store benchmark results
     */
    void benchmarkCompression(const QStringList& input_paths, const QString& output_dir);

signals:
    /**
     * @brief Emitted when a task starts
     * @param task_name Name of the task being started
     */
    void taskStarted(const QString& task_name);

    /**
     * @brief Emitted when task progress is updated
     * @param filename Current file being processed
     * @param progress Progress as a value between 0.0 and 1.0
     * @param processed Number of bytes processed
     * @param total Total number of bytes to process
     */
    void progressUpdated(const QString& filename, float progress, quint64 processed, quint64 total);

    /**
     * @brief Emitted when a task finishes
     * @param success Whether the task completed successfully
     * @param message Result message or error description
     */
    void taskFinished(bool success, const QString& message);

    /**
     * @brief Emitted when archive contents are ready
     * @param info Archive information
     * @param entries List of archive entries
     */
    void archiveContentsReady(const ArchiveInfo& info, const QList<ArchiveEntry>& entries);

    /**
     * @brief Emitted when benchmark results are ready
     * @param results Map of benchmark results by format name
     */
    void benchmarkCompleted(const QMap<QString, BenchmarkResult>& results);

private:
    mutable QMutex mutex_;
    bool is_cancelled_;
};

} // namespace FluxGui

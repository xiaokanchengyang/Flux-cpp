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

#include "async_task_executor.h"
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>
#include <QVariantMap>
#include <functional>
#include <memory>
#include <atomic>

namespace FluxGui {

/**
 * @brief Task type enumeration for worker thread
 */
enum class WorkerTaskType {
    Extract,
    Pack,
    List,
    Verify,
    Benchmark
};

/**
 * @brief Task data structure for worker thread
 */
struct WorkerTask {
    WorkerTaskType type;
    QVariantMap parameters;
    std::shared_ptr<TaskExecutor> executor;
    
    WorkerTask() = default;
    WorkerTask(WorkerTaskType t, const QVariantMap& params, std::shared_ptr<TaskExecutor> exec)
        : type(t), parameters(params), executor(std::move(exec)) {}
};

/**
 * @brief Asynchronous worker thread for archive operations
 * 
 * This class provides a dedicated thread for executing archive operations
 * without blocking the main GUI thread. It supports task queuing and
 * cancellation.
 */
class AsyncWorker : public QThread {
    Q_OBJECT

public:
    explicit AsyncWorker(QObject* parent = nullptr);
    ~AsyncWorker() override;

    // Delete copy and move operations
    AsyncWorker(const AsyncWorker&) = delete;
    AsyncWorker& operator=(const AsyncWorker&) = delete;
    AsyncWorker(AsyncWorker&&) = delete;
    AsyncWorker& operator=(AsyncWorker&&) = delete;

    /**
     * @brief Submit a task for execution
     * @param task Task to execute
     */
    void submitTask(const WorkerTask& task);

    /**
     * @brief Cancel the current task and clear the queue
     */
    void cancelAllTasks();

    /**
     * @brief Check if the worker is currently processing a task
     */
    bool isBusy() const;

    /**
     * @brief Get the number of queued tasks
     */
    int queuedTaskCount() const;

    /**
     * @brief Stop the worker thread gracefully
     */
    void stopWorker();

signals:
    /**
     * @brief Emitted when a task starts execution
     * @param task_name Name of the task
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

protected:
    /**
     * @brief Main thread execution loop
     */
    void run() override;

private slots:
    /**
     * @brief Handle task executor signals
     */
    void onTaskStarted(const QString& task_name);
    void onProgressUpdated(const QString& filename, float progress, quint64 processed, quint64 total);
    void onTaskFinished(bool success, const QString& message);
    void onArchiveContentsReady(const ArchiveInfo& info, const QList<ArchiveEntry>& entries);
    void onBenchmarkCompleted(const QMap<QString, BenchmarkResult>& results);

private:
    /**
     * @brief Execute a single task
     * @param task Task to execute
     */
    void executeTask(const WorkerTask& task);

    /**
     * @brief Get the next task from the queue
     * @return Next task or null task if queue is empty
     */
    WorkerTask getNextTask();

    /**
     * @brief Convert task type to string for display
     * @param type Task type
     * @return Human-readable task name
     */
    QString taskTypeToString(WorkerTaskType type) const;

private:
    mutable QMutex mutex_;
    QWaitCondition condition_;
    QQueue<WorkerTask> task_queue_;
    std::atomic<bool> should_stop_;
    std::atomic<bool> is_busy_;
    std::shared_ptr<TaskExecutor> current_executor_;
};

/**
 * @brief Factory functions for creating worker tasks
 */
namespace WorkerTaskFactory {

/**
 * @brief Create an extract task
 */
WorkerTask createExtractTask(const QString& archive_path, const QString& output_path,
                           const ExtractOptions& options, std::shared_ptr<TaskExecutor> executor);

/**
 * @brief Create a pack task
 */
WorkerTask createPackTask(const QStringList& input_paths, const QString& output_path,
                        const PackOptions& options, std::shared_ptr<TaskExecutor> executor);

/**
 * @brief Create a list task
 */
WorkerTask createListTask(const QString& archive_path, const QString& password,
                        std::shared_ptr<TaskExecutor> executor);

/**
 * @brief Create a verify task
 */
WorkerTask createVerifyTask(const QString& archive_path, const QString& password,
                          std::shared_ptr<TaskExecutor> executor);

/**
 * @brief Create a benchmark task
 */
WorkerTask createBenchmarkTask(const QStringList& input_paths, const QString& output_dir,
                             std::shared_ptr<TaskExecutor> executor);

} // namespace WorkerTaskFactory

} // namespace FluxGui

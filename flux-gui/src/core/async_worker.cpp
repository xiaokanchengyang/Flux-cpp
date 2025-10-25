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

#include "async_worker.h"
#include <QMutexLocker>
#include <QDebug>
#include <QCoreApplication>

namespace FluxGui {

AsyncWorker::AsyncWorker(QObject* parent)
    : QThread(parent)
    , should_stop_(false)
    , is_busy_(false)
{
}

AsyncWorker::~AsyncWorker() {
    stopWorker();
}

void AsyncWorker::submitTask(const WorkerTask& task) {
    QMutexLocker locker(&mutex_);
    
    if (should_stop_) {
        return;
    }
    
    task_queue_.enqueue(task);
    condition_.wakeOne();
    
    // Start the thread if it's not running
    if (!isRunning()) {
        start();
    }
}

void AsyncWorker::cancelAllTasks() {
    QMutexLocker locker(&mutex_);
    
    // Cancel current executor if running
    if (current_executor_) {
        current_executor_->cancel();
    }
    
    // Clear the task queue
    task_queue_.clear();
}

bool AsyncWorker::isBusy() const {
    return is_busy_.load();
}

int AsyncWorker::queuedTaskCount() const {
    QMutexLocker locker(&mutex_);
    return task_queue_.size();
}

void AsyncWorker::stopWorker() {
    {
        QMutexLocker locker(&mutex_);
        should_stop_ = true;
        
        // Cancel current task
        if (current_executor_) {
            current_executor_->cancel();
        }
        
        // Clear queue
        task_queue_.clear();
        condition_.wakeAll();
    }
    
    // Wait for thread to finish
    if (isRunning()) {
        wait(5000); // Wait up to 5 seconds
        if (isRunning()) {
            terminate(); // Force terminate if needed
            wait(1000);
        }
    }
}

void AsyncWorker::run() {
    while (!should_stop_) {
        WorkerTask task = getNextTask();
        
        if (should_stop_) {
            break;
        }
        
        if (task.executor) {
            executeTask(task);
        }
    }
}

WorkerTask AsyncWorker::getNextTask() {
    QMutexLocker locker(&mutex_);
    
    while (task_queue_.isEmpty() && !should_stop_) {
        condition_.wait(&mutex_);
    }
    
    if (should_stop_ || task_queue_.isEmpty()) {
        return WorkerTask{}; // Return empty task
    }
    
    return task_queue_.dequeue();
}

void AsyncWorker::executeTask(const WorkerTask& task) {
    if (should_stop_) {
        return;
    }
    
    is_busy_ = true;
    current_executor_ = task.executor;
    
    // Connect executor signals to worker signals
    connect(task.executor.get(), &TaskExecutor::taskStarted,
            this, &AsyncWorker::onTaskStarted, Qt::DirectConnection);
    connect(task.executor.get(), &TaskExecutor::progressUpdated,
            this, &AsyncWorker::onProgressUpdated, Qt::DirectConnection);
    connect(task.executor.get(), &TaskExecutor::taskFinished,
            this, &AsyncWorker::onTaskFinished, Qt::DirectConnection);
    connect(task.executor.get(), &TaskExecutor::archiveContentsReady,
            this, &AsyncWorker::onArchiveContentsReady, Qt::DirectConnection);
    connect(task.executor.get(), &TaskExecutor::benchmarkCompleted,
            this, &AsyncWorker::onBenchmarkCompleted, Qt::DirectConnection);
    
    try {
        // Execute the appropriate task based on type
        switch (task.type) {
        case WorkerTaskType::Extract: {
            QString archive_path = task.parameters.value("archive_path").toString();
            QString output_path = task.parameters.value("output_path").toString();
            ExtractOptions options = task.parameters.value("options").value<ExtractOptions>();
            
            task.executor->extractArchive(archive_path, output_path, options);
            break;
        }
        case WorkerTaskType::Pack: {
            QStringList input_paths = task.parameters.value("input_paths").toStringList();
            QString output_path = task.parameters.value("output_path").toString();
            PackOptions options = task.parameters.value("options").value<PackOptions>();
            
            task.executor->createArchive(input_paths, output_path, options);
            break;
        }
        case WorkerTaskType::List: {
            QString archive_path = task.parameters.value("archive_path").toString();
            QString password = task.parameters.value("password").toString();
            
            task.executor->listArchiveContents(archive_path, password);
            break;
        }
        case WorkerTaskType::Verify: {
            QString archive_path = task.parameters.value("archive_path").toString();
            QString password = task.parameters.value("password").toString();
            
            task.executor->verifyArchive(archive_path, password);
            break;
        }
        case WorkerTaskType::Benchmark: {
            QStringList input_paths = task.parameters.value("input_paths").toStringList();
            QString output_dir = task.parameters.value("output_dir").toString();
            
            task.executor->benchmarkCompression(input_paths, output_dir);
            break;
        }
        }
    } catch (const std::exception& e) {
        emit taskFinished(false, QString("Exception during task execution: %1").arg(e.what()));
    } catch (...) {
        emit taskFinished(false, "Unknown error during task execution");
    }
    
    // Disconnect signals
    disconnect(task.executor.get(), nullptr, this, nullptr);
    
    current_executor_.reset();
    is_busy_ = false;
}

QString AsyncWorker::taskTypeToString(WorkerTaskType type) const {
    switch (type) {
    case WorkerTaskType::Extract:
        return "Extract Archive";
    case WorkerTaskType::Pack:
        return "Create Archive";
    case WorkerTaskType::List:
        return "List Contents";
    case WorkerTaskType::Verify:
        return "Verify Archive";
    case WorkerTaskType::Benchmark:
        return "Benchmark Compression";
    default:
        return "Unknown Task";
    }
}

// Slot implementations for signal forwarding
void AsyncWorker::onTaskStarted(const QString& task_name) {
    emit taskStarted(task_name);
}

void AsyncWorker::onProgressUpdated(const QString& filename, float progress, 
                                   quint64 processed, quint64 total) {
    emit progressUpdated(filename, progress, processed, total);
}

void AsyncWorker::onTaskFinished(bool success, const QString& message) {
    emit taskFinished(success, message);
}

void AsyncWorker::onArchiveContentsReady(const ArchiveInfo& info, 
                                        const QList<ArchiveEntry>& entries) {
    emit archiveContentsReady(info, entries);
}

void AsyncWorker::onBenchmarkCompleted(const QMap<QString, BenchmarkResult>& results) {
    emit benchmarkCompleted(results);
}

// Factory function implementations
namespace WorkerTaskFactory {

WorkerTask createExtractTask(const QString& archive_path, const QString& output_path,
                           const ExtractOptions& options, std::shared_ptr<TaskExecutor> executor) {
    QVariantMap params;
    params["archive_path"] = archive_path;
    params["output_path"] = output_path;
    params["options"] = QVariant::fromValue(options);
    
    return WorkerTask(WorkerTaskType::Extract, params, std::move(executor));
}

WorkerTask createPackTask(const QStringList& input_paths, const QString& output_path,
                        const PackOptions& options, std::shared_ptr<TaskExecutor> executor) {
    QVariantMap params;
    params["input_paths"] = input_paths;
    params["output_path"] = output_path;
    params["options"] = QVariant::fromValue(options);
    
    return WorkerTask(WorkerTaskType::Pack, params, std::move(executor));
}

WorkerTask createListTask(const QString& archive_path, const QString& password,
                        std::shared_ptr<TaskExecutor> executor) {
    QVariantMap params;
    params["archive_path"] = archive_path;
    params["password"] = password;
    
    return WorkerTask(WorkerTaskType::List, params, std::move(executor));
}

WorkerTask createVerifyTask(const QString& archive_path, const QString& password,
                          std::shared_ptr<TaskExecutor> executor) {
    QVariantMap params;
    params["archive_path"] = archive_path;
    params["password"] = password;
    
    return WorkerTask(WorkerTaskType::Verify, params, std::move(executor));
}

WorkerTask createBenchmarkTask(const QStringList& input_paths, const QString& output_dir,
                             std::shared_ptr<TaskExecutor> executor) {
    QVariantMap params;
    params["input_paths"] = input_paths;
    params["output_dir"] = output_dir;
    
    return WorkerTask(WorkerTaskType::Benchmark, params, std::move(executor));
}

} // namespace WorkerTaskFactory

} // namespace FluxGui

// Register metatypes for Qt's signal/slot system
Q_DECLARE_METATYPE(FluxGui::ExtractOptions)
Q_DECLARE_METATYPE(FluxGui::PackOptions)
Q_DECLARE_METATYPE(FluxGui::ArchiveInfo)
Q_DECLARE_METATYPE(FluxGui::ArchiveEntry)
Q_DECLARE_METATYPE(FluxGui::BenchmarkResult)

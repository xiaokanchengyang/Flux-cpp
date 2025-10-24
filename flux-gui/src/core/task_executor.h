#pragma once

#include <flux-core/functional/result.h>
#include <flux-core/error/error_handling.h>
#include <QObject>
#include <QThread>
#include <QVariantMap>
#include <QStringList>
#include <QElapsedTimer>
#include <functional>
#include <unordered_map>
#include <memory>

namespace FluxGUI::Core {

/**
 * @brief Task types for GUI operations
 */
enum class TaskType {
    Extract,
    Pack,
    List,
    Benchmark,
    Verify
};

/**
 * @brief Task execution context
 */
struct TaskContext {
    TaskType type;
    QVariantMap parameters;
    std::function<void(const QString&, float)> progressCallback;
    std::function<void(bool, const QString&)> completionCallback;
    std::function<bool()> shouldStopCallback;
};

/**
 * @brief Task execution result
 */
struct TaskResult {
    bool success;
    QString message;
    QVariantMap data;
    qint64 executionTimeMs;
};

/**
 * @brief Modern task executor using functional programming
 */
class TaskExecutor : public QObject {
    Q_OBJECT

public:
    using TaskHandler = std::function<Flux::Error::ErrorResult<TaskResult>(const TaskContext&)>;
    using ExecutionResult = Flux::Error::ErrorResult<TaskResult>;
    
    explicit TaskExecutor(QObject* parent = nullptr);
    ~TaskExecutor() override;
    
    /**
     * @brief Execute task asynchronously
     */
    void executeAsync(const TaskContext& context);
    
    /**
     * @brief Register custom task handler
     */
    void registerHandler(TaskType type, TaskHandler handler);
    
    /**
     * @brief Cancel current task
     */
    void cancelCurrentTask();
    
    /**
     * @brief Check if task is running
     */
    bool isRunning() const;

signals:
    void progressUpdated(const QString& currentItem, float progress);
    void taskCompleted(bool success, const QString& message, const QVariantMap& data);
    void taskStarted(TaskType type);
    void taskCancelled();

private slots:
    void onWorkerFinished();

private:
    class WorkerThread;
    std::unique_ptr<WorkerThread> worker_;
    std::unordered_map<TaskType, TaskHandler> handlers_;
    
    /**
     * @brief Initialize default handlers
     */
    void initializeDefaultHandlers();
    
    /**
     * @brief Create worker thread for task execution
     */
    void createWorkerThread(const TaskContext& context);
};

/**
 * @brief Worker thread for task execution
 */
class TaskExecutor::WorkerThread : public QThread {
    Q_OBJECT

public:
    explicit WorkerThread(const TaskContext& context, 
                         const std::unordered_map<TaskType, TaskHandler>& handlers,
                         QObject* parent = nullptr);
    
    void requestStop();

signals:
    void progressUpdated(const QString& currentItem, float progress);
    void taskCompleted(bool success, const QString& message, const QVariantMap& data);

protected:
    void run() override;

private:
    TaskContext context_;
    const std::unordered_map<TaskType, TaskHandler>& handlers_;
    std::atomic<bool> shouldStop_{false};
    
    /**
     * @brief Execute task with error handling
     */
    TaskResult executeTask();
    
    /**
     * @brief Update progress safely
     */
    void updateProgress(const QString& item, float progress);
};

/**
 * @brief Default task handlers
 */
namespace DefaultHandlers {

/**
 * @brief Extract task handler
 */
Flux::Error::ErrorResult<TaskResult> handleExtract(const TaskContext& context);

/**
 * @brief Pack task handler
 */
Flux::Error::ErrorResult<TaskResult> handlePack(const TaskContext& context);

/**
 * @brief List task handler
 */
Flux::Error::ErrorResult<TaskResult> handleList(const TaskContext& context);

/**
 * @brief Benchmark task handler
 */
Flux::Error::ErrorResult<TaskResult> handleBenchmark(const TaskContext& context);

/**
 * @brief Verify task handler
 */
Flux::Error::ErrorResult<TaskResult> handleVerify(const TaskContext& context);

} // namespace DefaultHandlers

/**
 * @brief Utility functions for task execution
 */
namespace TaskUtils {

/**
 * @brief Create task context from parameters
 */
TaskContext createContext(TaskType type, const QVariantMap& params);

/**
 * @brief Simulate progress for demo purposes
 */
void simulateProgress(const TaskContext& context, int steps = 100, int delayMs = 50);

/**
 * @brief Convert TaskType to string
 */
QString taskTypeToString(TaskType type);

/**
 * @brief Measure execution time
 */
template<typename F>
auto measureExecution(F&& func) -> std::pair<std::invoke_result_t<F>, qint64> {
    QElapsedTimer timer;
    timer.start();
    
    if constexpr (std::is_void_v<std::invoke_result_t<F>>) {
        std::invoke(std::forward<F>(func));
        return {void{}, timer.elapsed()};
    } else {
        auto result = std::invoke(std::forward<F>(func));
        return {std::move(result), timer.elapsed()};
    }
}

} // namespace TaskUtils

} // namespace FluxGUI::Core

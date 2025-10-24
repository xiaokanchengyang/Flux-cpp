#pragma once

#include "../core/task_executor.h"
#include <flux-core/functional/operations.h>
#include <flux-core/error/error_handling.h>
#include <QObject>
#include <QStringList>
#include <functional>
#include <unordered_map>

namespace FluxGUI::Components {

/**
 * @brief GUI operation configuration
 */
struct OperationConfig {
    QString archivePath;
    QString outputPath;
    QStringList inputFiles;
    QStringList selectedFiles;
    QString password;
    bool overwriteExisting = false;
    bool createDirectories = true;
    int compressionLevel = 6;
    QString compressionMethod = "deflate";
};

/**
 * @brief Operation dispatcher for GUI actions
 * Bridges GUI events to core functionality using functional programming
 */
class OperationDispatcher : public QObject {
    Q_OBJECT

public:
    using OperationResult = Flux::Error::ErrorResult<void>;
    using ProgressCallback = std::function<void(const QString&, float)>;
    using CompletionCallback = std::function<void(bool, const QString&)>;
    
    explicit OperationDispatcher(QObject* parent = nullptr);
    ~OperationDispatcher() override;
    
    /**
     * @brief Extract archive with functional error handling
     */
    void extractArchive(const OperationConfig& config,
                       ProgressCallback progressCallback = nullptr,
                       CompletionCallback completionCallback = nullptr);
    
    /**
     * @brief Create archive from files
     */
    void createArchive(const OperationConfig& config,
                      ProgressCallback progressCallback = nullptr,
                      CompletionCallback completionCallback = nullptr);
    
    /**
     * @brief List archive contents
     */
    void listArchiveContents(const QString& archivePath,
                           std::function<void(const QStringList&)> resultCallback = nullptr,
                           CompletionCallback completionCallback = nullptr);
    
    /**
     * @brief Verify archive integrity
     */
    void verifyArchive(const QString& archivePath,
                      CompletionCallback completionCallback = nullptr);
    
    /**
     * @brief Run compression benchmark
     */
    void runBenchmark(const QStringList& inputFiles,
                     std::function<void(const QVariantMap&)> resultCallback = nullptr,
                     CompletionCallback completionCallback = nullptr);
    
    /**
     * @brief Cancel current operation
     */
    void cancelCurrentOperation();
    
    /**
     * @brief Check if operation is running
     */
    bool isOperationRunning() const;

signals:
    void operationStarted(const QString& operationType);
    void operationProgress(const QString& currentItem, float progress);
    void operationCompleted(bool success, const QString& message);
    void operationCancelled();
    void archiveContentsReady(const QStringList& fileList);
    void benchmarkResultsReady(const QVariantMap& results);

private:
    Core::TaskExecutor* taskExecutor_;
    
    /**
     * @brief Create task context from operation config
     */
    Core::TaskContext createTaskContext(Core::TaskType type, 
                                       const OperationConfig& config,
                                       ProgressCallback progressCallback,
                                       CompletionCallback completionCallback);
    
    /**
     * @brief Validate operation configuration
     */
    OperationResult validateConfig(const OperationConfig& config, Core::TaskType operation);
    
    /**
     * @brief Handle task completion
     */
    void handleTaskCompletion(bool success, const QString& message, const QVariantMap& data);
    
    /**
     * @brief Convert operation config to QVariantMap
     */
    QVariantMap configToVariantMap(const OperationConfig& config);
};

/**
 * @brief Functional utilities for operation dispatch
 */
namespace DispatchUtils {

/**
 * @brief Validate file paths
 */
Flux::Error::ErrorResult<void> validatePaths(const QStringList& paths);

/**
 * @brief Check archive format support
 */
Flux::Error::ErrorResult<void> validateArchiveFormat(const QString& path);

/**
 * @brief Create default operation config
 */
OperationConfig createDefaultConfig();

/**
 * @brief Merge operation configs
 */
OperationConfig mergeConfigs(const OperationConfig& base, const OperationConfig& override);

/**
 * @brief Convert Qt paths to std::filesystem::path
 */
std::vector<std::filesystem::path> convertPaths(const QStringList& paths);

/**
 * @brief Safe string conversion with error handling
 */
template<typename T>
Flux::Error::ErrorResult<T> safeConvert(const QString& str);

} // namespace DispatchUtils

} // namespace FluxGUI::Components

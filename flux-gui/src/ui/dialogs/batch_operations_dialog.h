#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QListWidget>
#include <QTreeWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QTextEdit>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QQueue>
#include <QFileInfo>
#include <QDateTime>
#include <memory>

QT_BEGIN_NAMESPACE
class QButtonGroup;
class QSplitter;
class QScrollArea;
QT_END_NAMESPACE

namespace FluxGUI::UI::Dialogs {

/**
 * Batch Operations Dialog
 * 
 * Advanced dialog for performing batch operations on multiple archives:
 * - Multi-archive selection and management
 * - Batch compression with different algorithms
 * - Batch extraction with path management
 * - Batch validation and integrity checking
 * - Progress tracking with detailed statistics
 * - Error handling and recovery options
 * - Operation scheduling and queuing
 */
class BatchOperationsDialog : public QDialog {
    Q_OBJECT

public:
    enum class OperationType {
        Compress,       // Create archives from files/folders
        Extract,        // Extract multiple archives
        Validate,       // Validate archive integrity
        Convert,        // Convert between archive formats
        Optimize,       // Re-compress for better compression
        Split,          // Split large archives
        Merge,          // Merge multiple archives
        Encrypt,        // Add encryption to archives
        Decrypt         // Remove encryption from archives
    };

    enum class CompressionLevel {
        Store = 0,      // No compression
        Fastest = 1,    // Fastest compression
        Fast = 3,       // Fast compression
        Normal = 6,     // Normal compression
        Maximum = 9,    // Maximum compression
        Ultra = 22      // Ultra compression (7z only)
    };

    enum class ArchiveFormat {
        Auto,           // Auto-detect from extension
        Zip,            // ZIP format
        SevenZ,         // 7-Zip format
        Tar,            // TAR format
        TarGz,          // TAR.GZ format
        TarBz2,         // TAR.BZ2 format
        TarXz,          // TAR.XZ format
        Rar             // RAR format (extract only)
    };

    struct OperationSettings {
        OperationType operation{OperationType::Compress};
        ArchiveFormat format{ArchiveFormat::Auto};
        CompressionLevel compressionLevel{CompressionLevel::Normal};
        QString outputDirectory;
        QString archivePassword;
        bool createSubfolders{true};
        bool overwriteExisting{false};
        bool preserveTimestamps{true};
        bool preservePermissions{true};
        bool deleteSourceAfterOperation{false};
        bool validateAfterOperation{true};
        int maxConcurrentOperations{4};
        qint64 maxArchiveSize{0}; // 0 = no limit
        QStringList excludePatterns;
        QStringList includePatterns;
    };

    struct BatchItem {
        QString sourcePath;
        QString targetPath;
        QString archiveName;
        ArchiveFormat format{ArchiveFormat::Auto};
        qint64 sourceSize{0};
        qint64 estimatedOutputSize{0};
        QDateTime createdTime;
        QString status;
        QString errorMessage;
        double progress{0.0};
        bool completed{false};
        bool hasError{false};
    };

    struct BatchStatistics {
        int totalItems{0};
        int completedItems{0};
        int failedItems{0};
        int skippedItems{0};
        qint64 totalSourceSize{0};
        qint64 totalOutputSize{0};
        qint64 processedSize{0};
        double compressionRatio{0.0};
        QDateTime startTime;
        QDateTime estimatedEndTime;
        qint64 elapsedTime{0};
        qint64 remainingTime{0};
        double averageSpeed{0.0}; // bytes per second
    };

    explicit BatchOperationsDialog(QWidget* parent = nullptr);
    ~BatchOperationsDialog() override;

    // Operation configuration
    void setOperationType(OperationType type);
    OperationType operationType() const { return m_settings.operation; }

    void setOperationSettings(const OperationSettings& settings);
    OperationSettings operationSettings() const { return m_settings; }

    // Batch management
    void addBatchItem(const BatchItem& item);
    void addBatchItems(const QList<BatchItem>& items);
    void removeBatchItem(int index);
    void clearBatchItems();
    QList<BatchItem> batchItems() const { return m_batchItems; }

    // Execution control
    void startBatchOperation();
    void pauseBatchOperation();
    void resumeBatchOperation();
    void cancelBatchOperation();
    bool isBatchRunning() const { return m_batchRunning; }

    // Statistics
    BatchStatistics batchStatistics() const { return m_statistics; }

    // Presets
    void savePreset(const QString& name);
    void loadPreset(const QString& name);
    QStringList availablePresets() const;
    void deletePreset(const QString& name);

signals:
    // Operation signals
    void batchStarted();
    void batchPaused();
    void batchResumed();
    void batchCompleted(bool success);
    void batchCancelled();

    // Progress signals
    void itemStarted(int index, const BatchItem& item);
    void itemProgress(int index, double progress);
    void itemCompleted(int index, bool success, const QString& message);
    void statisticsUpdated(const BatchStatistics& stats);

    // Error signals
    void itemError(int index, const QString& error);
    void criticalError(const QString& error);

protected:
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void onOperationTypeChanged();
    void onSettingsChanged();
    void onAddFilesClicked();
    void onAddFoldersClicked();
    void onRemoveItemsClicked();
    void onClearAllClicked();
    void onStartClicked();
    void onPauseClicked();
    void onCancelClicked();
    void onPresetSaveClicked();
    void onPresetLoadClicked();
    void onPresetDeleteClicked();
    void onItemSelectionChanged();
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onProgressTimerTimeout();
    void onWorkerFinished();
    void onWorkerError(const QString& error);
    void onWorkerProgress(int index, double progress);

private:
    // UI initialization
    void initializeUI();
    void createOperationTab();
    void createSettingsTab();
    void createItemsTab();
    void createProgressTab();
    void createLogTab();
    void setupLayouts();
    void connectSignals();

    // UI updates
    void updateOperationUI();
    void updateSettingsUI();
    void updateItemsList();
    void updateProgressDisplay();
    void updateStatistics();
    void updateButtonStates();

    // Operation management
    void validateSettings();
    void prepareOperation();
    void startWorkerThread();
    void stopWorkerThread();

    // Item management
    void addFilesToBatch(const QStringList& files);
    void addFoldersToBatch(const QStringList& folders);
    void updateItemEstimates();
    BatchItem createBatchItem(const QString& sourcePath) const;

    // Statistics calculation
    void calculateStatistics();
    void updateTimeEstimates();
    QString formatTime(qint64 milliseconds) const;
    QString formatSize(qint64 bytes) const;
    QString formatSpeed(double bytesPerSecond) const;

    // Preset management
    void saveSettingsToPreset(const QString& name);
    void loadSettingsFromPreset(const QString& name);
    QString getPresetFilePath(const QString& name) const;

    // Logging
    void logMessage(const QString& message, const QString& level = "INFO");
    void logError(const QString& error);
    void logWarning(const QString& warning);

    // Validation
    bool validateBatchItems() const;
    bool validateOutputDirectory() const;
    bool validateSettings() const;

private:
    // Settings
    OperationSettings m_settings;
    QList<BatchItem> m_batchItems;
    BatchStatistics m_statistics;

    // State
    bool m_batchRunning{false};
    bool m_batchPaused{false};
    int m_currentItemIndex{0};

    // UI components
    QTabWidget* m_tabWidget{nullptr};

    // Operation tab
    QWidget* m_operationTab{nullptr};
    QComboBox* m_operationTypeCombo{nullptr};
    QComboBox* m_archiveFormatCombo{nullptr};
    QComboBox* m_compressionLevelCombo{nullptr};

    // Settings tab
    QWidget* m_settingsTab{nullptr};
    QLineEdit* m_outputDirectoryEdit{nullptr};
    QPushButton* m_browseOutputButton{nullptr};
    QLineEdit* m_passwordEdit{nullptr};
    QCheckBox* m_createSubfoldersCheck{nullptr};
    QCheckBox* m_overwriteExistingCheck{nullptr};
    QCheckBox* m_preserveTimestampsCheck{nullptr};
    QCheckBox* m_preservePermissionsCheck{nullptr};
    QCheckBox* m_deleteSourceCheck{nullptr};
    QCheckBox* m_validateAfterCheck{nullptr};
    QSpinBox* m_maxConcurrentSpin{nullptr};
    QSpinBox* m_maxArchiveSizeSpin{nullptr};
    QTextEdit* m_excludePatternsEdit{nullptr};
    QTextEdit* m_includePatternsEdit{nullptr};

    // Items tab
    QWidget* m_itemsTab{nullptr};
    QTreeWidget* m_itemsTree{nullptr};
    QPushButton* m_addFilesButton{nullptr};
    QPushButton* m_addFoldersButton{nullptr};
    QPushButton* m_removeItemsButton{nullptr};
    QPushButton* m_clearAllButton{nullptr};
    QLabel* m_itemsCountLabel{nullptr};
    QLabel* m_totalSizeLabel{nullptr};

    // Progress tab
    QWidget* m_progressTab{nullptr};
    QProgressBar* m_overallProgressBar{nullptr};
    QLabel* m_currentItemLabel{nullptr};
    QProgressBar* m_currentItemProgressBar{nullptr};
    QLabel* m_statisticsLabel{nullptr};
    QLabel* m_timeRemainingLabel{nullptr};
    QLabel* m_speedLabel{nullptr};

    // Log tab
    QWidget* m_logTab{nullptr};
    QTextEdit* m_logTextEdit{nullptr};
    QPushButton* m_clearLogButton{nullptr};
    QPushButton* m_saveLogButton{nullptr};

    // Control buttons
    QHBoxLayout* m_buttonLayout{nullptr};
    QPushButton* m_startButton{nullptr};
    QPushButton* m_pauseButton{nullptr};
    QPushButton* m_cancelButton{nullptr};
    QPushButton* m_closeButton{nullptr};

    // Preset management
    QGroupBox* m_presetGroup{nullptr};
    QComboBox* m_presetCombo{nullptr};
    QPushButton* m_savePresetButton{nullptr};
    QPushButton* m_loadPresetButton{nullptr};
    QPushButton* m_deletePresetButton{nullptr};

    // Worker thread
    QThread* m_workerThread{nullptr};
    class BatchWorker* m_worker{nullptr};

    // Timers
    QTimer* m_progressTimer{nullptr};
    QTimer* m_statisticsTimer{nullptr};

    // Constants
    static constexpr int PROGRESS_UPDATE_INTERVAL = 250;
    static constexpr int STATISTICS_UPDATE_INTERVAL = 1000;
    static constexpr int MAX_LOG_ENTRIES = 10000;
    static constexpr qint64 DEFAULT_MAX_ARCHIVE_SIZE = 4LL * 1024 * 1024 * 1024; // 4GB
};

/**
 * Batch Operation Worker
 */
class BatchWorker : public QObject {
    Q_OBJECT

public:
    explicit BatchWorker(QObject* parent = nullptr);

    void setBatchItems(const QList<BatchOperationsDialog::BatchItem>& items);
    void setOperationSettings(const BatchOperationsDialog::OperationSettings& settings);

public slots:
    void startBatch();
    void pauseBatch();
    void resumeBatch();
    void cancelBatch();

signals:
    void itemStarted(int index);
    void itemProgress(int index, double progress);
    void itemCompleted(int index, bool success, const QString& message);
    void batchCompleted(bool success);
    void error(const QString& error);

private slots:
    void processNextItem();

private:
    void processItem(int index);
    void compressItem(const BatchOperationsDialog::BatchItem& item);
    void extractItem(const BatchOperationsDialog::BatchItem& item);
    void validateItem(const BatchOperationsDialog::BatchItem& item);
    void convertItem(const BatchOperationsDialog::BatchItem& item);

private:
    QList<BatchOperationsDialog::BatchItem> m_batchItems;
    BatchOperationsDialog::OperationSettings m_settings;
    int m_currentIndex{0};
    bool m_paused{false};
    bool m_cancelled{false};
    QMutex m_mutex;
};

} // namespace FluxGUI::UI::Dialogs

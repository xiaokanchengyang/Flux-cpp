#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QSlider>
#include <QSpinBox>
#include <QCheckBox>
#include <QProgressBar>
#include <QListWidget>
#include <QTreeWidget>
#include <QSplitter>
#include <QGroupBox>
#include <QTabWidget>
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QTimer>
#include <QPropertyAnimation>
#include <memory>

namespace FluxGUI::UI::Widgets {

/**
 * @brief Advanced compression widget with modern interface
 * 
 * The CompressionWidget provides a comprehensive compression interface featuring:
 * - Drag & drop file/folder selection
 * - Multiple compression formats (ZIP, 7Z, TAR, etc.)
 * - Advanced compression settings
 * - Real-time compression preview
 * - Batch compression support
 * - Progress monitoring with detailed statistics
 */
class CompressionWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Compression formats supported
     */
    enum class CompressionFormat {
        ZIP,
        SevenZip,
        TAR,
        TAR_GZ,
        TAR_BZ2,
        TAR_XZ,
        RAR  // Create support if available
    };

    /**
     * @brief Compression levels
     */
    enum class CompressionLevel {
        Store = 0,      // No compression
        Fastest = 1,    // Fastest compression
        Fast = 3,       // Fast compression
        Normal = 6,     // Normal compression
        Maximum = 9,    // Maximum compression
        Ultra = 10      // Ultra compression (7z only)
    };

    /**
     * @brief Construct the compression widget
     * @param parent Parent widget
     */
    explicit CompressionWidget(QWidget* parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~CompressionWidget() override = default;

public slots:
    /**
     * @brief Add files to compression list
     * @param filePaths List of file paths to add
     */
    void addFiles(const QStringList& filePaths);
    
    /**
     * @brief Add directory to compression list
     * @param directoryPath Path to directory
     */
    void addDirectory(const QString& directoryPath);
    
    /**
     * @brief Clear all files from compression list
     */
    void clearFiles();
    
    /**
     * @brief Set output directory
     * @param outputPath Path to output directory
     */
    void setOutputDirectory(const QString& outputPath);
    
    /**
     * @brief Start compression process
     */
    void startCompression();
    
    /**
     * @brief Cancel compression process
     */
    void cancelCompression();

signals:
    /**
     * @brief Emitted when compression starts
     * @param outputPath Path to output archive
     */
    void compressionStarted(const QString& outputPath);
    
    /**
     * @brief Emitted when compression progresses
     * @param percentage Progress percentage (0-100)
     * @param currentFile Currently processing file
     */
    void compressionProgress(int percentage, const QString& currentFile);
    
    /**
     * @brief Emitted when compression completes
     * @param outputPath Path to created archive
     * @param success True if compression succeeded
     */
    void compressionCompleted(const QString& outputPath, bool success);
    
    /**
     * @brief Emitted when compression is cancelled
     */
    void compressionCancelled();
    
    /**
     * @brief Emitted when an error occurs
     * @param error Error message
     */
    void errorOccurred(const QString& error);

protected:
    /**
     * @brief Handle drag enter events
     * @param event Drag enter event
     */
    void dragEnterEvent(QDragEnterEvent* event) override;
    
    /**
     * @brief Handle drop events
     * @param event Drop event
     */
    void dropEvent(QDropEvent* event) override;

private slots:
    /**
     * @brief Handle browse files button click
     */
    void browseFiles();
    
    /**
     * @brief Handle browse folder button click
     */
    void browseFolder();
    
    /**
     * @brief Handle browse output button click
     */
    void browseOutput();
    
    /**
     * @brief Handle remove selected files
     */
    void removeSelectedFiles();
    
    /**
     * @brief Handle compression format change
     */
    void onFormatChanged();
    
    /**
     * @brief Handle compression level change
     */
    void onCompressionLevelChanged();
    
    /**
     * @brief Handle advanced settings change
     */
    void onAdvancedSettingsChanged();
    
    /**
     * @brief Update compression preview
     */
    void updateCompressionPreview();
    
    /**
     * @brief Handle file list selection change
     */
    void onFileSelectionChanged();
    
    /**
     * @brief Update estimated size and time
     */
    void updateEstimates();

private:
    /**
     * @brief Initialize the user interface
     */
    void initializeUI();
    
    /**
     * @brief Create file selection section
     * @return File selection widget
     */
    QWidget* createFileSelectionSection();
    
    /**
     * @brief Create compression settings section
     * @return Settings widget
     */
    QWidget* createCompressionSettingsSection();
    
    /**
     * @brief Create output settings section
     * @return Output settings widget
     */
    QWidget* createOutputSettingsSection();
    
    /**
     * @brief Create progress section
     * @return Progress widget
     */
    QWidget* createProgressSection();
    
    /**
     * @brief Create advanced settings tab
     * @return Advanced settings widget
     */
    QWidget* createAdvancedSettingsTab();
    
    /**
     * @brief Create preview tab
     * @return Preview widget
     */
    QWidget* createPreviewTab();
    
    /**
     * @brief Setup drag and drop
     */
    void setupDragAndDrop();
    
    /**
     * @brief Connect signals and slots
     */
    void connectSignals();
    
    /**
     * @brief Update UI state based on current settings
     */
    void updateUIState();
    
    /**
     * @brief Validate compression settings
     * @return True if settings are valid
     */
    bool validateSettings();
    
    /**
     * @brief Get format display name
     * @param format Compression format
     * @return Display name
     */
    QString getFormatDisplayName(CompressionFormat format) const;
    
    /**
     * @brief Get format file extension
     * @param format Compression format
     * @return File extension
     */
    QString getFormatExtension(CompressionFormat format) const;
    
    /**
     * @brief Get compression level display name
     * @param level Compression level
     * @return Display name
     */
    QString getCompressionLevelDisplayName(CompressionLevel level) const;
    
    /**
     * @brief Calculate estimated compression ratio
     * @return Estimated ratio (0.0-1.0)
     */
    double calculateEstimatedRatio() const;
    
    /**
     * @brief Calculate total input size
     * @return Total size in bytes
     */
    qint64 calculateTotalSize() const;
    
    /**
     * @brief Format file size for display
     * @param bytes Size in bytes
     * @return Formatted size string
     */
    QString formatFileSize(qint64 bytes) const;
    
    /**
     * @brief Add file item to list
     * @param filePath Path to file
     * @param isDirectory True if item is directory
     */
    void addFileItem(const QString& filePath, bool isDirectory = false);

private:
    // Main layout
    QVBoxLayout* m_mainLayout = nullptr;
    QSplitter* m_mainSplitter = nullptr;
    
    // File selection section
    QWidget* m_fileSelectionWidget = nullptr;
    QVBoxLayout* m_fileSelectionLayout = nullptr;
    QHBoxLayout* m_fileButtonsLayout = nullptr;
    QPushButton* m_browseFilesButton = nullptr;
    QPushButton* m_browseFolderButton = nullptr;
    QPushButton* m_removeFilesButton = nullptr;
    QPushButton* m_clearFilesButton = nullptr;
    QTreeWidget* m_fileListWidget = nullptr;
    QLabel* m_fileCountLabel = nullptr;
    QLabel* m_totalSizeLabel = nullptr;
    
    // Settings section
    QTabWidget* m_settingsTabWidget = nullptr;
    
    // Basic settings tab
    QWidget* m_basicSettingsWidget = nullptr;
    QGridLayout* m_basicSettingsLayout = nullptr;
    QComboBox* m_formatComboBox = nullptr;
    QComboBox* m_compressionLevelComboBox = nullptr;
    QLineEdit* m_outputPathEdit = nullptr;
    QPushButton* m_browseOutputButton = nullptr;
    QLineEdit* m_archiveNameEdit = nullptr;
    QCheckBox* m_passwordProtectedCheckBox = nullptr;
    QLineEdit* m_passwordEdit = nullptr;
    QLineEdit* m_confirmPasswordEdit = nullptr;
    
    // Advanced settings tab
    QWidget* m_advancedSettingsWidget = nullptr;
    QGridLayout* m_advancedSettingsLayout = nullptr;
    QSpinBox* m_dictionarySizeSpinBox = nullptr;
    QSpinBox* m_wordSizeSpinBox = nullptr;
    QCheckBox* m_solidArchiveCheckBox = nullptr;
    QCheckBox* m_compressHeadersCheckBox = nullptr;
    QCheckBox* m_encryptFileNamesCheckBox = nullptr;
    QComboBox* m_encryptionMethodComboBox = nullptr;
    QSpinBox* m_threadCountSpinBox = nullptr;
    QCheckBox* m_deleteAfterCompressionCheckBox = nullptr;
    
    // Preview tab
    QWidget* m_previewWidget = nullptr;
    QVBoxLayout* m_previewLayout = nullptr;
    QLabel* m_estimatedSizeLabel = nullptr;
    QLabel* m_estimatedTimeLabel = nullptr;
    QLabel* m_compressionRatioLabel = nullptr;
    QProgressBar* m_estimatedProgressBar = nullptr;
    
    // Progress section
    QWidget* m_progressWidget = nullptr;
    QVBoxLayout* m_progressLayout = nullptr;
    QProgressBar* m_overallProgressBar = nullptr;
    QProgressBar* m_currentFileProgressBar = nullptr;
    QLabel* m_progressStatusLabel = nullptr;
    QLabel* m_currentFileLabel = nullptr;
    QLabel* m_speedLabel = nullptr;
    QLabel* m_timeRemainingLabel = nullptr;
    QPushButton* m_startButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
    
    // State
    QStringList m_inputFiles;
    QString m_outputDirectory;
    QString m_archiveName;
    CompressionFormat m_currentFormat = CompressionFormat::ZIP;
    CompressionLevel m_currentLevel = CompressionLevel::Normal;
    bool m_compressionInProgress = false;
    
    // Animation
    QPropertyAnimation* m_progressAnimation = nullptr;
    QTimer* m_updateTimer = nullptr;
    
    // Statistics
    qint64 m_totalInputSize = 0;
    qint64 m_processedSize = 0;
    QDateTime m_compressionStartTime;
};

} // namespace FluxGUI::UI::Widgets

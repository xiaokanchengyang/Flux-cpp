#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QProgressBar>
#include <QListWidget>
#include <QTreeWidget>
#include <QGroupBox>
#include <QTabWidget>
#include <QFileDialog>
#include <QTimer>
#include <QPropertyAnimation>

namespace FluxGUI::UI::Widgets {

/**
 * @brief Advanced extraction widget with selective extraction
 * 
 * The ExtractionWidget provides comprehensive extraction functionality:
 * - Archive file selection with format detection
 * - Selective file extraction with preview
 * - Multiple extraction modes (all, selected, filtered)
 * - Output directory customization
 * - Password handling for encrypted archives
 * - Progress monitoring with detailed statistics
 * - Batch extraction support
 */
class ExtractionWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Extraction modes available
     */
    enum class ExtractionMode {
        All,            // Extract all files
        Selected,       // Extract selected files only
        Filtered,       // Extract files matching filter
        ToSeparateFolders // Extract each archive to separate folder
    };

    /**
     * @brief Overwrite behavior options
     */
    enum class OverwriteMode {
        Ask,            // Ask for each file
        Skip,           // Skip existing files
        Overwrite,      // Overwrite existing files
        Rename          // Rename new files
    };

    /**
     * @brief Construct the extraction widget
     * @param parent Parent widget
     */
    explicit ExtractionWidget(QWidget* parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~ExtractionWidget() override = default;

public slots:
    /**
     * @brief Set archive file to extract
     * @param archivePath Path to archive file
     */
    void setArchiveFile(const QString& archivePath);
    
    /**
     * @brief Add archive files for batch extraction
     * @param archivePaths List of archive file paths
     */
    void addArchiveFiles(const QStringList& archivePaths);
    
    /**
     * @brief Clear all archive files
     */
    void clearArchiveFiles();
    
    /**
     * @brief Set output directory
     * @param outputPath Path to output directory
     */
    void setOutputDirectory(const QString& outputPath);
    
    /**
     * @brief Start extraction process
     */
    void startExtraction();
    
    /**
     * @brief Cancel extraction process
     */
    void cancelExtraction();

signals:
    /**
     * @brief Emitted when extraction starts
     * @param archivePath Path to archive being extracted
     */
    void extractionStarted(const QString& archivePath);
    
    /**
     * @brief Emitted when extraction progresses
     * @param percentage Progress percentage (0-100)
     * @param currentFile Currently extracting file
     */
    void extractionProgress(int percentage, const QString& currentFile);
    
    /**
     * @brief Emitted when extraction completes
     * @param outputPath Path where files were extracted
     * @param success True if extraction succeeded
     */
    void extractionCompleted(const QString& outputPath, bool success);
    
    /**
     * @brief Emitted when extraction is cancelled
     */
    void extractionCancelled();
    
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
     * @brief Handle browse archive button click
     */
    void browseArchive();
    
    /**
     * @brief Handle browse output button click
     */
    void browseOutput();
    
    /**
     * @brief Handle extraction mode change
     */
    void onExtractionModeChanged();
    
    /**
     * @brief Handle file selection change
     */
    void onFileSelectionChanged();
    
    /**
     * @brief Handle filter text change
     */
    void onFilterTextChanged();
    
    /**
     * @brief Handle password input
     */
    void onPasswordChanged();
    
    /**
     * @brief Update extraction preview
     */
    void updateExtractionPreview();
    
    /**
     * @brief Select all files
     */
    void selectAllFiles();
    
    /**
     * @brief Deselect all files
     */
    void deselectAllFiles();
    
    /**
     * @brief Invert file selection
     */
    void invertSelection();

private:
    /**
     * @brief Initialize the user interface
     */
    void initializeUI();
    
    /**
     * @brief Create archive selection section
     * @return Archive selection widget
     */
    QWidget* createArchiveSelectionSection();
    
    /**
     * @brief Create file selection section
     * @return File selection widget
     */
    QWidget* createFileSelectionSection();
    
    /**
     * @brief Create extraction settings section
     * @return Settings widget
     */
    QWidget* createExtractionSettingsSection();
    
    /**
     * @brief Create progress section
     * @return Progress widget
     */
    QWidget* createProgressSection();
    
    /**
     * @brief Setup drag and drop
     */
    void setupDragAndDrop();
    
    /**
     * @brief Connect signals and slots
     */
    void connectSignals();
    
    /**
     * @brief Update UI state
     */
    void updateUIState();
    
    /**
     * @brief Validate extraction settings
     * @return True if settings are valid
     */
    bool validateSettings();
    
    /**
     * @brief Load archive contents
     * @param archivePath Path to archive
     */
    void loadArchiveContents(const QString& archivePath);
    
    /**
     * @brief Apply file filter
     */
    void applyFileFilter();
    
    /**
     * @brief Calculate selected files size
     * @return Total size in bytes
     */
    qint64 calculateSelectedSize() const;
    
    /**
     * @brief Format file size for display
     * @param bytes Size in bytes
     * @return Formatted size string
     */
    QString formatFileSize(qint64 bytes) const;
    
    /**
     * @brief Get extraction mode display name
     * @param mode Extraction mode
     * @return Display name
     */
    QString getExtractionModeDisplayName(ExtractionMode mode) const;
    
    /**
     * @brief Get overwrite mode display name
     * @param mode Overwrite mode
     * @return Display name
     */
    QString getOverwriteModeDisplayName(OverwriteMode mode) const;

private:
    // Main layout
    QVBoxLayout* m_mainLayout = nullptr;
    QTabWidget* m_tabWidget = nullptr;
    
    // Archive selection tab
    QWidget* m_archiveSelectionWidget = nullptr;
    QVBoxLayout* m_archiveSelectionLayout = nullptr;
    QLineEdit* m_archivePathEdit = nullptr;
    QPushButton* m_browseArchiveButton = nullptr;
    QListWidget* m_archiveListWidget = nullptr;
    QPushButton* m_addArchiveButton = nullptr;
    QPushButton* m_removeArchiveButton = nullptr;
    QPushButton* m_clearArchivesButton = nullptr;
    
    // File selection tab
    QWidget* m_fileSelectionWidget = nullptr;
    QVBoxLayout* m_fileSelectionLayout = nullptr;
    QComboBox* m_extractionModeCombo = nullptr;
    QTreeWidget* m_fileTreeWidget = nullptr;
    QHBoxLayout* m_selectionButtonsLayout = nullptr;
    QPushButton* m_selectAllButton = nullptr;
    QPushButton* m_deselectAllButton = nullptr;
    QPushButton* m_invertSelectionButton = nullptr;
    QLineEdit* m_filterEdit = nullptr;
    QLabel* m_selectedCountLabel = nullptr;
    QLabel* m_selectedSizeLabel = nullptr;
    
    // Settings tab
    QWidget* m_settingsWidget = nullptr;
    QGridLayout* m_settingsLayout = nullptr;
    QLineEdit* m_outputPathEdit = nullptr;
    QPushButton* m_browseOutputButton = nullptr;
    QComboBox* m_overwriteModeCombo = nullptr;
    QCheckBox* m_preservePathsCheckBox = nullptr;
    QCheckBox* m_createSubfolderCheckBox = nullptr;
    QLineEdit* m_passwordEdit = nullptr;
    QCheckBox* m_showPasswordCheckBox = nullptr;
    QCheckBox* m_deleteAfterExtractionCheckBox = nullptr;
    QCheckBox* m_openOutputFolderCheckBox = nullptr;
    
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
    QStringList m_archiveFiles;
    QString m_currentArchive;
    QString m_outputDirectory;
    ExtractionMode m_extractionMode = ExtractionMode::All;
    OverwriteMode m_overwriteMode = OverwriteMode::Ask;
    bool m_extractionInProgress = false;
    
    // File data
    QStringList m_allFiles;
    QStringList m_selectedFiles;
    QStringList m_filteredFiles;
    
    // Animation
    QPropertyAnimation* m_progressAnimation = nullptr;
    QTimer* m_updateTimer = nullptr;
    
    // Statistics
    qint64 m_totalSize = 0;
    qint64 m_selectedSize = 0;
    qint64 m_extractedSize = 0;
    QDateTime m_extractionStartTime;
};

} // namespace FluxGUI::UI::Widgets

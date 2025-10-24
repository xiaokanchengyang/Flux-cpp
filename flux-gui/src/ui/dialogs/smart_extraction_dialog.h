#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QProgressBar>
#include <QTextEdit>
#include <QTabWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QSplitter>
#include <QFileInfo>
#include <QStringList>

namespace FluxGUI::UI::Dialogs {

/**
 * Smart extraction dialog with intelligent directory detection and options
 * Analyzes archive contents and suggests optimal extraction settings
 */
class SmartExtractionDialog : public QDialog {
    Q_OBJECT

public:
    struct ExtractionSettings {
        QString archivePath;
        QString outputPath;
        bool createSubfolder;
        QString subfolderName;
        bool overwriteExisting;
        bool preservePermissions;
        bool preserveTimestamps;
        bool extractSelectedOnly;
        QStringList selectedFiles;
        QStringList excludePatterns;
        bool openDestinationAfter;
        bool deleteArchiveAfter;
        QString password;
    };

    explicit SmartExtractionDialog(const QString& archivePath, QWidget* parent = nullptr);
    ~SmartExtractionDialog() override;

    ExtractionSettings getSettings() const;
    void setDefaultOutputPath(const QString& path);
    void setSelectedFiles(const QStringList& files);

signals:
    void extractionRequested(const ExtractionSettings& settings);

private slots:
    void onOutputPathBrowse();
    void onCreateSubfolderToggled(bool enabled);
    void onSubfolderNameChanged();
    void onAnalyzeArchive();
    void onSelectAll();
    void onSelectNone();
    void onTreeItemChanged(QTreeWidgetItem* item, int column);
    void onPreviewExtraction();
    void onAccept();
    void onReject();

private:
    void setupUI();
    void setupBasicTab();
    void setupFilesTab();
    void setupAdvancedTab();
    void connectSignals();
    
    void analyzeArchiveStructure();
    void updateRecommendations();
    void populateFileTree();
    void updateSelectionCount();
    void validateSettings();
    
    // Analysis functions
    struct ArchiveAnalysis {
        qint64 totalSize;
        qint64 totalFiles;
        qint64 totalFolders;
        bool hasRootFolder;
        QString suggestedRootFolder;
        bool hasMultipleRootItems;
        QStringList rootItems;
        QStringList fileTypes;
        bool hasExecutables;
        bool hasHiddenFiles;
        bool needsSubfolder;
        QString recommendedSubfolderName;
    };
    
    ArchiveAnalysis analyzeArchive(const QString& archivePath);
    bool shouldCreateSubfolder(const ArchiveAnalysis& analysis);
    QString suggestSubfolderName(const ArchiveAnalysis& analysis, const QString& archivePath);
    QString formatFileSize(qint64 size) const;
    QTreeWidgetItem* findOrCreateTreeItem(const QString& path, QTreeWidgetItem* parent = nullptr);

private:
    // Input data
    QString m_archivePath;
    ArchiveAnalysis m_analysis;
    
    // UI Components
    QTabWidget* m_tabWidget{nullptr};
    
    // Basic tab
    QWidget* m_basicTab{nullptr};
    QLineEdit* m_outputPathEdit{nullptr};
    QPushButton* m_browseBtn{nullptr};
    QCheckBox* m_createSubfolderCheck{nullptr};
    QLineEdit* m_subfolderNameEdit{nullptr};
    QLabel* m_previewPathLabel{nullptr};
    QCheckBox* m_overwriteCheck{nullptr};
    QCheckBox* m_openDestinationCheck{nullptr};
    
    // Files tab
    QWidget* m_filesTab{nullptr};
    QSplitter* m_filesSplitter{nullptr};
    QTreeWidget* m_fileTree{nullptr};
    QTextEdit* m_fileInfoEdit{nullptr};
    QPushButton* m_selectAllBtn{nullptr};
    QPushButton* m_selectNoneBtn{nullptr};
    QLabel* m_selectionCountLabel{nullptr};
    QCheckBox* m_extractSelectedOnlyCheck{nullptr};
    
    // Advanced tab
    QWidget* m_advancedTab{nullptr};
    QCheckBox* m_preservePermissionsCheck{nullptr};
    QCheckBox* m_preserveTimestampsCheck{nullptr};
    QCheckBox* m_deleteArchiveCheck{nullptr};
    QLineEdit* m_passwordEdit{nullptr};
    QTextEdit* m_excludePatternsEdit{nullptr};
    
    // Analysis and recommendations
    QGroupBox* m_analysisGroup{nullptr};
    QLabel* m_archiveSizeLabel{nullptr};
    QLabel* m_fileCountLabel{nullptr};
    QLabel* m_structureLabel{nullptr};
    QLabel* m_recommendationLabel{nullptr};
    QProgressBar* m_analysisProgress{nullptr};
    QPushButton* m_analyzeBtn{nullptr};
    QPushButton* m_previewBtn{nullptr};
    
    // Dialog buttons
    QPushButton* m_okBtn{nullptr};
    QPushButton* m_cancelBtn{nullptr};
    QPushButton* m_helpBtn{nullptr};
    
    // State
    bool m_analysisComplete{false};
    ExtractionSettings m_currentSettings;
    QStringList m_selectedFiles;
};

} // namespace FluxGUI::UI::Dialogs

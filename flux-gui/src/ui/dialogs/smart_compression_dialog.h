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
#include <QSlider>
#include <QSpinBox>
#include <QCheckBox>
#include <QProgressBar>
#include <QTextEdit>
#include <QTabWidget>
#include <QFileInfo>
#include <QStringList>

namespace FluxGUI::UI::Dialogs {

/**
 * Smart compression dialog with intelligent defaults and options
 * Analyzes input files and suggests optimal compression settings
 */
class SmartCompressionDialog : public QDialog {
    Q_OBJECT

public:
    struct CompressionSettings {
        QString format;
        QString outputPath;
        int compressionLevel;
        bool createSolidArchive;
        bool encryptArchive;
        QString password;
        bool deleteOriginalFiles;
        bool createSeparateArchives;
        QStringList excludePatterns;
        QString compressionMethod;
        int dictionarySize;
        int wordSize;
        bool enableMultiThreading;
    };

    explicit SmartCompressionDialog(const QStringList& inputPaths, QWidget* parent = nullptr);
    ~SmartCompressionDialog() override;

    CompressionSettings getSettings() const;
    void setDefaultOutputPath(const QString& path);

signals:
    void compressionRequested(const CompressionSettings& settings);

private slots:
    void onFormatChanged();
    void onCompressionLevelChanged(int level);
    void onOutputPathBrowse();
    void onAdvancedOptionsToggled(bool enabled);
    void onAnalyzeFiles();
    void onPreviewSize();
    void onAccept();
    void onReject();

private:
    void setupUI();
    void setupBasicTab();
    void setupAdvancedTab();
    void setupAnalysisTab();
    void connectSignals();
    
    void analyzeInputFiles();
    void updateRecommendations();
    void updateCompressionPreview();
    void updateFormatSpecificOptions();
    
    // Analysis functions
    struct FileAnalysis {
        qint64 totalSize;
        qint64 totalFiles;
        qint64 totalFolders;
        QStringList fileTypes;
        QStringList largeFiles;
        bool hasCompressedFiles;
        bool hasTextFiles;
        bool hasMediaFiles;
        double estimatedCompressionRatio;
    };
    
    FileAnalysis analyzeFiles(const QStringList& paths);
    QString suggestOptimalFormat(const FileAnalysis& analysis);
    int suggestCompressionLevel(const FileAnalysis& analysis, const QString& format);
    QString formatFileSize(qint64 size) const;
    QString getFormatDescription(const QString& format) const;

private:
    // Input data
    QStringList m_inputPaths;
    FileAnalysis m_analysis;
    
    // UI Components
    QTabWidget* m_tabWidget{nullptr};
    
    // Basic tab
    QWidget* m_basicTab{nullptr};
    QLineEdit* m_outputPathEdit{nullptr};
    QPushButton* m_browseBtn{nullptr};
    QComboBox* m_formatCombo{nullptr};
    QLabel* m_formatDescLabel{nullptr};
    QSlider* m_compressionSlider{nullptr};
    QSpinBox* m_compressionSpin{nullptr};
    QLabel* m_compressionDescLabel{nullptr};
    QCheckBox* m_deleteOriginalCheck{nullptr};
    
    // Advanced tab
    QWidget* m_advancedTab{nullptr};
    QComboBox* m_compressionMethodCombo{nullptr};
    QSpinBox* m_dictionarySizeSpin{nullptr};
    QSpinBox* m_wordSizeSpin{nullptr};
    QCheckBox* m_solidArchiveCheck{nullptr};
    QCheckBox* m_multiThreadCheck{nullptr};
    QCheckBox* m_encryptCheck{nullptr};
    QLineEdit* m_passwordEdit{nullptr};
    QLineEdit* m_confirmPasswordEdit{nullptr};
    QCheckBox* m_separateArchivesCheck{nullptr};
    QTextEdit* m_excludePatternsEdit{nullptr};
    
    // Analysis tab
    QWidget* m_analysisTab{nullptr};
    QLabel* m_totalSizeLabel{nullptr};
    QLabel* m_fileCountLabel{nullptr};
    QLabel* m_fileTypesLabel{nullptr};
    QLabel* m_recommendationLabel{nullptr};
    QLabel* m_estimatedSizeLabel{nullptr};
    QLabel* m_estimatedTimeLabel{nullptr};
    QProgressBar* m_analysisProgress{nullptr};
    QPushButton* m_analyzeBtn{nullptr};
    QPushButton* m_previewBtn{nullptr};
    
    // Dialog buttons
    QPushButton* m_okBtn{nullptr};
    QPushButton* m_cancelBtn{nullptr};
    QPushButton* m_helpBtn{nullptr};
    
    // State
    bool m_analysisComplete{false};
    CompressionSettings m_currentSettings;
};

} // namespace FluxGUI::UI::Dialogs





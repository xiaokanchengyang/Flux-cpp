#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QGroupBox>
#include <QThread>
#include <flux-core/packer.h>

/**
 * Worker thread for packing operations
 */
class PackWorkerThread : public QThread {
    Q_OBJECT

public:
    struct PackConfig {
        QStringList inputPaths;
        QString outputPath;
        QString format;
        int compressionLevel;
        int threadCount;
        QString password;
    };

    explicit PackWorkerThread(const PackConfig& config, QObject* parent = nullptr);

protected:
    void run() override;

signals:
    void progressUpdated(const QString& currentFile, int percentage, qint64 processed, qint64 total);
    void packingFinished(bool success, const QString& message);
    void packingStarted();

private:
    PackConfig m_config;
    std::atomic<bool> m_cancelled{false};

public slots:
    void cancel();
};

/**
 * Pack View
 * 
 * Provides functionality for creating archive files, including:
 * - File list management
 * - Format selection
 * - Compression settings
 * - Output path configuration
 * - Progress display
 */
class PackView : public QWidget {
    Q_OBJECT

public:
    explicit PackView(QWidget *parent = nullptr);
    
    /**
     * Add files to packing list
     * @param filePaths List of file paths
     */
    void addFiles(const QStringList& filePaths);

signals:
    void packingStarted();
    void packingFinished(bool success, const QString& message);

private slots:
    void onAddFiles();
    void onAddFolder();
    void onRemoveSelected();
    void onClearAll();
    void onBrowseOutput();
    void onStartPacking();
    void onFormatChanged();
    void onPackingStarted();
    void onProgressUpdated(const QString& currentFile, int percentage, qint64 processed, qint64 total);
    void onPackingFinished(bool success, const QString& message);

private:
    void setupUI();
    void setupFileList();
    void setupSettings();
    void setupActions();
    void updateUI();
    void validateSettings();

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    QVBoxLayout* m_mainLayout;
    
    // File list area
    QGroupBox* m_fileListGroup;
    QListWidget* m_fileList;
    QHBoxLayout* m_fileButtonsLayout;
    QPushButton* m_addFilesButton;
    QPushButton* m_addFolderButton;
    QPushButton* m_removeButton;
    QPushButton* m_clearButton;
    
    // Settings area
    QGroupBox* m_settingsGroup;
    QComboBox* m_formatCombo;
    QSpinBox* m_compressionSpin;
    QSpinBox* m_threadsSpin;
    QLineEdit* m_passwordEdit;
    QLineEdit* m_outputEdit;
    QPushButton* m_browseButton;
    
    // Action area
    QHBoxLayout* m_actionLayout;
    QPushButton* m_packButton;
    QPushButton* m_cancelButton;
    
    // Progress display
    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;
    
    // Worker thread
    PackWorkerThread* m_workerThread;
};












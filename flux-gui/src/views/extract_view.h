#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QProgressBar>
#include <QGroupBox>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QStandardPaths>

QT_BEGIN_NAMESPACE
class QDragEnterEvent;
class QDragMoveEvent;
class QDragLeaveEvent;
class QDropEvent;
QT_END_NAMESPACE

namespace FluxGUI::UI::Components {
    class UnifiedDropZone;
    class ContextMenuManager;
}

namespace FluxGUI::UI::Dialogs {
    class SmartExtractionDialog;
}

class ExtractView : public QWidget {
    Q_OBJECT

public:
    explicit ExtractView(QWidget *parent = nullptr);

signals:
    void extractionRequested(const QString& archivePath, const QString& outputPath);
    void archiveSelected(const QString& archivePath);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void onDropZoneFilesDropped(const QStringList& filePaths);
    void onDropZoneArchiveFilesDropped(const QStringList& archivePaths);
    void onSelectArchiveClicked();
    void onSelectOutputClicked();
    void onExtractClicked();
    void onExtractHereClicked();
    void onExtractWithOptionsClicked();
    void onOverwriteModeChanged();
    void onShowContextMenu(const QPoint& pos);
    
    // Context menu handlers
    void onContextExtractHere(const QString& archivePath);
    void onContextExtractToFolder(const QString& archivePath, const QString& outputPath);
    void onContextExtractWithOptions(const QString& archivePath);
    void onContextPreviewArchive(const QString& archivePath);
    void onContextShowProperties(const QString& archivePath);

private:
    void setupUI();
    void setupDropZone();
    void setupControlPanel();
    void setupProgressArea();
    void connectSignals();
    
    void setArchivePath(const QString& path);
    void setOutputPath(const QString& path);
    void updateExtractButton();
    void showExtractionProgress(bool show);
    bool isArchiveFile(const QString& filePath) const;
    QString getDefaultOutputPath(const QString& archivePath) const;
    void setupContextMenu();
    QString formatFileSize(qint64 size) const;

private:
    // UI Components
    QVBoxLayout* m_mainLayout{nullptr};
    FluxGUI::UI::Components::UnifiedDropZone* m_dropZone{nullptr};
    FluxGUI::UI::Components::ContextMenuManager* m_contextMenuManager{nullptr};
    
    // Control Panel
    QGroupBox* m_controlGroup{nullptr};
    QLineEdit* m_archivePathEdit{nullptr};
    QLineEdit* m_outputPathEdit{nullptr};
    QPushButton* m_selectArchiveBtn{nullptr};
    QPushButton* m_selectOutputBtn{nullptr};
    QPushButton* m_extractBtn{nullptr};
    QPushButton* m_extractHereBtn{nullptr};
    QPushButton* m_extractWithOptionsBtn{nullptr};
    
    // Options
    QCheckBox* m_overwriteCheck{nullptr};
    QCheckBox* m_createFolderCheck{nullptr};
    QComboBox* m_compressionCombo{nullptr};
    
    // Progress
    QGroupBox* m_progressGroup{nullptr};
    QProgressBar* m_progressBar{nullptr};
    QLabel* m_progressLabel{nullptr};
    QPushButton* m_cancelBtn{nullptr};
    
    // State
    QString m_currentArchivePath;
    QString m_currentOutputPath;
    bool m_isExtracting{false};
};


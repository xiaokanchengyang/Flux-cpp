#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QGroupBox>
#include <QHeaderView>
#include <QFileInfo>
#include <QDateTime>
#include <QMenu>
#include <QAction>
#include <QProgressBar>

QT_BEGIN_NAMESPACE
class QDragEnterEvent;
class QDragMoveEvent;
class QDragLeaveEvent;
class QDropEvent;
QT_END_NAMESPACE

namespace FluxGUI::UI::Components {
    class UnifiedDropZone;
}

class BrowseView : public QWidget {
    Q_OBJECT

public:
    explicit BrowseView(QWidget *parent = nullptr);

signals:
    void archiveOpened(const QString& archivePath);
    void fileExtractionRequested(const QString& archivePath, const QStringList& filePaths, const QString& outputPath);
    void archiveClosed();

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void onDropZoneFilesDropped(const QStringList& filePaths);
    void onDropZoneArchiveFilesDropped(const QStringList& archivePaths);
    void onOpenArchiveClicked();
    void onCloseArchiveClicked();
    void onExtractSelectedClicked();
    void onExtractAllClicked();
    void onRefreshClicked();
    void onTreeItemClicked(QTreeWidgetItem* item, int column);
    void onTreeItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onTableItemSelectionChanged();
    void onShowContextMenu(const QPoint& pos);

private:
    void setupUI();
    void setupDropZone();
    void setupToolbar();
    void setupContentArea();
    void setupStatusArea();
    void connectSignals();
    
    void openArchive(const QString& archivePath);
    void closeArchive();
    void loadArchiveContents();
    void populateFileTree();
    void populateFileTable(const QString& folderPath = "");
    void updateStatusInfo();
    void updateToolbarState();
    
    bool isArchiveFile(const QString& filePath) const;
    QString formatFileSize(qint64 size) const;
    QString getFileIcon(const QString& fileName) const;
    QTreeWidgetItem* findTreeItem(const QString& path);

private:
    // UI Components
    QVBoxLayout* m_mainLayout{nullptr};
    FluxGUI::UI::Components::UnifiedDropZone* m_dropZone{nullptr};
    
    // Toolbar
    QGroupBox* m_toolbarGroup{nullptr};
    QLineEdit* m_archivePathEdit{nullptr};
    QPushButton* m_openArchiveBtn{nullptr};
    QPushButton* m_closeArchiveBtn{nullptr};
    QPushButton* m_extractSelectedBtn{nullptr};
    QPushButton* m_extractAllBtn{nullptr};
    QPushButton* m_refreshBtn{nullptr};
    
    // Content area
    QSplitter* m_contentSplitter{nullptr};
    QTreeWidget* m_fileTree{nullptr};
    QTableWidget* m_fileTable{nullptr};
    
    // Status area
    QGroupBox* m_statusGroup{nullptr};
    QLabel* m_fileCountLabel{nullptr};
    QLabel* m_totalSizeLabel{nullptr};
    QLabel* m_compressionRatioLabel{nullptr};
    QProgressBar* m_loadingProgress{nullptr};
    
    // Context menu
    QMenu* m_contextMenu{nullptr};
    QAction* m_extractAction{nullptr};
    QAction* m_extractToAction{nullptr};
    QAction* m_viewPropertiesAction{nullptr};
    
    // State
    QString m_currentArchivePath;
    bool m_isArchiveOpen{false};
    bool m_isLoading{false};
    
    // Archive data (simplified structure for demo)
    struct ArchiveEntry {
        QString path;
        QString name;
        qint64 size;
        qint64 compressedSize;
        QDateTime modified;
        bool isDirectory;
    };
    QList<ArchiveEntry> m_archiveEntries;
};


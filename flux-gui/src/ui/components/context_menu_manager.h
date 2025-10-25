#pragma once

#include <QObject>
#include <QMenu>
#include <QAction>
#include <QFileInfo>
#include <QStringList>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QPoint;
QT_END_NAMESPACE

namespace FluxGUI::UI::Components {

/**
 * Context menu manager for intelligent file operations
 * Provides smart compression/extraction options based on file types and context
 */
class ContextMenuManager : public QObject {
    Q_OBJECT

public:
    explicit ContextMenuManager(QWidget* parent = nullptr);
    ~ContextMenuManager() override;

    // Main entry points
    void showFileContextMenu(const QString& filePath, const QPoint& globalPos);
    void showMultiFileContextMenu(const QStringList& filePaths, const QPoint& globalPos);
    void showFolderContextMenu(const QString& folderPath, const QPoint& globalPos);
    void showArchiveContextMenu(const QString& archivePath, const QPoint& globalPos);

    // Configuration
    void setEnabled(bool enabled);
    void setParentWidget(QWidget* parent);

signals:
    // Compression signals
    void compressToZip(const QStringList& paths, const QString& outputPath);
    void compressTo7z(const QStringList& paths, const QString& outputPath);
    void compressToTar(const QStringList& paths, const QString& outputPath);
    void compressWithOptions(const QStringList& paths, const QString& format);

    // Extraction signals
    void extractHere(const QString& archivePath);
    void extractToFolder(const QString& archivePath, const QString& outputPath);
    void extractWithOptions(const QString& archivePath);
    void extractSelected(const QString& archivePath, const QStringList& selectedFiles);

    // Preview signals
    void previewArchive(const QString& archivePath);
    void showArchiveProperties(const QString& archivePath);

    // File operations
    void openWith(const QString& filePath, const QString& application);
    void showInExplorer(const QString& path);

private slots:
    void onCompressToZip();
    void onCompressTo7z();
    void onCompressToTar();
    void onCompressWithDialog();
    void onExtractHere();
    void onExtractToFolder();
    void onExtractWithDialog();
    void onPreviewArchive();
    void onShowProperties();
    void onOpenWith();
    void onShowInExplorer();

private:
    void createActions();
    void createMenus();
    void updateMenuForContext(const QStringList& paths, bool isArchive, bool isFolder);
    
    // Smart analysis functions
    bool isArchiveFile(const QString& filePath) const;
    bool shouldCreateFolder(const QString& archivePath) const;
    QString suggestCompressionFormat(const QStringList& paths) const;
    QString suggestOutputName(const QStringList& paths, const QString& format) const;
    QString getDefaultExtractionPath(const QString& archivePath) const;
    
    // Menu building helpers
    void addCompressionOptions(QMenu* menu, const QStringList& paths);
    void addExtractionOptions(QMenu* menu, const QString& archivePath);
    void addArchivePreviewOptions(QMenu* menu, const QString& archivePath);
    void addGeneralFileOptions(QMenu* menu, const QStringList& paths);

private:
    QWidget* m_parentWidget{nullptr};
    QMenu* m_contextMenu{nullptr};
    
    // Actions for compression
    QAction* m_compressToZipAction{nullptr};
    QAction* m_compressTo7zAction{nullptr};
    QAction* m_compressToTarAction{nullptr};
    QAction* m_compressWithDialogAction{nullptr};
    
    // Actions for extraction
    QAction* m_extractHereAction{nullptr};
    QAction* m_extractToFolderAction{nullptr};
    QAction* m_extractWithDialogAction{nullptr};
    
    // Actions for archive preview
    QAction* m_previewArchiveAction{nullptr};
    QAction* m_showPropertiesAction{nullptr};
    
    // General actions
    QAction* m_openWithAction{nullptr};
    QAction* m_showInExplorerAction{nullptr};
    
    // Current context
    QStringList m_currentPaths;
    bool m_isArchiveContext{false};
    bool m_isFolderContext{false};
    bool m_enabled{true};
};

} // namespace FluxGUI::UI::Components





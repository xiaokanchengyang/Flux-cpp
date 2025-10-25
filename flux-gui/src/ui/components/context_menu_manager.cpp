#include "context_menu_manager.h"
#include <QApplication>
#include <QStyle>
#include <QFileInfo>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMimeDatabase>
#include <QMimeType>

namespace FluxGUI::UI::Components {

ContextMenuManager::ContextMenuManager(QWidget* parent)
    : QObject(parent)
    , m_parentWidget(parent)
{
    createActions();
    createMenus();
}

ContextMenuManager::~ContextMenuManager() = default;

void ContextMenuManager::createActions() {
    // Compression actions
    m_compressToZipAction = new QAction("Compress to ZIP", this);
    m_compressToZipAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    connect(m_compressToZipAction, &QAction::triggered, this, &ContextMenuManager::onCompressToZip);

    m_compressTo7zAction = new QAction("Compress to 7Z", this);
    m_compressTo7zAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    connect(m_compressTo7zAction, &QAction::triggered, this, &ContextMenuManager::onCompressTo7z);

    m_compressToTarAction = new QAction("Compress to TAR", this);
    m_compressToTarAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    connect(m_compressToTarAction, &QAction::triggered, this, &ContextMenuManager::onCompressToTar);

    m_compressWithDialogAction = new QAction("Compress with Options...", this);
    m_compressWithDialogAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
    connect(m_compressWithDialogAction, &QAction::triggered, this, &ContextMenuManager::onCompressWithDialog);

    // Extraction actions
    m_extractHereAction = new QAction("Extract Here", this);
    m_extractHereAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon));
    connect(m_extractHereAction, &QAction::triggered, this, &ContextMenuManager::onExtractHere);

    m_extractToFolderAction = new QAction("Extract to Folder", this);
    m_extractToFolderAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_DirIcon));
    connect(m_extractToFolderAction, &QAction::triggered, this, &ContextMenuManager::onExtractToFolder);

    m_extractWithDialogAction = new QAction("Extract with Options...", this);
    m_extractWithDialogAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
    connect(m_extractWithDialogAction, &QAction::triggered, this, &ContextMenuManager::onExtractWithDialog);

    // Preview actions
    m_previewArchiveAction = new QAction("Preview Contents", this);
    m_previewArchiveAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_FileDialogListView));
    connect(m_previewArchiveAction, &QAction::triggered, this, &ContextMenuManager::onPreviewArchive);

    m_showPropertiesAction = new QAction("Properties", this);
    m_showPropertiesAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_FileDialogInfoView));
    connect(m_showPropertiesAction, &QAction::triggered, this, &ContextMenuManager::onShowProperties);

    // General actions
    m_openWithAction = new QAction("Open with Flux", this);
    m_openWithAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
    connect(m_openWithAction, &QAction::triggered, this, &ContextMenuManager::onOpenWith);

    m_showInExplorerAction = new QAction("Show in Explorer", this);
    m_showInExplorerAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_DirLinkIcon));
    connect(m_showInExplorerAction, &QAction::triggered, this, &ContextMenuManager::onShowInExplorer);
}

void ContextMenuManager::createMenus() {
    m_contextMenu = new QMenu(m_parentWidget);
}

void ContextMenuManager::showFileContextMenu(const QString& filePath, const QPoint& globalPos) {
    if (!m_enabled) return;

    m_currentPaths = QStringList{filePath};
    m_isArchiveContext = isArchiveFile(filePath);
    m_isFolderContext = false;

    updateMenuForContext(m_currentPaths, m_isArchiveContext, m_isFolderContext);
    m_contextMenu->exec(globalPos);
}

void ContextMenuManager::showMultiFileContextMenu(const QStringList& filePaths, const QPoint& globalPos) {
    if (!m_enabled || filePaths.isEmpty()) return;

    m_currentPaths = filePaths;
    m_isArchiveContext = false;
    m_isFolderContext = false;

    // Check if all files are archives
    bool allArchives = true;
    for (const QString& path : filePaths) {
        if (!isArchiveFile(path)) {
            allArchives = false;
            break;
        }
    }
    m_isArchiveContext = allArchives && filePaths.size() == 1;

    updateMenuForContext(m_currentPaths, m_isArchiveContext, m_isFolderContext);
    m_contextMenu->exec(globalPos);
}

void ContextMenuManager::showFolderContextMenu(const QString& folderPath, const QPoint& globalPos) {
    if (!m_enabled) return;

    m_currentPaths = QStringList{folderPath};
    m_isArchiveContext = false;
    m_isFolderContext = true;

    updateMenuForContext(m_currentPaths, m_isArchiveContext, m_isFolderContext);
    m_contextMenu->exec(globalPos);
}

void ContextMenuManager::showArchiveContextMenu(const QString& archivePath, const QPoint& globalPos) {
    if (!m_enabled) return;

    m_currentPaths = QStringList{archivePath};
    m_isArchiveContext = true;
    m_isFolderContext = false;

    updateMenuForContext(m_currentPaths, m_isArchiveContext, m_isFolderContext);
    m_contextMenu->exec(globalPos);
}

void ContextMenuManager::updateMenuForContext(const QStringList& paths, bool isArchive, bool isFolder) {
    m_contextMenu->clear();

    if (isArchive && paths.size() == 1) {
        // Archive-specific menu
        addExtractionOptions(m_contextMenu, paths.first());
        m_contextMenu->addSeparator();
        addArchivePreviewOptions(m_contextMenu, paths.first());
        m_contextMenu->addSeparator();
        addGeneralFileOptions(m_contextMenu, paths);
    } else if (!isArchive) {
        // Regular files/folders - show compression options
        addCompressionOptions(m_contextMenu, paths);
        m_contextMenu->addSeparator();
        addGeneralFileOptions(m_contextMenu, paths);
    }
}

void ContextMenuManager::addCompressionOptions(QMenu* menu, const QStringList& paths) {
    QString suggestedFormat = suggestCompressionFormat(paths);
    
    // Quick compression options
    QMenu* compressMenu = menu->addMenu("Compress to");
    compressMenu->setIcon(QApplication::style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    
    compressMenu->addAction(m_compressToZipAction);
    compressMenu->addAction(m_compressTo7zAction);
    compressMenu->addAction(m_compressToTarAction);
    compressMenu->addSeparator();
    compressMenu->addAction(m_compressWithDialogAction);

    // Update action text with suggested names
    QString baseName = suggestOutputName(paths, "zip");
    m_compressToZipAction->setText(QString("ZIP (%1.zip)").arg(QFileInfo(baseName).baseName()));
    
    baseName = suggestOutputName(paths, "7z");
    m_compressTo7zAction->setText(QString("7Z (%1.7z)").arg(QFileInfo(baseName).baseName()));
    
    baseName = suggestOutputName(paths, "tar");
    m_compressToTarAction->setText(QString("TAR (%1.tar.gz)").arg(QFileInfo(baseName).baseName()));
}

void ContextMenuManager::addExtractionOptions(QMenu* menu, const QString& archivePath) {
    QFileInfo archiveInfo(archivePath);
    QString archiveName = archiveInfo.completeBaseName();
    
    // Smart extraction options
    bool shouldCreateDir = shouldCreateFolder(archivePath);
    
    if (shouldCreateDir) {
        m_extractHereAction->setText(QString("Extract to '%1' folder").arg(archiveName));
    } else {
        m_extractHereAction->setText("Extract Here");
    }
    
    menu->addAction(m_extractHereAction);
    menu->addAction(m_extractToFolderAction);
    menu->addSeparator();
    menu->addAction(m_extractWithDialogAction);
}

void ContextMenuManager::addArchivePreviewOptions(QMenu* menu, const QString& archivePath) {
    Q_UNUSED(archivePath)
    
    menu->addAction(m_previewArchiveAction);
    menu->addAction(m_showPropertiesAction);
}

void ContextMenuManager::addGeneralFileOptions(QMenu* menu, const QStringList& paths) {
    Q_UNUSED(paths)
    
    menu->addAction(m_openWithAction);
    menu->addAction(m_showInExplorerAction);
}

bool ContextMenuManager::isArchiveFile(const QString& filePath) const {
    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();
    QString completeSuffix = fileInfo.completeSuffix().toLower();
    
    QStringList archiveExtensions = {
        "zip", "7z", "rar", "tar", "gz", "bz2", "xz",
        "tar.gz", "tar.bz2", "tar.xz", "tgz", "tbz2", "txz"
    };
    
    return archiveExtensions.contains(suffix) || 
           archiveExtensions.contains(completeSuffix);
}

bool ContextMenuManager::shouldCreateFolder(const QString& archivePath) const {
    // Smart logic to determine if we should create a folder for extraction
    QFileInfo archiveInfo(archivePath);
    QString suffix = archiveInfo.suffix().toLower();
    
    // For certain archive types, always create folder
    if (suffix == "zip" || suffix == "7z") {
        return true;
    }
    
    // For tar archives, check if it's a single file or multiple files
    // In real implementation, we would peek into the archive
    // For now, assume we should create folder for most cases
    return true;
}

QString ContextMenuManager::suggestCompressionFormat(const QStringList& paths) const {
    // Analyze files to suggest best compression format
    qint64 totalSize = 0;
    bool hasLargeFiles = false;
    bool hasTextFiles = false;
    
    QMimeDatabase mimeDb;
    
    for (const QString& path : paths) {
        QFileInfo info(path);
        if (info.isFile()) {
            totalSize += info.size();
            if (info.size() > 100 * 1024 * 1024) { // 100MB
                hasLargeFiles = true;
            }
            
            QMimeType mimeType = mimeDb.mimeTypeForFile(path);
            if (mimeType.name().startsWith("text/")) {
                hasTextFiles = true;
            }
        }
    }
    
    // Suggest format based on analysis
    if (hasLargeFiles) {
        return "7z"; // Better compression for large files
    } else if (hasTextFiles) {
        return "tar.gz"; // Good for text files
    } else {
        return "zip"; // Universal compatibility
    }
}

QString ContextMenuManager::suggestOutputName(const QStringList& paths, const QString& format) const {
    if (paths.isEmpty()) return "archive";
    
    if (paths.size() == 1) {
        QFileInfo info(paths.first());
        if (info.isDir()) {
            return info.baseName();
        } else {
            return info.completeBaseName();
        }
    } else {
        // Multiple files - use parent directory name or generic name
        QFileInfo firstFile(paths.first());
        QString parentDir = firstFile.absolutePath();
        QFileInfo parentInfo(parentDir);
        return parentInfo.baseName().isEmpty() ? "archive" : parentInfo.baseName();
    }
}

QString ContextMenuManager::getDefaultExtractionPath(const QString& archivePath) const {
    QFileInfo archiveInfo(archivePath);
    QString baseName = archiveInfo.completeBaseName();
    QString parentDir = archiveInfo.absolutePath();
    
    if (shouldCreateFolder(archivePath)) {
        return QDir(parentDir).absoluteFilePath(baseName);
    } else {
        return parentDir;
    }
}

void ContextMenuManager::setEnabled(bool enabled) {
    m_enabled = enabled;
}

void ContextMenuManager::setParentWidget(QWidget* parent) {
    m_parentWidget = parent;
    if (m_contextMenu) {
        m_contextMenu->setParent(parent);
    }
}

// Slot implementations
void ContextMenuManager::onCompressToZip() {
    if (m_currentPaths.isEmpty()) return;
    
    QString outputName = suggestOutputName(m_currentPaths, "zip");
    QString outputPath = QFileInfo(m_currentPaths.first()).absolutePath() + "/" + outputName + ".zip";
    
    emit compressToZip(m_currentPaths, outputPath);
}

void ContextMenuManager::onCompressTo7z() {
    if (m_currentPaths.isEmpty()) return;
    
    QString outputName = suggestOutputName(m_currentPaths, "7z");
    QString outputPath = QFileInfo(m_currentPaths.first()).absolutePath() + "/" + outputName + ".7z";
    
    emit compressTo7z(m_currentPaths, outputPath);
}

void ContextMenuManager::onCompressToTar() {
    if (m_currentPaths.isEmpty()) return;
    
    QString outputName = suggestOutputName(m_currentPaths, "tar");
    QString outputPath = QFileInfo(m_currentPaths.first()).absolutePath() + "/" + outputName + ".tar.gz";
    
    emit compressToTar(m_currentPaths, outputPath);
}

void ContextMenuManager::onCompressWithDialog() {
    if (m_currentPaths.isEmpty()) return;
    
    QString suggestedFormat = suggestCompressionFormat(m_currentPaths);
    emit compressWithOptions(m_currentPaths, suggestedFormat);
}

void ContextMenuManager::onExtractHere() {
    if (m_currentPaths.isEmpty() || !m_isArchiveContext) return;
    
    emit extractHere(m_currentPaths.first());
}

void ContextMenuManager::onExtractToFolder() {
    if (m_currentPaths.isEmpty() || !m_isArchiveContext) return;
    
    QString defaultPath = getDefaultExtractionPath(m_currentPaths.first());
    emit extractToFolder(m_currentPaths.first(), defaultPath);
}

void ContextMenuManager::onExtractWithDialog() {
    if (m_currentPaths.isEmpty() || !m_isArchiveContext) return;
    
    emit extractWithOptions(m_currentPaths.first());
}

void ContextMenuManager::onPreviewArchive() {
    if (m_currentPaths.isEmpty() || !m_isArchiveContext) return;
    
    emit previewArchive(m_currentPaths.first());
}

void ContextMenuManager::onShowProperties() {
    if (m_currentPaths.isEmpty()) return;
    
    if (m_isArchiveContext) {
        emit showArchiveProperties(m_currentPaths.first());
    }
}

void ContextMenuManager::onOpenWith() {
    if (m_currentPaths.isEmpty()) return;
    
    emit openWith(m_currentPaths.first(), "flux");
}

void ContextMenuManager::onShowInExplorer() {
    if (m_currentPaths.isEmpty()) return;
    
    QString path = m_currentPaths.first();
    QFileInfo info(path);
    
    if (info.isDir()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    } else {
        QDesktopServices::openUrl(QUrl::fromLocalFile(info.absolutePath()));
    }
}

} // namespace FluxGUI::UI::Components





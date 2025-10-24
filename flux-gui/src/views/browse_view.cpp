#include "browse_view.h"
#include "../ui/components/unified_drop_zone.h"
#include <QMimeData>
#include <QUrl>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>
#include <QApplication>
#include <QStyle>
#include <QGridLayout>
#include <QSpacerItem>

using namespace FluxGUI::UI::Components;

BrowseView::BrowseView(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    connectSignals();
    setAcceptDrops(true);
}

void BrowseView::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(16);
    m_mainLayout->setContentsMargins(20, 20, 20, 20);
    
    setupDropZone();
    setupToolbar();
    setupContentArea();
    setupStatusArea();
    
    // Initially show drop zone, hide content
    m_dropZone->setVisible(true);
    m_contentSplitter->setVisible(false);
    updateToolbarState();
}

void BrowseView::setupDropZone() {
    m_dropZone = new UnifiedDropZone(this);
    m_dropZone->setDropMessage("Drop archive files here to browse contents");
    m_dropZone->setAcceptedFileTypes({
        "*.zip", "*.7z", "*.rar", "*.tar", "*.gz", "*.bz2", 
        "*.xz", "*.tar.gz", "*.tar.bz2", "*.tar.xz"
    });
    m_dropZone->setMaxFileCount(1); // Only one archive at a time for browsing
    m_dropZone->setMinimumHeight(200);
    
    m_mainLayout->addWidget(m_dropZone);
}

void BrowseView::setupToolbar() {
    m_toolbarGroup = new QGroupBox("Archive Browser", this);
    auto* toolbarLayout = new QGridLayout(m_toolbarGroup);
    toolbarLayout->setSpacing(12);
    
    // Archive path display
    auto* pathLabel = new QLabel("Archive:", this);
    m_archivePathEdit = new QLineEdit(this);
    m_archivePathEdit->setPlaceholderText("No archive opened...");
    m_archivePathEdit->setReadOnly(true);
    
    m_openArchiveBtn = new QPushButton("Open Archive", this);
    m_openArchiveBtn->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    
    m_closeArchiveBtn = new QPushButton("Close", this);
    m_closeArchiveBtn->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    m_closeArchiveBtn->setEnabled(false);
    
    toolbarLayout->addWidget(pathLabel, 0, 0);
    toolbarLayout->addWidget(m_archivePathEdit, 0, 1, 1, 2);
    toolbarLayout->addWidget(m_openArchiveBtn, 0, 3);
    toolbarLayout->addWidget(m_closeArchiveBtn, 0, 4);
    
    // Action buttons
    m_extractSelectedBtn = new QPushButton("Extract Selected", this);
    m_extractSelectedBtn->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    m_extractSelectedBtn->setEnabled(false);
    
    m_extractAllBtn = new QPushButton("Extract All", this);
    m_extractAllBtn->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    m_extractAllBtn->setEnabled(false);
    
    m_refreshBtn = new QPushButton("Refresh", this);
    m_refreshBtn->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    m_refreshBtn->setEnabled(false);
    
    auto* actionLayout = new QHBoxLayout();
    actionLayout->addWidget(m_extractSelectedBtn);
    actionLayout->addWidget(m_extractAllBtn);
    actionLayout->addWidget(m_refreshBtn);
    actionLayout->addStretch();
    
    toolbarLayout->addLayout(actionLayout, 1, 0, 1, 5);
    
    m_mainLayout->addWidget(m_toolbarGroup);
}

void BrowseView::setupContentArea() {
    m_contentSplitter = new QSplitter(Qt::Horizontal, this);
    
    // File tree (folder structure)
    m_fileTree = new QTreeWidget(this);
    m_fileTree->setHeaderLabel("Folder Structure");
    m_fileTree->setMinimumWidth(250);
    m_fileTree->setMaximumWidth(400);
    
    // File table (file details)
    m_fileTable = new QTableWidget(this);
    m_fileTable->setColumnCount(4);
    QStringList headers = {"Name", "Size", "Compressed", "Modified"};
    m_fileTable->setHorizontalHeaderLabels(headers);
    m_fileTable->horizontalHeader()->setStretchLastSection(true);
    m_fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_fileTable->setAlternatingRowColors(true);
    m_fileTable->setSortingEnabled(true);
    
    // Set context menu policy
    m_fileTable->setContextMenuPolicy(Qt::CustomContextMenu);
    
    m_contentSplitter->addWidget(m_fileTree);
    m_contentSplitter->addWidget(m_fileTable);
    m_contentSplitter->setStretchFactor(0, 0);
    m_contentSplitter->setStretchFactor(1, 1);
    
    m_mainLayout->addWidget(m_contentSplitter);
}

void BrowseView::setupStatusArea() {
    m_statusGroup = new QGroupBox("Archive Information", this);
    auto* statusLayout = new QHBoxLayout(m_statusGroup);
    
    m_fileCountLabel = new QLabel("Files: 0", this);
    m_totalSizeLabel = new QLabel("Total Size: 0 bytes", this);
    m_compressionRatioLabel = new QLabel("Compression: 0%", this);
    
    m_loadingProgress = new QProgressBar(this);
    m_loadingProgress->setVisible(false);
    m_loadingProgress->setRange(0, 0); // Indeterminate progress
    
    statusLayout->addWidget(m_fileCountLabel);
    statusLayout->addWidget(m_totalSizeLabel);
    statusLayout->addWidget(m_compressionRatioLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(m_loadingProgress);
    
    m_mainLayout->addWidget(m_statusGroup);
    
    // Create context menu
    m_contextMenu = new QMenu(this);
    m_extractAction = m_contextMenu->addAction("Extract to...");
    m_extractToAction = m_contextMenu->addAction("Extract here");
    m_contextMenu->addSeparator();
    m_viewPropertiesAction = m_contextMenu->addAction("Properties");
}

void BrowseView::connectSignals() {
    // Drop zone signals
    connect(m_dropZone, &UnifiedDropZone::filesDropped,
            this, &BrowseView::onDropZoneFilesDropped);
    connect(m_dropZone, &UnifiedDropZone::archiveFilesDropped,
            this, &BrowseView::onDropZoneArchiveFilesDropped);
    
    // Toolbar signals
    connect(m_openArchiveBtn, &QPushButton::clicked,
            this, &BrowseView::onOpenArchiveClicked);
    connect(m_closeArchiveBtn, &QPushButton::clicked,
            this, &BrowseView::onCloseArchiveClicked);
    connect(m_extractSelectedBtn, &QPushButton::clicked,
            this, &BrowseView::onExtractSelectedClicked);
    connect(m_extractAllBtn, &QPushButton::clicked,
            this, &BrowseView::onExtractAllClicked);
    connect(m_refreshBtn, &QPushButton::clicked,
            this, &BrowseView::onRefreshClicked);
    
    // Tree and table signals
    connect(m_fileTree, &QTreeWidget::itemClicked,
            this, &BrowseView::onTreeItemClicked);
    connect(m_fileTree, &QTreeWidget::itemDoubleClicked,
            this, &BrowseView::onTreeItemDoubleClicked);
    connect(m_fileTable, &QTableWidget::itemSelectionChanged,
            this, &BrowseView::onTableItemSelectionChanged);
    connect(m_fileTable, &QTableWidget::customContextMenuRequested,
            this, &BrowseView::onShowContextMenu);
    
    // Context menu signals
    connect(m_extractAction, &QAction::triggered, [this]() {
        onExtractSelectedClicked();
    });
    connect(m_extractToAction, &QAction::triggered, [this]() {
        // Extract to current directory logic
        onExtractSelectedClicked();
    });
}

void BrowseView::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        bool hasArchive = false;
        for (const QUrl& url : event->mimeData()->urls()) {
            if (url.isLocalFile() && isArchiveFile(url.toLocalFile())) {
                hasArchive = true;
                break;
            }
        }
        
        if (hasArchive) {
            event->acceptProposedAction();
            return;
        }
    }
    event->ignore();
}

void BrowseView::dragMoveEvent(QDragMoveEvent* event) {
    event->acceptProposedAction();
}

void BrowseView::dragLeaveEvent(QDragLeaveEvent* event) {
    event->accept();
}

void BrowseView::dropEvent(QDropEvent* event) {
    QStringList archiveFiles;
    
    for (const QUrl& url : event->mimeData()->urls()) {
        if (url.isLocalFile()) {
            QString filePath = url.toLocalFile();
            if (isArchiveFile(filePath)) {
                archiveFiles.append(filePath);
            }
        }
    }
    
    if (!archiveFiles.isEmpty()) {
        openArchive(archiveFiles.first()); // Take the first archive
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void BrowseView::onDropZoneFilesDropped(const QStringList& filePaths) {
    for (const QString& filePath : filePaths) {
        if (isArchiveFile(filePath)) {
            openArchive(filePath);
            break; // Take the first archive file
        }
    }
}

void BrowseView::onDropZoneArchiveFilesDropped(const QStringList& archivePaths) {
    if (!archivePaths.isEmpty()) {
        openArchive(archivePaths.first());
    }
}

void BrowseView::onOpenArchiveClicked() {
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Open Archive File",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "Archive Files (*.zip *.7z *.rar *.tar *.gz *.bz2 *.xz *.tar.gz *.tar.bz2 *.tar.xz);;All Files (*)"
    );
    
    if (!fileName.isEmpty()) {
        openArchive(fileName);
    }
}

void BrowseView::onCloseArchiveClicked() {
    closeArchive();
}

void BrowseView::onExtractSelectedClicked() {
    if (!m_isArchiveOpen) return;
    
    QList<QTableWidgetItem*> selectedItems = m_fileTable->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::information(this, "No Selection", 
                               "Please select files to extract.");
        return;
    }
    
    QString outputDir = QFileDialog::getExistingDirectory(
        this,
        "Select Extraction Directory",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
    );
    
    if (!outputDir.isEmpty()) {
        QStringList filePaths;
        // Collect selected file paths (simplified)
        for (int row = 0; row < m_fileTable->rowCount(); ++row) {
            if (m_fileTable->item(row, 0)->isSelected()) {
                filePaths.append(m_fileTable->item(row, 0)->text());
            }
        }
        
        emit fileExtractionRequested(m_currentArchivePath, filePaths, outputDir);
    }
}

void BrowseView::onExtractAllClicked() {
    if (!m_isArchiveOpen) return;
    
    QString outputDir = QFileDialog::getExistingDirectory(
        this,
        "Select Extraction Directory",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
    );
    
    if (!outputDir.isEmpty()) {
        QStringList allFiles;
        // Collect all file paths
        for (const auto& entry : m_archiveEntries) {
            if (!entry.isDirectory) {
                allFiles.append(entry.path);
            }
        }
        
        emit fileExtractionRequested(m_currentArchivePath, allFiles, outputDir);
    }
}

void BrowseView::onRefreshClicked() {
    if (m_isArchiveOpen) {
        loadArchiveContents();
    }
}

void BrowseView::onTreeItemClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column)
    
    if (item) {
        QString folderPath = item->data(0, Qt::UserRole).toString();
        populateFileTable(folderPath);
    }
}

void BrowseView::onTreeItemDoubleClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column)
    
    if (item) {
        // Expand/collapse the item
        item->setExpanded(!item->isExpanded());
    }
}

void BrowseView::onTableItemSelectionChanged() {
    bool hasSelection = !m_fileTable->selectedItems().isEmpty();
    m_extractSelectedBtn->setEnabled(hasSelection && m_isArchiveOpen);
}

void BrowseView::onShowContextMenu(const QPoint& pos) {
    if (m_fileTable->itemAt(pos)) {
        m_contextMenu->exec(m_fileTable->mapToGlobal(pos));
    }
}

void BrowseView::openArchive(const QString& archivePath) {
    if (m_isLoading) return;
    
    m_currentArchivePath = archivePath;
    m_archivePathEdit->setText(archivePath);
    
    // Show loading state
    m_isLoading = true;
    m_loadingProgress->setVisible(true);
    
    // Hide drop zone, show content
    m_dropZone->setVisible(false);
    m_contentSplitter->setVisible(true);
    
    // Load archive contents
    loadArchiveContents();
    
    m_isArchiveOpen = true;
    updateToolbarState();
    
    emit archiveOpened(archivePath);
}

void BrowseView::closeArchive() {
    m_currentArchivePath.clear();
    m_archivePathEdit->clear();
    m_archiveEntries.clear();
    
    // Clear UI
    m_fileTree->clear();
    m_fileTable->setRowCount(0);
    
    // Show drop zone, hide content
    m_dropZone->setVisible(true);
    m_contentSplitter->setVisible(false);
    
    m_isArchiveOpen = false;
    m_isLoading = false;
    m_loadingProgress->setVisible(false);
    
    updateToolbarState();
    updateStatusInfo();
    
    emit archiveClosed();
}

void BrowseView::loadArchiveContents() {
    // Simulate loading archive contents
    // In real implementation, this would use the archive library
    
    m_archiveEntries.clear();
    
    // Add some sample entries for demonstration
    ArchiveEntry entry1;
    entry1.path = "documents/readme.txt";
    entry1.name = "readme.txt";
    entry1.size = 1024;
    entry1.compressedSize = 512;
    entry1.modified = QDateTime::currentDateTime();
    entry1.isDirectory = false;
    m_archiveEntries.append(entry1);
    
    ArchiveEntry entry2;
    entry2.path = "images/";
    entry2.name = "images";
    entry2.size = 0;
    entry2.compressedSize = 0;
    entry2.modified = QDateTime::currentDateTime();
    entry2.isDirectory = true;
    m_archiveEntries.append(entry2);
    
    ArchiveEntry entry3;
    entry3.path = "images/photo.jpg";
    entry3.name = "photo.jpg";
    entry3.size = 2048000;
    entry3.compressedSize = 1536000;
    entry3.modified = QDateTime::currentDateTime();
    entry3.isDirectory = false;
    m_archiveEntries.append(entry3);
    
    populateFileTree();
    populateFileTable();
    updateStatusInfo();
    
    m_isLoading = false;
    m_loadingProgress->setVisible(false);
}

void BrowseView::populateFileTree() {
    m_fileTree->clear();
    
    QTreeWidgetItem* rootItem = new QTreeWidgetItem(m_fileTree);
    rootItem->setText(0, QFileInfo(m_currentArchivePath).baseName());
    rootItem->setData(0, Qt::UserRole, "");
    rootItem->setExpanded(true);
    
    // Build tree structure from archive entries
    for (const auto& entry : m_archiveEntries) {
        if (entry.isDirectory) {
            QTreeWidgetItem* dirItem = new QTreeWidgetItem(rootItem);
            dirItem->setText(0, entry.name);
            dirItem->setData(0, Qt::UserRole, entry.path);
            dirItem->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
        }
    }
}

void BrowseView::populateFileTable(const QString& folderPath) {
    m_fileTable->setRowCount(0);
    
    int row = 0;
    for (const auto& entry : m_archiveEntries) {
        // Filter by folder path if specified
        if (!folderPath.isEmpty() && !entry.path.startsWith(folderPath)) {
            continue;
        }
        
        if (!entry.isDirectory) {
            m_fileTable->insertRow(row);
            
            // Name
            QTableWidgetItem* nameItem = new QTableWidgetItem(entry.name);
            nameItem->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
            m_fileTable->setItem(row, 0, nameItem);
            
            // Size
            m_fileTable->setItem(row, 1, new QTableWidgetItem(formatFileSize(entry.size)));
            
            // Compressed size
            m_fileTable->setItem(row, 2, new QTableWidgetItem(formatFileSize(entry.compressedSize)));
            
            // Modified date
            m_fileTable->setItem(row, 3, new QTableWidgetItem(entry.modified.toString()));
            
            row++;
        }
    }
    
    m_fileTable->resizeColumnsToContents();
}

void BrowseView::updateStatusInfo() {
    if (!m_isArchiveOpen) {
        m_fileCountLabel->setText("Files: 0");
        m_totalSizeLabel->setText("Total Size: 0 bytes");
        m_compressionRatioLabel->setText("Compression: 0%");
        return;
    }
    
    int fileCount = 0;
    qint64 totalSize = 0;
    qint64 compressedSize = 0;
    
    for (const auto& entry : m_archiveEntries) {
        if (!entry.isDirectory) {
            fileCount++;
            totalSize += entry.size;
            compressedSize += entry.compressedSize;
        }
    }
    
    double compressionRatio = totalSize > 0 ? 
        (1.0 - static_cast<double>(compressedSize) / totalSize) * 100.0 : 0.0;
    
    m_fileCountLabel->setText(QString("Files: %1").arg(fileCount));
    m_totalSizeLabel->setText(QString("Total Size: %1").arg(formatFileSize(totalSize)));
    m_compressionRatioLabel->setText(QString("Compression: %1%").arg(compressionRatio, 0, 'f', 1));
}

void BrowseView::updateToolbarState() {
    m_closeArchiveBtn->setEnabled(m_isArchiveOpen);
    m_extractAllBtn->setEnabled(m_isArchiveOpen);
    m_refreshBtn->setEnabled(m_isArchiveOpen);
    
    bool hasSelection = !m_fileTable->selectedItems().isEmpty();
    m_extractSelectedBtn->setEnabled(hasSelection && m_isArchiveOpen);
}

bool BrowseView::isArchiveFile(const QString& filePath) const {
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

QString BrowseView::formatFileSize(qint64 size) const {
    if (size < 1024) {
        return QString("%1 B").arg(size);
    } else if (size < 1024 * 1024) {
        return QString("%1 KB").arg(size / 1024.0, 0, 'f', 1);
    } else if (size < 1024 * 1024 * 1024) {
        return QString("%1 MB").arg(size / (1024.0 * 1024.0), 0, 'f', 1);
    } else {
        return QString("%1 GB").arg(size / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
    }
}

QString BrowseView::getFileIcon(const QString& fileName) const {
    Q_UNUSED(fileName)
    // In real implementation, return appropriate icon based on file extension
    return QString();
}

QTreeWidgetItem* BrowseView::findTreeItem(const QString& path) {
    Q_UNUSED(path)
    // In real implementation, find tree item by path
    return nullptr;
}


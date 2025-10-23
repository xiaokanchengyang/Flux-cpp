#include "browse_page.h"
#include "../models/archive_model.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTreeView>
#include <QTextEdit>
#include <QLabel>
#include <QToolBar>
#include <QAction>
#include <QLineEdit>
#include <QProgressBar>
#include <QGroupBox>
#include <QTableWidget>
#include <QTabWidget>
#include <QScrollArea>
#include <QTimer>
#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QMenu>
#include <QContextMenuEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QDrag>
#include <QStandardPaths>
#include <QStringConverter>
#include <QImageReader>
#include <QPixmap>
#include <QMovie>
#include <QSettings>
#include <QDebug>

BrowsePage::BrowsePage(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_toolbarLayout(nullptr)
    , m_mainSplitter(nullptr)
    , m_rightSplitter(nullptr)
    , m_toolBar(nullptr)
    , m_searchEdit(nullptr)
    , m_extractAction(nullptr)
    , m_addAction(nullptr)
    , m_deleteAction(nullptr)
    , m_refreshAction(nullptr)
    , m_expandAllAction(nullptr)
    , m_collapseAllAction(nullptr)
    , m_propertiesAction(nullptr)
    , m_treeView(nullptr)
    , m_archiveModel(nullptr)
    , m_proxyModel(nullptr)
    , m_previewTabs(nullptr)
    , m_textPreview(nullptr)
    , m_imagePreview(nullptr)
    , m_imageScrollArea(nullptr)
    , m_hexPreview(nullptr)
    , m_previewStatus(nullptr)
    , m_infoGroup(nullptr)
    , m_infoTable(nullptr)
    , m_statsGroup(nullptr)
    , m_totalFilesLabel(nullptr)
    , m_totalSizeLabel(nullptr)
    , m_compressionRatioLabel(nullptr)
    , m_compressionBar(nullptr)
    , m_previewTimer(nullptr)
    , m_searchTimer(nullptr)
    , m_autoPreview(true)
    , m_maxPreviewSize(1024 * 1024) // 1MB
{
    setupUI();
    
    // Create timers
    m_previewTimer = new QTimer(this);
    m_previewTimer->setSingleShot(true);
    m_previewTimer->setInterval(500); // 500ms delay
    connect(m_previewTimer, &QTimer::timeout, this, &BrowsePage::onPreviewTimer);
    
    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(300); // 300ms delay
    connect(m_searchTimer, &QTimer::timeout, this, &BrowsePage::updateStatistics);
    
    // Load settings
    QSettings settings;
    m_autoPreview = settings.value("browse/autoPreview", true).toBool();
    m_maxPreviewSize = settings.value("browse/maxPreviewSize", 1024 * 1024).toInt();
    
    updateActions();
}

BrowsePage::~BrowsePage() {
    // Save settings
    QSettings settings;
    settings.setValue("browse/autoPreview", m_autoPreview);
    settings.setValue("browse/maxPreviewSize", m_maxPreviewSize);
}

void BrowsePage::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(8, 8, 8, 8);
    m_mainLayout->setSpacing(8);
    
    setupToolBar();
    
    // Create main splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal);
    m_mainLayout->addWidget(m_mainSplitter);
    
    setupTreeView();
    
    // Create right splitter
    m_rightSplitter = new QSplitter(Qt::Vertical);
    m_mainSplitter->addWidget(m_rightSplitter);
    
    setupPreviewPanel();
    setupInfoPanel();
    setupStatusPanel();
    
    // Set splitter ratios
    m_mainSplitter->setSizes({400, 600});
    m_rightSplitter->setSizes({300, 200, 100});
    
    // Set splitter properties
    m_mainSplitter->setChildrenCollapsible(false);
    m_rightSplitter->setChildrenCollapsible(true);
}

void BrowsePage::setupToolBar() {
    m_toolbarLayout = new QHBoxLayout();
    m_toolbarLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->addLayout(m_toolbarLayout);
    
    // Create toolbar
    m_toolBar = new QToolBar();
    m_toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_toolBar->setIconSize(QSize(16, 16));
    
    // Extract button
    m_extractAction = m_toolBar->addAction(QIcon(":/icons/extract.png"), "Extract");
    m_extractAction->setToolTip("Extract selected files and folders");
    connect(m_extractAction, &QAction::triggered, this, &BrowsePage::onExtractSelected);
    
    // Add button
    m_addAction = m_toolBar->addAction(QIcon(":/icons/new.png"), "Add");
    m_addAction->setToolTip("Add files to archive");
    connect(m_addAction, &QAction::triggered, this, &BrowsePage::onAddFiles);
    
    // Delete button
    m_deleteAction = m_toolBar->addAction(QIcon(":/icons/folder.png"), "Delete");
    m_deleteAction->setToolTip("Delete selected files from archive");
    connect(m_deleteAction, &QAction::triggered, this, &BrowsePage::onDeleteSelected);
    
    m_toolBar->addSeparator();
    
    // Refresh button
    m_refreshAction = m_toolBar->addAction(QIcon(":/icons/home.png"), "Refresh");
    m_refreshAction->setToolTip("Refresh archive contents");
    connect(m_refreshAction, &QAction::triggered, this, &BrowsePage::onRefresh);
    
    // Expand/collapse buttons
    m_expandAllAction = m_toolBar->addAction("Expand All");
    connect(m_expandAllAction, &QAction::triggered, this, &BrowsePage::onExpandAll);
    
    m_collapseAllAction = m_toolBar->addAction("Collapse All");
    connect(m_collapseAllAction, &QAction::triggered, this, &BrowsePage::onCollapseAll);
    
    m_toolBar->addSeparator();
    
    // Properties button
    m_propertiesAction = m_toolBar->addAction("Properties");
    m_propertiesAction->setToolTip("Show detailed properties of selected file");
    connect(m_propertiesAction, &QAction::triggered, this, &BrowsePage::onProperties);
    
    m_toolbarLayout->addWidget(m_toolBar);
    
    // Search box
    m_toolbarLayout->addStretch();
    QLabel* searchLabel = new QLabel("Search:");
    m_toolbarLayout->addWidget(searchLabel);
    
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("Enter filename to search...");
    m_searchEdit->setMaximumWidth(200);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &BrowsePage::onSearchTextChanged);
    m_toolbarLayout->addWidget(m_searchEdit);
}

void BrowsePage::setupTreeView() {
    // Create model
    m_archiveModel = new ArchiveModel(this);
    connect(m_archiveModel, &ArchiveModel::dataUpdated, this, &BrowsePage::updateStatistics);
    
    // Create proxy model for sorting and filtering
    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_archiveModel);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setRecursiveFilteringEnabled(true);
    m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    
    // Create tree view
    m_treeView = new QTreeView();
    m_treeView->setModel(m_proxyModel);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeView->setDragEnabled(true);
    m_treeView->setDragDropMode(QAbstractItemView::DragOnly);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setSortingEnabled(true);
    m_treeView->sortByColumn(ArchiveModel::NameColumn, Qt::AscendingOrder);
    
    // Set column widths
    QHeaderView* header = m_treeView->header();
    header->setStretchLastSection(false);
    header->resizeSection(ArchiveModel::NameColumn, 250);
    header->resizeSection(ArchiveModel::SizeColumn, 80);
    header->resizeSection(ArchiveModel::CompressedSizeColumn, 100);
    header->resizeSection(ArchiveModel::RatioColumn, 60);
    header->resizeSection(ArchiveModel::ModifiedColumn, 120);
    header->resizeSection(ArchiveModel::PermissionsColumn, 80);
    header->resizeSection(ArchiveModel::CrcColumn, 80);
    header->setSectionResizeMode(ArchiveModel::NameColumn, QHeaderView::Stretch);
    
    // Connect signals
    connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &BrowsePage::onTreeSelectionChanged);
    connect(m_treeView, &QTreeView::doubleClicked, this, &BrowsePage::onTreeDoubleClicked);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &BrowsePage::onTreeContextMenu);
    
    m_mainSplitter->addWidget(m_treeView);
}

void BrowsePage::setupPreviewPanel() {
    m_previewTabs = new QTabWidget();
    m_previewTabs->setTabPosition(QTabWidget::South);
    
    // Text preview tab
    m_textPreview = new QTextEdit();
    m_textPreview->setReadOnly(true);
    m_textPreview->setFont(QFont("Consolas", 10));
    m_previewTabs->addTab(m_textPreview, "Text");
    
    // Image preview tab
    m_imagePreview = new QLabel();
    m_imagePreview->setAlignment(Qt::AlignCenter);
    m_imagePreview->setStyleSheet("border: 1px solid #ccc; background-color: #f9f9f9;");
    m_imagePreview->setMinimumSize(200, 150);
    
    m_imageScrollArea = new QScrollArea();
    m_imageScrollArea->setWidget(m_imagePreview);
    m_imageScrollArea->setWidgetResizable(true);
    m_previewTabs->addTab(m_imageScrollArea, "Image");
    
    // Hex preview tab
    m_hexPreview = new QTextEdit();
    m_hexPreview->setReadOnly(true);
    m_hexPreview->setFont(QFont("Consolas", 9));
    m_previewTabs->addTab(m_hexPreview, "Hex");
    
    // Preview status label
    m_previewStatus = new QLabel("Select a file to preview its contents");
    m_previewStatus->setAlignment(Qt::AlignCenter);
    m_previewStatus->setStyleSheet("color: #666; font-style: italic; padding: 20px;");
    
    // Create preview container
    QWidget* previewContainer = new QWidget();
    QVBoxLayout* previewLayout = new QVBoxLayout(previewContainer);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    previewLayout->addWidget(m_previewStatus);
    previewLayout->addWidget(m_previewTabs);
    
    m_previewTabs->setVisible(false);
    
    m_rightSplitter->addWidget(previewContainer);
}

void BrowsePage::setupInfoPanel() {
    m_infoGroup = new QGroupBox("File Information");
    QVBoxLayout* infoLayout = new QVBoxLayout(m_infoGroup);
    
    m_infoTable = new QTableWidget(0, 2);
    m_infoTable->setHorizontalHeaderLabels({"Property", "Value"});
    m_infoTable->horizontalHeader()->setStretchLastSection(true);
    m_infoTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_infoTable->verticalHeader()->setVisible(false);
    m_infoTable->setAlternatingRowColors(true);
    m_infoTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    infoLayout->addWidget(m_infoTable);
    m_rightSplitter->addWidget(m_infoGroup);
}

void BrowsePage::setupStatusPanel() {
    m_statsGroup = new QGroupBox("Statistics");
    QVBoxLayout* statsLayout = new QVBoxLayout(m_statsGroup);
    
    m_totalFilesLabel = new QLabel("Files: 0");
    m_totalSizeLabel = new QLabel("Total size: 0 B");
    m_compressionRatioLabel = new QLabel("Average compression: 0%");
    
    m_compressionBar = new QProgressBar();
    m_compressionBar->setRange(0, 100);
    m_compressionBar->setValue(0);
    m_compressionBar->setFormat("Compression: %p%");
    
    statsLayout->addWidget(m_totalFilesLabel);
    statsLayout->addWidget(m_totalSizeLabel);
    statsLayout->addWidget(m_compressionRatioLabel);
    statsLayout->addWidget(m_compressionBar);
    statsLayout->addStretch();
    
    m_rightSplitter->addWidget(m_statsGroup);
}

void BrowsePage::openArchive(const QString& archivePath) {
    m_currentArchivePath = archivePath;
    
    // Clear current content
    m_archiveModel->clear();
    clearPreview();
    updateActions();
    
    // This should trigger a background task to read archive contents
    // In actual implementation, this would notify MainWindow via signal to start WorkerThread
    emit previewRequested(archivePath, ""); // Empty path means list all files
}

void BrowsePage::closeArchive() {
    m_currentArchivePath.clear();
    m_archiveModel->clear();
    clearPreview();
    updateActions();
}

QStringList BrowsePage::getSelectedPaths() const {
    QModelIndexList selectedIndexes = m_treeView->selectionModel()->selectedRows();
    QModelIndexList sourceIndexes;
    
    // Convert to source model indexes
    for (const QModelIndex& index : selectedIndexes) {
        sourceIndexes << m_proxyModel->mapToSource(index);
    }
    
    return m_archiveModel->getSelectedPaths(sourceIndexes);
}

void BrowsePage::setArchiveEntries(const QList<ArchiveEntry>& entries) {
    m_archiveModel->setEntries(entries);
    
    // Expand root node
    m_treeView->expandToDepth(0);
    
    // Adjust column widths
    resizeTreeColumns();
    
    updateStatistics();
}

void BrowsePage::setPreviewContent(const QString& filePath, const QByteArray& content, const QString& mimeType) {
    if (filePath != m_currentPreviewPath) {
        return; // Preview content is outdated
    }
    
    m_previewStatus->setVisible(false);
    m_previewTabs->setVisible(true);
    
    if (mimeType.startsWith("text/") || mimeType.contains("json") || mimeType.contains("xml")) {
        // Text preview
        QString encoding = detectTextEncoding(content);
        showTextPreview(content, encoding);
        m_previewTabs->setCurrentIndex(0); // Text tab
    } else if (mimeType.startsWith("image/")) {
        // Image preview
        showImagePreview(content);
        m_previewTabs->setCurrentIndex(1); // Image tab
    } else {
        // Hex preview
        showHexPreview(content);
        m_previewTabs->setCurrentIndex(2); // Hex tab
    }
}

void BrowsePage::showError(const QString& message) {
    QMessageBox::warning(this, "Error", message);
}

void BrowsePage::onTreeSelectionChanged() {
    updateActions();
    
    QModelIndexList selectedIndexes = m_treeView->selectionModel()->selectedRows();
    if (selectedIndexes.size() == 1) {
        QModelIndex sourceIndex = m_proxyModel->mapToSource(selectedIndexes.first());
        const ArchiveEntry* entry = m_archiveModel->getEntry(sourceIndex);
        if (entry) {
            updateFileInfo(entry);
            
            // If it's a file and auto preview is enabled, start preview
            if (!entry->isDirectory && m_autoPreview && entry->size <= m_maxPreviewSize) {
                m_currentPreviewPath = entry->path;
                m_previewTimer->start();
            } else {
                clearPreview();
            }
        }
    } else {
        // Multiple selection or no selection
        m_infoTable->setRowCount(0);
        clearPreview();
    }
}

void BrowsePage::onTreeDoubleClicked(const QModelIndex& index) {
    QModelIndex sourceIndex = m_proxyModel->mapToSource(index);
    const ArchiveEntry* entry = m_archiveModel->getEntry(sourceIndex);
    if (entry && !entry->isDirectory) {
        // Double-click file, request extraction to temp directory and open
        QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        emit extractRequested(m_currentArchivePath, {entry->path}, tempDir);
    }
}

void BrowsePage::onTreeContextMenu(const QPoint& pos) {
    QModelIndex index = m_treeView->indexAt(pos);
    if (!index.isValid()) return;
    
    QMenu contextMenu(this);
    
    // Extract
    QAction* extractAction = contextMenu.addAction(QIcon(":/icons/extract.png"), "Extract...");
    connect(extractAction, &QAction::triggered, this, &BrowsePage::onExtractSelected);
    
    contextMenu.addSeparator();
    
    // Copy path
    QAction* copyPathAction = contextMenu.addAction("Copy Path");
    connect(copyPathAction, &QAction::triggered, this, &BrowsePage::onCopyPath);
    
    contextMenu.addSeparator();
    
    // Properties
    QAction* propertiesAction = contextMenu.addAction("Properties");
    connect(propertiesAction, &QAction::triggered, this, &BrowsePage::onProperties);
    
    contextMenu.exec(m_treeView->mapToGlobal(pos));
}

void BrowsePage::onSearchTextChanged(const QString& text) {
    m_proxyModel->setFilterFixedString(text);
    m_searchTimer->start();
}

void BrowsePage::onExtractSelected() {
    QStringList selectedPaths = getSelectedPaths();
    if (selectedPaths.isEmpty()) {
        QMessageBox::information(this, "Info", "Please select files or folders to extract first.");
        return;
    }
    
    QString outputDir = QFileDialog::getExistingDirectory(
        this,
        "Select extraction directory",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
    );
    
    if (!outputDir.isEmpty()) {
        emit extractRequested(m_currentArchivePath, selectedPaths, outputDir);
    }
}

void BrowsePage::onAddFiles() {
    QStringList filePaths = QFileDialog::getOpenFileNames(
        this,
        "Select files to add",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
    );
    
    if (!filePaths.isEmpty()) {
        emit addFilesRequested(m_currentArchivePath, filePaths);
    }
}

void BrowsePage::onDeleteSelected() {
    QStringList selectedPaths = getSelectedPaths();
    if (selectedPaths.isEmpty()) {
        QMessageBox::information(this, "Info", "Please select files or folders to delete first.");
        return;
    }
    
    int ret = QMessageBox::question(
        this,
        "Confirm Deletion",
        QString("Are you sure you want to delete the selected %1 items from the archive?\n\nThis operation cannot be undone.").arg(selectedPaths.size()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (ret == QMessageBox::Yes) {
        emit deleteFilesRequested(m_currentArchivePath, selectedPaths);
    }
}

void BrowsePage::onRefresh() {
    if (!m_currentArchivePath.isEmpty()) {
        openArchive(m_currentArchivePath);
    }
}

void BrowsePage::onExpandAll() {
    m_treeView->expandAll();
}

void BrowsePage::onCollapseAll() {
    m_treeView->collapseAll();
}

void BrowsePage::onCopyPath() {
    QStringList selectedPaths = getSelectedPaths();
    if (!selectedPaths.isEmpty()) {
        QApplication::clipboard()->setText(selectedPaths.join('\n'));
    }
}

void BrowsePage::onProperties() {
    // Display detailed properties dialog
    QModelIndexList selectedIndexes = m_treeView->selectionModel()->selectedRows();
    if (selectedIndexes.size() == 1) {
        QModelIndex sourceIndex = m_proxyModel->mapToSource(selectedIndexes.first());
        const ArchiveEntry* entry = m_archiveModel->getEntry(sourceIndex);
        if (entry) {
            // Here we could create a detailed properties dialog
            QString info = QString(
                "Filename: %1\n"
                "Path: %2\n"
                "Size: %3\n"
                "Compressed size: %4\n"
                "Compression ratio: %5%\n"
                "Modified: %6\n"
                "Permissions: %7\n"
                "CRC32: %8"
            ).arg(entry->name)
             .arg(entry->path)
             .arg(entry->size)
             .arg(entry->compressedSize)
             .arg(entry->compressionRatio(), 0, 'f', 1)
             .arg(entry->modified.toString())
             .arg(entry->permissions)
             .arg(entry->crc32, 8, 16, QChar('0'));
            
            QMessageBox::information(this, "File Properties", info);
        }
    }
}

void BrowsePage::updateStatistics() {
    ArchiveModel::Statistics stats = m_archiveModel->getStatistics();
    
    m_totalFilesLabel->setText(QString("Files: %1 (Folders: %2)")
                              .arg(stats.totalFiles)
                              .arg(stats.totalDirectories));
    
    // Format file size
    auto formatSize = [](qint64 size) -> QString {
        const QStringList units = {"B", "KB", "MB", "GB", "TB"};
        double sizeDouble = size;
        int unitIndex = 0;
        
        while (sizeDouble >= 1024.0 && unitIndex < units.size() - 1) {
            sizeDouble /= 1024.0;
            unitIndex++;
        }
        
        if (unitIndex == 0) {
            return QString("%1 %2").arg(size).arg(units[unitIndex]);
        } else {
            return QString("%1 %2").arg(sizeDouble, 0, 'f', 1).arg(units[unitIndex]);
        }
    };
    
    m_totalSizeLabel->setText(QString("Total size: %1 → %2")
                             .arg(formatSize(stats.totalSize))
                             .arg(formatSize(stats.totalCompressedSize)));
    
    m_compressionRatioLabel->setText(QString("Average compression: %1%")
                                    .arg(stats.averageCompressionRatio, 0, 'f', 1));
    
    m_compressionBar->setValue(static_cast<int>(stats.averageCompressionRatio));
}

void BrowsePage::onPreviewTimer() {
    if (!m_currentPreviewPath.isEmpty()) {
        emit previewRequested(m_currentArchivePath, m_currentPreviewPath);
    }
}

void BrowsePage::updateActions() {
    bool hasArchive = !m_currentArchivePath.isEmpty();
    bool hasSelection = !getSelectedPaths().isEmpty();
    
    m_extractAction->setEnabled(hasArchive && hasSelection);
    m_addAction->setEnabled(hasArchive);
    m_deleteAction->setEnabled(hasArchive && hasSelection);
    m_refreshAction->setEnabled(hasArchive);
    m_expandAllAction->setEnabled(hasArchive);
    m_collapseAllAction->setEnabled(hasArchive);
    m_propertiesAction->setEnabled(hasArchive && hasSelection);
}

void BrowsePage::updateFileInfo(const ArchiveEntry* entry) {
    if (!entry) {
        m_infoTable->setRowCount(0);
        return;
    }
    
    QStringList properties = {
        "Name", entry->name,
        "Path", entry->path,
        "Type", entry->isDirectory ? "Folder" : "File",
        "Size", QString::number(entry->size),
        "Compressed size", QString::number(entry->compressedSize),
        "Compression ratio", QString("%1%").arg(entry->compressionRatio(), 0, 'f', 1),
        "Modified", entry->modified.toString(),
        "Permissions", entry->permissions,
        "MIME type", entry->mimeType,
        "CRC32", QString("%1").arg(entry->crc32, 8, 16, QChar('0')).toUpper()
    };
    
    m_infoTable->setRowCount(properties.size() / 2);
    
    for (int i = 0; i < properties.size(); i += 2) {
        int row = i / 2;
        m_infoTable->setItem(row, 0, new QTableWidgetItem(properties[i]));
        m_infoTable->setItem(row, 1, new QTableWidgetItem(properties[i + 1]));
    }
}

void BrowsePage::clearPreview() {
    m_currentPreviewPath.clear();
    m_previewStatus->setVisible(true);
    m_previewTabs->setVisible(false);
    m_textPreview->clear();
    m_imagePreview->clear();
    m_hexPreview->clear();
}

void BrowsePage::showImagePreview(const QByteArray& data) {
    QPixmap pixmap;
    if (pixmap.loadFromData(data)) {
        // Scale image to fit preview area
        QSize previewSize = m_imageScrollArea->size() - QSize(20, 20);
        if (pixmap.size().width() > previewSize.width() || pixmap.size().height() > previewSize.height()) {
            pixmap = pixmap.scaled(previewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        m_imagePreview->setPixmap(pixmap);
    } else {
        m_imagePreview->setText("Unable to display image");
    }
}

void BrowsePage::showTextPreview(const QByteArray& data, const QString& encoding) {
    // Decode text using specified encoding
    QString text;
    if (encoding.toLower() == "utf-8" || encoding.isEmpty()) {
        text = QString::fromUtf8(data);
    } else {
        // For other encodings, try UTF-8 first, if that fails use local encoding
        text = QString::fromUtf8(data);
        if (text.contains(QChar::ReplacementCharacter)) {
            text = QString::fromLocal8Bit(data);
        }
    }
    
    // Limit preview length
    if (text.length() > 10000) {
        text = text.left(10000) + "\n\n... (content truncated)";
    }
    
    m_textPreview->setPlainText(text);
}

void BrowsePage::showHexPreview(const QByteArray& data) {
    QString hexText;
    const int bytesPerLine = 16;
    
    for (int i = 0; i < data.size(); i += bytesPerLine) {
        // Address
        hexText += QString("%1: ").arg(i, 8, 16, QChar('0')).toUpper();
        
        // Hex data
        QString hexPart;
        QString asciiPart;
        
        for (int j = 0; j < bytesPerLine && (i + j) < data.size(); ++j) {
            unsigned char byte = static_cast<unsigned char>(data[i + j]);
            hexPart += QString("%1 ").arg(byte, 2, 16, QChar('0')).toUpper();
            asciiPart += (byte >= 32 && byte <= 126) ? QChar(byte) : QChar('.');
        }
        
        // Pad with spaces
        while (hexPart.length() < bytesPerLine * 3) {
            hexPart += " ";
        }
        
        hexText += hexPart + " " + asciiPart + "\n";
        
        // Limit preview length
        if (i > 1024) {
            hexText += "\n... (content truncated)";
            break;
        }
    }
    
    m_hexPreview->setPlainText(hexText);
}

QString BrowsePage::detectTextEncoding(const QByteArray& data) const {
    // Simple encoding detection
    if (data.startsWith("\xEF\xBB\xBF")) {
        return "UTF-8";
    }
    
    // Check if valid UTF-8
    QString testText = QString::fromUtf8(data);
    if (!testText.contains(QChar::ReplacementCharacter)) {
        return "UTF-8";
    }
    
    // Default to system encoding
    return "UTF-8"; // Default to UTF-8 in Qt6
}

void BrowsePage::resizeTreeColumns() {
    m_treeView->resizeColumnToContents(ArchiveModel::SizeColumn);
    m_treeView->resizeColumnToContents(ArchiveModel::CompressedSizeColumn);
    m_treeView->resizeColumnToContents(ArchiveModel::RatioColumn);
    m_treeView->resizeColumnToContents(ArchiveModel::ModifiedColumn);
    m_treeView->resizeColumnToContents(ArchiveModel::PermissionsColumn);
    m_treeView->resizeColumnToContents(ArchiveModel::CrcColumn);
}


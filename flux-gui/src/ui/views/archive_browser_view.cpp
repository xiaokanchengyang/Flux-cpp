#include "archive_browser_view.h"

#include <QApplication>
#include <QHeaderView>
#include <QScrollBar>
#include <QSplitter>
#include <QStackedWidget>
#include <QToolButton>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QDateTimeEdit>
#include <QGroupBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QButtonGroup>
#include <QRadioButton>
#include <QTextEdit>
#include <QScrollArea>
#include <QFrame>
#include <QSizePolicy>
#include <QStyle>
#include <QStyleOption>
#include <QPainter>
#include <QPixmap>
#include <QMimeDatabase>
#include <QMimeType>
#include <QFileIconProvider>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QClipboard>
#include <QMessageBox>
#include <QInputDialog>
#include <QProgressDialog>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <QDebug>

namespace FluxGUI::UI::Views {

ArchiveBrowserView::ArchiveBrowserView(QWidget* parent)
    : QWidget(parent)
    , m_currentViewMode(ViewMode::Tree)
    , m_selectionMode(SelectionMode::Extended)
    , m_navigationIndex(-1)
    , m_previewPanelVisible(false)
    , m_minSizeFilter(0)
    , m_maxSizeFilter(std::numeric_limits<qint64>::max())
    , m_highContrastMode(false)
    , m_fontScale(1.0)
    , m_showHiddenFiles(false)
    , m_autoRefresh(true)
{
    setObjectName("ArchiveBrowserView");
    setAcceptDrops(true);
    
    initializeUI();
    setupConnections();
    applyStyles();
    
    // Initialize search timer
    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(SEARCH_DELAY);
    connect(m_searchTimer, &QTimer::timeout, this, &ArchiveBrowserView::onFilterChanged);
    
    qDebug() << "ArchiveBrowserView initialized";
}

ArchiveBrowserView::~ArchiveBrowserView() = default;

void ArchiveBrowserView::openArchive(const QString& archivePath) {
    if (archivePath.isEmpty() || !isValidArchivePath(archivePath)) {
        emit archiveError("Invalid archive path: " + archivePath);
        return;
    }
    
    // Close current archive if open
    if (isArchiveOpen()) {
        closeArchive();
    }
    
    m_currentArchivePath = archivePath;
    m_currentPath = "/";
    
    // Show loading progress
    m_loadingProgress->setVisible(true);
    m_loadingProgress->setRange(0, 0); // Indeterminate progress
    
    // Setup archive model
    setupArchiveModel();
    
    // Load archive contents asynchronously
    QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, [this, watcher]() {
        bool success = watcher->result();
        m_loadingProgress->setVisible(false);
        
        if (success) {
            populateModel();
            updateNavigationState();
            updateBreadcrumbs();
            emit archiveOpened(m_currentArchivePath);
            
            m_statusLabel->setText(QString("Archive loaded: %1 items").arg(m_archiveFiles.size()));
        } else {
            emit archiveError("Failed to load archive: " + m_currentArchivePath);
        }
        
        watcher->deleteLater();
    });
    
    QFuture<bool> future = QtConcurrent::run([this, archivePath]() {
        return loadArchiveContents(archivePath);
    });
    watcher->setFuture(future);
    
    qDebug() << "Opening archive:" << archivePath;
}

void ArchiveBrowserView::closeArchive() {
    if (!isArchiveOpen()) return;
    
    // Clear data
    m_currentArchivePath.clear();
    m_currentPath.clear();
    m_archiveFiles.clear();
    m_fileInfoCache.clear();
    m_navigationHistory.clear();
    m_navigationIndex = -1;
    
    // Clear models
    clearModel();
    
    // Update UI
    updateNavigationState();
    updateBreadcrumbs();
    clearPreview();
    
    m_statusLabel->setText("No archive open");
    
    emit archiveClosed();
    
    qDebug() << "Archive closed";
}

void ArchiveBrowserView::refreshArchive() {
    if (!isArchiveOpen()) return;
    
    QString currentPath = m_currentPath;
    openArchive(m_currentArchivePath);
    navigateToPath(currentPath);
}

void ArchiveBrowserView::setViewMode(ViewMode mode) {
    if (m_currentViewMode == mode) return;
    
    ViewMode oldMode = m_currentViewMode;
    m_currentViewMode = mode;
    
    // Animate view transition
    animateViewTransition();
    
    switchViewMode(mode);
    updateViewConfiguration();
    
    qDebug() << "View mode changed from" << static_cast<int>(oldMode) 
             << "to" << static_cast<int>(mode);
}

void ArchiveBrowserView::setSelectionMode(SelectionMode mode) {
    if (m_selectionMode == mode) return;
    
    m_selectionMode = mode;
    
    QAbstractItemView::SelectionMode qtMode;
    switch (mode) {
        case SelectionMode::Single:
            qtMode = QAbstractItemView::SingleSelection;
            break;
        case SelectionMode::Multiple:
            qtMode = QAbstractItemView::MultiSelection;
            break;
        case SelectionMode::Extended:
            qtMode = QAbstractItemView::ExtendedSelection;
            break;
    }
    
    if (m_treeView) m_treeView->setSelectionMode(qtMode);
    if (m_listView) m_listView->setSelectionMode(qtMode);
    if (m_iconView) m_iconView->setSelectionMode(qtMode);
}

void ArchiveBrowserView::navigateToPath(const QString& path) {
    if (path == m_currentPath) return;
    
    if (!isValidFilePath(path)) {
        qWarning() << "Invalid navigation path:" << path;
        return;
    }
    
    QString oldPath = m_currentPath;
    m_currentPath = path;
    
    // Add to navigation history
    addToNavigationHistory(path);
    
    // Update model filtering
    applyFilters();
    
    // Update UI
    updateNavigationState();
    updateBreadcrumbs();
    
    emit pathChanged(m_currentPath);
    
    qDebug() << "Navigated from" << oldPath << "to" << path;
}

void ArchiveBrowserView::navigateUp() {
    if (m_currentPath == "/" || m_currentPath.isEmpty()) return;
    
    QString parentPath = QFileInfo(m_currentPath).path();
    if (parentPath.isEmpty() || parentPath == ".") {
        parentPath = "/";
    }
    
    navigateToPath(parentPath);
}

void ArchiveBrowserView::navigateBack() {
    if (m_navigationIndex <= 0) return;
    
    m_navigationIndex--;
    QString path = m_navigationHistory.at(m_navigationIndex);
    
    // Navigate without adding to history
    QString oldPath = m_currentPath;
    m_currentPath = path;
    
    applyFilters();
    updateNavigationState();
    updateBreadcrumbs();
    
    emit pathChanged(m_currentPath);
    
    qDebug() << "Navigated back from" << oldPath << "to" << path;
}

void ArchiveBrowserView::navigateForward() {
    if (m_navigationIndex >= m_navigationHistory.size() - 1) return;
    
    m_navigationIndex++;
    QString path = m_navigationHistory.at(m_navigationIndex);
    
    // Navigate without adding to history
    QString oldPath = m_currentPath;
    m_currentPath = path;
    
    applyFilters();
    updateNavigationState();
    updateBreadcrumbs();
    
    emit pathChanged(m_currentPath);
    
    qDebug() << "Navigated forward from" << oldPath << "to" << path;
}

QStringList ArchiveBrowserView::selectedFiles() const {
    QStringList files;
    
    QAbstractItemView* currentView = nullptr;
    switch (m_currentViewMode) {
        case ViewMode::Tree:
            currentView = m_treeView;
            break;
        case ViewMode::List:
        case ViewMode::Details:
            currentView = m_listView;
            break;
        case ViewMode::Icons:
            currentView = m_iconView;
            break;
    }
    
    if (!currentView || !m_proxyModel) return files;
    
    QModelIndexList selectedIndexes = currentView->selectionModel()->selectedRows();
    for (const QModelIndex& index : selectedIndexes) {
        QModelIndex sourceIndex = m_proxyModel->mapToSource(index);
        if (sourceIndex.isValid()) {
            QString fileName = m_archiveModel->item(sourceIndex.row())->text();
            files.append(fileName);
        }
    }
    
    return files;
}

QStringList ArchiveBrowserView::selectedPaths() const {
    QStringList paths;
    QStringList files = selectedFiles();
    
    for (const QString& file : files) {
        QString fullPath = m_currentPath;
        if (!fullPath.endsWith('/')) fullPath += '/';
        fullPath += file;
        paths.append(fullPath);
    }
    
    return paths;
}

int ArchiveBrowserView::selectedCount() const {
    return selectedFiles().size();
}

qint64 ArchiveBrowserView::selectedSize() const {
    qint64 totalSize = 0;
    QStringList files = selectedFiles();
    
    for (const QString& file : files) {
        auto it = m_fileInfoCache.find(file);
        if (it != m_fileInfoCache.end()) {
            totalSize += it->second.size;
        }
    }
    
    return totalSize;
}

void ArchiveBrowserView::selectAll() {
    QAbstractItemView* currentView = getCurrentView();
    if (currentView) {
        currentView->selectAll();
    }
}

void ArchiveBrowserView::selectNone() {
    QAbstractItemView* currentView = getCurrentView();
    if (currentView) {
        currentView->clearSelection();
    }
}

void ArchiveBrowserView::invertSelection() {
    QAbstractItemView* currentView = getCurrentView();
    if (!currentView || !m_proxyModel) return;
    
    QItemSelectionModel* selectionModel = currentView->selectionModel();
    QModelIndex topLeft = m_proxyModel->index(0, 0);
    QModelIndex bottomRight = m_proxyModel->index(m_proxyModel->rowCount() - 1, 0);
    
    QItemSelection allItems(topLeft, bottomRight);
    selectionModel->select(allItems, QItemSelectionModel::Toggle);
}

void ArchiveBrowserView::selectByPattern(const QString& pattern) {
    if (pattern.isEmpty()) return;
    
    QAbstractItemView* currentView = getCurrentView();
    if (!currentView || !m_proxyModel) return;
    
    QRegularExpression regex(pattern, QRegularExpression::CaseInsensitiveOption);
    if (!regex.isValid()) return;
    
    QItemSelectionModel* selectionModel = currentView->selectionModel();
    selectionModel->clearSelection();
    
    for (int row = 0; row < m_proxyModel->rowCount(); ++row) {
        QModelIndex index = m_proxyModel->index(row, 0);
        QString fileName = index.data().toString();
        
        if (regex.match(fileName).hasMatch()) {
            selectionModel->select(index, QItemSelectionModel::Select);
        }
    }
}

void ArchiveBrowserView::setSearchText(const QString& text) {
    if (m_searchText == text) return;
    
    m_searchText = text;
    m_searchEdit->setText(text);
    
    // Restart search timer
    m_searchTimer->stop();
    m_searchTimer->start();
}

void ArchiveBrowserView::setFileTypeFilter(const QString& filter) {
    if (m_fileTypeFilter == filter) return;
    
    m_fileTypeFilter = filter;
    applyFilters();
}

void ArchiveBrowserView::setSizeFilter(qint64 minSize, qint64 maxSize) {
    if (m_minSizeFilter == minSize && m_maxSizeFilter == maxSize) return;
    
    m_minSizeFilter = minSize;
    m_maxSizeFilter = maxSize;
    applyFilters();
}

void ArchiveBrowserView::setDateFilter(const QDateTime& from, const QDateTime& to) {
    if (m_fromDateFilter == from && m_toDateFilter == to) return;
    
    m_fromDateFilter = from;
    m_toDateFilter = to;
    applyFilters();
}

void ArchiveBrowserView::clearFilters() {
    m_searchText.clear();
    m_fileTypeFilter.clear();
    m_minSizeFilter = 0;
    m_maxSizeFilter = std::numeric_limits<qint64>::max();
    m_fromDateFilter = QDateTime();
    m_toDateFilter = QDateTime();
    
    m_searchEdit->clear();
    applyFilters();
}

void ArchiveBrowserView::showPreviewPanel(bool show) {
    if (m_previewPanelVisible == show) return;
    
    m_previewPanelVisible = show;
    
    // Animate preview panel toggle
    animatePreviewToggle();
    
    if (show) {
        // Update preview for currently selected file
        QStringList selected = selectedFiles();
        if (!selected.isEmpty()) {
            updatePreview(selected.first());
        }
    } else {
        clearPreview();
    }
    
    m_previewAction->setChecked(show);
}

void ArchiveBrowserView::setHighContrastMode(bool enabled) {
    if (m_highContrastMode == enabled) return;
    
    m_highContrastMode = enabled;
    applyStyles();
}

void ArchiveBrowserView::setFontScale(double scale) {
    if (qFuzzyCompare(m_fontScale, scale)) return;
    
    m_fontScale = scale;
    
    // Update font sizes
    QFont font = this->font();
    font.setPointSizeF(font.pointSizeF() * scale);
    setFont(font);
    
    applyStyles();
}

// Event handlers implementation
void ArchiveBrowserView::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void ArchiveBrowserView::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void ArchiveBrowserView::dropEvent(QDropEvent* event) {
    QStringList filePaths;
    
    for (const QUrl& url : event->mimeData()->urls()) {
        if (url.isLocalFile()) {
            filePaths.append(url.toLocalFile());
        }
    }
    
    if (!filePaths.isEmpty()) {
        emit addFilesRequested(filePaths, m_currentPath);
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void ArchiveBrowserView::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Delete:
            if (!selectedFiles().isEmpty()) {
                emit deleteRequested(selectedPaths());
            }
            break;
            
        case Qt::Key_F2:
            if (selectedCount() == 1) {
                QString oldName = selectedFiles().first();
                bool ok;
                QString newName = QInputDialog::getText(
                    this, "Rename File", "New name:", QLineEdit::Normal, oldName, &ok);
                if (ok && !newName.isEmpty() && newName != oldName) {
                    emit renameRequested(oldName, newName);
                }
            }
            break;
            
        case Qt::Key_F5:
            refreshArchive();
            break;
            
        case Qt::Key_Backspace:
        case Qt::Key_Alt + Qt::Key_Left:
            navigateBack();
            break;
            
        case Qt::Key_Alt + Qt::Key_Right:
            navigateForward();
            break;
            
        case Qt::Key_Alt + Qt::Key_Up:
            navigateUp();
            break;
            
        case Qt::Key_Escape:
            selectNone();
            break;
            
        case Qt::Key_F3:
            showPreviewPanel(!m_previewPanelVisible);
            break;
            
        default:
            QWidget::keyPressEvent(event);
            break;
    }
}

void ArchiveBrowserView::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    
    // Adjust preview panel size
    if (m_previewPanelVisible) {
        resizePreviewPanel();
    }
}

void ArchiveBrowserView::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    
    // Focus the current view
    QAbstractItemView* currentView = getCurrentView();
    if (currentView) {
        currentView->setFocus();
    }
}

// Private implementation methods would continue here...
// Due to length constraints, I'll provide the key initialization methods

void ArchiveBrowserView::initializeUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    createToolbar();
    createMainView();
    createPreviewPanel();
    createStatusArea();
    setupLayouts();
}

void ArchiveBrowserView::createToolbar() {
    m_toolbar = new QToolBar();
    m_toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    
    // Navigation actions
    m_backAction = m_toolbar->addAction("Back");
    m_backAction->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    m_backAction->setEnabled(false);
    
    m_forwardAction = m_toolbar->addAction("Forward");
    m_forwardAction->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
    m_forwardAction->setEnabled(false);
    
    m_upAction = m_toolbar->addAction("Up");
    m_upAction->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
    m_upAction->setEnabled(false);
    
    m_toolbar->addSeparator();
    
    // View actions
    m_refreshAction = m_toolbar->addAction("Refresh");
    m_refreshAction->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    
    m_viewModeAction = m_toolbar->addAction("View Mode");
    m_previewAction = m_toolbar->addAction("Preview");
    m_previewAction->setCheckable(true);
    
    m_toolbar->addSeparator();
    
    // Search
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("Search files...");
    m_searchEdit->setMaximumWidth(200);
    m_toolbar->addWidget(m_searchEdit);
    
    m_mainLayout->addWidget(m_toolbar);
}

void ArchiveBrowserView::createMainView() {
    m_contentLayout = new QHBoxLayout();
    
    m_mainSplitter = new QSplitter(Qt::Horizontal);
    
    // Create view container
    m_viewContainer = new QWidget();
    QVBoxLayout* viewLayout = new QVBoxLayout(m_viewContainer);
    
    // Breadcrumb navigation
    m_breadcrumbWidget = new QWidget();
    m_breadcrumbLayout = new QHBoxLayout(m_breadcrumbWidget);
    viewLayout->addWidget(m_breadcrumbWidget);
    
    // View stack
    m_viewStack = new QStackedWidget();
    
    // Create different views
    m_treeView = new QTreeView();
    m_listView = new QListView();
    m_iconView = new QListView();
    
    m_viewStack->addWidget(m_treeView);
    m_viewStack->addWidget(m_listView);
    m_viewStack->addWidget(m_iconView);
    
    viewLayout->addWidget(m_viewStack);
    
    m_mainSplitter->addWidget(m_viewContainer);
    m_contentLayout->addWidget(m_mainSplitter);
    
    QWidget* contentWidget = new QWidget();
    contentWidget->setLayout(m_contentLayout);
    m_mainLayout->addWidget(contentWidget);
}

void ArchiveBrowserView::setupConnections() {
    // Toolbar connections
    connect(m_backAction, &QAction::triggered, this, &ArchiveBrowserView::navigateBack);
    connect(m_forwardAction, &QAction::triggered, this, &ArchiveBrowserView::navigateForward);
    connect(m_upAction, &QAction::triggered, this, &ArchiveBrowserView::navigateUp);
    connect(m_refreshAction, &QAction::triggered, this, &ArchiveBrowserView::refreshArchive);
    connect(m_previewAction, &QAction::toggled, this, &ArchiveBrowserView::showPreviewPanel);
    
    // Search connections
    connect(m_searchEdit, &QLineEdit::textChanged, this, &ArchiveBrowserView::setSearchText);
}

QAbstractItemView* ArchiveBrowserView::getCurrentView() const {
    switch (m_currentViewMode) {
        case ViewMode::Tree:
            return m_treeView;
        case ViewMode::List:
        case ViewMode::Details:
            return m_listView;
        case ViewMode::Icons:
            return m_iconView;
    }
    return nullptr;
}

bool ArchiveBrowserView::loadArchiveContents(const QString& archivePath) {
    // This would integrate with the actual archive library
    // For now, return a placeholder implementation
    Q_UNUSED(archivePath)
    
    // Simulate loading time
    QThread::msleep(500);
    
    // Create sample data
    m_archiveFiles.clear();
    
    ArchiveFileInfo rootFile;
    rootFile.name = "sample.txt";
    rootFile.path = "/sample.txt";
    rootFile.type = "text/plain";
    rootFile.size = 1024;
    rootFile.compressedSize = 512;
    rootFile.modified = QDateTime::currentDateTime();
    rootFile.isDirectory = false;
    rootFile.isEncrypted = false;
    rootFile.compressionRatio = 0.5;
    
    m_archiveFiles.push_back(rootFile);
    m_fileInfoCache[rootFile.name] = rootFile;
    
    return true;
}

} // namespace FluxGUI::UI::Views

#include "rich_file_display.h"

#include <QApplication>
#include <QStyle>
#include <QStyleOption>
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QContextMenuEvent>
#include <QResizeEvent>
#include <QScrollBar>
#include <QHeaderView>
#include <QSplitter>
#include <QScrollArea>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QEasingCurve>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QDir>
#include <QFileIconProvider>
#include <QImageReader>
#include <QPixmapCache>
#include <QConcurrentRun>
#include <QFutureWatcher>
#include <QElapsedTimer>
#include <QLocale>
#include <QCollator>
#include <QRegularExpression>
#include <QToolTip>
#include <QHelpEvent>
#include <QDebug>

namespace FluxGUI::UI::Components {

RichFileDisplay::RichFileDisplay(QWidget* parent)
    : QWidget(parent)
    , m_model(new QStandardItemModel(this))
    , m_proxyModel(new QSortFilterProxyModel(this))
    , m_thumbnailTimer(new QTimer(this))
{
    initializeUI();
    setupModel();
    
    // Setup thumbnail generation timer
    m_thumbnailTimer->setSingleShot(false);
    m_thumbnailTimer->setInterval(THUMBNAIL_GENERATION_INTERVAL);
    connect(m_thumbnailTimer, &QTimer::timeout, this, &RichFileDisplay::startThumbnailGeneration);
    
    // Initialize animation system
    m_opacityEffect = std::make_unique<QGraphicsOpacityEffect>();
    m_viewTransitionAnimation = std::make_unique<QPropertyAnimation>(m_opacityEffect.get(), "opacity");
    m_viewTransitionAnimation->setDuration(ANIMATION_DURATION);
    m_viewTransitionAnimation->setEasingCurve(QEasingCurve::OutCubic);
    
    setAccessibilityEnabled(true);
}

RichFileDisplay::~RichFileDisplay() = default;

void RichFileDisplay::initializeUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    createToolbar();
    createViews();
    createPreviewPanel();
    createMetadataPanel();
    setupLayouts();
    
    // Set default view mode
    setViewMode(ViewMode::Details);
}

void RichFileDisplay::createViews() {
    // Create main splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    
    // Create list view
    m_listView = new QListView(this);
    m_listView->setViewMode(QListView::ListMode);
    m_listView->setResizeMode(QListView::Adjust);
    m_listView->setUniformItemSizes(true);
    m_listView->setAlternatingRowColors(true);
    
    // Create tree view
    m_treeView = new QTreeView(this);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setSortingEnabled(true);
    m_treeView->setAllColumnsShowFocus(true);
    
    // Create details view (table)
    m_detailsView = new QTableView(this);
    m_detailsView->setAlternatingRowColors(true);
    m_detailsView->setSortingEnabled(true);
    m_detailsView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_detailsView->horizontalHeader()->setStretchLastSection(true);
    m_detailsView->verticalHeader()->setVisible(false);
    
    // Create grid view (custom widget)
    m_gridView = new QWidget(this);
    // TODO: Implement custom grid view widget
    
    // Set current view to details
    m_currentView = m_detailsView;
    m_mainSplitter->addWidget(m_currentView);
    
    // Connect view signals
    connect(m_listView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &RichFileDisplay::onSelectionChanged);
    connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &RichFileDisplay::onSelectionChanged);
    connect(m_detailsView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &RichFileDisplay::onSelectionChanged);
    
    connect(m_listView, &QListView::doubleClicked, this, &RichFileDisplay::onItemDoubleClicked);
    connect(m_treeView, &QTreeView::doubleClicked, this, &RichFileDisplay::onItemDoubleClicked);
    connect(m_detailsView, &QTableView::doubleClicked, this, &RichFileDisplay::onItemDoubleClicked);
    
    connect(m_listView, &QListView::activated, this, &RichFileDisplay::onItemActivated);
    connect(m_treeView, &QTreeView::activated, this, &RichFileDisplay::onItemActivated);
    connect(m_detailsView, &QTableView::activated, this, &RichFileDisplay::onItemActivated);
}

void RichFileDisplay::createToolbar() {
    m_toolbar = new QWidget(this);
    m_toolbarLayout = new QHBoxLayout(m_toolbar);
    m_toolbarLayout->setContentsMargins(8, 4, 8, 4);
    
    // View mode button
    m_viewModeButton = new QToolButton(this);
    m_viewModeButton->setText("Details");
    m_viewModeButton->setToolTip("Change view mode");
    m_viewModeButton->setPopupMode(QToolButton::InstantPopup);
    
    QMenu* viewModeMenu = new QMenu(m_viewModeButton);
    viewModeMenu->addAction("List", [this]() { setViewMode(ViewMode::List); });
    viewModeMenu->addAction("Tree", [this]() { setViewMode(ViewMode::Tree); });
    viewModeMenu->addAction("Grid", [this]() { setViewMode(ViewMode::Grid); });
    viewModeMenu->addAction("Details", [this]() { setViewMode(ViewMode::Details); });
    viewModeMenu->addAction("Timeline", [this]() { setViewMode(ViewMode::Timeline); });
    viewModeMenu->addAction("Size", [this]() { setViewMode(ViewMode::Size); });
    m_viewModeButton->setMenu(viewModeMenu);
    
    // Sort button
    m_sortButton = new QToolButton(this);
    m_sortButton->setText("Sort");
    m_sortButton->setToolTip("Sort files");
    m_sortButton->setPopupMode(QToolButton::InstantPopup);
    
    QMenu* sortMenu = new QMenu(m_sortButton);
    sortMenu->addAction("Name", [this]() { setSorting(SortCriteria::Name); });
    sortMenu->addAction("Size", [this]() { setSorting(SortCriteria::Size); });
    sortMenu->addAction("Type", [this]() { setSorting(SortCriteria::Type); });
    sortMenu->addAction("Modified", [this]() { setSorting(SortCriteria::Modified); });
    sortMenu->addAction("Compressed", [this]() { setSorting(SortCriteria::Compressed); });
    sortMenu->addAction("Path", [this]() { setSorting(SortCriteria::Path); });
    m_sortButton->setMenu(sortMenu);
    
    // Group button
    m_groupButton = new QToolButton(this);
    m_groupButton->setText("Group");
    m_groupButton->setToolTip("Group files");
    m_groupButton->setPopupMode(QToolButton::InstantPopup);
    
    QMenu* groupMenu = new QMenu(m_groupButton);
    groupMenu->addAction("None", [this]() { setGrouping(GroupingMode::None); });
    groupMenu->addAction("Type", [this]() { setGrouping(GroupingMode::Type); });
    groupMenu->addAction("Size", [this]() { setGrouping(GroupingMode::Size); });
    groupMenu->addAction("Date", [this]() { setGrouping(GroupingMode::Date); });
    groupMenu->addAction("Directory", [this]() { setGrouping(GroupingMode::Directory); });
    m_groupButton->setMenu(groupMenu);
    
    // Filter button
    m_filterButton = new QToolButton(this);
    m_filterButton->setText("Filter");
    m_filterButton->setToolTip("Filter files");
    m_filterButton->setCheckable(true);
    
    // Status label
    m_statusLabel = new QLabel(this);
    m_statusLabel->setText("0 items");
    
    // Add to toolbar layout
    m_toolbarLayout->addWidget(m_viewModeButton);
    m_toolbarLayout->addWidget(m_sortButton);
    m_toolbarLayout->addWidget(m_groupButton);
    m_toolbarLayout->addWidget(m_filterButton);
    m_toolbarLayout->addStretch();
    m_toolbarLayout->addWidget(m_statusLabel);
}

void RichFileDisplay::createPreviewPanel() {
    m_previewPanel = new QWidget(this);
    m_previewPanel->setMinimumWidth(200);
    m_previewPanel->setMaximumWidth(400);
    
    QVBoxLayout* previewLayout = new QVBoxLayout(m_previewPanel);
    previewLayout->setContentsMargins(8, 8, 8, 8);
    
    QLabel* previewTitle = new QLabel("Preview", m_previewPanel);
    previewTitle->setStyleSheet("font-weight: bold; font-size: 12px;");
    previewLayout->addWidget(previewTitle);
    
    m_previewArea = new QScrollArea(m_previewPanel);
    m_previewArea->setWidgetResizable(true);
    m_previewArea->setAlignment(Qt::AlignCenter);
    
    m_previewLabel = new QLabel(m_previewArea);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setText("No preview available");
    m_previewArea->setWidget(m_previewLabel);
    
    previewLayout->addWidget(m_previewArea);
    
    // Initially hidden
    m_previewPanel->setVisible(m_settings.showPreview);
}

void RichFileDisplay::createMetadataPanel() {
    m_metadataPanel = new QWidget(this);
    m_metadataPanel->setMinimumWidth(200);
    m_metadataPanel->setMaximumWidth(400);
    
    m_metadataLayout = new QVBoxLayout(m_metadataPanel);
    m_metadataLayout->setContentsMargins(8, 8, 8, 8);
    
    m_metadataTitle = new QLabel("Properties", m_metadataPanel);
    m_metadataTitle->setStyleSheet("font-weight: bold; font-size: 12px;");
    m_metadataLayout->addWidget(m_metadataTitle);
    
    m_metadataLayout->addStretch();
    
    // Initially hidden
    m_metadataPanel->setVisible(m_settings.showMetadata);
}

void RichFileDisplay::setupLayouts() {
    // Add toolbar to main layout
    m_mainLayout->addWidget(m_toolbar);
    
    // Create right splitter for preview and metadata
    m_rightSplitter = new QSplitter(Qt::Vertical, this);
    m_rightSplitter->addWidget(m_previewPanel);
    m_rightSplitter->addWidget(m_metadataPanel);
    m_rightSplitter->setSizes({200, 200});
    
    // Add right panel to main splitter
    m_mainSplitter->addWidget(m_rightSplitter);
    m_mainSplitter->setSizes({600, 400});
    
    // Add main splitter to layout
    m_mainLayout->addWidget(m_mainSplitter);
}

void RichFileDisplay::setupModel() {
    // Setup proxy model
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setDynamicSortFilter(true);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    
    // Set model headers
    m_model->setHorizontalHeaderLabels({
        "Name", "Size", "Compressed", "Ratio", "Modified", "Type", "Path"
    });
    
    // Connect model signals
    connect(m_model, &QStandardItemModel::dataChanged,
            this, &RichFileDisplay::updateAccessibilityInfo);
    connect(m_proxyModel, &QSortFilterProxyModel::layoutChanged,
            this, &RichFileDisplay::updateAccessibilityInfo);
}

void RichFileDisplay::setFiles(const QList<FileItem>& files) {
    m_files = files;
    populateModel();
    updateItemEstimates();
    
    if (m_settings.showThumbnails) {
        generateThumbnails(true);
    }
    
    updateAccessibilityInfo();
}

void RichFileDisplay::addFile(const FileItem& file) {
    m_files.append(file);
    updateModelItem(file);
    updateItemEstimates();
    
    if (m_settings.showThumbnails) {
        m_thumbnailQueue.enqueue(file);
        if (!m_thumbnailGenerationActive) {
            m_thumbnailTimer->start();
        }
    }
    
    updateAccessibilityInfo();
}

void RichFileDisplay::removeFile(const QString& path) {
    auto it = std::find_if(m_files.begin(), m_files.end(),
                          [&path](const FileItem& item) { return item.path == path; });
    
    if (it != m_files.end()) {
        m_files.erase(it);
        populateModel();
        updateItemEstimates();
        updateAccessibilityInfo();
    }
}

void RichFileDisplay::updateFile(const FileItem& file) {
    auto it = std::find_if(m_files.begin(), m_files.end(),
                          [&file](const FileItem& item) { return item.path == file.path; });
    
    if (it != m_files.end()) {
        *it = file;
        updateModelItem(file);
        updateAccessibilityInfo();
    }
}

void RichFileDisplay::clear() {
    m_files.clear();
    m_model->clear();
    m_thumbnailQueue.clear();
    m_thumbnailTimer->stop();
    m_thumbnailGenerationActive = false;
    m_thumbnailsGenerated = 0;
    
    // Reset model headers
    m_model->setHorizontalHeaderLabels({
        "Name", "Size", "Compressed", "Ratio", "Modified", "Type", "Path"
    });
    
    updateAccessibilityInfo();
}

void RichFileDisplay::setViewMode(ViewMode mode) {
    if (m_settings.viewMode == mode) {
        return;
    }
    
    ViewMode oldMode = m_settings.viewMode;
    m_settings.viewMode = mode;
    
    if (m_settings.animateChanges) {
        animateViewTransition(oldMode, mode);
    } else {
        switchView(mode);
    }
    
    emit viewModeChanged(mode);
}

void RichFileDisplay::switchView(ViewMode mode) {
    // Remove current view from splitter
    if (m_currentView) {
        m_mainSplitter->widget(0)->setParent(nullptr);
    }
    
    // Set new current view
    switch (mode) {
    case ViewMode::List:
        m_currentView = m_listView;
        m_viewModeButton->setText("List");
        break;
    case ViewMode::Tree:
        m_currentView = m_treeView;
        m_viewModeButton->setText("Tree");
        break;
    case ViewMode::Grid:
        m_currentView = m_gridView;
        m_viewModeButton->setText("Grid");
        break;
    case ViewMode::Details:
        m_currentView = m_detailsView;
        m_viewModeButton->setText("Details");
        break;
    case ViewMode::Timeline:
        // TODO: Implement timeline view
        m_currentView = m_detailsView;
        m_viewModeButton->setText("Timeline");
        break;
    case ViewMode::Size:
        // TODO: Implement size view
        m_currentView = m_detailsView;
        m_viewModeButton->setText("Size");
        break;
    }
    
    // Add new view to splitter
    m_mainSplitter->insertWidget(0, m_currentView);
    
    // Set model for view
    if (auto* itemView = qobject_cast<QAbstractItemView*>(m_currentView)) {
        itemView->setModel(m_proxyModel);
        
        // Setup custom delegate if needed
        if (m_settings.showThumbnails || m_settings.showMetadata || m_settings.showCompressionInfo) {
            auto* delegate = new RichFileItemDelegate(this);
            delegate->setShowThumbnails(m_settings.showThumbnails);
            delegate->setShowMetadata(m_settings.showMetadata);
            delegate->setShowCompressionInfo(m_settings.showCompressionInfo);
            delegate->setIconSize(m_settings.iconSize);
            itemView->setItemDelegate(delegate);
        }
    }
    
    updateViewSettings();
    syncViewSelection();
}

void RichFileDisplay::populateModel() {
    m_model->clear();
    m_model->setHorizontalHeaderLabels({
        "Name", "Size", "Compressed", "Ratio", "Modified", "Type", "Path"
    });
    
    for (const auto& file : m_files) {
        updateModelItem(file);
    }
    
    applyFilters();
    applySorting();
    applyGrouping();
    
    // Update status
    m_statusLabel->setText(QString("%1 items").arg(m_files.size()));
}

void RichFileDisplay::updateModelItem(const FileItem& file) {
    // Find existing item or create new
    QStandardItem* nameItem = nullptr;
    
    for (int row = 0; row < m_model->rowCount(); ++row) {
        auto* item = m_model->item(row, 0);
        if (item && item->data(Qt::UserRole).toString() == file.path) {
            nameItem = item;
            break;
        }
    }
    
    if (!nameItem) {
        nameItem = new QStandardItem();
        m_model->appendRow({
            nameItem,
            new QStandardItem(),
            new QStandardItem(),
            new QStandardItem(),
            new QStandardItem(),
            new QStandardItem(),
            new QStandardItem()
        });
    }
    
    int row = nameItem->row();
    
    // Update item data
    nameItem->setText(file.name);
    nameItem->setIcon(file.icon.isNull() ? getFileTypeIcon(file) : file.icon);
    nameItem->setData(file.path, Qt::UserRole);
    nameItem->setData(QVariant::fromValue(file), Qt::UserRole + 1);
    
    m_model->item(row, 1)->setText(formatFileSize(file.size));
    m_model->item(row, 1)->setData(file.size, Qt::UserRole);
    
    m_model->item(row, 2)->setText(formatFileSize(file.compressedSize));
    m_model->item(row, 2)->setData(file.compressedSize, Qt::UserRole);
    
    m_model->item(row, 3)->setText(formatCompressionRatio(file.compressionRatio));
    m_model->item(row, 3)->setData(file.compressionRatio, Qt::UserRole);
    
    m_model->item(row, 4)->setText(file.modified.toString("yyyy-MM-dd hh:mm"));
    m_model->item(row, 4)->setData(file.modified, Qt::UserRole);
    
    m_model->item(row, 5)->setText(file.typeDescription);
    m_model->item(row, 5)->setData(file.mimeType.name(), Qt::UserRole);
    
    m_model->item(row, 6)->setText(file.relativePath);
    m_model->item(row, 6)->setData(file.relativePath, Qt::UserRole);
}

void RichFileDisplay::updateItemEstimates() {
    // Update total size and count
    qint64 totalSize = 0;
    qint64 totalCompressed = 0;
    
    for (const auto& file : m_files) {
        totalSize += file.size;
        totalCompressed += file.compressedSize;
    }
    
    double avgRatio = totalSize > 0 ? (double)totalCompressed / totalSize : 0.0;
    
    QString statusText = QString("%1 items, %2")
                        .arg(m_files.size())
                        .arg(formatFileSize(totalSize));
    
    if (totalCompressed > 0) {
        statusText += QString(" (%1 compressed, %2% ratio)")
                     .arg(formatFileSize(totalCompressed))
                     .arg(QString::number(avgRatio * 100, 'f', 1));
    }
    
    m_statusLabel->setText(statusText);
}

QIcon RichFileDisplay::getFileTypeIcon(const FileItem& file) {
    static QFileIconProvider iconProvider;
    
    if (file.isDirectory) {
        return iconProvider.icon(QFileIconProvider::Folder);
    }
    
    // Use mime type to get appropriate icon
    QString iconName;
    QString mimeType = file.mimeType.name();
    
    if (mimeType.startsWith("image/")) {
        iconName = "image-x-generic";
    } else if (mimeType.startsWith("audio/")) {
        iconName = "audio-x-generic";
    } else if (mimeType.startsWith("video/")) {
        iconName = "video-x-generic";
    } else if (mimeType.startsWith("text/")) {
        iconName = "text-x-generic";
    } else if (mimeType == "application/pdf") {
        iconName = "application-pdf";
    } else if (mimeType.contains("archive") || mimeType.contains("zip") || mimeType.contains("tar")) {
        iconName = "package-x-generic";
    } else {
        iconName = "text-x-generic";
    }
    
    QIcon icon = QIcon::fromTheme(iconName);
    if (icon.isNull()) {
        icon = iconProvider.icon(QFileIconProvider::File);
    }
    
    return icon;
}

QString RichFileDisplay::formatFileSize(qint64 size) const {
    if (size < 0) return "Unknown";
    if (size == 0) return "0 bytes";
    
    const QStringList units = {"bytes", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double sizeDouble = size;
    
    while (sizeDouble >= 1024.0 && unitIndex < units.size() - 1) {
        sizeDouble /= 1024.0;
        ++unitIndex;
    }
    
    return QString("%1 %2").arg(QString::number(sizeDouble, 'f', unitIndex > 0 ? 1 : 0), units[unitIndex]);
}

QString RichFileDisplay::formatCompressionRatio(double ratio) const {
    if (ratio <= 0.0) return "N/A";
    return QString("%1%").arg(QString::number(ratio * 100, 'f', 1));
}

void RichFileDisplay::applyFilters() {
    // TODO: Implement filtering logic
    m_proxyModel->setFilterRegularExpression(QRegularExpression(m_textFilter, QRegularExpression::CaseInsensitiveOption));
}

void RichFileDisplay::applySorting() {
    int column = 0;
    switch (m_settings.sortBy) {
    case SortCriteria::Name: column = 0; break;
    case SortCriteria::Size: column = 1; break;
    case SortCriteria::Type: column = 5; break;
    case SortCriteria::Modified: column = 4; break;
    case SortCriteria::Compressed: column = 2; break;
    case SortCriteria::Path: column = 6; break;
    }
    
    m_proxyModel->sort(column, m_settings.sortOrder);
}

void RichFileDisplay::applyGrouping() {
    // TODO: Implement grouping logic
}

void RichFileDisplay::animateViewTransition(ViewMode fromMode, ViewMode toMode) {
    if (!m_currentView) {
        switchView(toMode);
        return;
    }
    
    // Apply opacity effect to current view
    m_currentView->setGraphicsEffect(m_opacityEffect.get());
    
    // Animate fade out, switch view, then fade in
    m_viewTransitionAnimation->setStartValue(1.0);
    m_viewTransitionAnimation->setEndValue(0.0);
    
    connect(m_viewTransitionAnimation.get(), &QPropertyAnimation::finished, this, [this, toMode]() {
        switchView(toMode);
        
        // Fade in new view
        m_viewTransitionAnimation->setStartValue(0.0);
        m_viewTransitionAnimation->setEndValue(1.0);
        m_viewTransitionAnimation->start();
        
        // Disconnect this lambda
        disconnect(m_viewTransitionAnimation.get(), &QPropertyAnimation::finished, this, nullptr);
    });
    
    m_viewTransitionAnimation->start();
}

void RichFileDisplay::generateThumbnails(bool async) {
    if (m_files.isEmpty()) return;
    
    m_thumbnailQueue.clear();
    for (const auto& file : m_files) {
        if (!file.isDirectory && file.mimeType.name().startsWith("image/")) {
            m_thumbnailQueue.enqueue(file);
        }
    }
    
    if (!m_thumbnailQueue.isEmpty()) {
        m_thumbnailGenerationActive = true;
        m_thumbnailsGenerated = 0;
        emit thumbnailGenerationStarted(m_thumbnailQueue.size());
        
        if (async) {
            m_thumbnailTimer->start();
        } else {
            while (!m_thumbnailQueue.isEmpty()) {
                startThumbnailGeneration();
            }
        }
    }
}

void RichFileDisplay::startThumbnailGeneration() {
    if (m_thumbnailQueue.isEmpty()) {
        m_thumbnailTimer->stop();
        m_thumbnailGenerationActive = false;
        emit thumbnailGenerationFinished();
        return;
    }
    
    FileItem file = m_thumbnailQueue.dequeue();
    generateThumbnail(file);
    
    ++m_thumbnailsGenerated;
    emit thumbnailGenerationProgress(m_thumbnailsGenerated, m_thumbnailsGenerated + m_thumbnailQueue.size());
}

void RichFileDisplay::generateThumbnail(const FileItem& file) {
    QPixmap thumbnail = createThumbnail(file, QSize(m_settings.thumbnailSize, m_settings.thumbnailSize));
    
    if (!thumbnail.isNull()) {
        // Update file item with thumbnail
        auto it = std::find_if(m_files.begin(), m_files.end(),
                              [&file](FileItem& item) { return item.path == file.path; });
        
        if (it != m_files.end()) {
            it->thumbnail = thumbnail;
            updateModelItem(*it);
        }
    }
}

QPixmap RichFileDisplay::createThumbnail(const FileItem& file, const QSize& size) {
    if (!file.mimeType.name().startsWith("image/")) {
        return QPixmap();
    }
    
    QImageReader reader(file.path);
    if (!reader.canRead()) {
        return QPixmap();
    }
    
    // Scale image to fit thumbnail size
    QSize imageSize = reader.size();
    if (imageSize.isValid()) {
        imageSize.scale(size, Qt::KeepAspectRatio);
        reader.setScaledSize(imageSize);
    }
    
    QImage image = reader.read();
    if (image.isNull()) {
        return QPixmap();
    }
    
    return QPixmap::fromImage(image);
}

void RichFileDisplay::updateAccessibilityInfo() {
    if (!m_accessibilityEnabled) return;
    
    // Update accessibility properties for screen readers
    setAccessibleName("File Display");
    setAccessibleDescription(QString("Displaying %1 files in %2 mode")
                           .arg(m_files.size())
                           .arg(m_viewModeButton->text()));
}

QString RichFileDisplay::getItemAccessibilityText(const FileItem& file) const {
    QString text = QString("%1, %2").arg(file.name, formatFileSize(file.size));
    
    if (file.isDirectory) {
        text += ", folder";
    } else {
        text += QString(", %1 file").arg(file.typeDescription);
        if (file.compressionRatio > 0) {
            text += QString(", compressed to %1").arg(formatCompressionRatio(file.compressionRatio));
        }
    }
    
    text += QString(", modified %1").arg(file.modified.toString("MMMM d, yyyy"));
    
    return text;
}

// Slot implementations
void RichFileDisplay::onSelectionChanged() {
    QStringList selectedFiles;
    
    if (auto* itemView = qobject_cast<QAbstractItemView*>(m_currentView)) {
        QModelIndexList selection = itemView->selectionModel()->selectedRows();
        for (const auto& index : selection) {
            QString path = m_proxyModel->data(index, Qt::UserRole).toString();
            selectedFiles.append(path);
        }
    }
    
    emit selectionChanged(selectedFiles);
    
    // Update preview and metadata for single selection
    if (selectedFiles.size() == 1) {
        showPreview(selectedFiles.first());
        showMetadata(selectedFiles.first());
    } else {
        hidePreview();
        hideMetadata();
    }
}

void RichFileDisplay::onItemDoubleClicked(const QModelIndex& index) {
    QString filePath = m_proxyModel->data(index, Qt::UserRole).toString();
    emit fileDoubleClicked(filePath);
}

void RichFileDisplay::onItemActivated(const QModelIndex& index) {
    QString filePath = m_proxyModel->data(index, Qt::UserRole).toString();
    emit fileActivated(filePath);
}

void RichFileDisplay::showPreview(const QString& filePath) {
    auto it = std::find_if(m_files.begin(), m_files.end(),
                          [&filePath](const FileItem& item) { return item.path == filePath; });
    
    if (it != m_files.end()) {
        updatePreview(filePath);
        m_previewPanel->setVisible(true);
    }
}

void RichFileDisplay::hidePreview() {
    m_previewPanel->setVisible(false);
}

void RichFileDisplay::showMetadata(const QString& filePath) {
    auto it = std::find_if(m_files.begin(), m_files.end(),
                          [&filePath](const FileItem& item) { return item.path == filePath; });
    
    if (it != m_files.end()) {
        updateMetadataDisplay(*it);
        m_metadataPanel->setVisible(true);
    }
}

void RichFileDisplay::hideMetadata() {
    m_metadataPanel->setVisible(false);
}

void RichFileDisplay::updatePreview(const QString& filePath) {
    auto it = std::find_if(m_files.begin(), m_files.end(),
                          [&filePath](const FileItem& item) { return item.path == filePath; });
    
    if (it == m_files.end()) {
        m_previewLabel->setText("File not found");
        return;
    }
    
    const FileItem& file = *it;
    
    if (!file.thumbnail.isNull()) {
        m_previewLabel->setPixmap(file.thumbnail.scaled(m_previewArea->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else if (file.mimeType.name().startsWith("image/")) {
        // Try to load image directly
        QPixmap pixmap(file.path);
        if (!pixmap.isNull()) {
            m_previewLabel->setPixmap(pixmap.scaled(m_previewArea->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            m_previewLabel->setText("Cannot load image");
        }
    } else if (file.mimeType.name().startsWith("text/")) {
        // Show text preview
        QFile textFile(file.path);
        if (textFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = QString::fromUtf8(textFile.readAll().left(1024)); // First 1KB
            m_previewLabel->setText(content);
        } else {
            m_previewLabel->setText("Cannot read text file");
        }
    } else {
        m_previewLabel->setText("No preview available");
    }
}

void RichFileDisplay::updateMetadataDisplay(const FileItem& file) {
    // Clear existing metadata widgets
    while (m_metadataLayout->count() > 1) {
        QLayoutItem* item = m_metadataLayout->takeAt(1);
        delete item->widget();
        delete item;
    }
    
    // Add metadata labels
    auto addMetadataRow = [this](const QString& label, const QString& value) {
        QLabel* labelWidget = new QLabel(label + ":", this);
        labelWidget->setStyleSheet("font-weight: bold; color: #666;");
        QLabel* valueWidget = new QLabel(value, this);
        valueWidget->setWordWrap(true);
        
        m_metadataLayout->addWidget(labelWidget);
        m_metadataLayout->addWidget(valueWidget);
    };
    
    addMetadataRow("Name", file.name);
    addMetadataRow("Size", formatFileSize(file.size));
    
    if (file.compressedSize > 0) {
        addMetadataRow("Compressed", formatFileSize(file.compressedSize));
        addMetadataRow("Ratio", formatCompressionRatio(file.compressionRatio));
    }
    
    addMetadataRow("Type", file.typeDescription);
    addMetadataRow("Modified", file.modified.toString("yyyy-MM-dd hh:mm:ss"));
    addMetadataRow("Path", file.relativePath);
    
    if (!file.tags.isEmpty()) {
        addMetadataRow("Tags", file.tags.join(", "));
    }
    
    // Add custom metadata
    for (auto it = file.metadata.begin(); it != file.metadata.end(); ++it) {
        addMetadataRow(it.key(), it.value().toString());
    }
    
    m_metadataLayout->addStretch();
}

// Public API implementations
void RichFileDisplay::setDisplaySettings(const DisplaySettings& settings) {
    m_settings = settings;
    updateViewSettings();
    
    if (m_settings.viewMode != viewMode()) {
        setViewMode(m_settings.viewMode);
    }
    
    if (m_settings.showThumbnails && !m_thumbnailGenerationActive) {
        generateThumbnails(true);
    }
}

void RichFileDisplay::setSorting(SortCriteria criteria, Qt::SortOrder order) {
    m_settings.sortBy = criteria;
    m_settings.sortOrder = order;
    applySorting();
    emit sortingChanged(criteria, order);
}

void RichFileDisplay::setGrouping(GroupingMode mode) {
    m_settings.grouping = mode;
    applyGrouping();
    emit groupingChanged(mode);
}

void RichFileDisplay::setFilter(const QString& filter) {
    m_textFilter = filter;
    applyFilters();
}

QStringList RichFileDisplay::selectedFiles() const {
    QStringList selected;
    
    if (auto* itemView = qobject_cast<QAbstractItemView*>(m_currentView)) {
        QModelIndexList selection = itemView->selectionModel()->selectedRows();
        for (const auto& index : selection) {
            selected.append(m_proxyModel->data(index, Qt::UserRole).toString());
        }
    }
    
    return selected;
}

int RichFileDisplay::selectionCount() const {
    if (auto* itemView = qobject_cast<QAbstractItemView*>(m_currentView)) {
        return itemView->selectionModel()->selectedRows().size();
    }
    return 0;
}

void RichFileDisplay::updateViewSettings() {
    // Update view-specific settings
    if (auto* itemView = qobject_cast<QAbstractItemView*>(m_currentView)) {
        // Update delegate settings if custom delegate is used
        if (auto* delegate = qobject_cast<RichFileItemDelegate*>(itemView->itemDelegate())) {
            delegate->setShowThumbnails(m_settings.showThumbnails);
            delegate->setShowMetadata(m_settings.showMetadata);
            delegate->setShowCompressionInfo(m_settings.showCompressionInfo);
            delegate->setIconSize(m_settings.iconSize);
        }
    }
    
    // Update panel visibility
    m_previewPanel->setVisible(m_settings.showPreview);
    m_metadataPanel->setVisible(m_settings.showMetadata);
}

// RichFileItemDelegate implementation
RichFileItemDelegate::RichFileItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void RichFileItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (m_showThumbnails || m_showMetadata || m_showCompressionInfo) {
        paintFileItem(painter, option, index);
    } else {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

QSize RichFileItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    
    if (m_showThumbnails) {
        size.setHeight(qMax(size.height(), m_iconSize + 8));
    }
    
    if (m_showMetadata) {
        size.setHeight(size.height() + 20); // Extra space for metadata
    }
    
    return size;
}

void RichFileItemDelegate::paintFileItem(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    // Get file item data
    QVariant fileData = index.data(Qt::UserRole + 1);
    if (!fileData.canConvert<RichFileDisplay::FileItem>()) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }
    
    RichFileDisplay::FileItem file = fileData.value<RichFileDisplay::FileItem>();
    
    painter->save();
    
    // Draw selection background
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    }
    
    QRect rect = option.rect.adjusted(4, 2, -4, -2);
    
    // Draw thumbnail or icon
    QRect iconRect(rect.left(), rect.top(), m_iconSize, m_iconSize);
    if (m_showThumbnails && !file.thumbnail.isNull()) {
        paintThumbnail(painter, iconRect, file.thumbnail);
    } else {
        QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
        icon.paint(painter, iconRect);
    }
    
    // Draw text
    QRect textRect = rect.adjusted(m_iconSize + 8, 0, 0, 0);
    
    // File name
    QFont nameFont = option.font;
    nameFont.setBold(true);
    painter->setFont(nameFont);
    painter->setPen(option.state & QStyle::State_Selected ? option.palette.highlightedText().color() : option.palette.text().color());
    
    QRect nameRect = textRect;
    nameRect.setHeight(painter->fontMetrics().height());
    painter->drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, file.name);
    
    // Metadata
    if (m_showMetadata) {
        QFont metaFont = option.font;
        metaFont.setPointSize(metaFont.pointSize() - 1);
        painter->setFont(metaFont);
        painter->setPen(option.palette.mid().color());
        
        QRect metaRect = textRect.adjusted(0, nameRect.height() + 2, 0, 0);
        QString metaText = QString("%1 • %2").arg(file.typeDescription, QLocale().toString(file.modified, QLocale::ShortFormat));
        painter->drawText(metaRect, Qt::AlignLeft | Qt::AlignTop, metaText);
    }
    
    // Compression info
    if (m_showCompressionInfo && file.compressionRatio > 0) {
        paintCompressionInfo(painter, QRect(rect.right() - 60, rect.top(), 60, rect.height()), file.compressionRatio);
    }
    
    painter->restore();
}

void RichFileItemDelegate::paintThumbnail(QPainter* painter, const QRect& rect, const QPixmap& thumbnail) const {
    QPixmap scaled = thumbnail.scaled(rect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QRect targetRect = rect;
    targetRect.setSize(scaled.size());
    targetRect.moveCenter(rect.center());
    
    painter->drawPixmap(targetRect, scaled);
}

void RichFileItemDelegate::paintCompressionInfo(QPainter* painter, const QRect& rect, double ratio) const {
    // Draw compression ratio as a small progress bar
    QRect barRect = rect.adjusted(10, rect.height() / 2 - 3, -10, -rect.height() / 2 + 3);
    
    painter->setPen(QPen(QColor(200, 200, 200), 1));
    painter->drawRect(barRect);
    
    QRect fillRect = barRect.adjusted(1, 1, -1, -1);
    fillRect.setWidth(fillRect.width() * ratio);
    
    QColor fillColor = ratio < 0.5 ? QColor(76, 175, 80) : ratio < 0.8 ? QColor(255, 193, 7) : QColor(244, 67, 54);
    painter->fillRect(fillRect, fillColor);
    
    // Draw percentage text
    painter->setPen(QColor(100, 100, 100));
    QFont font = painter->font();
    font.setPointSize(font.pointSize() - 2);
    painter->setFont(font);
    painter->drawText(rect, Qt::AlignCenter, QString("%1%").arg(QString::number(ratio * 100, 'f', 0)));
}

void RichFileItemDelegate::setShowThumbnails(bool show) {
    m_showThumbnails = show;
}

void RichFileItemDelegate::setShowMetadata(bool show) {
    m_showMetadata = show;
}

void RichFileItemDelegate::setShowCompressionInfo(bool show) {
    m_showCompressionInfo = show;
}

void RichFileItemDelegate::setIconSize(int size) {
    m_iconSize = size;
}

} // namespace FluxGUI::UI::Components

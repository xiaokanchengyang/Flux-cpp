#include "virtualized_archive_view.h"

#include <QApplication>
#include <QStyle>
#include <QStyleOption>
#include <QPainter>
#include <QScrollBar>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QTimer>
#include <QElapsedTimer>
#include <QThread>
#include <QMutexLocker>
#include <QCoreApplication>
#include <QDebug>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <cmath>
#include <algorithm>

namespace FluxGUI::UI::Components {

VirtualizedArchiveView::VirtualizedArchiveView(QWidget* parent)
    : QAbstractItemView(parent)
    , m_itemCache(DEFAULT_CACHE_SIZE)
{
    // Setup scroll bars
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    // Setup selection
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    
    // Setup timers
    m_cacheCleanupTimer = new QTimer(this);
    m_cacheCleanupTimer->setInterval(CACHE_CLEANUP_INTERVAL);
    m_cacheCleanupTimer->setSingleShot(false);
    connect(m_cacheCleanupTimer, &QTimer::timeout, this, &VirtualizedArchiveView::onCacheCleanupTimer);
    m_cacheCleanupTimer->start();
    
    m_performanceTimer = new QTimer(this);
    m_performanceTimer->setInterval(PERFORMANCE_UPDATE_INTERVAL);
    m_performanceTimer->setSingleShot(false);
    connect(m_performanceTimer, &QTimer::timeout, this, &VirtualizedArchiveView::onPerformanceTimer);
    
    m_scrollTimer = new QTimer(this);
    m_scrollTimer->setInterval(SCROLL_TIMER_INTERVAL);
    m_scrollTimer->setSingleShot(true);
    
    // Connect scroll bar signals
    connect(verticalScrollBar(), &QScrollBar::valueChanged,
            this, &VirtualizedArchiveView::onVerticalScrollBarValueChanged);
    connect(horizontalScrollBar(), &QScrollBar::valueChanged,
            this, &VirtualizedArchiveView::onHorizontalScrollBarValueChanged);
    
    // Setup background loading
    m_backgroundThread = new QThread(this);
    m_backgroundWatcher = new QFutureWatcher<void>(this);
    connect(m_backgroundWatcher, &QFutureWatcher<void>::finished,
            this, &VirtualizedArchiveView::onBackgroundLoadingFinished);
    
    // Initialize viewport
    calculateViewport();
    updateLayout();
    
    // Enable mouse tracking for hover effects
    setMouseTracking(true);
    
    // Set focus policy
    setFocusPolicy(Qt::StrongFocus);
    
    // Setup default item delegate
    setItemDelegate(new VirtualizedItemDelegate(this));
}

VirtualizedArchiveView::~VirtualizedArchiveView() {
    stopBackgroundLoading();
    if (m_backgroundThread->isRunning()) {
        m_backgroundThread->quit();
        m_backgroundThread->wait();
    }
}

void VirtualizedArchiveView::setModel(QAbstractItemModel* model) {
    if (this->model()) {
        disconnect(this->model(), nullptr, this, nullptr);
    }
    
    QAbstractItemView::setModel(model);
    
    if (model) {
        connect(model, &QAbstractItemModel::dataChanged,
                this, &VirtualizedArchiveView::onModelDataChanged);
        connect(model, &QAbstractItemModel::rowsInserted,
                this, &VirtualizedArchiveView::onModelRowsInserted);
        connect(model, &QAbstractItemModel::rowsRemoved,
                this, &VirtualizedArchiveView::onModelRowsRemoved);
        connect(model, &QAbstractItemModel::modelReset,
                this, &VirtualizedArchiveView::onModelReset);
    }
    
    // Clear cache and recalculate layout
    clearCache();
    updateLayout();
    viewport()->update();
}

QRect VirtualizedArchiveView::visualRect(const QModelIndex& index) const {
    if (!index.isValid() || !model()) {
        return QRect();
    }
    
    return calculateItemRect(index.row());
}

QModelIndex VirtualizedArchiveView::indexAt(const QPoint& point) const {
    if (!model()) {
        return QModelIndex();
    }
    
    int index = indexFromPoint(point);
    if (index >= 0 && index < model()->rowCount()) {
        return model()->index(index, 0);
    }
    
    return QModelIndex();
}

void VirtualizedArchiveView::scrollTo(const QModelIndex& index, ScrollHint hint) {
    if (!index.isValid() || !model()) {
        return;
    }
    
    QRect itemRect = calculateItemRect(index.row());
    QRect viewportRect = viewport()->rect();
    
    int scrollValue = verticalScrollBar()->value();
    
    switch (hint) {
    case EnsureVisible:
        if (itemRect.top() < viewportRect.top()) {
            scrollValue = itemRect.top();
        } else if (itemRect.bottom() > viewportRect.bottom()) {
            scrollValue = itemRect.bottom() - viewportRect.height();
        }
        break;
    case PositionAtTop:
        scrollValue = itemRect.top();
        break;
    case PositionAtBottom:
        scrollValue = itemRect.bottom() - viewportRect.height();
        break;
    case PositionAtCenter:
        scrollValue = itemRect.center().y() - viewportRect.height() / 2;
        break;
    }
    
    verticalScrollBar()->setValue(scrollValue);
}

QModelIndex VirtualizedArchiveView::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) {
    if (!model()) {
        return QModelIndex();
    }
    
    QModelIndex current = currentIndex();
    int row = current.isValid() ? current.row() : 0;
    
    switch (cursorAction) {
    case MoveUp:
        row = qMax(0, row - 1);
        break;
    case MoveDown:
        row = qMin(model()->rowCount() - 1, row + 1);
        break;
    case MoveLeft:
        if (m_viewMode == ViewMode::Grid) {
            row = qMax(0, row - 1);
        }
        break;
    case MoveRight:
        if (m_viewMode == ViewMode::Grid) {
            row = qMin(model()->rowCount() - 1, row + 1);
        }
        break;
    case MoveHome:
        row = 0;
        break;
    case MoveEnd:
        row = model()->rowCount() - 1;
        break;
    case MovePageUp:
        row = qMax(0, row - m_viewportInfo.totalVisibleItems);
        break;
    case MovePageDown:
        row = qMin(model()->rowCount() - 1, row + m_viewportInfo.totalVisibleItems);
        break;
    default:
        return current;
    }
    
    return model()->index(row, 0);
}

void VirtualizedArchiveView::setViewMode(ViewMode mode) {
    if (m_viewMode == mode) {
        return;
    }
    
    m_viewMode = mode;
    updateLayout();
    viewport()->update();
}

void VirtualizedArchiveView::setScrollMode(ScrollMode mode) {
    m_scrollMode = mode;
    updateScrollBars();
}

void VirtualizedArchiveView::setItemSize(const QSize& size) {
    m_itemSize = size;
    updateLayout();
    viewport()->update();
}

void VirtualizedArchiveView::setItemSpacing(int spacing) {
    m_itemSpacing = spacing;
    updateLayout();
    viewport()->update();
}

void VirtualizedArchiveView::setBufferSize(int items) {
    m_bufferSize = qMax(1, items);
    calculateViewport();
}

void VirtualizedArchiveView::setCacheSize(int maxItems) {
    m_itemCache.setMaxCost(maxItems);
}

int VirtualizedArchiveView::cacheSize() const {
    return m_itemCache.maxCost();
}

void VirtualizedArchiveView::setLazyLoadingEnabled(bool enabled) {
    m_lazyLoadingEnabled = enabled;
    if (!enabled) {
        // Preload all visible items
        preloadRange(m_viewportInfo.firstVisibleIndex, m_viewportInfo.lastVisibleIndex);
    }
}

void VirtualizedArchiveView::setBackgroundLoadingEnabled(bool enabled) {
    m_backgroundLoadingEnabled = enabled;
    if (enabled && !m_backgroundLoadingActive) {
        startBackgroundLoading();
    } else if (!enabled && m_backgroundLoadingActive) {
        stopBackgroundLoading();
    }
}

void VirtualizedArchiveView::setPredictiveLoadingEnabled(bool enabled) {
    m_predictiveLoadingEnabled = enabled;
}

void VirtualizedArchiveView::setPerformanceMonitoringEnabled(bool enabled) {
    m_performanceMonitoringEnabled = enabled;
    if (enabled) {
        m_performanceTimer->start();
        startPerformanceTimer();
    } else {
        m_performanceTimer->stop();
    }
}

VirtualizedArchiveView::ViewportInfo VirtualizedArchiveView::viewportInfo() const {
    return m_viewportInfo;
}

void VirtualizedArchiveView::clearCache() {
    QMutexLocker locker(&m_cacheMutex);
    m_itemCache.clear();
    updateCacheStatistics();
}

void VirtualizedArchiveView::preloadRange(int startIndex, int endIndex) {
    if (!model() || !m_lazyLoadingEnabled) {
        return;
    }
    
    for (int i = startIndex; i <= endIndex && i < model()->rowCount(); ++i) {
        if (!isItemLoaded(i)) {
            loadItemData(i);
        }
    }
}

void VirtualizedArchiveView::invalidateRange(int startIndex, int endIndex) {
    QMutexLocker locker(&m_cacheMutex);
    
    for (int i = startIndex; i <= endIndex; ++i) {
        m_itemCache.remove(i);
    }
    
    updateCacheStatistics();
}

void VirtualizedArchiveView::resetPerformanceMetrics() {
    m_performanceMetrics = PerformanceMetrics();
    m_lastFrameTime = 0;
    m_frameCount = 0;
}

// Protected event handlers
void VirtualizedArchiveView::paintEvent(QPaintEvent* event) {
    if (!model()) {
        return;
    }
    
    QElapsedTimer timer;
    if (m_performanceMonitoringEnabled) {
        timer.start();
    }
    
    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    // Render background
    renderBackground(&painter, event->rect());
    
    // Render visible items
    renderVisibleItems(&painter, event->rect());
    
    // Render selection
    renderSelection(&painter);
    
    if (m_performanceMonitoringEnabled) {
        m_performanceMetrics.renderTime = timer.elapsed();
        m_performanceMetrics.itemsRendered = m_viewportInfo.totalVisibleItems;
        ++m_frameCount;
    }
}

void VirtualizedArchiveView::resizeEvent(QResizeEvent* event) {
    QAbstractItemView::resizeEvent(event);
    updateLayout();
    calculateViewport();
    
    // Trigger background loading for new visible items
    if (m_backgroundLoadingEnabled) {
        scheduleBackgroundLoad(m_viewportInfo.firstVisibleIndex, m_viewportInfo.lastVisibleIndex);
    }
}

void VirtualizedArchiveView::scrollContentsBy(int dx, int dy) {
    QAbstractItemView::scrollContentsBy(dx, dy);
    
    if (m_performanceMonitoringEnabled) {
        QElapsedTimer timer;
        timer.start();
        calculateViewport();
        m_performanceMetrics.scrollTime = timer.elapsed();
    } else {
        calculateViewport();
    }
    
    // Trigger predictive loading
    if (m_predictiveLoadingEnabled && m_backgroundLoadingEnabled) {
        int predictiveStart = qMax(0, m_viewportInfo.firstVisibleIndex - PREDICTIVE_LOAD_THRESHOLD);
        int predictiveEnd = qMin(model() ? model()->rowCount() - 1 : 0, 
                                m_viewportInfo.lastVisibleIndex + PREDICTIVE_LOAD_THRESHOLD);
        scheduleBackgroundLoad(predictiveStart, predictiveEnd);
    }
    
    emit viewportChanged(m_viewportInfo);
}

void VirtualizedArchiveView::wheelEvent(QWheelEvent* event) {
    if (m_scrollMode == ScrollMode::Pixel) {
        // Smooth pixel scrolling
        int delta = event->angleDelta().y();
        int scrollAmount = delta / 8; // Convert from 1/8 degrees to pixels
        
        verticalScrollBar()->setValue(verticalScrollBar()->value() - scrollAmount);
        event->accept();
    } else {
        QAbstractItemView::wheelEvent(event);
    }
}

void VirtualizedArchiveView::keyPressEvent(QKeyEvent* event) {
    // Handle keyboard navigation
    switch (event->key()) {
    case Qt::Key_Home:
        if (event->modifiers() & Qt::ControlModifier) {
            scrollTo(model()->index(0, 0), PositionAtTop);
            setCurrentIndex(model()->index(0, 0));
            event->accept();
            return;
        }
        break;
    case Qt::Key_End:
        if (event->modifiers() & Qt::ControlModifier) {
            int lastRow = model()->rowCount() - 1;
            scrollTo(model()->index(lastRow, 0), PositionAtBottom);
            setCurrentIndex(model()->index(lastRow, 0));
            event->accept();
            return;
        }
        break;
    case Qt::Key_PageUp:
    case Qt::Key_PageDown: {
        int direction = (event->key() == Qt::Key_PageUp) ? -1 : 1;
        int newRow = currentIndex().row() + direction * m_viewportInfo.totalVisibleItems;
        newRow = qBound(0, newRow, model()->rowCount() - 1);
        
        QModelIndex newIndex = model()->index(newRow, 0);
        scrollTo(newIndex, EnsureVisible);
        setCurrentIndex(newIndex);
        event->accept();
        return;
    }
    }
    
    QAbstractItemView::keyPressEvent(event);
}

void VirtualizedArchiveView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        QModelIndex index = indexAt(event->pos());
        if (index.isValid()) {
            setCurrentIndex(index);
            
            // Handle selection
            QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::ClearAndSelect;
            if (event->modifiers() & Qt::ControlModifier) {
                flags = QItemSelectionModel::Toggle;
            } else if (event->modifiers() & Qt::ShiftModifier) {
                flags = QItemSelectionModel::SelectCurrent;
            }
            
            selectionModel()->select(index, flags);
        }
    }
    
    QAbstractItemView::mousePressEvent(event);
}

void VirtualizedArchiveView::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        QModelIndex index = indexAt(event->pos());
        if (index.isValid()) {
            emit doubleClicked(index);
        }
    }
    
    QAbstractItemView::mouseDoubleClickEvent(event);
}

// QAbstractItemView implementation
int VirtualizedArchiveView::horizontalOffset() const {
    return horizontalScrollBar()->value();
}

int VirtualizedArchiveView::verticalOffset() const {
    return verticalScrollBar()->value();
}

bool VirtualizedArchiveView::isIndexHidden(const QModelIndex& index) const {
    Q_UNUSED(index)
    return false; // No hidden items in virtualized view
}

void VirtualizedArchiveView::setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags flags) {
    if (!model()) {
        return;
    }
    
    QItemSelection selection;
    
    // Find all items that intersect with the selection rectangle
    for (int i = m_viewportInfo.firstVisibleIndex; i <= m_viewportInfo.lastVisibleIndex; ++i) {
        QRect itemRect = calculateItemRect(i);
        if (itemRect.intersects(rect)) {
            QModelIndex index = model()->index(i, 0);
            selection.select(index, index);
        }
    }
    
    selectionModel()->select(selection, flags);
}

QRegion VirtualizedArchiveView::visualRegionForSelection(const QItemSelection& selection) const {
    QRegion region;
    
    for (const auto& range : selection) {
        for (int row = range.top(); row <= range.bottom(); ++row) {
            QRect itemRect = calculateItemRect(row);
            region += itemRect;
        }
    }
    
    return region;
}

// Private implementation
void VirtualizedArchiveView::updateLayout() {
    if (!model()) {
        return;
    }
    
    QElapsedTimer timer;
    if (m_performanceMonitoringEnabled) {
        timer.start();
    }
    
    // Calculate layout parameters
    m_itemsPerRow = calculateItemsPerRow();
    m_rowCount = calculateRowCount();
    m_contentSize = QSize(
        m_itemsPerRow * (m_itemSize.width() + m_itemSpacing) - m_itemSpacing,
        m_rowCount * (m_itemSize.height() + m_itemSpacing) - m_itemSpacing
    );
    
    updateScrollBars();
    calculateViewport();
    
    m_layoutDirty = false;
    
    if (m_performanceMonitoringEnabled) {
        m_performanceMetrics.layoutTime = timer.elapsed();
    }
}

void VirtualizedArchiveView::calculateViewport() {
    if (!model()) {
        m_viewportInfo = ViewportInfo();
        return;
    }
    
    QRect viewportRect = viewport()->rect();
    int scrollOffset = verticalScrollBar()->value();
    
    // Calculate visible range
    int firstVisibleRow = scrollOffset / (m_itemSize.height() + m_itemSpacing);
    int lastVisibleRow = (scrollOffset + viewportRect.height()) / (m_itemSize.height() + m_itemSpacing);
    
    // Convert to item indices
    m_viewportInfo.firstVisibleIndex = qMax(0, firstVisibleRow * m_itemsPerRow);
    m_viewportInfo.lastVisibleIndex = qMin(model()->rowCount() - 1, 
                                          (lastVisibleRow + 1) * m_itemsPerRow - 1);
    
    // Add buffer
    int bufferStart = qMax(0, m_viewportInfo.firstVisibleIndex - m_bufferSize);
    int bufferEnd = qMin(model()->rowCount() - 1, m_viewportInfo.lastVisibleIndex + m_bufferSize);
    
    m_viewportInfo.firstVisibleIndex = bufferStart;
    m_viewportInfo.lastVisibleIndex = bufferEnd;
    m_viewportInfo.totalVisibleItems = bufferEnd - bufferStart + 1;
    m_viewportInfo.bufferSize = m_bufferSize;
    m_viewportInfo.viewportRect = viewportRect;
}

void VirtualizedArchiveView::updateScrollBars() {
    if (!model()) {
        return;
    }
    
    // Vertical scroll bar
    int maxVertical = qMax(0, m_contentSize.height() - viewport()->height());
    verticalScrollBar()->setRange(0, maxVertical);
    verticalScrollBar()->setPageStep(viewport()->height());
    verticalScrollBar()->setSingleStep(m_itemSize.height() + m_itemSpacing);
    
    // Horizontal scroll bar
    int maxHorizontal = qMax(0, m_contentSize.width() - viewport()->width());
    horizontalScrollBar()->setRange(0, maxHorizontal);
    horizontalScrollBar()->setPageStep(viewport()->width());
    horizontalScrollBar()->setSingleStep(m_itemSize.width() + m_itemSpacing);
}

QSize VirtualizedArchiveView::calculateItemSize() const {
    switch (m_viewMode) {
    case ViewMode::List:
        return QSize(viewport()->width() - 20, 24);
    case ViewMode::Details:
        return QSize(viewport()->width() - 20, 32);
    case ViewMode::Grid:
        return m_itemSize;
    case ViewMode::Compact:
        return QSize(200, 20);
    }
    return m_itemSize;
}

int VirtualizedArchiveView::calculateItemsPerRow() const {
    if (m_viewMode == ViewMode::Grid) {
        int availableWidth = viewport()->width() - m_itemSpacing;
        return qMax(1, availableWidth / (m_itemSize.width() + m_itemSpacing));
    }
    return 1; // Single column for other modes
}

int VirtualizedArchiveView::calculateRowCount() const {
    if (!model()) {
        return 0;
    }
    
    if (m_viewMode == ViewMode::Grid) {
        return (model()->rowCount() + m_itemsPerRow - 1) / m_itemsPerRow;
    }
    return model()->rowCount();
}

QRect VirtualizedArchiveView::calculateItemRect(int index) const {
    if (m_viewMode == ViewMode::Grid) {
        int row = index / m_itemsPerRow;
        int col = index % m_itemsPerRow;
        
        int x = col * (m_itemSize.width() + m_itemSpacing) + m_itemSpacing;
        int y = row * (m_itemSize.height() + m_itemSpacing) + m_itemSpacing;
        
        return QRect(x - horizontalScrollBar()->value(), 
                    y - verticalScrollBar()->value(),
                    m_itemSize.width(), 
                    m_itemSize.height());
    } else {
        int y = index * (m_itemSize.height() + m_itemSpacing) + m_itemSpacing;
        return QRect(m_itemSpacing - horizontalScrollBar()->value(),
                    y - verticalScrollBar()->value(),
                    calculateItemSize().width(),
                    m_itemSize.height());
    }
}

QPoint VirtualizedArchiveView::calculateItemPosition(int index) const {
    return calculateItemRect(index).topLeft();
}

void VirtualizedArchiveView::renderVisibleItems(QPainter* painter, const QRect& rect) {
    if (!model()) {
        return;
    }
    
    QStyleOptionViewItem option = viewOptions();
    
    for (int i = m_viewportInfo.firstVisibleIndex; i <= m_viewportInfo.lastVisibleIndex; ++i) {
        if (i >= model()->rowCount()) {
            break;
        }
        
        QRect itemRect = calculateItemRect(i);
        if (!itemRect.intersects(rect)) {
            continue;
        }
        
        QModelIndex index = model()->index(i, 0);
        renderItem(painter, index, itemRect);
    }
}

void VirtualizedArchiveView::renderItem(QPainter* painter, const QModelIndex& index, const QRect& itemRect) {
    QStyleOptionViewItem option = viewOptions();
    option.rect = itemRect;
    option.state = QStyle::State_Enabled;
    
    // Set selection state
    if (selectionModel()->isSelected(index)) {
        option.state |= QStyle::State_Selected;
    }
    
    // Set focus state
    if (index == currentIndex()) {
        option.state |= QStyle::State_HasFocus;
    }
    
    // Use item delegate to paint
    itemDelegate()->paint(painter, option, index);
}

void VirtualizedArchiveView::renderBackground(QPainter* painter, const QRect& rect) {
    // Fill with background color
    painter->fillRect(rect, palette().base());
    
    // Draw grid lines for grid view
    if (m_viewMode == ViewMode::Grid) {
        painter->setPen(QPen(palette().mid().color(), 1, Qt::DotLine));
        
        // Vertical lines
        for (int col = 0; col <= m_itemsPerRow; ++col) {
            int x = col * (m_itemSize.width() + m_itemSpacing) - horizontalScrollBar()->value();
            painter->drawLine(x, rect.top(), x, rect.bottom());
        }
        
        // Horizontal lines
        int rowHeight = m_itemSize.height() + m_itemSpacing;
        int startRow = (rect.top() + verticalScrollBar()->value()) / rowHeight;
        int endRow = (rect.bottom() + verticalScrollBar()->value()) / rowHeight + 1;
        
        for (int row = startRow; row <= endRow; ++row) {
            int y = row * rowHeight - verticalScrollBar()->value();
            painter->drawLine(rect.left(), y, rect.right(), y);
        }
    }
}

void VirtualizedArchiveView::renderSelection(QPainter* painter) {
    if (!selectionModel()) {
        return;
    }
    
    QItemSelection selection = selectionModel()->selection();
    if (selection.isEmpty()) {
        return;
    }
    
    painter->save();
    painter->setPen(QPen(palette().highlight().color(), 2));
    painter->setBrush(QBrush(palette().highlight().color(), Qt::NoBrush));
    
    for (const auto& range : selection) {
        for (int row = range.top(); row <= range.bottom(); ++row) {
            QRect itemRect = calculateItemRect(row);
            painter->drawRect(itemRect.adjusted(1, 1, -1, -1));
        }
    }
    
    painter->restore();
}

// Data management
VirtualizedArchiveView::ItemData* VirtualizedArchiveView::getItemData(int index) {
    QMutexLocker locker(&m_cacheMutex);
    
    ItemData* data = m_itemCache.object(index);
    if (data) {
        data->lastAccessed = QDateTime::currentMSecsSinceEpoch();
        data->accessCount++;
        m_performanceMetrics.cacheHits++;
        return data;
    }
    
    m_performanceMetrics.cacheMisses++;
    
    if (m_lazyLoadingEnabled) {
        // Load data on demand
        locker.unlock();
        loadItemData(index);
        locker.relock();
        return m_itemCache.object(index);
    }
    
    return nullptr;
}

void VirtualizedArchiveView::loadItemData(int index) {
    if (!model() || index < 0 || index >= model()->rowCount()) {
        return;
    }
    
    QModelIndex modelIndex = model()->index(index, 0);
    
    auto* data = new ItemData();
    data->displayData = model()->data(modelIndex, Qt::DisplayRole);
    data->decorationData = model()->data(modelIndex, Qt::DecorationRole);
    data->toolTipData = model()->data(modelIndex, Qt::ToolTipRole);
    data->loaded = true;
    data->lastAccessed = QDateTime::currentMSecsSinceEpoch();
    data->accessCount = 1;
    
    QMutexLocker locker(&m_cacheMutex);
    m_itemCache.insert(index, data, 1);
    updateCacheStatistics();
}

void VirtualizedArchiveView::unloadItemData(int index) {
    QMutexLocker locker(&m_cacheMutex);
    m_itemCache.remove(index);
    updateCacheStatistics();
}

bool VirtualizedArchiveView::isItemLoaded(int index) const {
    QMutexLocker locker(&m_cacheMutex);
    return m_itemCache.contains(index);
}

// Background loading
void VirtualizedArchiveView::startBackgroundLoading() {
    if (m_backgroundLoadingActive || !m_backgroundLoadingEnabled) {
        return;
    }
    
    m_backgroundLoadingActive = true;
    emit backgroundLoadingStarted();
}

void VirtualizedArchiveView::stopBackgroundLoading() {
    if (!m_backgroundLoadingActive) {
        return;
    }
    
    m_backgroundLoadingActive = false;
    
    if (m_backgroundWatcher->isRunning()) {
        m_backgroundWatcher->cancel();
        m_backgroundWatcher->waitForFinished();
    }
    
    QMutexLocker locker(&m_loadingMutex);
    m_loadingQueue.clear();
}

void VirtualizedArchiveView::scheduleBackgroundLoad(int startIndex, int endIndex) {
    if (!m_backgroundLoadingEnabled || !model()) {
        return;
    }
    
    QMutexLocker locker(&m_loadingMutex);
    
    // Limit queue size
    if (m_loadingQueue.size() >= MAX_BACKGROUND_QUEUE_SIZE) {
        return;
    }
    
    m_loadingQueue.enqueue({startIndex, endIndex});
    m_loadingCondition.wakeOne();
    
    // Start background loading if not already running
    if (!m_backgroundWatcher->isRunning()) {
        QFuture<void> future = QtConcurrent::run([this]() {
            QMutexLocker locker(&m_loadingMutex);
            
            while (m_backgroundLoadingActive && !m_loadingQueue.isEmpty()) {
                auto range = m_loadingQueue.dequeue();
                locker.unlock();
                
                // Load items in range
                for (int i = range.first; i <= range.second && i < model()->rowCount(); ++i) {
                    if (!isItemLoaded(i)) {
                        loadItemData(i);
                    }
                    
                    if (!m_backgroundLoadingActive) {
                        break;
                    }
                }
                
                locker.relock();
            }
        });
        
        m_backgroundWatcher->setFuture(future);
    }
}

// Cache management
void VirtualizedArchiveView::cleanupCache() {
    QMutexLocker locker(&m_cacheMutex);
    
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    QList<int> keysToRemove;
    
    // Find items to remove based on age and access count
    for (auto it = m_itemCache.begin(); it != m_itemCache.end(); ++it) {
        ItemData* data = it.value();
        if (data && (currentTime - data->lastAccessed) > CACHE_ITEM_LIFETIME) {
            keysToRemove.append(it.key());
        }
    }
    
    // Remove old items
    for (int key : keysToRemove) {
        m_itemCache.remove(key);
    }
    
    updateCacheStatistics();
}

void VirtualizedArchiveView::updateCacheStatistics() {
    m_performanceMetrics.memoryUsage = calculateMemoryUsage();
    emit cacheStatusChanged(m_itemCache.size(), m_itemCache.maxCost());
}

qint64 VirtualizedArchiveView::calculateMemoryUsage() const {
    // Rough estimate of memory usage
    return m_itemCache.size() * sizeof(ItemData) + 
           m_itemCache.size() * 100; // Approximate data size
}

// Performance monitoring
void VirtualizedArchiveView::startPerformanceTimer() {
    m_lastFrameTime = QDateTime::currentMSecsSinceEpoch();
    m_frameCount = 0;
}

void VirtualizedArchiveView::updatePerformanceMetrics() {
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 elapsed = currentTime - m_lastFrameTime;
    
    if (elapsed > 0) {
        m_performanceMetrics.fps = (m_frameCount * 1000.0) / elapsed;
    }
    
    emit performanceMetricsUpdated(m_performanceMetrics);
}

// Utility functions
int VirtualizedArchiveView::indexFromPoint(const QPoint& point) const {
    if (m_viewMode == ViewMode::Grid) {
        int col = (point.x() + horizontalScrollBar()->value()) / (m_itemSize.width() + m_itemSpacing);
        int row = (point.y() + verticalScrollBar()->value()) / (m_itemSize.height() + m_itemSpacing);
        
        col = qBound(0, col, m_itemsPerRow - 1);
        row = qBound(0, row, m_rowCount - 1);
        
        return row * m_itemsPerRow + col;
    } else {
        int index = (point.y() + verticalScrollBar()->value()) / (m_itemSize.height() + m_itemSpacing);
        return qBound(0, index, model() ? model()->rowCount() - 1 : 0);
    }
}

QList<int> VirtualizedArchiveView::visibleIndexes() const {
    QList<int> indexes;
    for (int i = m_viewportInfo.firstVisibleIndex; i <= m_viewportInfo.lastVisibleIndex; ++i) {
        indexes.append(i);
    }
    return indexes;
}

bool VirtualizedArchiveView::isIndexVisible(int index) const {
    return index >= m_viewportInfo.firstVisibleIndex && index <= m_viewportInfo.lastVisibleIndex;
}

void VirtualizedArchiveView::ensureIndexVisible(int index) {
    if (!model() || index < 0 || index >= model()->rowCount()) {
        return;
    }
    
    QModelIndex modelIndex = model()->index(index, 0);
    scrollTo(modelIndex, EnsureVisible);
}

// Slot implementations
void VirtualizedArchiveView::onModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight) {
    // Invalidate cache for changed items
    for (int row = topLeft.row(); row <= bottomRight.row(); ++row) {
        unloadItemData(row);
    }
    
    // Update viewport
    viewport()->update();
}

void VirtualizedArchiveView::onModelRowsInserted(const QModelIndex& parent, int first, int last) {
    Q_UNUSED(parent)
    
    // Clear cache as indices have changed
    clearCache();
    updateLayout();
    viewport()->update();
}

void VirtualizedArchiveView::onModelRowsRemoved(const QModelIndex& parent, int first, int last) {
    Q_UNUSED(parent)
    
    // Clear cache as indices have changed
    clearCache();
    updateLayout();
    viewport()->update();
}

void VirtualizedArchiveView::onModelReset() {
    clearCache();
    updateLayout();
    viewport()->update();
}

void VirtualizedArchiveView::onVerticalScrollBarValueChanged(int value) {
    Q_UNUSED(value)
    
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    m_lastScrollTime = currentTime;
    
    // Start smooth scrolling timer
    if (m_smoothScrolling) {
        m_scrollTimer->start();
    }
}

void VirtualizedArchiveView::onHorizontalScrollBarValueChanged(int value) {
    Q_UNUSED(value)
    viewport()->update();
}

void VirtualizedArchiveView::onBackgroundLoadingFinished() {
    emit backgroundLoadingFinished();
}

void VirtualizedArchiveView::onCacheCleanupTimer() {
    cleanupCache();
}

void VirtualizedArchiveView::onPerformanceTimer() {
    updatePerformanceMetrics();
}

// BackgroundLoader implementation
BackgroundLoader::BackgroundLoader(QAbstractItemModel* model, QObject* parent)
    : QObject(parent)
    , m_model(model)
{
}

void BackgroundLoader::loadRange(int startIndex, int endIndex) {
    if (!m_model || m_stopRequested) {
        return;
    }
    
    for (int i = startIndex; i <= endIndex && i < m_model->rowCount(); ++i) {
        if (m_stopRequested) {
            break;
        }
        
        QModelIndex index = m_model->index(i, 0);
        QVariant data = m_model->data(index, Qt::DisplayRole);
        
        emit itemLoaded(i, data);
        
        // Small delay to prevent blocking
        QThread::msleep(1);
    }
    
    if (!m_stopRequested) {
        emit rangeLoaded(startIndex, endIndex);
    }
    
    emit loadingFinished();
}

void BackgroundLoader::stop() {
    m_stopRequested = true;
}

// VirtualizedItemDelegate implementation
VirtualizedItemDelegate::VirtualizedItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
    , m_thumbnailCache(1000)
{
}

void VirtualizedItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (m_fastPaintingEnabled) {
        paintFast(painter, option, index);
    } else {
        paintDetailed(painter, option, index);
    }
}

QSize VirtualizedItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    return size.expandedTo(m_thumbnailSize);
}

void VirtualizedItemDelegate::setFastPaintingEnabled(bool enabled) {
    m_fastPaintingEnabled = enabled;
}

void VirtualizedItemDelegate::setCachingEnabled(bool enabled) {
    m_cachingEnabled = enabled;
    if (!enabled) {
        m_thumbnailCache.clear();
    }
}

void VirtualizedItemDelegate::setThumbnailSize(const QSize& size) {
    m_thumbnailSize = size;
}

void VirtualizedItemDelegate::paintFast(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    // Fast painting for high performance
    painter->save();
    
    // Draw background
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
        painter->setPen(option.palette.highlightedText().color());
    } else {
        painter->setPen(option.palette.text().color());
    }
    
    // Draw text only
    QString text = index.data(Qt::DisplayRole).toString();
    painter->drawText(option.rect.adjusted(4, 2, -4, -2), Qt::AlignLeft | Qt::AlignVCenter, text);
    
    painter->restore();
}

void VirtualizedItemDelegate::paintDetailed(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    // Detailed painting with full features
    QStyledItemDelegate::paint(painter, option, index);
    
    // Add thumbnail if available and caching is enabled
    if (m_cachingEnabled) {
        QPixmap thumbnail = getCachedThumbnail(index);
        if (!thumbnail.isNull()) {
            QRect iconRect = option.rect;
            iconRect.setSize(m_thumbnailSize);
            iconRect.moveCenter(option.rect.center());
            
            painter->drawPixmap(iconRect, thumbnail);
        }
    }
}

QPixmap VirtualizedItemDelegate::getCachedThumbnail(const QModelIndex& index) const {
    if (!m_cachingEnabled) {
        return QPixmap();
    }
    
    QString key = QString::number(index.row());
    QPixmap* cached = m_thumbnailCache.object(key);
    
    if (cached) {
        return *cached;
    }
    
    // Generate thumbnail (simplified)
    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    if (!icon.isNull()) {
        QPixmap pixmap = icon.pixmap(m_thumbnailSize);
        m_thumbnailCache.insert(key, new QPixmap(pixmap));
        return pixmap;
    }
    
    return QPixmap();
}

} // namespace FluxGUI::UI::Components

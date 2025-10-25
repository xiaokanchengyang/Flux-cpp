#pragma once

#include <QAbstractItemView>
#include <QAbstractItemModel>
#include <QScrollBar>
#include <QTimer>
#include <QCache>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QFuture>
#include <QFutureWatcher>
#include <memory>

QT_BEGIN_NAMESPACE
class QStyleOptionViewItem;
class QPainter;
class QStyledItemDelegate;
QT_END_NAMESPACE

namespace FluxGUI::UI::Components {

/**
 * Virtualized Archive View
 * 
 * High-performance view component for displaying large archive contents:
 * - Virtualized scrolling for millions of items
 * - Lazy loading with background data fetching
 * - Efficient memory management with LRU caching
 * - Smooth scrolling with predictive loading
 * - Background thumbnail generation
 * - Optimized painting and layout
 */
class VirtualizedArchiveView : public QAbstractItemView {
    Q_OBJECT

public:
    enum class ViewMode {
        List,           // Simple list with icons
        Details,        // Detailed table view
        Grid,           // Grid with large thumbnails
        Compact         // Compact list view
    };

    enum class ScrollMode {
        Item,           // Scroll by items
        Pixel,          // Smooth pixel scrolling
        Page            // Page-based scrolling
    };

    struct ViewportInfo {
        int firstVisibleIndex{0};
        int lastVisibleIndex{0};
        int totalVisibleItems{0};
        int bufferSize{0};
        QRect viewportRect;
    };

    struct PerformanceMetrics {
        qint64 renderTime{0};
        qint64 layoutTime{0};
        qint64 scrollTime{0};
        int itemsRendered{0};
        int cacheHits{0};
        int cacheMisses{0};
        double fps{0.0};
        qint64 memoryUsage{0};
    };

    explicit VirtualizedArchiveView(QWidget* parent = nullptr);
    ~VirtualizedArchiveView() override;

    // View configuration
    void setViewMode(ViewMode mode);
    ViewMode viewMode() const { return m_viewMode; }

    void setScrollMode(ScrollMode mode);
    ScrollMode scrollMode() const { return m_scrollMode; }

    void setItemSize(const QSize& size);
    QSize itemSize() const { return m_itemSize; }

    void setItemSpacing(int spacing);
    int itemSpacing() const { return m_itemSpacing; }

    // Performance settings
    void setBufferSize(int items);
    int bufferSize() const { return m_bufferSize; }

    void setCacheSize(int maxItems);
    int cacheSize() const;

    void setLazyLoadingEnabled(bool enabled);
    bool isLazyLoadingEnabled() const { return m_lazyLoadingEnabled; }

    void setBackgroundLoadingEnabled(bool enabled);
    bool isBackgroundLoadingEnabled() const { return m_backgroundLoadingEnabled; }

    void setPredictiveLoadingEnabled(bool enabled);
    bool isPredictiveLoadingEnabled() const { return m_predictiveLoadingEnabled; }

    // Viewport information
    ViewportInfo viewportInfo() const;
    QRect visualRect(const QModelIndex& index) const override;
    QModelIndex indexAt(const QPoint& point) const override;

    // Selection and navigation
    void scrollTo(const QModelIndex& index, ScrollHint hint = EnsureVisible) override;
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;

    // Performance monitoring
    PerformanceMetrics performanceMetrics() const { return m_performanceMetrics; }
    void resetPerformanceMetrics();
    void setPerformanceMonitoringEnabled(bool enabled);

    // Cache management
    void clearCache();
    void preloadRange(int startIndex, int endIndex);
    void invalidateRange(int startIndex, int endIndex);

    // QAbstractItemView implementation
    void setModel(QAbstractItemModel* model) override;
    QRect visualRect(const QModelIndex& index) const;
    void scrollTo(const QModelIndex& index, ScrollHint hint = EnsureVisible);
    QModelIndex indexAt(const QPoint& point) const;

signals:
    // Performance signals
    void performanceMetricsUpdated(const PerformanceMetrics& metrics);
    void viewportChanged(const ViewportInfo& info);
    
    // Loading signals
    void itemsRequested(int startIndex, int endIndex);
    void backgroundLoadingStarted();
    void backgroundLoadingFinished();
    
    // Cache signals
    void cacheStatusChanged(int used, int total);

protected:
    // Event handling
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void scrollContentsBy(int dx, int dy) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

    // QAbstractItemView implementation
    int horizontalOffset() const override;
    int verticalOffset() const override;
    bool isIndexHidden(const QModelIndex& index) const override;
    void setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags flags) override;
    QRegion visualRegionForSelection(const QItemSelection& selection) const override;

private slots:
    void onModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
    void onModelRowsInserted(const QModelIndex& parent, int first, int last);
    void onModelRowsRemoved(const QModelIndex& parent, int first, int last);
    void onModelReset();
    void onVerticalScrollBarValueChanged(int value);
    void onHorizontalScrollBarValueChanged(int value);
    void onBackgroundLoadingFinished();
    void onCacheCleanupTimer();
    void onPerformanceTimer();

private:
    // Layout calculations
    void updateLayout();
    void calculateViewport();
    void updateScrollBars();
    QSize calculateItemSize() const;
    int calculateItemsPerRow() const;
    int calculateRowCount() const;
    QRect calculateItemRect(int index) const;
    QPoint calculateItemPosition(int index) const;

    // Rendering
    void renderVisibleItems(QPainter* painter, const QRect& rect);
    void renderItem(QPainter* painter, const QModelIndex& index, const QRect& itemRect);
    void renderBackground(QPainter* painter, const QRect& rect);
    void renderSelection(QPainter* painter);

    // Data management
    struct ItemData {
        QVariant displayData;
        QVariant decorationData;
        QVariant toolTipData;
        QPixmap thumbnail;
        bool loaded{false};
        qint64 lastAccessed{0};
        int accessCount{0};
    };

    ItemData* getItemData(int index);
    void loadItemData(int index);
    void unloadItemData(int index);
    bool isItemLoaded(int index) const;

    // Background loading
    void startBackgroundLoading();
    void stopBackgroundLoading();
    void scheduleBackgroundLoad(int startIndex, int endIndex);

    // Cache management
    void cleanupCache();
    void updateCacheStatistics();
    qint64 calculateMemoryUsage() const;

    // Performance monitoring
    void startPerformanceTimer();
    void updatePerformanceMetrics();
    void measureRenderTime(std::function<void()> renderFunc);

    // Utility functions
    int indexFromPoint(const QPoint& point) const;
    QList<int> visibleIndexes() const;
    bool isIndexVisible(int index) const;
    void ensureIndexVisible(int index);

private:
    // View configuration
    ViewMode m_viewMode{ViewMode::Details};
    ScrollMode m_scrollMode{ScrollMode::Item};
    QSize m_itemSize{200, 32};
    int m_itemSpacing{2};

    // Performance settings
    int m_bufferSize{50};
    bool m_lazyLoadingEnabled{true};
    bool m_backgroundLoadingEnabled{true};
    bool m_predictiveLoadingEnabled{true};
    bool m_performanceMonitoringEnabled{false};

    // Layout state
    ViewportInfo m_viewportInfo;
    int m_itemsPerRow{1};
    int m_rowCount{0};
    QSize m_contentSize;
    bool m_layoutDirty{true};

    // Data cache
    QCache<int, ItemData> m_itemCache;
    QMutex m_cacheMutex;
    static constexpr int DEFAULT_CACHE_SIZE = 10000;

    // Background loading
    QThread* m_backgroundThread{nullptr};
    QFutureWatcher<void>* m_backgroundWatcher{nullptr};
    QQueue<QPair<int, int>> m_loadingQueue;
    QMutex m_loadingMutex;
    QWaitCondition m_loadingCondition;
    bool m_backgroundLoadingActive{false};

    // Timers
    QTimer* m_cacheCleanupTimer{nullptr};
    QTimer* m_performanceTimer{nullptr};
    QTimer* m_scrollTimer{nullptr};

    // Performance metrics
    PerformanceMetrics m_performanceMetrics;
    qint64 m_lastFrameTime{0};
    int m_frameCount{0};

    // Scroll state
    int m_lastVerticalValue{0};
    int m_lastHorizontalValue{0};
    qint64 m_lastScrollTime{0};
    bool m_smoothScrolling{false};

    // Constants
    static constexpr int CACHE_CLEANUP_INTERVAL = 30000; // 30 seconds
    static constexpr int PERFORMANCE_UPDATE_INTERVAL = 1000; // 1 second
    static constexpr int SCROLL_TIMER_INTERVAL = 16; // ~60 FPS
    static constexpr int PREDICTIVE_LOAD_THRESHOLD = 10; // items
    static constexpr int MAX_BACKGROUND_QUEUE_SIZE = 100;
    static constexpr qint64 CACHE_ITEM_LIFETIME = 300000; // 5 minutes
};

/**
 * Background Loading Worker
 */
class BackgroundLoader : public QObject {
    Q_OBJECT

public:
    explicit BackgroundLoader(QAbstractItemModel* model, QObject* parent = nullptr);

public slots:
    void loadRange(int startIndex, int endIndex);
    void stop();

signals:
    void itemLoaded(int index, const QVariant& data);
    void rangeLoaded(int startIndex, int endIndex);
    void loadingFinished();

private:
    QAbstractItemModel* m_model{nullptr};
    bool m_stopRequested{false};
};

/**
 * Performance Optimized Item Delegate
 */
class VirtualizedItemDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit VirtualizedItemDelegate(QObject* parent = nullptr);

    // QStyledItemDelegate implementation
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    // Performance optimizations
    void setFastPaintingEnabled(bool enabled);
    void setCachingEnabled(bool enabled);
    void setThumbnailSize(const QSize& size);

private:
    void paintFast(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    void paintDetailed(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    QPixmap getCachedThumbnail(const QModelIndex& index) const;

private:
    bool m_fastPaintingEnabled{false};
    bool m_cachingEnabled{true};
    QSize m_thumbnailSize{32, 32};
    mutable QCache<QString, QPixmap> m_thumbnailCache;
};

} // namespace FluxGUI::UI::Components

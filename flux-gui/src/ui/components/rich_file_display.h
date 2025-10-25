#pragma once

#include <QWidget>
#include <QAbstractItemView>
#include <QStyledItemDelegate>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QToolButton>
#include <QMenu>
#include <QTimer>
#include <QPixmap>
#include <QIcon>
#include <QMimeType>
#include <QFileInfo>
#include <QDateTime>
#include <memory>

QT_BEGIN_NAMESPACE
class QListView;
class QTreeView;
class QTableView;
class QSplitter;
class QScrollArea;
class QGraphicsOpacityEffect;
class QPropertyAnimation;
QT_END_NAMESPACE

namespace FluxGUI::UI::Components {

/**
 * Rich File Display Component
 * 
 * Advanced file display system with:
 * - Multiple view modes (list, tree, grid, details)
 * - File type icons and thumbnails
 * - Rich metadata tooltips and previews
 * - Size visualization and compression indicators
 * - File relationship mapping and grouping
 * - Search and filtering capabilities
 * - Batch selection and operations
 */
class RichFileDisplay : public QWidget {
    Q_OBJECT

public:
    enum class ViewMode {
        List,           // Simple list view
        Tree,           // Hierarchical tree view
        Grid,           // Grid view with large icons
        Details,        // Detailed table view
        Timeline,       // Timeline view by date
        Size            // Size-based visualization
    };

    enum class SortCriteria {
        Name,           // Alphabetical by name
        Size,           // By file size
        Type,           // By file type/extension
        Modified,       // By modification date
        Compressed,     // By compression ratio
        Path            // By full path
    };

    enum class GroupingMode {
        None,           // No grouping
        Type,           // Group by file type
        Size,           // Group by size ranges
        Date,           // Group by date ranges
        Directory       // Group by parent directory
    };

    struct FileItem {
        QString name;
        QString path;
        QString relativePath;
        qint64 size{0};
        qint64 compressedSize{0};
        QDateTime modified;
        QMimeType mimeType;
        QString typeDescription;
        QIcon icon;
        QPixmap thumbnail;
        bool isDirectory{false};
        bool isArchive{false};
        QStringList tags;
        QVariantMap metadata;
        double compressionRatio{0.0};
    };

    struct DisplaySettings {
        ViewMode viewMode{ViewMode::Details};
        SortCriteria sortBy{SortCriteria::Name};
        Qt::SortOrder sortOrder{Qt::AscendingOrder};
        GroupingMode grouping{GroupingMode::None};
        bool showThumbnails{true};
        bool showPreview{true};
        bool showMetadata{true};
        bool showCompressionInfo{true};
        bool showSizeVisualization{true};
        int iconSize{32};
        int thumbnailSize{128};
        bool animateChanges{true};
    };

    explicit RichFileDisplay(QWidget* parent = nullptr);
    ~RichFileDisplay() override;

    // Data management
    void setFiles(const QList<FileItem>& files);
    void addFile(const FileItem& file);
    void removeFile(const QString& path);
    void updateFile(const FileItem& file);
    void clear();

    // View configuration
    void setViewMode(ViewMode mode);
    ViewMode viewMode() const { return m_settings.viewMode; }
    void setDisplaySettings(const DisplaySettings& settings);
    DisplaySettings displaySettings() const { return m_settings; }

    // Selection management
    QStringList selectedFiles() const;
    void setSelectedFiles(const QStringList& paths);
    void selectAll();
    void clearSelection();
    int selectionCount() const;

    // Filtering and search
    void setFilter(const QString& filter);
    void setTypeFilter(const QStringList& types);
    void setSizeFilter(qint64 minSize, qint64 maxSize);
    void setDateFilter(const QDateTime& from, const QDateTime& to);
    void clearFilters();

    // Sorting and grouping
    void setSorting(SortCriteria criteria, Qt::SortOrder order = Qt::AscendingOrder);
    void setGrouping(GroupingMode mode);

    // Preview and metadata
    void showPreview(const QString& filePath);
    void hidePreview();
    void showMetadata(const QString& filePath);
    void hideMetadata();

    // Thumbnail management
    void generateThumbnails(bool async = true);
    void clearThumbnails();
    void setThumbnailSize(int size);

    // Context menu
    void setContextMenu(QMenu* menu);
    QMenu* contextMenu() const { return m_contextMenu; }

    // Accessibility
    void setAccessibilityEnabled(bool enabled);
    QString getAccessibilityInfo(const QString& filePath) const;

signals:
    // Selection signals
    void selectionChanged(const QStringList& selectedFiles);
    void fileDoubleClicked(const QString& filePath);
    void fileActivated(const QString& filePath);

    // View signals
    void viewModeChanged(ViewMode newMode);
    void sortingChanged(SortCriteria criteria, Qt::SortOrder order);
    void groupingChanged(GroupingMode mode);

    // Context signals
    void contextMenuRequested(const QString& filePath, const QPoint& position);
    void previewRequested(const QString& filePath);
    void metadataRequested(const QString& filePath);

    // Progress signals
    void thumbnailGenerationStarted(int totalFiles);
    void thumbnailGenerationProgress(int completed, int total);
    void thumbnailGenerationFinished();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onSelectionChanged();
    void onItemDoubleClicked(const QModelIndex& index);
    void onItemActivated(const QModelIndex& index);
    void onContextMenuRequested(const QPoint& position);
    void onSortingChanged(int column, Qt::SortOrder order);
    void onThumbnailGenerated(const QString& filePath, const QPixmap& thumbnail);
    void onViewModeChanged();

private:
    // UI initialization
    void initializeUI();
    void createViews();
    void createToolbar();
    void createPreviewPanel();
    void createMetadataPanel();
    void setupLayouts();

    // View management
    void switchView(ViewMode mode);
    void updateViewSettings();
    void syncViewSelection();

    // Model management
    void setupModel();
    void populateModel();
    void updateModelItem(const FileItem& file);
    void applyFilters();
    void applySorting();
    void applyGrouping();

    // Thumbnail generation
    void startThumbnailGeneration();
    void generateThumbnail(const FileItem& file);
    QPixmap createThumbnail(const FileItem& file, const QSize& size);
    QIcon getFileTypeIcon(const FileItem& file);

    // Preview system
    void updatePreview(const QString& filePath);
    QWidget* createPreviewWidget(const FileItem& file);
    void updateMetadataDisplay(const FileItem& file);

    // Visual helpers
    void animateViewTransition(ViewMode fromMode, ViewMode toMode);
    void updateSizeVisualization();
    void updateCompressionIndicators();
    QString formatFileSize(qint64 size) const;
    QString formatCompressionRatio(double ratio) const;
    QColor getCompressionColor(double ratio) const;

    // Accessibility helpers
    void updateAccessibilityInfo();
    QString getItemAccessibilityText(const FileItem& file) const;

private:
    // Settings
    DisplaySettings m_settings;
    bool m_accessibilityEnabled{true};

    // Data
    QList<FileItem> m_files;
    QStandardItemModel* m_model{nullptr};
    QSortFilterProxyModel* m_proxyModel{nullptr};

    // UI components
    QVBoxLayout* m_mainLayout{nullptr};
    QHBoxLayout* m_toolbarLayout{nullptr};
    QSplitter* m_mainSplitter{nullptr};
    QSplitter* m_rightSplitter{nullptr};

    // Views
    QListView* m_listView{nullptr};
    QTreeView* m_treeView{nullptr};
    QTableView* m_detailsView{nullptr};
    QWidget* m_gridView{nullptr};
    QAbstractItemView* m_currentView{nullptr};

    // Toolbar
    QWidget* m_toolbar{nullptr};
    QToolButton* m_viewModeButton{nullptr};
    QToolButton* m_sortButton{nullptr};
    QToolButton* m_groupButton{nullptr};
    QToolButton* m_filterButton{nullptr};
    QLabel* m_statusLabel{nullptr};

    // Preview panel
    QWidget* m_previewPanel{nullptr};
    QScrollArea* m_previewArea{nullptr};
    QLabel* m_previewLabel{nullptr};
    QWidget* m_previewContent{nullptr};

    // Metadata panel
    QWidget* m_metadataPanel{nullptr};
    QVBoxLayout* m_metadataLayout{nullptr};
    QLabel* m_metadataTitle{nullptr};

    // Context menu
    QMenu* m_contextMenu{nullptr};

    // Thumbnail generation
    QTimer* m_thumbnailTimer{nullptr};
    QQueue<FileItem> m_thumbnailQueue;
    bool m_thumbnailGenerationActive{false};
    int m_thumbnailsGenerated{0};

    // Animation
    std::unique_ptr<QPropertyAnimation> m_viewTransitionAnimation;
    std::unique_ptr<QGraphicsOpacityEffect> m_opacityEffect;

    // Filtering
    QString m_textFilter;
    QStringList m_typeFilter;
    qint64 m_minSizeFilter{0};
    qint64 m_maxSizeFilter{std::numeric_limits<qint64>::max()};
    QDateTime m_fromDateFilter;
    QDateTime m_toDateFilter;

    // Constants
    static constexpr int DEFAULT_ICON_SIZE = 32;
    static constexpr int DEFAULT_THUMBNAIL_SIZE = 128;
    static constexpr int THUMBNAIL_GENERATION_INTERVAL = 10;
    static constexpr int MAX_THUMBNAIL_CACHE = 1000;
    static constexpr int ANIMATION_DURATION = 250;
};

/**
 * Custom Item Delegate for Rich File Display
 */
class RichFileItemDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit RichFileItemDelegate(QObject* parent = nullptr);

    // QStyledItemDelegate implementation
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    // Configuration
    void setShowThumbnails(bool show);
    void setShowMetadata(bool show);
    void setShowCompressionInfo(bool show);
    void setIconSize(int size);

private:
    void paintFileItem(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    void paintThumbnail(QPainter* painter, const QRect& rect, const QPixmap& thumbnail) const;
    void paintMetadata(QPainter* painter, const QRect& rect, const RichFileDisplay::FileItem& file) const;
    void paintCompressionInfo(QPainter* painter, const QRect& rect, double ratio) const;
    void paintSizeVisualization(QPainter* painter, const QRect& rect, qint64 size, qint64 maxSize) const;

private:
    bool m_showThumbnails{true};
    bool m_showMetadata{true};
    bool m_showCompressionInfo{true};
    int m_iconSize{32};
};

} // namespace FluxGUI::UI::Components

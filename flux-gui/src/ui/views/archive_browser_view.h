#pragma once

#include <QWidget>
#include <QTreeView>
#include <QListView>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QLineEdit>
#include <QLabel>
#include <QProgressBar>
#include <QMenu>
#include <QAction>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QFileSystemModel>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>

#include <memory>
#include <unordered_map>

namespace FluxGUI::UI::Views {

/**
 * @brief Modern archive browsing interface with advanced features
 * 
 * This view provides a comprehensive interface for browsing archive contents,
 * supporting multiple view modes, search, preview, and batch operations.
 * 
 * Features:
 * - Tree and list view modes
 * - Real-time search and filtering
 * - File preview panel
 * - Drag and drop support
 * - Context menus with smart actions
 * - Batch selection and operations
 * - Breadcrumb navigation
 * - File type categorization
 * - Size and date sorting
 * - Keyboard navigation
 */
class ArchiveBrowserView : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief View display modes
     */
    enum class ViewMode {
        Tree,           ///< Hierarchical tree view
        List,           ///< Flat list view
        Icons,          ///< Large icon view
        Details         ///< Detailed list with columns
    };

    /**
     * @brief File selection modes
     */
    enum class SelectionMode {
        Single,         ///< Single file selection
        Multiple,       ///< Multiple file selection
        Extended        ///< Extended selection with ranges
    };

    /**
     * @brief Archive file information structure
     */
    struct ArchiveFileInfo {
        QString name;
        QString path;
        QString type;
        qint64 size;
        qint64 compressedSize;
        QDateTime modified;
        QString permissions;
        bool isDirectory;
        bool isEncrypted;
        double compressionRatio;
        QString comment;
    };

    explicit ArchiveBrowserView(QWidget* parent = nullptr);
    ~ArchiveBrowserView() override;

    // Archive management
    void openArchive(const QString& archivePath);
    void closeArchive();
    void refreshArchive();
    bool isArchiveOpen() const { return !m_currentArchivePath.isEmpty(); }
    QString currentArchivePath() const { return m_currentArchivePath; }

    // View configuration
    void setViewMode(ViewMode mode);
    ViewMode viewMode() const { return m_currentViewMode; }
    
    void setSelectionMode(SelectionMode mode);
    SelectionMode selectionMode() const { return m_selectionMode; }

    // Navigation
    void navigateToPath(const QString& path);
    void navigateUp();
    void navigateBack();
    void navigateForward();
    QString currentPath() const { return m_currentPath; }

    // Selection and operations
    QStringList selectedFiles() const;
    QStringList selectedPaths() const;
    int selectedCount() const;
    qint64 selectedSize() const;

    void selectAll();
    void selectNone();
    void invertSelection();
    void selectByPattern(const QString& pattern);

    // Search and filtering
    void setSearchText(const QString& text);
    void setFileTypeFilter(const QString& filter);
    void setSizeFilter(qint64 minSize, qint64 maxSize);
    void setDateFilter(const QDateTime& from, const QDateTime& to);
    void clearFilters();

    // Preview
    void showPreviewPanel(bool show);
    bool isPreviewPanelVisible() const { return m_previewPanelVisible; }

    // Accessibility
    void setHighContrastMode(bool enabled);
    void setFontScale(double scale);

Q_SIGNALS:
    // Archive events
    void archiveOpened(const QString& path);
    void archiveClosed();
    void archiveError(const QString& error);

    // Navigation events
    void pathChanged(const QString& path);
    void navigationStateChanged(bool canGoBack, bool canGoForward, bool canGoUp);

    // Selection events
    void selectionChanged(const QStringList& selectedFiles);
    void fileActivated(const QString& filePath);
    void fileDoubleClicked(const QString& filePath);

    // Operation requests
    void extractRequested(const QStringList& files, const QString& destination);
    void deleteRequested(const QStringList& files);
    void renameRequested(const QString& oldName, const QString& newName);
    void addFilesRequested(const QStringList& files, const QString& destination);
    void createFolderRequested(const QString& name, const QString& path);

    // Context menu requests
    void contextMenuRequested(const QPoint& position, const QStringList& selectedFiles);

    // Preview events
    void previewRequested(const QString& filePath);
    void previewClosed();

protected:
    // Event handlers
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private Q_SLOTS:
    // UI event handlers
    void onViewModeChanged();
    void onSearchTextChanged(const QString& text);
    void onItemSelectionChanged();
    void onItemActivated(const QModelIndex& index);
    void onItemDoubleClicked(const QModelIndex& index);
    void onCustomContextMenuRequested(const QPoint& position);

    // Navigation handlers
    void onNavigateUp();
    void onNavigateBack();
    void onNavigateForward();
    void onBreadcrumbClicked(const QString& path);

    // Filter handlers
    void onFilterChanged();
    void onSortOrderChanged();

    // Preview handlers
    void onPreviewToggled(bool enabled);
    void onPreviewFileChanged(const QString& filePath);

    // Animation handlers
    void onFadeAnimationFinished();
    void onSlideAnimationFinished();

private:
    // UI initialization
    void initializeUI();
    void createToolbar();
    void createMainView();
    void createPreviewPanel();
    void createStatusArea();
    void setupLayouts();
    void setupConnections();
    void applyStyles();

    // Model management
    void setupArchiveModel();
    void populateModel();
    void updateModel();
    void clearModel();

    // View management
    void switchViewMode(ViewMode mode);
    void updateViewConfiguration();
    void syncViewSelection();

    // Navigation management
    void updateNavigationState();
    void updateBreadcrumbs();
    void addToNavigationHistory(const QString& path);

    // Filter management
    void applyFilters();
    void updateFilteredModel();
    bool matchesFilter(const ArchiveFileInfo& fileInfo) const;

    // Context menu management
    void createContextMenu();
    void updateContextMenu(const QStringList& selectedFiles);
    void showContextMenu(const QPoint& position);

    // Preview management
    void updatePreview(const QString& filePath);
    void clearPreview();
    void resizePreviewPanel();

    // Animation management
    void animateViewTransition();
    void animatePreviewToggle();
    void fadeInWidget(QWidget* widget);
    void fadeOutWidget(QWidget* widget);

    // Utility methods
    QString formatFileSize(qint64 size) const;
    QString formatDateTime(const QDateTime& dateTime) const;
    QString getFileTypeDescription(const QString& fileName) const;
    QIcon getFileIcon(const ArchiveFileInfo& fileInfo) const;
    QString getCompressionRatioText(double ratio) const;

    // Data validation
    bool isValidArchivePath(const QString& path) const;
    bool isValidFilePath(const QString& path) const;

private:
    // Core data
    QString m_currentArchivePath;
    QString m_currentPath;
    ViewMode m_currentViewMode;
    SelectionMode m_selectionMode;
    
    // Navigation history
    QStringList m_navigationHistory;
    int m_navigationIndex;
    
    // UI components
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_contentLayout;
    QSplitter* m_mainSplitter;
    
    // Toolbar
    QToolBar* m_toolbar;
    QAction* m_backAction;
    QAction* m_forwardAction;
    QAction* m_upAction;
    QAction* m_refreshAction;
    QAction* m_viewModeAction;
    QAction* m_previewAction;
    
    // Search and filter
    QLineEdit* m_searchEdit;
    QWidget* m_filterWidget;
    QTimer* m_searchTimer;
    
    // Breadcrumb navigation
    QWidget* m_breadcrumbWidget;
    QHBoxLayout* m_breadcrumbLayout;
    
    // Main view area
    QWidget* m_viewContainer;
    QStackedWidget* m_viewStack;
    QTreeView* m_treeView;
    QListView* m_listView;
    QListView* m_iconView;
    
    // Models
    std::unique_ptr<QStandardItemModel> m_archiveModel;
    std::unique_ptr<QSortFilterProxyModel> m_proxyModel;
    
    // Preview panel
    QWidget* m_previewPanel;
    QVBoxLayout* m_previewLayout;
    QLabel* m_previewLabel;
    QScrollArea* m_previewScrollArea;
    bool m_previewPanelVisible;
    
    // Status area
    QWidget* m_statusArea;
    QLabel* m_statusLabel;
    QLabel* m_selectionLabel;
    QProgressBar* m_loadingProgress;
    
    // Context menu
    std::unique_ptr<QMenu> m_contextMenu;
    
    // Animations
    std::unique_ptr<QPropertyAnimation> m_fadeAnimation;
    std::unique_ptr<QPropertyAnimation> m_slideAnimation;
    std::unique_ptr<QGraphicsOpacityEffect> m_opacityEffect;
    
    // Filter state
    QString m_searchText;
    QString m_fileTypeFilter;
    qint64 m_minSizeFilter;
    qint64 m_maxSizeFilter;
    QDateTime m_fromDateFilter;
    QDateTime m_toDateFilter;
    
    // Archive data
    std::vector<ArchiveFileInfo> m_archiveFiles;
    std::unordered_map<QString, ArchiveFileInfo> m_fileInfoCache;
    
    // Configuration
    bool m_highContrastMode;
    double m_fontScale;
    bool m_showHiddenFiles;
    bool m_autoRefresh;
    
    // Constants
    static constexpr int ANIMATION_DURATION = 250;
    static constexpr int SEARCH_DELAY = 300;
    static constexpr int MAX_PREVIEW_SIZE = 1024 * 1024; // 1MB
    static constexpr int BREADCRUMB_MAX_ITEMS = 10;
};

} // namespace FluxGUI::UI::Views

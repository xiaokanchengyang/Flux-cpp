#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QListWidget>
#include <QSplitter>
#include <QToolBar>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QProgressBar>
#include <QMenu>
#include <QContextMenuEvent>
#include <QFileInfo>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>

namespace FluxGUI::UI::Widgets {

/**
 * @brief Modern archive explorer widget with dual-pane interface
 * 
 * The ArchiveExplorerWidget provides comprehensive archive browsing with:
 * - Tree view for archive structure navigation
 * - List view for file details and thumbnails
 * - Context menu operations (extract, delete, rename)
 * - Search and filter functionality
 * - Drag & drop support for adding/extracting files
 * - Real-time preview for supported file types
 */
class ArchiveExplorerWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief View modes for file display
     */
    enum class ViewMode {
        List,       // Detailed list view
        Icons,      // Large icon view
        Tiles,      // Tile view with previews
        Tree        // Tree structure view
    };

    /**
     * @brief Construct the archive explorer widget
     * @param parent Parent widget
     */
    explicit ArchiveExplorerWidget(QWidget* parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~ArchiveExplorerWidget() override = default;

public slots:
    /**
     * @brief Open archive file
     * @param archivePath Path to archive file
     */
    void openArchive(const QString& archivePath);
    
    /**
     * @brief Close current archive
     */
    void closeArchive();
    
    /**
     * @brief Refresh archive contents
     */
    void refreshArchive();
    
    /**
     * @brief Set view mode
     * @param mode View mode to set
     */
    void setViewMode(ViewMode mode);
    
    /**
     * @brief Navigate to path within archive
     * @param path Internal archive path
     */
    void navigateToPath(const QString& path);

signals:
    /**
     * @brief Emitted when archive is opened
     * @param archivePath Path to opened archive
     */
    void archiveOpened(const QString& archivePath);
    
    /**
     * @brief Emitted when archive is closed
     */
    void archiveClosed();
    
    /**
     * @brief Emitted when files are selected for extraction
     * @param filePaths List of selected file paths
     */
    void extractRequested(const QStringList& filePaths);
    
    /**
     * @brief Emitted when files are to be added to archive
     * @param filePaths List of file paths to add
     */
    void addFilesRequested(const QStringList& filePaths);

protected:
    /**
     * @brief Handle context menu events
     * @param event Context menu event
     */
    void contextMenuEvent(QContextMenuEvent* event) override;
    
    /**
     * @brief Handle drag enter events
     * @param event Drag enter event
     */
    void dragEnterEvent(QDragEnterEvent* event) override;
    
    /**
     * @brief Handle drop events
     * @param event Drop event
     */
    void dropEvent(QDropEvent* event) override;

private slots:
    /**
     * @brief Handle tree item selection
     */
    void onTreeSelectionChanged();
    
    /**
     * @brief Handle list item selection
     */
    void onListSelectionChanged();
    
    /**
     * @brief Handle search text change
     */
    void onSearchTextChanged();
    
    /**
     * @brief Handle extract selected action
     */
    void extractSelected();
    
    /**
     * @brief Handle add files action
     */
    void addFiles();
    
    /**
     * @brief Handle delete selected action
     */
    void deleteSelected();

private:
    /**
     * @brief Initialize the user interface
     */
    void initializeUI();
    
    /**
     * @brief Create toolbar
     * @return Toolbar widget
     */
    QToolBar* createToolBar();
    
    /**
     * @brief Create context menu
     * @return Context menu
     */
    QMenu* createContextMenu();
    
    /**
     * @brief Setup drag and drop
     */
    void setupDragAndDrop();
    
    /**
     * @brief Connect signals and slots
     */
    void connectSignals();
    
    /**
     * @brief Update UI state
     */
    void updateUIState();

private:
    // Layout components
    QVBoxLayout* m_mainLayout = nullptr;
    QSplitter* m_mainSplitter = nullptr;
    
    // Toolbar
    QToolBar* m_toolBar = nullptr;
    QPushButton* m_backButton = nullptr;
    QPushButton* m_forwardButton = nullptr;
    QPushButton* m_upButton = nullptr;
    QLineEdit* m_pathEdit = nullptr;
    QLineEdit* m_searchEdit = nullptr;
    QComboBox* m_viewModeCombo = nullptr;
    
    // Main views
    QTreeWidget* m_treeWidget = nullptr;
    QListWidget* m_listWidget = nullptr;
    
    // Status
    QLabel* m_statusLabel = nullptr;
    QProgressBar* m_progressBar = nullptr;
    
    // Context menu
    QMenu* m_contextMenu = nullptr;
    
    // State
    QString m_currentArchivePath;
    QString m_currentInternalPath;
    ViewMode m_currentViewMode = ViewMode::List;
    QStringList m_selectedFiles;
};

} // namespace FluxGUI::UI::Widgets

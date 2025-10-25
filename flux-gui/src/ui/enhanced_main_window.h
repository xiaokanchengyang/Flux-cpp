#pragma once

#include "components/smart_navigation_panel.h"
#include "components/visual_feedback_manager.h"
#include "components/rich_file_display.h"
#include "components/virtualized_archive_view.h"
#include "managers/accessibility_manager.h"
#include "managers/context_menu_manager.h"
#include "dialogs/batch_operations_dialog.h"

#include <QMainWindow>
#include <QSplitter>
#include <QStackedWidget>
#include <QToolBar>
#include <QStatusBar>
#include <QMenuBar>
#include <QProgressBar>
#include <QLabel>
#include <QTimer>
#include <QSettings>
#include <memory>

QT_BEGIN_NAMESPACE
class QDragEnterEvent;
class QDropEvent;
class QCloseEvent;
class QResizeEvent;
QT_END_NAMESPACE

namespace FluxGUI::UI {

/**
 * Enhanced Main Window
 * 
 * Modern main window integrating all GUI enhancements:
 * - Smart navigation panel with contextual actions
 * - Rich file display with virtualized performance
 * - Visual feedback system with animations and notifications
 * - Comprehensive accessibility support
 * - Context-aware menus and shortcuts
 * - Batch operations with progress tracking
 * - Responsive design with adaptive layouts
 */
class EnhancedMainWindow : public QMainWindow {
    Q_OBJECT

public:
    enum class ViewMode {
        Welcome,        // Welcome screen with quick actions
        FileBrowser,    // File system browser
        ArchiveViewer,  // Archive content viewer
        BatchOperations // Batch operations interface
    };

    enum class Theme {
        Auto,           // Follow system theme
        Light,          // Light theme
        Dark,           // Dark theme
        HighContrast    // High contrast theme
    };

    explicit EnhancedMainWindow(QWidget* parent = nullptr);
    ~EnhancedMainWindow() override;

    // View management
    void setViewMode(ViewMode mode);
    ViewMode currentViewMode() const { return m_currentViewMode; }

    // Theme management
    void setTheme(Theme theme);
    Theme currentTheme() const { return m_currentTheme; }

    // Archive operations
    void openArchive(const QString& filePath);
    void createArchive(const QStringList& files);
    void extractArchive(const QString& archivePath, const QString& destination = QString());

    // File operations
    void addFilesToArchive(const QStringList& files);
    void removeFilesFromArchive(const QStringList& files);
    void previewFile(const QString& filePath);

    // Batch operations
    void showBatchOperationsDialog();
    void startBatchOperation(const QList<Dialogs::BatchOperationsDialog::BatchItem>& items);

    // Settings
    void showPreferencesDialog();
    void loadSettings();
    void saveSettings();

signals:
    // View signals
    void viewModeChanged(ViewMode newMode);
    void themeChanged(Theme newTheme);

    // Operation signals
    void archiveOpened(const QString& filePath);
    void archiveCreated(const QString& filePath);
    void archiveExtracted(const QString& archivePath, const QString& destination);
    void filesAdded(const QStringList& files);
    void filesRemoved(const QStringList& files);

    // UI signals
    void navigationRequested(const QString& path);
    void filePreviewRequested(const QString& filePath);
    void contextMenuRequested(const QPoint& position, const QStringList& selectedFiles);

protected:
    // Event handling
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void changeEvent(QEvent* event) override;

private slots:
    // Navigation panel slots
    void onNavigationItemClicked(Components::SmartNavigationPanel::NavigationItem item);
    void onBreadcrumbItemClicked(const QString& path);
    void onRecentFileRequested(const QString& filePath);
    void onQuickActionTriggered(QAction* action);

    // File display slots
    void onFileSelectionChanged(const QStringList& selectedFiles);
    void onFileDoubleClicked(const QString& filePath);
    void onFileContextMenuRequested(const QString& filePath, const QPoint& position);

    // Menu and toolbar slots
    void onNewArchiveAction();
    void onOpenArchiveAction();
    void onExtractArchiveAction();
    void onAddFilesAction();
    void onRemoveFilesAction();
    void onBatchOperationsAction();
    void onPreferencesAction();
    void onAboutAction();
    void onExitAction();

    // View mode slots
    void onWelcomeViewRequested();
    void onFileBrowserViewRequested();
    void onArchiveViewerRequested();
    void onBatchOperationsViewRequested();

    // Theme slots
    void onThemeChanged();
    void onSystemThemeChanged();

    // Progress slots
    void onOperationStarted(const QString& operation);
    void onOperationProgress(int percentage, const QString& status);
    void onOperationFinished(bool success, const QString& message);

    // Accessibility slots
    void onAccessibilitySettingsChanged();
    void onKeyboardNavigationRequested();

private:
    // UI initialization
    void initializeUI();
    void createMenuBar();
    void createToolBars();
    void createStatusBar();
    void createCentralWidget();
    void setupSplitters();
    void setupStackedWidget();

    // Component initialization
    void initializeNavigationPanel();
    void initializeFileDisplay();
    void initializeArchiveView();
    void initializeManagers();

    // Layout management
    void updateLayout();
    void adjustSplitterSizes();
    void saveLayoutState();
    void restoreLayoutState();

    // View management
    void switchToWelcomeView();
    void switchToFileBrowserView();
    void switchToArchiveViewerView();
    void switchToBatchOperationsView();
    void updateViewSpecificUI();

    // Theme management
    void applyTheme(Theme theme);
    void loadThemeStylesheet(const QString& themeName);
    void updateIconTheme();
    void detectSystemTheme();

    // File operations
    void handleDroppedFiles(const QStringList& files);
    void updateRecentFiles(const QString& filePath);
    void validateFileOperations();

    // Progress management
    void showProgressBar();
    void hideProgressBar();
    void updateProgressBar(int percentage, const QString& text);

    // Context menu management
    void setupContextMenus();
    void showFileContextMenu(const QStringList& files, const QPoint& position);
    void showNavigationContextMenu(const QPoint& position);

    // Accessibility helpers
    void setupAccessibility();
    void updateAccessibilityInfo();
    void announceViewChange(ViewMode mode);

    // Settings helpers
    void loadWindowSettings();
    void saveWindowSettings();
    void loadUISettings();
    void saveUISettings();
    void resetToDefaults();

    // Utility functions
    QString getViewModeTitle(ViewMode mode) const;
    QIcon getViewModeIcon(ViewMode mode) const;
    bool isArchiveFile(const QString& filePath) const;
    QString formatFileSize(qint64 size) const;
    void showNotification(const QString& message, Components::VisualFeedbackManager::FeedbackType type);

private:
    // Current state
    ViewMode m_currentViewMode{ViewMode::Welcome};
    Theme m_currentTheme{Theme::Auto};
    QString m_currentArchivePath;
    QStringList m_selectedFiles;
    bool m_operationInProgress{false};

    // Core components
    std::unique_ptr<Components::SmartNavigationPanel> m_navigationPanel;
    std::unique_ptr<Components::RichFileDisplay> m_fileDisplay;
    std::unique_ptr<Components::VirtualizedArchiveView> m_archiveView;
    std::unique_ptr<Components::VisualFeedbackManager> m_feedbackManager;
    std::unique_ptr<Managers::AccessibilityManager> m_accessibilityManager;
    std::unique_ptr<Managers::ContextMenuManager> m_contextMenuManager;

    // Layout components
    QSplitter* m_mainSplitter{nullptr};
    QSplitter* m_contentSplitter{nullptr};
    QStackedWidget* m_centralStack{nullptr};

    // View widgets
    QWidget* m_welcomeView{nullptr};
    QWidget* m_fileBrowserView{nullptr};
    QWidget* m_archiveViewerView{nullptr};
    QWidget* m_batchOperationsView{nullptr};

    // Menu and toolbar
    QMenuBar* m_menuBar{nullptr};
    QMenu* m_fileMenu{nullptr};
    QMenu* m_editMenu{nullptr};
    QMenu* m_viewMenu{nullptr};
    QMenu* m_toolsMenu{nullptr};
    QMenu* m_helpMenu{nullptr};

    QToolBar* m_mainToolBar{nullptr};
    QToolBar* m_viewToolBar{nullptr};

    // Status bar
    QStatusBar* m_statusBar{nullptr};
    QLabel* m_statusLabel{nullptr};
    QLabel* m_selectionLabel{nullptr};
    QProgressBar* m_progressBar{nullptr};
    QLabel* m_progressLabel{nullptr};

    // Actions
    QAction* m_newArchiveAction{nullptr};
    QAction* m_openArchiveAction{nullptr};
    QAction* m_extractArchiveAction{nullptr};
    QAction* m_addFilesAction{nullptr};
    QAction* m_removeFilesAction{nullptr};
    QAction* m_batchOperationsAction{nullptr};
    QAction* m_preferencesAction{nullptr};
    QAction* m_aboutAction{nullptr};
    QAction* m_exitAction{nullptr};

    // View mode actions
    QActionGroup* m_viewModeGroup{nullptr};
    QAction* m_welcomeViewAction{nullptr};
    QAction* m_fileBrowserViewAction{nullptr};
    QAction* m_archiveViewerViewAction{nullptr};
    QAction* m_batchOperationsViewAction{nullptr};

    // Theme actions
    QActionGroup* m_themeGroup{nullptr};
    QAction* m_autoThemeAction{nullptr};
    QAction* m_lightThemeAction{nullptr};
    QAction* m_darkThemeAction{nullptr};
    QAction* m_highContrastThemeAction{nullptr};

    // Dialogs
    std::unique_ptr<Dialogs::BatchOperationsDialog> m_batchDialog;

    // Timers
    QTimer* m_statusTimer{nullptr};
    QTimer* m_themeDetectionTimer{nullptr};

    // Settings
    QSettings* m_settings{nullptr};

    // Constants
    static constexpr int STATUS_MESSAGE_TIMEOUT = 5000;
    static constexpr int THEME_DETECTION_INTERVAL = 1000;
    static constexpr int NAVIGATION_PANEL_MIN_WIDTH = 200;
    static constexpr int NAVIGATION_PANEL_MAX_WIDTH = 400;
    static constexpr int FILE_DISPLAY_MIN_WIDTH = 300;
    static constexpr int SPLITTER_HANDLE_WIDTH = 1;
};

} // namespace FluxGUI::UI

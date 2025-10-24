#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QShortcut>
#include <QSystemTrayIcon>
#include <QMenu>
#include <memory>

QT_BEGIN_NAMESPACE
class QDragEnterEvent;
class QDropEvent;
class QResizeEvent;
class QKeyEvent;
QT_END_NAMESPACE

namespace FluxGUI::UI {

// Forward declarations
class UnifiedDropZone;
class ModernToolbar;
class SmartStatusBar;
class ContextualSidebar;
class WelcomeView;
class ArchiveView;
class SettingsView;
class NotificationManager;
class KeyboardShortcutManager;

/**
 * Modern Main Window with improved UX design
 * 
 * Key improvements:
 * - Unified drag-and-drop experience
 * - Context-aware interface that adapts to user actions
 * - Streamlined navigation with clear visual hierarchy
 * - Comprehensive keyboard shortcuts
 * - Smart progress feedback with detailed status
 * - Onboarding flow for new users
 */
class ModernMainWindow : public QMainWindow {
    Q_OBJECT

public:
    enum class ViewMode {
        Welcome,     // Initial landing page
        Archive,     // Archive browsing/editing
        Settings     // Application settings
    };

    explicit ModernMainWindow(QWidget* parent = nullptr);
    ~ModernMainWindow() override;

    // Public interface
    void openArchive(const QString& filePath);
    void createArchive(const QStringList& inputFiles);
    void extractArchive(const QString& archivePath, const QString& outputPath = QString());

signals:
    // User action signals
    void archiveOpened(const QString& filePath);
    void archiveCreated(const QString& outputPath);
    void extractionRequested(const QString& archivePath, const QString& outputPath);
    void settingsRequested();

protected:
    // Event handlers
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;

private slots:
    // Navigation
    void switchToView(ViewMode mode);
    void onBackRequested();
    void onHomeRequested();
    
    // File operations
    void onFilesDropped(const QStringList& files);
    void onArchiveFileDropped(const QString& archivePath);
    void onRegularFilesDropped(const QStringList& files);
    
    // Progress and feedback
    void onOperationStarted(const QString& operation, const QString& details);
    void onOperationProgress(int percentage, const QString& currentItem);
    void onOperationFinished(bool success, const QString& message);
    void onOperationCancelled();
    
    // Settings and preferences
    void onThemeChanged();
    void onLanguageChanged();
    void onFirstRunCompleted();
    
    // System integration
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onMinimizeToTray();

private:
    // UI setup
    void initializeUI();
    void createCentralWidget();
    void createToolbar();
    void createStatusBar();
    void createSystemTray();
    void setupKeyboardShortcuts();
    void setupAnimations();
    void applyModernStyling();
    
    // Navigation helpers
    void showWelcomeView();
    void showArchiveView(const QString& archivePath = QString());
    void showSettingsView();
    void updateNavigationState();
    
    // Drag and drop helpers
    void handleDroppedFiles(const QStringList& filePaths);
    QStringList categorizeFiles(const QStringList& filePaths, QStringList& archives, QStringList& regular);
    void showDropFeedback(bool show, const QString& message = QString());
    
    // User experience helpers
    void showOnboardingIfNeeded();
    void showContextualHelp(const QString& topic);
    void updateRecentFiles(const QString& filePath);
    void saveWindowState();
    void restoreWindowState();

private:
    // Core UI components
    QWidget* m_centralWidget{nullptr};
    QVBoxLayout* m_mainLayout{nullptr};
    QStackedWidget* m_viewStack{nullptr};
    
    // Modern UI components
    std::unique_ptr<UnifiedDropZone> m_dropZone;
    std::unique_ptr<ModernToolbar> m_toolbar;
    std::unique_ptr<SmartStatusBar> m_statusBar;
    std::unique_ptr<NotificationManager> m_notifications;
    std::unique_ptr<KeyboardShortcutManager> m_shortcuts;
    
    // Views
    std::unique_ptr<WelcomeView> m_welcomeView;
    std::unique_ptr<ArchiveView> m_archiveView;
    std::unique_ptr<SettingsView> m_settingsView;
    
    // System integration
    std::unique_ptr<QSystemTrayIcon> m_trayIcon;
    std::unique_ptr<QMenu> m_trayMenu;
    
    // State management
    ViewMode m_currentView{ViewMode::Welcome};
    QString m_currentArchivePath;
    QStringList m_recentFiles;
    bool m_isFirstRun{true};
    bool m_operationInProgress{false};
    
    // Animation and effects
    std::unique_ptr<QPropertyAnimation> m_fadeAnimation;
    std::unique_ptr<QGraphicsOpacityEffect> m_opacityEffect;
    
    // Timers
    std::unique_ptr<QTimer> m_statusUpdateTimer;
    std::unique_ptr<QTimer> m_autosaveTimer;
    
    // Constants
    static constexpr int ANIMATION_DURATION = 250;
    static constexpr int STATUS_UPDATE_INTERVAL = 1000;
    static constexpr int AUTOSAVE_INTERVAL = 30000;
    static constexpr int MAX_RECENT_FILES = 10;
};

} // namespace FluxGUI::UI

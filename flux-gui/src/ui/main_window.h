#pragma once

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QToolBar>
#include <QMenuBar>
#include <QStatusBar>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSplitter>
#include <QTreeWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QComboBox>
#include <QLineEdit>
#include <QAction>
#include <QActionGroup>
#include <QTimer>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <memory>

// Forward declarations
namespace FluxGUI::Core { class ThemeManager; }
namespace FluxGUI::UI::Widgets { 
    class WelcomeWidget; 
    class ArchiveExplorerWidget;
    class CompressionWidget;
    class ExtractionWidget;
    class SettingsWidget;
}

namespace FluxGUI::UI {

/**
 * @brief Main application window with modern Material Design interface
 * 
 * The MainWindow provides a comprehensive archive management interface with:
 * - Modern dark/light theme support
 * - Drag & drop functionality
 * - Multi-tab archive browsing
 * - Integrated compression/extraction tools
 * - Real-time progress monitoring
 * - Customizable workspace layout
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief Construct the main window
     * @param parent Parent widget
     */
    explicit MainWindow(QWidget* parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~MainWindow() override;

protected:
    /**
     * @brief Handle close event
     * @param event Close event
     */
    void closeEvent(QCloseEvent* event) override;
    
    /**
     * @brief Handle drag enter events for file dropping
     * @param event Drag enter event
     */
    void dragEnterEvent(QDragEnterEvent* event) override;
    
    /**
     * @brief Handle drop events for file dropping
     * @param event Drop event
     */
    void dropEvent(QDropEvent* event) override;
    
    /**
     * @brief Handle key press events
     * @param event Key press event
     */
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    /**
     * @brief Handle theme changes
     */
    void onThemeChanged();
    
    /**
     * @brief Toggle between dark and light themes
     */
    void toggleTheme();
    
    /**
     * @brief Show welcome page
     */
    void showWelcomePage();
    
    /**
     * @brief Show archive explorer
     */
    void showArchiveExplorer();
    
    /**
     * @brief Show compression tools
     */
    void showCompressionTools();
    
    /**
     * @brief Show extraction tools
     */
    void showExtractionTools();
    
    /**
     * @brief Show settings dialog
     */
    void showSettings();
    
    /**
     * @brief Open archive file
     */
    void openArchive();
    
    /**
     * @brief Create new archive
     */
    void createArchive();
    
    /**
     * @brief Extract archive
     */
    void extractArchive();
    
    /**
     * @brief Show about dialog
     */
    void showAbout();
    
    /**
     * @brief Handle recent file selection
     */
    void openRecentFile();
    
    /**
     * @brief Update status bar
     * @param message Status message
     */
    void updateStatus(const QString& message);
    
    /**
     * @brief Update progress bar
     * @param value Progress value (0-100)
     */
    void updateProgress(int value);
    
    /**
     * @brief Handle operation completion
     */
    void onOperationCompleted();
    
    /**
     * @brief Handle operation error
     * @param error Error message
     */
    void onOperationError(const QString& error);

private:
    /**
     * @brief Initialize the user interface
     */
    void initializeUI();
    
    /**
     * @brief Setup menu bar
     */
    void setupMenuBar();
    
    /**
     * @brief Setup toolbar
     */
    void setupToolBar();
    
    /**
     * @brief Setup status bar
     */
    void setupStatusBar();
    
    /**
     * @brief Setup central widget
     */
    void setupCentralWidget();
    
    /**
     * @brief Create actions
     */
    void createActions();
    
    /**
     * @brief Connect signals and slots
     */
    void connectSignals();
    
    /**
     * @brief Load application settings
     */
    void loadSettings();
    
    /**
     * @brief Save application settings
     */
    void saveSettings();
    
    /**
     * @brief Update recent files menu
     */
    void updateRecentFilesMenu();
    
    /**
     * @brief Add file to recent files list
     * @param filePath Path to the file
     */
    void addToRecentFiles(const QString& filePath);
    
    /**
     * @brief Setup drag and drop
     */
    void setupDragAndDrop();
    
    /**
     * @brief Apply current theme
     */
    void applyTheme();
    
    /**
     * @brief Update window title
     * @param filePath Optional file path to display
     */
    void updateWindowTitle(const QString& filePath = QString());

private:
    // Core components
    Core::ThemeManager* m_themeManager = nullptr;
    
    // UI Components
    QStackedWidget* m_centralStack = nullptr;
    QToolBar* m_mainToolBar = nullptr;
    QStatusBar* m_statusBar = nullptr;
    
    // Status bar widgets
    QLabel* m_statusLabel = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QLabel* m_themeLabel = nullptr;
    QPushButton* m_themeToggle = nullptr;
    
    // Page widgets
    Widgets::WelcomeWidget* m_welcomeWidget = nullptr;
    Widgets::ArchiveExplorerWidget* m_explorerWidget = nullptr;
    Widgets::CompressionWidget* m_compressionWidget = nullptr;
    Widgets::ExtractionWidget* m_extractionWidget = nullptr;
    Widgets::SettingsWidget* m_settingsWidget = nullptr;
    
    // Actions
    QAction* m_newArchiveAction = nullptr;
    QAction* m_openArchiveAction = nullptr;
    QAction* m_extractAction = nullptr;
    QAction* m_addFilesAction = nullptr;
    QAction* m_removeFilesAction = nullptr;
    QAction* m_settingsAction = nullptr;
    QAction* m_aboutAction = nullptr;
    QAction* m_exitAction = nullptr;
    QAction* m_themeToggleAction = nullptr;
    
    // View actions
    QActionGroup* m_viewGroup = nullptr;
    QAction* m_welcomeViewAction = nullptr;
    QAction* m_explorerViewAction = nullptr;
    QAction* m_compressionViewAction = nullptr;
    QAction* m_extractionViewAction = nullptr;
    
    // Recent files
    QMenu* m_recentFilesMenu = nullptr;
    QStringList m_recentFiles;
    static constexpr int MaxRecentFiles = 10;
    
    // Current state
    QString m_currentArchivePath;
    bool m_isOperationInProgress = false;
    
    // Timers
    QTimer* m_statusTimer = nullptr;
};

} // namespace FluxGUI::UI

#include "main_window.h"

#include <QApplication>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QStackedWidget>
#include <QLabel>
#include <QProgressBar>
#include <QAction>
#include <QActionGroup>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QTimer>
#include <QDebug>

#include "widgets/welcome_widget.h"
#include "widgets/archive_explorer_widget.h"
#include "widgets/compression_widget.h"
#include "widgets/extraction_widget.h"
#include "widgets/settings_widget.h"
#include "components/modern_toolbar.h"
#include "components/status_bar.h"
#include "dialogs/about_dialog.h"
#include "dialogs/preferences_dialog.h"

#include "../core/theme/theme_manager.h"
#include "../core/config/settings_manager.h"
#include "../core/archive/archive_manager.h"

namespace FluxGUI::UI {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_stackedWidget(nullptr)
    , m_welcomeWidget(nullptr)
    , m_archiveExplorerWidget(nullptr)
    , m_compressionWidget(nullptr)
    , m_extractionWidget(nullptr)
    , m_settingsWidget(nullptr)
    , m_modernToolbar(nullptr)
    , m_customStatusBar(nullptr)
    , m_currentMode(Mode::Welcome)
{
    // Set window properties
    setWindowTitle("Flux Archive Manager");
    setWindowIcon(QIcon(":/icons/flux-logo.png"));
    setMinimumSize(800, 600);
    
    // Enable drag and drop
    setAcceptDrops(true);
    
    // Initialize UI
    initializeUI();
    
    // Create actions and menus
    createActions();
    createMenus();
    
    // Connect signals
    connectSignals();
    
    // Load settings
    loadSettings();
    
    // Set initial mode
    setMode(Mode::Welcome);
    
    qDebug() << "MainWindow initialized";
}

MainWindow::~MainWindow() {
    saveSettings();
}

void MainWindow::setMode(Mode mode) {
    if (m_currentMode == mode) {
        return;
    }
    
    Mode previousMode = m_currentMode;
    m_currentMode = mode;
    
    // Update stacked widget
    switch (mode) {
        case Mode::Welcome:
            m_stackedWidget->setCurrentWidget(m_welcomeWidget);
            break;
        case Mode::ArchiveExplorer:
            m_stackedWidget->setCurrentWidget(m_archiveExplorerWidget);
            break;
        case Mode::Compression:
            m_stackedWidget->setCurrentWidget(m_compressionWidget);
            break;
        case Mode::Extraction:
            m_stackedWidget->setCurrentWidget(m_extractionWidget);
            break;
        case Mode::Settings:
            m_stackedWidget->setCurrentWidget(m_settingsWidget);
            break;
    }
    
    // Update toolbar
    if (m_modernToolbar) {
        m_modernToolbar->setMode(static_cast<Components::ModernToolbar::Mode>(mode));
    }
    
    // Update window title
    updateWindowTitle();
    
    // Update actions
    updateActions();
    
    emit modeChanged(mode, previousMode);
    
    qDebug() << "Mode changed to:" << static_cast<int>(mode);
}

MainWindow::Mode MainWindow::currentMode() const {
    return m_currentMode;
}

void MainWindow::openArchive(const QString& filePath) {
    if (filePath.isEmpty()) {
        return;
    }
    
    // Switch to archive explorer mode
    setMode(Mode::ArchiveExplorer);
    
    // Load archive in explorer widget
    if (m_archiveExplorerWidget) {
        m_archiveExplorerWidget->openArchive(filePath);
    }
    
    // Update recent files
    addToRecentFiles(filePath);
    
    qDebug() << "Opened archive:" << filePath;
}

void MainWindow::createArchive(const QStringList& files) {
    // Switch to compression mode
    setMode(Mode::Compression);
    
    // Set files in compression widget
    if (m_compressionWidget && !files.isEmpty()) {
        m_compressionWidget->setInputFiles(files);
    }
    
    qDebug() << "Creating archive with" << files.size() << "files";
}

void MainWindow::extractArchive(const QString& archivePath) {
    if (archivePath.isEmpty()) {
        return;
    }
    
    // Switch to extraction mode
    setMode(Mode::Extraction);
    
    // Set archive in extraction widget
    if (m_extractionWidget) {
        m_extractionWidget->setArchivePath(archivePath);
    }
    
    qDebug() << "Extracting archive:" << archivePath;
}

void MainWindow::showSettings() {
    setMode(Mode::Settings);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    // Save settings before closing
    saveSettings();
    
    // Check for active operations
    auto& archiveManager = Core::Archive::ArchiveManager::instance();
    if (!archiveManager.activeOperations().isEmpty()) {
        int ret = QMessageBox::question(this, "Flux Archive Manager",
            "There are active operations. Do you want to cancel them and exit?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        
        if (ret == QMessageBox::Yes) {
            archiveManager.cancelAllOperations();
        } else {
            event->ignore();
            return;
        }
    }
    
    event->accept();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        // Check if any of the URLs are files we can handle
        bool hasValidFiles = false;
        for (const QUrl& url : event->mimeData()->urls()) {
            if (url.isLocalFile()) {
                QString filePath = url.toLocalFile();
                QFileInfo fileInfo(filePath);
                
                if (fileInfo.isFile()) {
                    // Check if it's an archive or regular file
                    auto& archiveManager = Core::Archive::ArchiveManager::instance();
                    if (archiveManager.isExtensionSupported(fileInfo.suffix()) || fileInfo.isFile()) {
                        hasValidFiles = true;
                        break;
                    }
                }
            }
        }
        
        if (hasValidFiles) {
            event->acceptProposedAction();
        }
    }
}

void MainWindow::dropEvent(QDropEvent* event) {
    QStringList archiveFiles;
    QStringList regularFiles;
    
    // Categorize dropped files
    for (const QUrl& url : event->mimeData()->urls()) {
        if (url.isLocalFile()) {
            QString filePath = url.toLocalFile();
            QFileInfo fileInfo(filePath);
            
            if (fileInfo.isFile()) {
                auto& archiveManager = Core::Archive::ArchiveManager::instance();
                if (archiveManager.isExtensionSupported(fileInfo.suffix())) {
                    archiveFiles.append(filePath);
                } else {
                    regularFiles.append(filePath);
                }
            } else if (fileInfo.isDir()) {
                regularFiles.append(filePath);
            }
        }
    }
    
    // Handle dropped files
    if (!archiveFiles.isEmpty()) {
        // If only one archive, open it; otherwise show selection dialog
        if (archiveFiles.size() == 1) {
            openArchive(archiveFiles.first());
        } else {
            // TODO: Show archive selection dialog
            openArchive(archiveFiles.first());
        }
    } else if (!regularFiles.isEmpty()) {
        // Create archive from regular files
        createArchive(regularFiles);
    }
    
    event->acceptProposedAction();
}

void MainWindow::initializeUI() {
    // Create central widget
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    
    // Create main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(m_centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Create modern toolbar
    m_modernToolbar = new Components::ModernToolbar(this);
    mainLayout->addWidget(m_modernToolbar);
    
    // Create stacked widget for different modes
    m_stackedWidget = new QStackedWidget(this);
    mainLayout->addWidget(m_stackedWidget);
    
    // Create widgets for different modes
    m_welcomeWidget = new Widgets::WelcomeWidget(this);
    m_archiveExplorerWidget = new Widgets::ArchiveExplorerWidget(this);
    m_compressionWidget = new Widgets::CompressionWidget(this);
    m_extractionWidget = new Widgets::ExtractionWidget(this);
    m_settingsWidget = new Widgets::SettingsWidget(this);
    
    // Add widgets to stacked widget
    m_stackedWidget->addWidget(m_welcomeWidget);
    m_stackedWidget->addWidget(m_archiveExplorerWidget);
    m_stackedWidget->addWidget(m_compressionWidget);
    m_stackedWidget->addWidget(m_extractionWidget);
    m_stackedWidget->addWidget(m_settingsWidget);
    
    // Create custom status bar
    m_customStatusBar = new Components::StatusBar(this);
    setStatusBar(m_customStatusBar);
}

void MainWindow::createActions() {
    // File actions
    m_newArchiveAction = new QAction(QIcon(":/icons/add.svg"), "&New Archive...", this);
    m_newArchiveAction->setShortcut(QKeySequence::New);
    m_newArchiveAction->setStatusTip("Create a new archive");
    
    m_openArchiveAction = new QAction(QIcon(":/icons/folder-open.svg"), "&Open Archive...", this);
    m_openArchiveAction->setShortcut(QKeySequence::Open);
    m_openArchiveAction->setStatusTip("Open an existing archive");
    
    m_extractHereAction = new QAction(QIcon(":/icons/extract.svg"), "Extract &Here", this);
    m_extractHereAction->setShortcut(QKeySequence("Ctrl+Shift+E"));
    m_extractHereAction->setStatusTip("Extract archive to current directory");
    
    m_extractToAction = new QAction(QIcon(":/icons/extract.svg"), "&Extract To...", this);
    m_extractToAction->setShortcut(QKeySequence("Ctrl+E"));
    m_extractToAction->setStatusTip("Extract archive to specified directory");
    
    m_exitAction = new QAction(QIcon(":/icons/exit.svg"), "E&xit", this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setStatusTip("Exit the application");
    
    // View actions
    m_welcomeViewAction = new QAction(QIcon(":/icons/home.svg"), "&Welcome", this);
    m_welcomeViewAction->setCheckable(true);
    m_welcomeViewAction->setStatusTip("Show welcome screen");
    
    m_explorerViewAction = new QAction(QIcon(":/icons/folder.svg"), "&Explorer", this);
    m_explorerViewAction->setCheckable(true);
    m_explorerViewAction->setStatusTip("Show archive explorer");
    
    m_compressionViewAction = new QAction(QIcon(":/icons/compress.svg"), "&Compression", this);
    m_compressionViewAction->setCheckable(true);
    m_compressionViewAction->setStatusTip("Show compression interface");
    
    m_extractionViewAction = new QAction(QIcon(":/icons/extract.svg"), "E&xtraction", this);
    m_extractionViewAction->setCheckable(true);
    m_extractionViewAction->setStatusTip("Show extraction interface");
    
    // Create view action group
    m_viewActionGroup = new QActionGroup(this);
    m_viewActionGroup->addAction(m_welcomeViewAction);
    m_viewActionGroup->addAction(m_explorerViewAction);
    m_viewActionGroup->addAction(m_compressionViewAction);
    m_viewActionGroup->addAction(m_extractionViewAction);
    
    // Tools actions
    m_settingsAction = new QAction(QIcon(":/icons/settings.svg"), "&Settings...", this);
    m_settingsAction->setShortcut(QKeySequence::Preferences);
    m_settingsAction->setStatusTip("Open application settings");
    
    // Help actions
    m_aboutAction = new QAction(QIcon(":/icons/about.svg"), "&About", this);
    m_aboutAction->setStatusTip("Show information about the application");
    
    m_aboutQtAction = new QAction("About &Qt", this);
    m_aboutQtAction->setStatusTip("Show information about Qt");
}

void MainWindow::createMenus() {
    // File menu
    QMenu* fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction(m_newArchiveAction);
    fileMenu->addAction(m_openArchiveAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_extractHereAction);
    fileMenu->addAction(m_extractToAction);
    fileMenu->addSeparator();
    
    // Recent files submenu
    m_recentFilesMenu = fileMenu->addMenu(QIcon(":/icons/folder.svg"), "&Recent Files");
    updateRecentFilesMenu();
    
    fileMenu->addSeparator();
    fileMenu->addAction(m_exitAction);
    
    // View menu
    QMenu* viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction(m_welcomeViewAction);
    viewMenu->addAction(m_explorerViewAction);
    viewMenu->addAction(m_compressionViewAction);
    viewMenu->addAction(m_extractionViewAction);
    
    // Tools menu
    QMenu* toolsMenu = menuBar()->addMenu("&Tools");
    toolsMenu->addAction(m_settingsAction);
    
    // Help menu
    QMenu* helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction(m_aboutAction);
    helpMenu->addAction(m_aboutQtAction);
}

void MainWindow::connectSignals() {
    // File actions
    connect(m_newArchiveAction, &QAction::triggered, this, &MainWindow::onNewArchive);
    connect(m_openArchiveAction, &QAction::triggered, this, &MainWindow::onOpenArchive);
    connect(m_extractHereAction, &QAction::triggered, this, &MainWindow::onExtractHere);
    connect(m_extractToAction, &QAction::triggered, this, &MainWindow::onExtractTo);
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);
    
    // View actions
    connect(m_welcomeViewAction, &QAction::triggered, [this]() { setMode(Mode::Welcome); });
    connect(m_explorerViewAction, &QAction::triggered, [this]() { setMode(Mode::ArchiveExplorer); });
    connect(m_compressionViewAction, &QAction::triggered, [this]() { setMode(Mode::Compression); });
    connect(m_extractionViewAction, &QAction::triggered, [this]() { setMode(Mode::Extraction); });
    
    // Tools actions
    connect(m_settingsAction, &QAction::triggered, this, &MainWindow::showSettings);
    
    // Help actions
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
    connect(m_aboutQtAction, &QAction::triggered, qApp, &QApplication::aboutQt);
    
    // Widget signals
    if (m_welcomeWidget) {
        connect(m_welcomeWidget, &Widgets::WelcomeWidget::openArchiveRequested, 
                this, &MainWindow::openArchive);
        connect(m_welcomeWidget, &Widgets::WelcomeWidget::createArchiveRequested, 
                this, [this]() { setMode(Mode::Compression); });
        connect(m_welcomeWidget, &Widgets::WelcomeWidget::extractArchiveRequested, 
                this, [this]() { setMode(Mode::Extraction); });
    }
    
    // Toolbar signals
    if (m_modernToolbar) {
        connect(m_modernToolbar, &Components::ModernToolbar::modeChangeRequested,
                this, [this](int mode) { setMode(static_cast<Mode>(mode)); });
    }
    
    // Theme manager signals
    auto& themeManager = Core::Theme::ThemeManager::instance();
    connect(&themeManager, &Core::Theme::ThemeManager::themeChanged,
            this, &MainWindow::onThemeChanged);
}

void MainWindow::loadSettings() {
    auto& settingsManager = Core::Config::SettingsManager::instance();
    
    // Window geometry
    QSize size = settingsManager.windowSize();
    QPoint position = settingsManager.windowPosition();
    bool maximized = settingsManager.windowMaximized();
    
    resize(size);
    if (position.x() >= 0 && position.y() >= 0) {
        move(position);
    }
    if (maximized) {
        showMaximized();
    }
    
    qDebug() << "Loaded window settings";
}

void MainWindow::saveSettings() {
    auto& settingsManager = Core::Config::SettingsManager::instance();
    
    // Window geometry
    settingsManager.setWindowSize(size());
    settingsManager.setWindowPosition(pos());
    settingsManager.setWindowMaximized(isMaximized());
    
    qDebug() << "Saved window settings";
}

void MainWindow::updateWindowTitle() {
    QString title = "Flux Archive Manager";
    
    switch (m_currentMode) {
        case Mode::Welcome:
            title += " - Welcome";
            break;
        case Mode::ArchiveExplorer:
            title += " - Explorer";
            break;
        case Mode::Compression:
            title += " - Create Archive";
            break;
        case Mode::Extraction:
            title += " - Extract Archive";
            break;
        case Mode::Settings:
            title += " - Settings";
            break;
    }
    
    setWindowTitle(title);
}

void MainWindow::updateActions() {
    // Update view actions
    switch (m_currentMode) {
        case Mode::Welcome:
            m_welcomeViewAction->setChecked(true);
            break;
        case Mode::ArchiveExplorer:
            m_explorerViewAction->setChecked(true);
            break;
        case Mode::Compression:
            m_compressionViewAction->setChecked(true);
            break;
        case Mode::Extraction:
            m_extractionViewAction->setChecked(true);
            break;
        case Mode::Settings:
            // Settings doesn't have a view action
            break;
    }
}

void MainWindow::updateRecentFilesMenu() {
    if (!m_recentFilesMenu) {
        return;
    }
    
    m_recentFilesMenu->clear();
    
    auto& settingsManager = Core::Config::SettingsManager::instance();
    QStringList recentFiles = settingsManager.value("paths/recentFiles", QStringList()).toStringList();
    
    if (recentFiles.isEmpty()) {
        QAction* noFilesAction = m_recentFilesMenu->addAction("No recent files");
        noFilesAction->setEnabled(false);
        return;
    }
    
    for (const QString& filePath : recentFiles) {
        QFileInfo fileInfo(filePath);
        QAction* action = m_recentFilesMenu->addAction(fileInfo.fileName());
        action->setStatusTip(filePath);
        action->setData(filePath);
        
        connect(action, &QAction::triggered, [this, filePath]() {
            openArchive(filePath);
        });
    }
    
    m_recentFilesMenu->addSeparator();
    QAction* clearAction = m_recentFilesMenu->addAction("Clear Recent Files");
    connect(clearAction, &QAction::triggered, [this]() {
        auto& settingsManager = Core::Config::SettingsManager::instance();
        settingsManager.setValue("paths/recentFiles", QStringList());
        updateRecentFilesMenu();
    });
}

void MainWindow::addToRecentFiles(const QString& filePath) {
    auto& settingsManager = Core::Config::SettingsManager::instance();
    QStringList recentFiles = settingsManager.value("paths/recentFiles", QStringList()).toStringList();
    
    // Remove if already exists
    recentFiles.removeAll(filePath);
    
    // Add to beginning
    recentFiles.prepend(filePath);
    
    // Limit to maximum count
    int maxRecentFiles = settingsManager.value("advanced/maxRecentFiles", 10).toInt();
    while (recentFiles.size() > maxRecentFiles) {
        recentFiles.removeLast();
    }
    
    // Save and update menu
    settingsManager.setValue("paths/recentFiles", recentFiles);
    updateRecentFilesMenu();
}

// Slot implementations
void MainWindow::onNewArchive() {
    setMode(Mode::Compression);
}

void MainWindow::onOpenArchive() {
    QString filePath = QFileDialog::getOpenFileName(this,
        "Open Archive", QString(),
        "Archive Files (*.zip *.7z *.rar *.tar *.gz *.bz2 *.xz);;All Files (*)");
    
    if (!filePath.isEmpty()) {
        openArchive(filePath);
    }
}

void MainWindow::onExtractHere() {
    // TODO: Implement extract here functionality
    QMessageBox::information(this, "Extract Here", "Extract here functionality will be implemented.");
}

void MainWindow::onExtractTo() {
    setMode(Mode::Extraction);
}

void MainWindow::onAbout() {
    Dialogs::AboutDialog dialog(this);
    dialog.exec();
}

void MainWindow::onThemeChanged(const QString& themeName, const QString& previousTheme) {
    Q_UNUSED(previousTheme)
    qDebug() << "Theme changed to:" << themeName;
    
    // Update status bar message
    if (m_customStatusBar) {
        m_customStatusBar->showMessage(QString("Theme changed to %1").arg(themeName), 3000);
    }
}

} // namespace FluxGUI::UI
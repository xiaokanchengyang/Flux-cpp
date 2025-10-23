#include "mainwindow.h"
#include "views/welcome_view.h"
#include "views/pack_view.h"
#include "views/extract_view.h"
#include "views/archive_browser_view.h"
#include "views/settings_view.h"
#include "utils/task_manager.h"
#include "utils/theme_manager.h"

#include <QApplication>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>
#include <QStackedWidget>
#include <QListWidget>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QFileDialog>
#include <QDesktopServices>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_mainSplitter(nullptr)
    , m_navigationList(nullptr)
    , m_contentStack(nullptr)
    , m_currentViewIndex(WelcomeViewIndex)
    , m_statusUpdateTimer(new QTimer(this))
{
    // Set window properties
    setWindowTitle("Flux Archive Tool");
    setMinimumSize(MIN_WINDOW_WIDTH, MIN_WINDOW_HEIGHT);
    resize(1200, 800);
    
    // Enable drag and drop
    setAcceptDrops(true);
    
    // Initialize task manager
    m_taskManager = std::make_unique<TaskManager>(this);
    
    // Setup UI
    setupUI();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupConnections();
    
    // Apply styles
    applyStyles();
    
    // Load settings
    loadSettings();
    
    // Show welcome view
    showWelcomeView();
    
    // Start status update timer
    m_statusUpdateTimer->start(1000); // Update every second
}

MainWindow::~MainWindow() {
    saveSettings();
}

void MainWindow::setupUI() {
    // Create main splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(m_mainSplitter);
    
    setupNavigationPanel();
    setupCentralWidget();
    
    // Set splitter ratios
    m_mainSplitter->setSizes({NAVIGATION_WIDTH, width() - NAVIGATION_WIDTH});
    m_mainSplitter->setCollapsible(0, false); // Navigation panel not collapsible
}

void MainWindow::setupNavigationPanel() {
    m_navigationList = new QListWidget();
    m_navigationList->setFixedWidth(NAVIGATION_WIDTH);
    m_navigationList->setFrameStyle(QFrame::NoFrame);
    
    // Add navigation items
    QStringList navigationItems = {
        "🏠 Welcome",
        "📦 Pack",
        "📂 Extract",
        "🗂️ Browse",
        "⚙️ Settings"
    };
    
    for (const QString& item : navigationItems) {
        m_navigationList->addItem(item);
    }
    
    // Set default selected item
    m_navigationList->setCurrentRow(WelcomeViewIndex);
    
    m_mainSplitter->addWidget(m_navigationList);
}

void MainWindow::setupCentralWidget() {
    m_contentStack = new QStackedWidget();
    
    // Create individual views
    m_welcomeView = std::make_unique<WelcomeView>();
    m_packView = std::make_unique<PackView>();
    m_extractView = std::make_unique<ExtractView>();
    m_archiveBrowserView = std::make_unique<ArchiveBrowserView>();
    m_settingsView = std::make_unique<SettingsView>();
    
    // Add to stack
    m_contentStack->addWidget(m_welcomeView.get());
    m_contentStack->addWidget(m_packView.get());
    m_contentStack->addWidget(m_extractView.get());
    m_contentStack->addWidget(m_archiveBrowserView.get());
    m_contentStack->addWidget(m_settingsView.get());
    
    m_mainSplitter->addWidget(m_contentStack);
}

void MainWindow::setupMenuBar() {
    // File menu
    m_fileMenu = menuBar()->addMenu("&File");
    
    m_newArchiveAction = new QAction("&New Archive...", this);
    m_newArchiveAction->setShortcut(QKeySequence::New);
    m_newArchiveAction->setIcon(QIcon(":/icons/new.png"));
    m_fileMenu->addAction(m_newArchiveAction);
    
    m_openArchiveAction = new QAction("&Open Archive...", this);
    m_openArchiveAction->setShortcut(QKeySequence::Open);
    m_openArchiveAction->setIcon(QIcon(":/icons/open.png"));
    m_fileMenu->addAction(m_openArchiveAction);
    
    m_fileMenu->addSeparator();
    
    m_extractArchiveAction = new QAction("&Extract Archive...", this);
    m_extractArchiveAction->setShortcut(QKeySequence("Ctrl+E"));
    m_extractArchiveAction->setIcon(QIcon(":/icons/extract.png"));
    m_fileMenu->addAction(m_extractArchiveAction);
    
    m_fileMenu->addSeparator();
    
    m_exitAction = new QAction("E&xit", this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_fileMenu->addAction(m_exitAction);
    
    // Tools menu
    m_toolsMenu = menuBar()->addMenu("&Tools");
    
    m_settingsAction = new QAction("&Settings...", this);
    m_settingsAction->setShortcut(QKeySequence::Preferences);
    m_settingsAction->setIcon(QIcon(":/icons/settings.png"));
    m_toolsMenu->addAction(m_settingsAction);
    
    // View menu
    m_viewMenu = menuBar()->addMenu("&View");
    
    // Theme submenu
    QMenu* themeMenu = m_viewMenu->addMenu("Theme");
    QAction* lightThemeAction = themeMenu->addAction("Light Theme");
    QAction* darkThemeAction = themeMenu->addAction("Dark Theme");
    QAction* autoThemeAction = themeMenu->addAction("Follow System");
    
    // Help menu
    m_helpMenu = menuBar()->addMenu("&Help");
    
    m_aboutAction = new QAction("&About Flux...", this);
    m_helpMenu->addAction(m_aboutAction);
    
    QAction* aboutQtAction = new QAction("About &Qt...", this);
    m_helpMenu->addAction(aboutQtAction);
    connect(aboutQtAction, &QAction::triggered, qApp, &QApplication::aboutQt);
}

void MainWindow::setupToolBar() {
    m_mainToolBar = addToolBar("Main Toolbar");
    m_mainToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    
    m_mainToolBar->addAction(m_newArchiveAction);
    m_mainToolBar->addAction(m_openArchiveAction);
    m_mainToolBar->addSeparator();
    m_mainToolBar->addAction(m_extractArchiveAction);
    m_mainToolBar->addSeparator();
    m_mainToolBar->addAction(m_settingsAction);
}

void MainWindow::setupStatusBar() {
    m_statusLabel = new QLabel("Ready");
    statusBar()->addWidget(m_statusLabel);
    
    statusBar()->addPermanentWidget(new QLabel("|"));
    
    m_taskLabel = new QLabel();
    statusBar()->addPermanentWidget(m_taskLabel);
    
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    m_progressBar->setMaximumWidth(200);
    statusBar()->addPermanentWidget(m_progressBar);
    
    // Version information
    QLabel* versionLabel = new QLabel(QString("v%1").arg(QApplication::applicationVersion()));
    versionLabel->setStyleSheet("color: gray;");
    statusBar()->addPermanentWidget(versionLabel);
}

void MainWindow::setupConnections() {
    // Navigation connections
    connect(m_navigationList, &QListWidget::currentRowChanged,
            this, &MainWindow::onNavigationItemClicked);
    
    // Menu action connections
    connect(m_newArchiveAction, &QAction::triggered, this, &MainWindow::onNewArchive);
    connect(m_openArchiveAction, &QAction::triggered, this, &MainWindow::onOpenArchive);
    connect(m_extractArchiveAction, &QAction::triggered, this, &MainWindow::onExtractArchive);
    connect(m_settingsAction, &QAction::triggered, this, &MainWindow::onSettings);
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
    connect(m_exitAction, &QAction::triggered, this, &MainWindow::onExit);
    
    // Task manager connections
    connect(m_taskManager.get(), &TaskManager::taskStarted,
            this, &MainWindow::onTaskStarted);
    connect(m_taskManager.get(), &TaskManager::taskProgress,
            this, &MainWindow::onTaskProgress);
    connect(m_taskManager.get(), &TaskManager::taskFinished,
            this, &MainWindow::onTaskFinished);
    
    // Status update timer
    connect(m_statusUpdateTimer, &QTimer::timeout, this, &MainWindow::updateStatusBar);
    
    // Theme manager connections
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &MainWindow::onThemeChanged);
}

void MainWindow::applyStyles() {
    // Apply modern styles
    QString styleSheet = R"(
        QMainWindow {
            background-color: #f5f5f5;
        }
        
        QListWidget {
            background-color: #ffffff;
            border: none;
            border-right: 1px solid #e0e0e0;
            font-size: 14px;
            padding: 8px 0px;
        }
        
        QListWidget::item {
            padding: 12px 16px;
            border-bottom: 1px solid #f0f0f0;
        }
        
        QListWidget::item:selected {
            background-color: #007acc;
            color: white;
        }
        
        QListWidget::item:hover {
            background-color: #e3f2fd;
        }
        
        QToolBar {
            background-color: #ffffff;
            border-bottom: 1px solid #e0e0e0;
            spacing: 8px;
            padding: 4px;
        }
        
        QStatusBar {
            background-color: #ffffff;
            border-top: 1px solid #e0e0e0;
        }
    )";
    
    setStyleSheet(styleSheet);
}

void MainWindow::onNavigationItemClicked(int index) {
    switchToView(index);
}

void MainWindow::switchToView(int viewIndex) {
    if (viewIndex < 0 || viewIndex >= m_contentStack->count()) {
        return;
    }
    
    m_currentViewIndex = viewIndex;
    m_contentStack->setCurrentIndex(viewIndex);
    
    // Update status bar
    const QStringList viewNames = {"Welcome", "Pack", "Extract", "Browse", "Settings"};
    if (viewIndex < viewNames.size()) {
        m_statusLabel->setText(QString("Current view: %1").arg(viewNames[viewIndex]));
    }
}

void MainWindow::showWelcomeView() {
    m_navigationList->setCurrentRow(WelcomeViewIndex);
    switchToView(WelcomeViewIndex);
}

void MainWindow::showPackView() {
    m_navigationList->setCurrentRow(PackViewIndex);
    switchToView(PackViewIndex);
}

void MainWindow::showExtractView() {
    m_navigationList->setCurrentRow(ExtractViewIndex);
    switchToView(ExtractViewIndex);
}

void MainWindow::showArchiveBrowserView() {
    m_navigationList->setCurrentRow(ArchiveBrowserViewIndex);
    switchToView(ArchiveBrowserViewIndex);
}

void MainWindow::showSettingsView() {
    m_navigationList->setCurrentRow(SettingsViewIndex);
    switchToView(SettingsViewIndex);
}

void MainWindow::onNewArchive() {
    showPackView();
}

void MainWindow::onOpenArchive() {
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Open Archive File",
        QString(),
        "Archive Files (*.zip *.7z *.tar.gz *.tar.xz *.tar.zst);;All Files (*.*)"
    );
    
    if (!filePath.isEmpty()) {
        openArchive(filePath);
    }
}

void MainWindow::onExtractArchive() {
    showExtractView();
}

void MainWindow::onSettings() {
    showSettingsView();
}

void MainWindow::onAbout() {
    QMessageBox::about(this, "About Flux",
        QString("<h2>Flux Archive Tool</h2>"
                "<p>Version: %1</p>"
                "<p>Modern cross-platform archive tool</p>"
                "<p>Built with C++20 and Qt 6</p>"
                "<p>Supports multiple archive formats with high-performance compression and extraction.</p>"
                "<p>Copyright © 2024 Flux Team</p>")
        .arg(QApplication::applicationVersion()));
}

void MainWindow::onExit() {
    close();
}

void MainWindow::openArchive(const QString& filePath) {
    if (!QFile::exists(filePath)) {
        QMessageBox::warning(this, "Error", "File does not exist: " + filePath);
        return;
    }
    
    if (!isArchiveFile(filePath)) {
        QMessageBox::warning(this, "Error", "Unsupported archive format: " + filePath);
        return;
    }
    
    m_currentArchivePath = filePath;
    
    // Switch to archive browser view
    showArchiveBrowserView();
    
    // Notify archive browser view to load file
    m_archiveBrowserView->loadArchive(filePath);
}

bool MainWindow::isArchiveFile(const QString& filePath) {
    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();
    QString fileName = fileInfo.fileName().toLower();
    
    return suffix == "zip" || suffix == "7z" ||
           fileName.endsWith(".tar.gz") || fileName.endsWith(".tgz") ||
           fileName.endsWith(".tar.xz") || fileName.endsWith(".txz") ||
           fileName.endsWith(".tar.zst") || fileName.endsWith(".tar.zstd");
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event) {
    QStringList filePaths;
    for (const QUrl& url : event->mimeData()->urls()) {
        if (url.isLocalFile()) {
            filePaths << url.toLocalFile();
        }
    }
    
    if (!filePaths.isEmpty()) {
        handleDroppedFiles(filePaths);
    }
    
    event->acceptProposedAction();
}

void MainWindow::handleDroppedFiles(const QStringList& filePaths) {
    // Check if there are archive files
    QStringList archiveFiles;
    QStringList otherFiles;
    
    for (const QString& filePath : filePaths) {
        if (isArchiveFile(filePath)) {
            archiveFiles << filePath;
        } else {
            otherFiles << filePath;
        }
    }
    
    if (archiveFiles.size() == 1 && otherFiles.isEmpty()) {
        // Single archive file - open for browsing
        openArchive(archiveFiles.first());
    } else if (!otherFiles.isEmpty()) {
        // Has other files - switch to pack view
        showPackView();
        m_packView->addFiles(filePaths);
    } else if (archiveFiles.size() > 1) {
        // Multiple archive files - ask user for action
        QMessageBox::information(this, "Multiple Archive Files", 
            "Multiple archive files detected. Please open each file individually for browsing.");
    }
}

void MainWindow::onTaskStarted(const QString& taskName) {
    m_taskLabel->setText(taskName);
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
}

void MainWindow::onTaskProgress(int percentage, const QString& status) {
    m_progressBar->setValue(percentage);
    if (!status.isEmpty()) {
        m_statusLabel->setText(status);
    }
}

void MainWindow::onTaskFinished(bool success, const QString& message) {
    m_progressBar->setVisible(false);
    m_taskLabel->clear();
    
    if (success) {
        m_statusLabel->setText("Operation completed: " + message);
    } else {
        m_statusLabel->setText("Operation failed: " + message);
        QMessageBox::warning(this, "Operation Failed", message);
    }
}

void MainWindow::onThemeChanged() {
    applyStyles();
}

void MainWindow::updateStatusBar() {
    // Periodically update status bar information
    // Here you can add memory usage, task status and other information
}

void MainWindow::closeEvent(QCloseEvent *event) {
    // Check if there are active tasks
    if (m_taskManager->hasActiveTasks()) {
        int ret = QMessageBox::question(this, "Confirm Exit",
            "Tasks are currently running. Are you sure you want to exit?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        
        if (ret == QMessageBox::No) {
            event->ignore();
            return;
        }
        
        // Cancel all tasks
        m_taskManager->cancelAllTasks();
    }
    
    saveSettings();
    event->accept();
}

void MainWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::WindowStateChange) {
        // Handle window state changes
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::loadSettings() {
    QSettings settings;
    
    // Restore window geometry
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    
    // Restore splitter state
    if (m_mainSplitter) {
        m_mainSplitter->restoreState(settings.value("splitterState").toByteArray());
    }
    
    // Restore last view
    int lastView = settings.value("lastView", WelcomeViewIndex).toInt();
    switchToView(lastView);
}

void MainWindow::saveSettings() {
    QSettings settings;
    
    // Save window geometry
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    
    // Save splitter state
    if (m_mainSplitter) {
        settings.setValue("splitterState", m_mainSplitter->saveState());
    }
    
    // Save current view
    settings.setValue("lastView", m_currentViewIndex);
}





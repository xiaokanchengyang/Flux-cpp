#include "main_window.h"
#include "views/welcome_view.h"
#include "views/pack_view.h"
#include "views/extract_view.h"
#include "views/browse_view.h"
#include "views/browse_page.h"
#include "views/settings_page.h"

#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QAction>
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>
#include <QFileInfo>
#include <QLabel>
#include <QStackedWidget>
#include <QListWidget>
#include <QListWidgetItem>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QProgressBar>
#include <QTimer>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QIODevice>
#include <array>

// Temporary page classes - will be replaced by real pages later
class HomePage : public QWidget {
    Q_OBJECT
public:
    HomePage(QWidget* parent = nullptr) : QWidget(parent) {
        auto layout = new QVBoxLayout(this);
        auto label = new QLabel("🏠 Welcome to Flux Archive Manager", this);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("font-size: 24px; font-weight: bold; color: #2c3e50; margin: 50px;");
        layout->addWidget(label);
        
        auto descLabel = new QLabel("Modern archive management tool\nSupports ZIP, TAR, 7Z and other formats", this);
        descLabel->setAlignment(Qt::AlignCenter);
        descLabel->setStyleSheet("font-size: 16px; color: #7f8c8d; margin: 20px;");
        layout->addWidget(descLabel);
    }
};

class PackPage : public QWidget {
    Q_OBJECT
public:
    PackPage(QWidget* parent = nullptr) : QWidget(parent) {
        auto layout = new QVBoxLayout(this);
        auto label = new QLabel("📦 Pack Archive", this);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("font-size: 24px; font-weight: bold; color: #27ae60; margin: 50px;");
        layout->addWidget(label);
        
        auto descLabel = new QLabel("Create new archive files\nSupports multiple compression formats and levels", this);
        descLabel->setAlignment(Qt::AlignCenter);
        descLabel->setStyleSheet("font-size: 16px; color: #7f8c8d; margin: 20px;");
        layout->addWidget(descLabel);
    }
};

// BrowsePage and SettingsPage are now defined in separate files

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_menuBar(nullptr)
    , m_toolBar(nullptr)
    , m_statusBar(nullptr)
    , m_centralWidget(nullptr)
    , m_mainLayout(nullptr)
    , m_splitter(nullptr)
    , m_sidebarWidget(nullptr)
    , m_navigationList(nullptr)
    , m_sidebarLayout(nullptr)
    , m_stackedWidget(nullptr)
    , m_statusLabel(nullptr)
    , m_progressBar(nullptr)
    , m_taskLabel(nullptr)
    , m_homePage(nullptr)
    , m_packPage(nullptr)
    , m_browsePage(nullptr)
    , m_settingsPage(nullptr)
    , m_workerThread(nullptr)
    , m_isDarkTheme(false)
{
    setupUI();
    setupWorkerThread();
    showView(static_cast<int>(ViewIndex::Home));
    
    // Set window properties
    setWindowTitle("Flux Archive Manager");
    setMinimumSize(1000, 700);
    resize(1200, 800);
    
    // Center the window
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    move(screenGeometry.center() - rect().center());
    
    // Load stylesheet
    loadStyleSheet();
    
    // Setup window effects
    setupWindowEffects();
}

MainWindow::~MainWindow() {
    if (m_workerThread) {
        m_workerThread->stopTask();
        m_workerThread->wait(3000); // Wait up to 3 seconds
        if (m_workerThread->isRunning()) {
            m_workerThread->terminate();
            m_workerThread->wait(1000);
        }
        // unique_ptr will automatically delete
    }
}

void MainWindow::setupUI() {
    setupMenus();
    setupToolBar();
    setupStatusBar();
    setupCentralWidget();
}

void MainWindow::setupMenus() {
    m_menuBar = menuBar();
    
    // File menu
    QMenu* fileMenu = m_menuBar->addMenu("&File");
    
    QAction* newAction = fileMenu->addAction("&New Archive");
    newAction->setShortcut(QKeySequence::New);
    newAction->setIcon(QIcon(":/icons/new.png"));
    connect(newAction, &QAction::triggered, this, &MainWindow::onNewArchive);
    
    QAction* openAction = fileMenu->addAction("&Open Archive");
    openAction->setShortcut(QKeySequence::Open);
    openAction->setIcon(QIcon(":/icons/open.png"));
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenArchive);
    
    fileMenu->addSeparator();
    
    QAction* extractAction = fileMenu->addAction("&Extract Archive");
    extractAction->setShortcut(QKeySequence("Ctrl+E"));
    extractAction->setIcon(QIcon(":/icons/extract.png"));
    connect(extractAction, &QAction::triggered, this, &MainWindow::onExtractArchive);
    
    fileMenu->addSeparator();
    
    QAction* settingsAction = fileMenu->addAction("&Settings");
    settingsAction->setShortcut(QKeySequence::Preferences);
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onSettings);
    
    fileMenu->addSeparator();
    
    QAction* exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    
    // View menu
    QMenu* viewMenu = m_menuBar->addMenu("&View");
    
    QAction* homeAction = viewMenu->addAction("&Home");
    homeAction->setShortcut(QKeySequence("Ctrl+H"));
    connect(homeAction, &QAction::triggered, [this]() { showView(static_cast<int>(ViewIndex::Home)); });
    
    QAction* packAction = viewMenu->addAction("&Pack");
    packAction->setShortcut(QKeySequence("Ctrl+P"));
    connect(packAction, &QAction::triggered, [this]() { showView(static_cast<int>(ViewIndex::Pack)); });
    
    QAction* browseAction = viewMenu->addAction("&Browse");
    browseAction->setShortcut(QKeySequence("Ctrl+B"));
    connect(browseAction, &QAction::triggered, [this]() { showView(static_cast<int>(ViewIndex::Browse)); });
    
    // Help menu
    QMenu* helpMenu = m_menuBar->addMenu("&Help");
    
    QAction* aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::setupToolBar() {
    m_toolBar = addToolBar("Main Toolbar");
    m_toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_toolBar->setMovable(false);
    
    // New archive
    QAction* newAction = m_toolBar->addAction(QIcon(":/icons/new.png"), "New");
    connect(newAction, &QAction::triggered, this, &MainWindow::onNewArchive);
    
    // Open archive
    QAction* openAction = m_toolBar->addAction(QIcon(":/icons/open.png"), "Open");
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenArchive);
    
    // Extract archive
    QAction* extractAction = m_toolBar->addAction(QIcon(":/icons/extract.png"), "Extract");
    connect(extractAction, &QAction::triggered, this, &MainWindow::onExtractArchive);
    
    m_toolBar->addSeparator();
    
    // Return to home
    QAction* homeAction = m_toolBar->addAction(QIcon(":/icons/home.png"), "Home");
    connect(homeAction, &QAction::triggered, [this]() { showView(static_cast<int>(ViewIndex::Home)); });
}

void MainWindow::setupStatusBar() {
    m_statusBar = statusBar();
    
    // Status label
    m_statusLabel = new QLabel("Ready");
    m_statusBar->addWidget(m_statusLabel);
    
    // Task label
    m_taskLabel = new QLabel("");
    m_taskLabel->setVisible(false);
    m_statusBar->addWidget(m_taskLabel);
    
    // Progress bar
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    m_progressBar->setMaximumWidth(200);
    m_statusBar->addWidget(m_progressBar);
    
    // Version info
    QLabel* versionLabel = new QLabel(QString("v%1").arg(QApplication::applicationVersion()));
    versionLabel->setStyleSheet("color: #7f8c8d;");
    m_statusBar->addPermanentWidget(versionLabel);
}

void MainWindow::setupCentralWidget() {
    // Create central widget
    m_centralWidget = new QWidget();
    setCentralWidget(m_centralWidget);
    
    // Create main layout
    m_mainLayout = new QHBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    // Create splitter
    m_splitter = new QSplitter(Qt::Horizontal);
    m_mainLayout->addWidget(m_splitter);
    
    // Setup sidebar
    setupSidebar();
    
    // Create main view area
    m_stackedWidget = new QStackedWidget();
    
    // Create pages
    m_homePage = std::make_unique<HomePage>();
    m_packPage = std::make_unique<PackPage>();
    m_browsePage = std::make_unique<::BrowsePage>(); // Use the real BrowsePage
    m_settingsPage = std::make_unique<SettingsPage>();
    
    // Add to stack
    m_stackedWidget->addWidget(m_homePage.get());      // ViewIndex::Home = 0
    m_stackedWidget->addWidget(m_packPage.get());      // ViewIndex::Pack = 1
    m_stackedWidget->addWidget(m_browsePage.get());    // ViewIndex::Browse = 2
    m_stackedWidget->addWidget(m_settingsPage.get());  // ViewIndex::Settings = 3
    
    // Add to splitter
    m_splitter->addWidget(m_sidebarWidget);
    m_splitter->addWidget(m_stackedWidget);
    
    // Set splitter ratios
    m_splitter->setSizes({200, 800});
    m_splitter->setCollapsible(0, false); // Sidebar not collapsible
}

void MainWindow::setupSidebar() {
    // 创建侧边栏部件
    m_sidebarWidget = new QWidget();
    m_sidebarWidget->setFixedWidth(200);
    m_sidebarWidget->setStyleSheet(
        "QWidget {"
        "    background-color: #f8f9fa;"
        "    border-right: 1px solid #dee2e6;"
        "}"
    );
    
    // Create sidebar layout
    m_sidebarLayout = new QVBoxLayout(m_sidebarWidget);
    m_sidebarLayout->setContentsMargins(0, 10, 0, 10);
    m_sidebarLayout->setSpacing(0);
    
    // Create navigation list
    m_navigationList = new QListWidget();
    m_navigationList->setFrameShape(QFrame::NoFrame);
    m_navigationList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_navigationList->setStyleSheet(
        "QListWidget {"
        "    background-color: transparent;"
        "    border: none;"
        "    outline: none;"
        "}"
        "QListWidget::item {"
        "    padding: 12px 20px;"
        "    border: none;"
        "    font-size: 14px;"
        "    font-weight: 500;"
        "}"
        "QListWidget::item:hover {"
        "    background-color: #e9ecef;"
        "}"
        "QListWidget::item:selected {"
        "    background-color: #007bff;"
        "    color: white;"
        "}"
    );
    
    // Add navigation items
    QStringList navigationItems = {"🏠 Home", "📦 Pack", "🔍 Browse", "⚙️ Settings"};
    for (const QString& item : navigationItems) {
        QListWidgetItem* listItem = new QListWidgetItem(item);
        listItem->setSizeHint(QSize(180, 45));
        m_navigationList->addItem(listItem);
    }
    
    // Set default selected item
    m_navigationList->setCurrentRow(0);
    
    // Connect signals
    connect(m_navigationList, &QListWidget::currentRowChanged,
            this, &MainWindow::onNavigationItemClicked);
    
    // Add to layout
    m_sidebarLayout->addWidget(m_navigationList);
    m_sidebarLayout->addStretch();
}

void MainWindow::setupWorkerThread() {
    m_workerThread = std::make_unique<WorkerThread>(this);
    
    // Connect signals to slots, using queued connection to ensure thread safety
    connect(m_workerThread.get(), &WorkerThread::progressUpdated,
            this, &MainWindow::onProgressUpdated, Qt::QueuedConnection);
    connect(m_workerThread.get(), &WorkerThread::taskFinished,
            this, &MainWindow::onTaskFinished, Qt::QueuedConnection);
    connect(m_workerThread.get(), &WorkerThread::taskStarted,
            this, &MainWindow::onTaskStarted, Qt::QueuedConnection);
}

void MainWindow::showView(int index) {
    if (index >= 0 && index < m_stackedWidget->count()) {
        m_stackedWidget->setCurrentIndex(index);
        m_navigationList->setCurrentRow(index);
        
        // Update status bar using constexpr array
        constexpr std::array statusMessages = {
            "Welcome to Flux Archive Manager",
            "Create new archive file", 
            "Browse archive contents",
            "Application settings"
        };
        
        if (index >= 0 && index < static_cast<int>(statusMessages.size())) {
            updateStatusMessage(statusMessages[index]);
        }
    }
}

void MainWindow::updateStatusMessage(const QString& message) {
    m_statusLabel->setText(message);
}

void MainWindow::loadStyleSheet() {
    // Load stylesheet from resource file
    QFile styleFile(":/theme.qss");
    if (styleFile.open(QIODevice::ReadOnly)) {
        QString styleSheet = QString::fromUtf8(styleFile.readAll());
        setStyleSheet(styleSheet);
        styleFile.close();
    } else {
        // If unable to load resource file, use basic style
        QString fallbackStyle = R"(
            QMainWindow { background-color: #ffffff; }
            QMenuBar { background-color: #f8f9fa; border-bottom: 1px solid #dee2e6; }
            QToolBar { background-color: #f8f9fa; border-bottom: 1px solid #dee2e6; }
            QStatusBar { background-color: #f8f9fa; border-top: 1px solid #dee2e6; }
        )";
        setStyleSheet(fallbackStyle);
    }
}

void MainWindow::setupWindowEffects() {
    // Set window properties to support modern effects
    setAttribute(Qt::WA_TranslucentBackground, false); // Temporarily disable transparent background
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint); // Optional: frameless window
    setWindowFlags(windowFlags() & ~Qt::FramelessWindowHint); // Restore frame
}

// Slot function implementations
void MainWindow::onNavigationItemClicked(int index) {
    showView(index);
}

void MainWindow::onNewArchive() {
    showView(static_cast<int>(ViewIndex::Pack));
}

void MainWindow::onOpenArchive() {
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Open Archive File",
        QString(),
        "Archive Files (*.zip *.7z *.tar.gz *.tar.xz *.tar.zst);;All Files (*.*)"
    );
    
    if (!filePath.isEmpty()) {
        showView(static_cast<int>(ViewIndex::Browse));
        updateStatusMessage(QString("Opened: %1").arg(QFileInfo(filePath).fileName()));
        
        // Add to recent files
        QSettings settings;
        QStringList recentFiles = settings.value("recentFiles").toStringList();
        recentFiles.removeAll(filePath);
        recentFiles.prepend(filePath);
        while (recentFiles.size() > 10) {
            recentFiles.removeLast();
        }
        settings.setValue("recentFiles", recentFiles);
        
        // Start background task to list archive contents
        QVariantMap params;
        params["archivePath"] = filePath;
        m_workerThread->startTask(WorkerThread::WorkerThread::TaskType::List, params);
    }
}

void MainWindow::onExtractArchive() {
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Select Archive File to Extract",
        QString(),
        "Archive Files (*.zip *.7z *.tar.gz *.tar.xz *.tar.zst);;All Files (*.*)"
    );
    
    if (!filePath.isEmpty()) {
        QString outputDir = QFileDialog::getExistingDirectory(
            this,
            "Select Extract Directory",
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
        );
        
        if (!outputDir.isEmpty()) {
            // Start background extract task
            QVariantMap params;
            params["archivePath"] = filePath;
            params["outputPath"] = outputDir;
            m_workerThread->startTask(WorkerThread::WorkerThread::TaskType::Extract, params);
        }
    }
}

void MainWindow::onAbout() {
    QMessageBox::about(this, "About Flux Archive Manager",
        QString("<h3>Flux Archive Manager</h3>"
                "<p>Version: %1</p>"
                "<p>Modern archive file management tool</p>"
                "<p>✨ <b>Features:</b></p>"
                "<ul>"
                "<li>🚀 Smooth user interface</li>"
                "<li>💪 Powerful archive processing capabilities</li>"
                "<li>🎨 Beautiful modern design</li>"
                "<li>📊 Smart compression benchmarking</li>"
                "</ul>"
                "<p>Supports ZIP, 7Z, TAR.GZ, TAR.XZ, TAR.ZSTD and other formats</p>"
                "<p>Built with Qt 6 and C++20</p>")
        .arg(QApplication::applicationVersion()));
}

void MainWindow::onSettings() {
    showView(static_cast<int>(ViewIndex::Settings));
}

void MainWindow::onProgressUpdated(const QString& currentFile, float percentage) {
    m_progressBar->setValue(static_cast<int>(percentage * 100));
    m_taskLabel->setText(QString("Processing: %1").arg(currentFile));
    
    if (!m_progressBar->isVisible()) {
        m_progressBar->setVisible(true);
        m_taskLabel->setVisible(true);
    }
}

void MainWindow::onTaskFinished(bool success, const QString& message) {
    m_progressBar->setVisible(false);
    m_taskLabel->setVisible(false);
    
    if (success) {
        updateStatusMessage(message);
    } else {
        QMessageBox::warning(this, "Task Failed", message);
        updateStatusMessage("Task failed");
    }
}

void MainWindow::onTaskStarted(const QString& taskName) {
    updateStatusMessage(QString("Executing: %1").arg(taskName));
    m_progressBar->setValue(0);
    m_progressBar->setVisible(true);
    m_taskLabel->setText(taskName);
    m_taskLabel->setVisible(true);
}

void MainWindow::onRecentFileRequested(const QString& filePath) {
    QFileInfo fileInfo(filePath);
    if (fileInfo.exists()) {
        showView(static_cast<int>(ViewIndex::Browse));
        updateStatusMessage(QString("Opened: %1").arg(fileInfo.fileName()));
        
        // Start background task to list archive contents
        QVariantMap params;
        params["archivePath"] = filePath;
        m_workerThread->startTask(WorkerThread::WorkerThread::TaskType::List, params);
    } else {
        QMessageBox::warning(this, "File Not Found", 
                           QString("File %1 does not exist or has been moved.").arg(filePath));
        
        // Remove from recent files list
        QSettings settings;
        QStringList recentFiles = settings.value("recentFiles").toStringList();
        recentFiles.removeAll(filePath);
        settings.setValue("recentFiles", recentFiles);
    }
}

void MainWindow::onThemeChanged() {
    m_isDarkTheme = !m_isDarkTheme;
    loadStyleSheet();
}

// WorkerThread implementation
WorkerThread::WorkerThread(QObject* parent)
    : QThread(parent)
    , m_currentTask(WorkerThread::TaskType::Extract)
    , m_shouldStop(false)
{
}

void WorkerThread::startTask(TaskType type, const QVariantMap& parameters) {
    QMutexLocker locker(&m_mutex);
    
    if (isRunning()) {
        return; // Task already running
    }
    
    m_currentTask = type;
    m_taskParameters = parameters;
    m_shouldStop = false;
    
    start();
}

void WorkerThread::stopTask() {
    QMutexLocker locker(&m_mutex);
    m_shouldStop = true;
}

bool WorkerThread::isRunning() const {
    return QThread::isRunning();
}

void WorkerThread::run() {
    QString taskName;
    
    switch (m_currentTask) {
    case WorkerThread::TaskType::Extract:
        taskName = "Extract Archive";
        emit taskStarted(taskName);
        executeExtractTask(m_taskParameters);
        break;
    case WorkerThread::TaskType::Pack:
        taskName = "Create Archive";
        emit taskStarted(taskName);
        executePackTask(m_taskParameters);
        break;
    case WorkerThread::TaskType::List:
        taskName = "Read Archive Contents";
        emit taskStarted(taskName);
        executeListTask(m_taskParameters);
        break;
    case WorkerThread::TaskType::Benchmark:
        taskName = "Performance Benchmark";
        emit taskStarted(taskName);
        executeBenchmarkTask(m_taskParameters);
        break;
    }
}

void WorkerThread::executeExtractTask(const QVariantMap& params) {
    QString archivePath = params.value("archivePath").toString();
    QString outputPath = params.value("outputPath").toString();
    
    // Simulate extraction process
    QElapsedTimer timer;
    timer.start();
    
    for (int i = 0; i <= 100 && !m_shouldStop; ++i) {
        msleep(50); // Simulate work
        emit progressUpdated(QString("file_%1.txt").arg(i), i / 100.0f);
    }
    
    if (m_shouldStop) {
        emit taskFinished(false, "Task cancelled");
    } else {
        emit taskFinished(true, QString("Successfully extracted to %1 (Time: %2ms)")
                         .arg(outputPath).arg(timer.elapsed()));
    }
}

void WorkerThread::executePackTask(const QVariantMap& params) {
    QString outputPath = params.value("outputPath").toString();
    QStringList inputFiles = params.value("inputFiles").toStringList();
    
    // Simulate packing process
    QElapsedTimer timer;
    timer.start();
    
    for (int i = 0; i < inputFiles.size() && !m_shouldStop; ++i) {
        msleep(100); // Simulate work
        float progress = static_cast<float>(i + 1) / inputFiles.size();
        emit progressUpdated(inputFiles[i], progress);
    }
    
    if (m_shouldStop) {
        emit taskFinished(false, "Task cancelled");
    } else {
        emit taskFinished(true, QString("Successfully created archive %1 (Time: %2ms)")
                         .arg(outputPath).arg(timer.elapsed()));
    }
}

void WorkerThread::executeListTask(const QVariantMap& params) {
    QString archivePath = params.value("archivePath").toString();
    
    // Simulate reading archive contents
    msleep(500);
    
    if (m_shouldStop) {
        emit taskFinished(false, "Task cancelled");
        return;
    }
    
    // Simulate file list
    QStringList fileList;
    for (int i = 1; i <= 50; ++i) {
        fileList << QString("folder_%1/file_%2.txt").arg(i / 10 + 1).arg(i);
    }
    
    emit archiveListReady(fileList);
    emit taskFinished(true, QString("Successfully read archive contents, %1 files total").arg(fileList.size()));
}

void WorkerThread::executeBenchmarkTask(const QVariantMap& params) {
    QStringList inputFiles = params.value("inputFiles").toStringList();
    
    // Simulate benchmark test
    QVariantMap results;
    QStringList algorithms = {"ZIP-Deflate", "ZIP-Store", "ZSTD-1", "ZSTD-3", "ZSTD-5"};
    
    for (int i = 0; i < algorithms.size() && !m_shouldStop; ++i) {
        QString algorithm = algorithms[i];
        emit progressUpdated(QString("Testing %1").arg(algorithm), static_cast<float>(i) / algorithms.size());
        
        msleep(1000); // Simulate test time
        
        // Simulate result data
        QVariantMap algorithmResult;
        algorithmResult["compressionRatio"] = 0.3 + (i * 0.1);
        algorithmResult["compressionSpeed"] = 50 + (i * 10);
        algorithmResult["decompressionSpeed"] = 100 + (i * 20);
        
        results[algorithm] = algorithmResult;
    }
    
    if (m_shouldStop) {
        emit taskFinished(false, "Benchmark test cancelled");
    } else {
        emit benchmarkResultReady(results);
        emit taskFinished(true, "Benchmark test completed");
    }
}

#include "main_window.moc"
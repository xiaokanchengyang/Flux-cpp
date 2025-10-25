#include "enhanced_main_window.h"
#include <QApplication>
#include <QScreen>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QStyleFactory>

namespace FluxGUI::UI {

EnhancedMainWindow::EnhancedMainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_settings(new QSettings(this))
    , m_statusTimer(new QTimer(this))
    , m_themeDetectionTimer(new QTimer(this))
{
    setObjectName("EnhancedMainWindow");
    setWindowTitle(tr("Flux Archive Manager"));
    setWindowIcon(QIcon(":/icons/app.svg"));
    setMinimumSize(800, 600);
    
    // Enable drag and drop
    setAcceptDrops(true);
    
    // Initialize components
    initializeUI();
    initializeManagers();
    
    // Setup timers
    m_statusTimer->setSingleShot(true);
    connect(m_statusTimer, &QTimer::timeout, [this]() {
        if (m_statusLabel) {
            m_statusLabel->clear();
        }
    });
    
    m_themeDetectionTimer->setInterval(THEME_DETECTION_INTERVAL);
    connect(m_themeDetectionTimer, &QTimer::timeout,
            this, &EnhancedMainWindow::onSystemThemeChanged);
    
    // Load settings and apply theme
    loadSettings();
    detectSystemTheme();
    
    // Set initial view
    setViewMode(ViewMode::Welcome);
    
    // Setup accessibility
    setupAccessibility();
    
    // Show welcome message
    showNotification(tr("Welcome to Flux Archive Manager"), 
                    Components::VisualFeedbackManager::FeedbackType::Information);
}

EnhancedMainWindow::~EnhancedMainWindow() {
    saveSettings();
}

void EnhancedMainWindow::setViewMode(ViewMode mode) {
    if (m_currentViewMode == mode) return;
    
    ViewMode oldMode = m_currentViewMode;
    m_currentViewMode = mode;
    
    // Update navigation panel mode
    if (m_navigationPanel) {
        Components::SmartNavigationPanel::NavigationMode navMode;
        switch (mode) {
        case ViewMode::Welcome:
            navMode = Components::SmartNavigationPanel::NavigationMode::Welcome;
            break;
        case ViewMode::FileBrowser:
            navMode = Components::SmartNavigationPanel::NavigationMode::Creation;
            break;
        case ViewMode::ArchiveViewer:
            navMode = Components::SmartNavigationPanel::NavigationMode::Archive;
            break;
        case ViewMode::BatchOperations:
            navMode = Components::SmartNavigationPanel::NavigationMode::Settings;
            break;
        }
        m_navigationPanel->setMode(navMode);
    }
    
    // Switch view
    switch (mode) {
    case ViewMode::Welcome:
        switchToWelcomeView();
        break;
    case ViewMode::FileBrowser:
        switchToFileBrowserView();
        break;
    case ViewMode::ArchiveViewer:
        switchToArchiveViewerView();
        break;
    case ViewMode::BatchOperations:
        switchToBatchOperationsView();
        break;
    }
    
    updateViewSpecificUI();
    
    // Announce change for accessibility
    if (m_accessibilityManager) {
        announceViewChange(mode);
    }
    
    emit viewModeChanged(mode);
}

void EnhancedMainWindow::setTheme(Theme theme) {
    if (m_currentTheme == theme) return;
    
    m_currentTheme = theme;
    applyTheme(theme);
    
    emit themeChanged(theme);
}

void EnhancedMainWindow::openArchive(const QString& filePath) {
    if (filePath.isEmpty() || !QFileInfo::exists(filePath)) {
        showNotification(tr("Archive file not found: %1").arg(filePath),
                        Components::VisualFeedbackManager::FeedbackType::Error);
        return;
    }
    
    m_currentArchivePath = filePath;
    updateRecentFiles(filePath);
    
    // Switch to archive viewer
    setViewMode(ViewMode::ArchiveViewer);
    
    // Load archive content
    if (m_archiveView) {
        // This would integrate with the actual archive loading logic
        showNotification(tr("Loading archive: %1").arg(QFileInfo(filePath).fileName()),
                        Components::VisualFeedbackManager::FeedbackType::Information);
    }
    
    emit archiveOpened(filePath);
}

void EnhancedMainWindow::createArchive(const QStringList& files) {
    if (files.isEmpty()) {
        showNotification(tr("No files selected for archive creation"),
                        Components::VisualFeedbackManager::FeedbackType::Warning);
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Create Archive"),
        QString(),
        tr("Archive Files (*.zip *.7z *.tar.gz *.tar.bz2);;ZIP Files (*.zip);;7-Zip Files (*.7z);;TAR.GZ Files (*.tar.gz);;TAR.BZ2 Files (*.tar.bz2)")
    );
    
    if (fileName.isEmpty()) return;
    
    // Start progress indication
    if (m_feedbackManager) {
        m_feedbackManager->startProgress(tr("Creating archive"), true);
    }
    
    // This would integrate with the actual archive creation logic
    showNotification(tr("Creating archive: %1").arg(QFileInfo(fileName).fileName()),
                    Components::VisualFeedbackManager::FeedbackType::Information);
    
    emit archiveCreated(fileName);
}

void EnhancedMainWindow::extractArchive(const QString& archivePath, const QString& destination) {
    if (archivePath.isEmpty() || !QFileInfo::exists(archivePath)) {
        showNotification(tr("Archive file not found"),
                        Components::VisualFeedbackManager::FeedbackType::Error);
        return;
    }
    
    QString extractPath = destination;
    if (extractPath.isEmpty()) {
        extractPath = QFileDialog::getExistingDirectory(
            this,
            tr("Extract Archive To"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
        );
        
        if (extractPath.isEmpty()) return;
    }
    
    // Start progress indication
    if (m_feedbackManager) {
        m_feedbackManager->startProgress(tr("Extracting archive"), true);
    }
    
    showNotification(tr("Extracting to: %1").arg(extractPath),
                    Components::VisualFeedbackManager::FeedbackType::Information);
    
    emit archiveExtracted(archivePath, extractPath);
}

void EnhancedMainWindow::showBatchOperationsDialog() {
    if (!m_batchDialog) {
        m_batchDialog = std::make_unique<Dialogs::BatchOperationsDialog>(this);
        
        // Connect signals
        connect(m_batchDialog.get(), &Dialogs::BatchOperationsDialog::batchStarted,
                this, [this]() {
                    showNotification(tr("Batch operation started"),
                                   Components::VisualFeedbackManager::FeedbackType::Information);
                });
        
        connect(m_batchDialog.get(), &Dialogs::BatchOperationsDialog::batchCompleted,
                this, [this](bool success) {
                    if (success) {
                        showNotification(tr("Batch operation completed successfully"),
                                       Components::VisualFeedbackManager::FeedbackType::Success);
                    } else {
                        showNotification(tr("Batch operation completed with errors"),
                                       Components::VisualFeedbackManager::FeedbackType::Warning);
                    }
                });
    }
    
    m_batchDialog->show();
    m_batchDialog->raise();
    m_batchDialog->activateWindow();
}

void EnhancedMainWindow::closeEvent(QCloseEvent* event) {
    if (m_operationInProgress) {
        int result = QMessageBox::question(
            this,
            tr("Operation in Progress"),
            tr("An operation is currently in progress. Do you want to cancel it and exit?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        
        if (result == QMessageBox::No) {
            event->ignore();
            return;
        }
    }
    
    saveSettings();
    event->accept();
}

void EnhancedMainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    updateLayout();
}

void EnhancedMainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        
        // Visual feedback for drag operation
        if (m_feedbackManager) {
            // This would show a visual indicator for the drop zone
        }
    }
}

void EnhancedMainWindow::dropEvent(QDropEvent* event) {
    const QMimeData* mimeData = event->mimeData();
    if (!mimeData->hasUrls()) return;
    
    QStringList files;
    for (const QUrl& url : mimeData->urls()) {
        if (url.isLocalFile()) {
            files.append(url.toLocalFile());
        }
    }
    
    if (!files.isEmpty()) {
        handleDroppedFiles(files);
        event->acceptProposedAction();
    }
}

void EnhancedMainWindow::changeEvent(QEvent* event) {
    QMainWindow::changeEvent(event);
    
    if (event->type() == QEvent::StyleChange) {
        // Handle system theme changes
        onSystemThemeChanged();
    }
}

void EnhancedMainWindow::initializeUI() {
    createMenuBar();
    createToolBars();
    createStatusBar();
    createCentralWidget();
}

void EnhancedMainWindow::createMenuBar() {
    m_menuBar = menuBar();
    
    // File menu
    m_fileMenu = m_menuBar->addMenu(tr("&File"));
    
    m_newArchiveAction = m_fileMenu->addAction(QIcon(":/icons/add-circle.svg"), tr("&New Archive..."));
    m_newArchiveAction->setShortcut(QKeySequence::New);
    m_newArchiveAction->setStatusTip(tr("Create a new archive"));
    connect(m_newArchiveAction, &QAction::triggered, this, &EnhancedMainWindow::onNewArchiveAction);
    
    m_openArchiveAction = m_fileMenu->addAction(QIcon(":/icons/folder-open.svg"), tr("&Open Archive..."));
    m_openArchiveAction->setShortcut(QKeySequence::Open);
    m_openArchiveAction->setStatusTip(tr("Open an existing archive"));
    connect(m_openArchiveAction, &QAction::triggered, this, &EnhancedMainWindow::onOpenArchiveAction);
    
    m_fileMenu->addSeparator();
    
    m_extractArchiveAction = m_fileMenu->addAction(QIcon(":/icons/download.svg"), tr("&Extract Archive..."));
    m_extractArchiveAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E));
    m_extractArchiveAction->setStatusTip(tr("Extract the current archive"));
    connect(m_extractArchiveAction, &QAction::triggered, this, &EnhancedMainWindow::onExtractArchiveAction);
    
    m_fileMenu->addSeparator();
    
    m_exitAction = m_fileMenu->addAction(QIcon(":/icons/x.svg"), tr("E&xit"));
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setStatusTip(tr("Exit the application"));
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);
    
    // Edit menu
    m_editMenu = m_menuBar->addMenu(tr("&Edit"));
    
    m_addFilesAction = m_editMenu->addAction(QIcon(":/icons/plus.svg"), tr("&Add Files..."));
    m_addFilesAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_A));
    m_addFilesAction->setStatusTip(tr("Add files to the current archive"));
    connect(m_addFilesAction, &QAction::triggered, this, &EnhancedMainWindow::onAddFilesAction);
    
    m_removeFilesAction = m_editMenu->addAction(QIcon(":/icons/minus.svg"), tr("&Remove Files"));
    m_removeFilesAction->setShortcut(QKeySequence::Delete);
    m_removeFilesAction->setStatusTip(tr("Remove selected files from the archive"));
    connect(m_removeFilesAction, &QAction::triggered, this, &EnhancedMainWindow::onRemoveFilesAction);
    
    // View menu
    m_viewMenu = m_menuBar->addMenu(tr("&View"));
    
    // View mode submenu
    QMenu* viewModeMenu = m_viewMenu->addMenu(tr("View Mode"));
    m_viewModeGroup = new QActionGroup(this);
    
    m_welcomeViewAction = viewModeMenu->addAction(tr("Welcome"));
    m_welcomeViewAction->setCheckable(true);
    m_welcomeViewAction->setChecked(true);
    m_viewModeGroup->addAction(m_welcomeViewAction);
    connect(m_welcomeViewAction, &QAction::triggered, this, &EnhancedMainWindow::onWelcomeViewRequested);
    
    m_fileBrowserViewAction = viewModeMenu->addAction(tr("File Browser"));
    m_fileBrowserViewAction->setCheckable(true);
    m_viewModeGroup->addAction(m_fileBrowserViewAction);
    connect(m_fileBrowserViewAction, &QAction::triggered, this, &EnhancedMainWindow::onFileBrowserViewRequested);
    
    m_archiveViewerViewAction = viewModeMenu->addAction(tr("Archive Viewer"));
    m_archiveViewerViewAction->setCheckable(true);
    m_viewModeGroup->addAction(m_archiveViewerViewAction);
    connect(m_archiveViewerViewAction, &QAction::triggered, this, &EnhancedMainWindow::onArchiveViewerRequested);
    
    // Theme submenu
    QMenu* themeMenu = m_viewMenu->addMenu(tr("Theme"));
    m_themeGroup = new QActionGroup(this);
    
    m_autoThemeAction = themeMenu->addAction(tr("Auto"));
    m_autoThemeAction->setCheckable(true);
    m_autoThemeAction->setChecked(true);
    m_themeGroup->addAction(m_autoThemeAction);
    
    m_lightThemeAction = themeMenu->addAction(tr("Light"));
    m_lightThemeAction->setCheckable(true);
    m_themeGroup->addAction(m_lightThemeAction);
    
    m_darkThemeAction = themeMenu->addAction(tr("Dark"));
    m_darkThemeAction->setCheckable(true);
    m_themeGroup->addAction(m_darkThemeAction);
    
    m_highContrastThemeAction = themeMenu->addAction(tr("High Contrast"));
    m_highContrastThemeAction->setCheckable(true);
    m_themeGroup->addAction(m_highContrastThemeAction);
    
    connect(m_themeGroup, &QActionGroup::triggered, this, &EnhancedMainWindow::onThemeChanged);
    
    // Tools menu
    m_toolsMenu = m_menuBar->addMenu(tr("&Tools"));
    
    m_batchOperationsAction = m_toolsMenu->addAction(QIcon(":/icons/layers.svg"), tr("&Batch Operations..."));
    m_batchOperationsAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_B));
    m_batchOperationsAction->setStatusTip(tr("Perform batch operations on multiple archives"));
    connect(m_batchOperationsAction, &QAction::triggered, this, &EnhancedMainWindow::onBatchOperationsAction);
    
    m_toolsMenu->addSeparator();
    
    m_preferencesAction = m_toolsMenu->addAction(QIcon(":/icons/settings.svg"), tr("&Preferences..."));
    m_preferencesAction->setShortcut(QKeySequence::Preferences);
    m_preferencesAction->setStatusTip(tr("Configure application preferences"));
    connect(m_preferencesAction, &QAction::triggered, this, &EnhancedMainWindow::onPreferencesAction);
    
    // Help menu
    m_helpMenu = m_menuBar->addMenu(tr("&Help"));
    
    m_aboutAction = m_helpMenu->addAction(QIcon(":/icons/info.svg"), tr("&About"));
    m_aboutAction->setStatusTip(tr("Show information about the application"));
    connect(m_aboutAction, &QAction::triggered, this, &EnhancedMainWindow::onAboutAction);
}

void EnhancedMainWindow::createToolBars() {
    // Main toolbar
    m_mainToolBar = addToolBar(tr("Main"));
    m_mainToolBar->setObjectName("MainToolBar");
    m_mainToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    
    m_mainToolBar->addAction(m_newArchiveAction);
    m_mainToolBar->addAction(m_openArchiveAction);
    m_mainToolBar->addAction(m_extractArchiveAction);
    m_mainToolBar->addSeparator();
    m_mainToolBar->addAction(m_addFilesAction);
    m_mainToolBar->addAction(m_removeFilesAction);
    m_mainToolBar->addSeparator();
    m_mainToolBar->addAction(m_batchOperationsAction);
    
    // View toolbar
    m_viewToolBar = addToolBar(tr("View"));
    m_viewToolBar->setObjectName("ViewToolBar");
    m_viewToolBar->addActions(m_viewModeGroup->actions());
}

void EnhancedMainWindow::createStatusBar() {
    m_statusBar = statusBar();
    
    m_statusLabel = new QLabel(this);
    m_statusLabel->setMinimumWidth(200);
    m_statusBar->addWidget(m_statusLabel);
    
    m_statusBar->addPermanentWidget(new QLabel("|"));
    
    m_selectionLabel = new QLabel(this);
    m_selectionLabel->setMinimumWidth(150);
    m_statusBar->addPermanentWidget(m_selectionLabel);
    
    m_statusBar->addPermanentWidget(new QLabel("|"));
    
    m_progressLabel = new QLabel(this);
    m_progressLabel->setVisible(false);
    m_statusBar->addPermanentWidget(m_progressLabel);
    
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    m_progressBar->setMaximumWidth(200);
    m_statusBar->addPermanentWidget(m_progressBar);
}

void EnhancedMainWindow::createCentralWidget() {
    // Create main splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->setObjectName("MainSplitter");
    m_mainSplitter->setHandleWidth(SPLITTER_HANDLE_WIDTH);
    
    // Initialize navigation panel
    initializeNavigationPanel();
    
    // Create content area
    m_centralStack = new QStackedWidget(this);
    m_centralStack->setObjectName("CentralStack");
    
    // Create view widgets
    switchToWelcomeView();
    switchToFileBrowserView();
    switchToArchiveViewerView();
    switchToBatchOperationsView();
    
    // Add to splitter
    m_mainSplitter->addWidget(m_navigationPanel.get());
    m_mainSplitter->addWidget(m_centralStack);
    
    // Set splitter proportions
    m_mainSplitter->setSizes({NAVIGATION_PANEL_MIN_WIDTH, width() - NAVIGATION_PANEL_MIN_WIDTH});
    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 1);
    
    setCentralWidget(m_mainSplitter);
}

void EnhancedMainWindow::initializeNavigationPanel() {
    m_navigationPanel = std::make_unique<Components::SmartNavigationPanel>(this);
    
    // Connect navigation signals
    connect(m_navigationPanel.get(), &Components::SmartNavigationPanel::navigationItemClicked,
            this, &EnhancedMainWindow::onNavigationItemClicked);
    connect(m_navigationPanel.get(), &Components::SmartNavigationPanel::breadcrumbItemClicked,
            this, &EnhancedMainWindow::onBreadcrumbItemClicked);
    connect(m_navigationPanel.get(), &Components::SmartNavigationPanel::recentFileRequested,
            this, &EnhancedMainWindow::onRecentFileRequested);
    connect(m_navigationPanel.get(), &Components::SmartNavigationPanel::quickActionTriggered,
            this, &EnhancedMainWindow::onQuickActionTriggered);
    
    // Add quick actions
    QList<QAction*> quickActions = {
        m_newArchiveAction,
        m_openArchiveAction,
        m_extractArchiveAction,
        m_batchOperationsAction
    };
    m_navigationPanel->setQuickActions(quickActions);
}

void EnhancedMainWindow::initializeFileDisplay() {
    m_fileDisplay = std::make_unique<Components::RichFileDisplay>(this);
    
    // Connect file display signals
    connect(m_fileDisplay.get(), &Components::RichFileDisplay::selectionChanged,
            this, &EnhancedMainWindow::onFileSelectionChanged);
    connect(m_fileDisplay.get(), &Components::RichFileDisplay::fileDoubleClicked,
            this, &EnhancedMainWindow::onFileDoubleClicked);
    connect(m_fileDisplay.get(), &Components::RichFileDisplay::contextMenuRequested,
            this, &EnhancedMainWindow::onFileContextMenuRequested);
}

void EnhancedMainWindow::initializeArchiveView() {
    m_archiveView = std::make_unique<Components::VirtualizedArchiveView>(this);
    
    // Configure for optimal performance
    m_archiveView->setLazyLoadingEnabled(true);
    m_archiveView->setBackgroundLoadingEnabled(true);
    m_archiveView->setPredictiveLoadingEnabled(true);
    m_archiveView->setBufferSize(100);
}

void EnhancedMainWindow::initializeManagers() {
    // Initialize feedback manager
    m_feedbackManager = std::make_unique<Components::VisualFeedbackManager>(this);
    m_feedbackManager->setToastParent(this);
    m_feedbackManager->setToastPosition(Components::VisualFeedbackManager::ToastPosition::TopRight);
    
    // Initialize accessibility manager
    m_accessibilityManager = std::make_unique<Managers::AccessibilityManager>(this);
    connect(m_accessibilityManager.get(), &Managers::AccessibilityManager::settingsChanged,
            this, &EnhancedMainWindow::onAccessibilitySettingsChanged);
    
    // Initialize context menu manager
    m_contextMenuManager = std::make_unique<Managers::ContextMenuManager>(this);
    setupContextMenus();
}

void EnhancedMainWindow::switchToWelcomeView() {
    if (!m_welcomeView) {
        m_welcomeView = new QWidget(this);
        m_welcomeView->setObjectName("WelcomeView");
        
        QVBoxLayout* layout = new QVBoxLayout(m_welcomeView);
        layout->setAlignment(Qt::AlignCenter);
        
        QLabel* welcomeLabel = new QLabel(tr("Welcome to Flux Archive Manager"), m_welcomeView);
        welcomeLabel->setStyleSheet("font-size: 24px; font-weight: bold; margin: 20px;");
        welcomeLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(welcomeLabel);
        
        QLabel* descriptionLabel = new QLabel(tr("Create, open, and manage archive files with ease"), m_welcomeView);
        descriptionLabel->setStyleSheet("font-size: 14px; color: rgba(0,0,0,0.6); margin: 10px;");
        descriptionLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(descriptionLabel);
        
        m_centralStack->addWidget(m_welcomeView);
    }
    
    m_centralStack->setCurrentWidget(m_welcomeView);
}

void EnhancedMainWindow::switchToFileBrowserView() {
    if (!m_fileBrowserView) {
        m_fileBrowserView = new QWidget(this);
        m_fileBrowserView->setObjectName("FileBrowserView");
        
        QVBoxLayout* layout = new QVBoxLayout(m_fileBrowserView);
        layout->setContentsMargins(0, 0, 0, 0);
        
        // Initialize file display if not already done
        if (!m_fileDisplay) {
            initializeFileDisplay();
        }
        
        layout->addWidget(m_fileDisplay.get());
        m_centralStack->addWidget(m_fileBrowserView);
    }
    
    m_centralStack->setCurrentWidget(m_fileBrowserView);
}

void EnhancedMainWindow::switchToArchiveViewerView() {
    if (!m_archiveViewerView) {
        m_archiveViewerView = new QWidget(this);
        m_archiveViewerView->setObjectName("ArchiveViewerView");
        
        QVBoxLayout* layout = new QVBoxLayout(m_archiveViewerView);
        layout->setContentsMargins(0, 0, 0, 0);
        
        // Initialize archive view if not already done
        if (!m_archiveView) {
            initializeArchiveView();
        }
        
        layout->addWidget(m_archiveView.get());
        m_centralStack->addWidget(m_archiveViewerView);
    }
    
    m_centralStack->setCurrentWidget(m_archiveViewerView);
}

void EnhancedMainWindow::switchToBatchOperationsView() {
    if (!m_batchOperationsView) {
        m_batchOperationsView = new QWidget(this);
        m_batchOperationsView->setObjectName("BatchOperationsView");
        
        QVBoxLayout* layout = new QVBoxLayout(m_batchOperationsView);
        layout->setAlignment(Qt::AlignCenter);
        
        QLabel* label = new QLabel(tr("Batch Operations"), m_batchOperationsView);
        label->setStyleSheet("font-size: 18px; font-weight: bold;");
        label->setAlignment(Qt::AlignCenter);
        layout->addWidget(label);
        
        QPushButton* openBatchButton = new QPushButton(tr("Open Batch Operations Dialog"), m_batchOperationsView);
        connect(openBatchButton, &QPushButton::clicked, this, &EnhancedMainWindow::showBatchOperationsDialog);
        layout->addWidget(openBatchButton);
        
        m_centralStack->addWidget(m_batchOperationsView);
    }
    
    m_centralStack->setCurrentWidget(m_batchOperationsView);
}

void EnhancedMainWindow::updateViewSpecificUI() {
    // Update window title
    QString title = tr("Flux Archive Manager");
    switch (m_currentViewMode) {
    case ViewMode::Welcome:
        title += tr(" - Welcome");
        break;
    case ViewMode::FileBrowser:
        title += tr(" - File Browser");
        break;
    case ViewMode::ArchiveViewer:
        if (!m_currentArchivePath.isEmpty()) {
            title += tr(" - %1").arg(QFileInfo(m_currentArchivePath).fileName());
        } else {
            title += tr(" - Archive Viewer");
        }
        break;
    case ViewMode::BatchOperations:
        title += tr(" - Batch Operations");
        break;
    }
    setWindowTitle(title);
    
    // Update action states
    bool hasArchive = !m_currentArchivePath.isEmpty();
    bool hasSelection = !m_selectedFiles.isEmpty();
    
    m_extractArchiveAction->setEnabled(hasArchive);
    m_addFilesAction->setEnabled(hasArchive);
    m_removeFilesAction->setEnabled(hasArchive && hasSelection);
    
    // Update view mode actions
    switch (m_currentViewMode) {
    case ViewMode::Welcome:
        m_welcomeViewAction->setChecked(true);
        break;
    case ViewMode::FileBrowser:
        m_fileBrowserViewAction->setChecked(true);
        break;
    case ViewMode::ArchiveViewer:
        m_archiveViewerViewAction->setChecked(true);
        break;
    case ViewMode::BatchOperations:
        // No specific action for batch operations view
        break;
    }
}

void EnhancedMainWindow::applyTheme(Theme theme) {
    QString themeName;
    switch (theme) {
    case Theme::Auto:
        detectSystemTheme();
        return; // detectSystemTheme will call applyTheme again
    case Theme::Light:
        themeName = "light";
        break;
    case Theme::Dark:
        themeName = "dark";
        break;
    case Theme::HighContrast:
        themeName = "high-contrast";
        break;
    }
    
    loadThemeStylesheet(themeName);
    updateIconTheme();
    
    // Update theme actions
    switch (theme) {
    case Theme::Auto:
        m_autoThemeAction->setChecked(true);
        break;
    case Theme::Light:
        m_lightThemeAction->setChecked(true);
        break;
    case Theme::Dark:
        m_darkThemeAction->setChecked(true);
        break;
    case Theme::HighContrast:
        m_highContrastThemeAction->setChecked(true);
        break;
    }
}

void EnhancedMainWindow::loadThemeStylesheet(const QString& themeName) {
    QString styleSheetPath = QString(":/themes/%1.qss").arg(themeName);
    QFile file(styleSheetPath);
    
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&file);
        QString styleSheet = stream.readAll();
        qApp->setStyleSheet(styleSheet);
    }
}

void EnhancedMainWindow::detectSystemTheme() {
    // This is a simplified theme detection
    // In a real implementation, you would check system settings
    QPalette palette = qApp->palette();
    bool isDark = palette.color(QPalette::Window).lightness() < 128;
    
    Theme detectedTheme = isDark ? Theme::Dark : Theme::Light;
    if (m_currentTheme == Theme::Auto) {
        loadThemeStylesheet(detectedTheme == Theme::Dark ? "dark" : "light");
        updateIconTheme();
    }
}

void EnhancedMainWindow::handleDroppedFiles(const QStringList& files) {
    if (files.isEmpty()) return;
    
    // Check if any files are archives
    QStringList archiveFiles;
    QStringList regularFiles;
    
    for (const QString& file : files) {
        if (isArchiveFile(file)) {
            archiveFiles.append(file);
        } else {
            regularFiles.append(file);
        }
    }
    
    if (archiveFiles.size() == 1 && regularFiles.isEmpty()) {
        // Single archive file - open it
        openArchive(archiveFiles.first());
    } else if (!regularFiles.isEmpty()) {
        // Regular files - create archive
        createArchive(regularFiles);
    } else if (archiveFiles.size() > 1) {
        // Multiple archives - show batch operations
        showBatchOperationsDialog();
        // Add archives to batch dialog
        // This would require integration with the batch dialog
    }
}

void EnhancedMainWindow::showNotification(const QString& message, Components::VisualFeedbackManager::FeedbackType type) {
    if (m_feedbackManager) {
        m_feedbackManager->showToast(message, type);
    }
    
    // Also show in status bar
    if (m_statusLabel) {
        m_statusLabel->setText(message);
        m_statusTimer->start(STATUS_MESSAGE_TIMEOUT);
    }
}

void EnhancedMainWindow::setupAccessibility() {
    if (!m_accessibilityManager) return;
    
    // Register main window components
    m_accessibilityManager->registerWidget(this, "MainWindow", tr("Flux Archive Manager main window"));
    
    if (m_navigationPanel) {
        m_accessibilityManager->registerWidget(m_navigationPanel.get(), "NavigationPanel", tr("Navigation panel"));
    }
    
    if (m_centralStack) {
        m_accessibilityManager->registerWidget(m_centralStack, "ContentArea", tr("Main content area"));
    }
    
    // Set up keyboard shortcuts
    m_accessibilityManager->registerGlobalShortcut(QKeySequence(Qt::CTRL + Qt::Key_1), tr("Switch to Welcome view"), 
        [this]() { setViewMode(ViewMode::Welcome); });
    m_accessibilityManager->registerGlobalShortcut(QKeySequence(Qt::CTRL + Qt::Key_2), tr("Switch to File Browser"), 
        [this]() { setViewMode(ViewMode::FileBrowser); });
    m_accessibilityManager->registerGlobalShortcut(QKeySequence(Qt::CTRL + Qt::Key_3), tr("Switch to Archive Viewer"), 
        [this]() { setViewMode(ViewMode::ArchiveViewer); });
}

void EnhancedMainWindow::announceViewChange(ViewMode mode) {
    if (!m_accessibilityManager) return;
    
    QString message = tr("Switched to %1").arg(getViewModeTitle(mode));
    m_accessibilityManager->announce(message, Managers::AccessibilityManager::AnnouncementPriority::Medium);
}

QString EnhancedMainWindow::getViewModeTitle(ViewMode mode) const {
    switch (mode) {
    case ViewMode::Welcome:
        return tr("Welcome");
    case ViewMode::FileBrowser:
        return tr("File Browser");
    case ViewMode::ArchiveViewer:
        return tr("Archive Viewer");
    case ViewMode::BatchOperations:
        return tr("Batch Operations");
    }
    return QString();
}

bool EnhancedMainWindow::isArchiveFile(const QString& filePath) const {
    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();
    
    QStringList archiveExtensions = {"zip", "7z", "rar", "tar", "gz", "bz2", "xz", "lzma"};
    return archiveExtensions.contains(suffix);
}

void EnhancedMainWindow::loadSettings() {
    // Window geometry
    restoreGeometry(m_settings->value("window/geometry").toByteArray());
    restoreState(m_settings->value("window/state").toByteArray());
    
    // Theme
    int themeValue = m_settings->value("ui/theme", static_cast<int>(Theme::Auto)).toInt();
    setTheme(static_cast<Theme>(themeValue));
    
    // Recent files
    QStringList recentFiles = m_settings->value("files/recent").toStringList();
    if (m_navigationPanel) {
        m_navigationPanel->setRecentFiles(recentFiles);
    }
    
    // Splitter sizes
    QByteArray splitterState = m_settings->value("ui/splitter_state").toByteArray();
    if (!splitterState.isEmpty() && m_mainSplitter) {
        m_mainSplitter->restoreState(splitterState);
    }
}

void EnhancedMainWindow::saveSettings() {
    // Window geometry
    m_settings->setValue("window/geometry", saveGeometry());
    m_settings->setValue("window/state", saveState());
    
    // Theme
    m_settings->setValue("ui/theme", static_cast<int>(m_currentTheme));
    
    // Splitter state
    if (m_mainSplitter) {
        m_settings->setValue("ui/splitter_state", m_mainSplitter->saveState());
    }
    
    m_settings->sync();
}

// Slot implementations
void EnhancedMainWindow::onNavigationItemClicked(Components::SmartNavigationPanel::NavigationItem item) {
    using NavItem = Components::SmartNavigationPanel::NavigationItem;
    
    switch (item) {
    case NavItem::Home:
        setViewMode(ViewMode::Welcome);
        break;
    case NavItem::CreateArchive:
        onNewArchiveAction();
        break;
    case NavItem::OpenArchive:
        onOpenArchiveAction();
        break;
    case NavItem::ExtractArchive:
        onExtractArchiveAction();
        break;
    case NavItem::BrowseArchive:
        setViewMode(ViewMode::ArchiveViewer);
        break;
    case NavItem::Settings:
        onPreferencesAction();
        break;
    case NavItem::Help:
        onAboutAction();
        break;
    default:
        break;
    }
}

void EnhancedMainWindow::onNewArchiveAction() {
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        tr("Select Files to Archive"),
        QString(),
        tr("All Files (*)")
    );
    
    if (!files.isEmpty()) {
        createArchive(files);
    }
}

void EnhancedMainWindow::onOpenArchiveAction() {
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Open Archive"),
        QString(),
        tr("Archive Files (*.zip *.7z *.rar *.tar *.tar.gz *.tar.bz2 *.tar.xz);;All Files (*)")
    );
    
    if (!fileName.isEmpty()) {
        openArchive(fileName);
    }
}

void EnhancedMainWindow::onExtractArchiveAction() {
    if (m_currentArchivePath.isEmpty()) {
        QString fileName = QFileDialog::getOpenFileName(
            this,
            tr("Select Archive to Extract"),
            QString(),
            tr("Archive Files (*.zip *.7z *.rar *.tar *.tar.gz *.tar.bz2 *.tar.xz);;All Files (*)")
        );
        
        if (!fileName.isEmpty()) {
            extractArchive(fileName);
        }
    } else {
        extractArchive(m_currentArchivePath);
    }
}

void EnhancedMainWindow::onAddFilesAction() {
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        tr("Select Files to Add"),
        QString(),
        tr("All Files (*)")
    );
    
    if (!files.isEmpty()) {
        // This would integrate with archive modification logic
        showNotification(tr("Adding %1 files to archive").arg(files.size()),
                        Components::VisualFeedbackManager::FeedbackType::Information);
    }
}

void EnhancedMainWindow::onRemoveFilesAction() {
    if (m_selectedFiles.isEmpty()) {
        showNotification(tr("No files selected for removal"),
                        Components::VisualFeedbackManager::FeedbackType::Warning);
        return;
    }
    
    int result = QMessageBox::question(
        this,
        tr("Remove Files"),
        tr("Are you sure you want to remove %1 selected files from the archive?").arg(m_selectedFiles.size()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (result == QMessageBox::Yes) {
        // This would integrate with archive modification logic
        showNotification(tr("Removing %1 files from archive").arg(m_selectedFiles.size()),
                        Components::VisualFeedbackManager::FeedbackType::Information);
        m_selectedFiles.clear();
        updateViewSpecificUI();
    }
}

void EnhancedMainWindow::onBatchOperationsAction() {
    showBatchOperationsDialog();
}

void EnhancedMainWindow::onPreferencesAction() {
    // This would show a preferences dialog
    showNotification(tr("Preferences dialog not yet implemented"),
                    Components::VisualFeedbackManager::FeedbackType::Information);
}

void EnhancedMainWindow::onWelcomeViewRequested() {
    setViewMode(ViewMode::Welcome);
}

void EnhancedMainWindow::onFileBrowserViewRequested() {
    setViewMode(ViewMode::FileBrowser);
}

void EnhancedMainWindow::onArchiveViewerRequested() {
    setViewMode(ViewMode::ArchiveViewer);
}

void EnhancedMainWindow::onThemeChanged(QAction* action) {
    if (action == m_autoThemeAction) {
        setTheme(Theme::Auto);
    } else if (action == m_lightThemeAction) {
        setTheme(Theme::Light);
    } else if (action == m_darkThemeAction) {
        setTheme(Theme::Dark);
    } else if (action == m_highContrastThemeAction) {
        setTheme(Theme::HighContrast);
    }
}

void EnhancedMainWindow::onSystemThemeChanged() {
    if (m_currentTheme == Theme::Auto) {
        detectSystemTheme();
    }
}

void EnhancedMainWindow::onBreadcrumbItemClicked(const QString& path) {
    // Handle breadcrumb navigation
    if (m_fileDisplay) {
        // This would navigate to the specified path
        showNotification(tr("Navigating to: %1").arg(path),
                        Components::VisualFeedbackManager::FeedbackType::Information);
    }
}

void EnhancedMainWindow::onRecentFileRequested(const QString& filePath) {
    if (QFileInfo::exists(filePath)) {
        openArchive(filePath);
    } else {
        showNotification(tr("Recent file not found: %1").arg(filePath),
                        Components::VisualFeedbackManager::FeedbackType::Error);
        // Remove from recent files
        updateRecentFiles(filePath, true);
    }
}

void EnhancedMainWindow::onQuickActionTriggered(QAction* action) {
    if (action) {
        action->trigger();
    }
}

void EnhancedMainWindow::onFileSelectionChanged(const QStringList& selectedFiles) {
    m_selectedFiles = selectedFiles;
    updateViewSpecificUI();
    
    // Update status bar
    if (m_selectionLabel) {
        if (selectedFiles.isEmpty()) {
            m_selectionLabel->setText(tr("No selection"));
        } else {
            m_selectionLabel->setText(tr("%1 selected").arg(selectedFiles.size()));
        }
    }
}

void EnhancedMainWindow::onFileDoubleClicked(const QString& filePath) {
    // Handle file double-click
    if (isArchiveFile(filePath)) {
        openArchive(filePath);
    } else {
        // This would open the file with the default application
        showNotification(tr("Opening file: %1").arg(QFileInfo(filePath).fileName()),
                        Components::VisualFeedbackManager::FeedbackType::Information);
    }
}

void EnhancedMainWindow::onFileContextMenuRequested(const QString& filePath, const QPoint& position) {
    if (m_contextMenuManager) {
        QMenu* menu = m_contextMenuManager->createFileContextMenu(filePath, this);
        if (menu) {
            menu->exec(position);
            menu->deleteLater();
        }
    }
}

void EnhancedMainWindow::onAccessibilitySettingsChanged(const Managers::AccessibilityManager::AccessibilitySettings& settings) {
    // Handle accessibility settings changes
    if (settings.highContrastMode) {
        setTheme(Theme::HighContrast);
    }
    
    // Update UI based on accessibility settings
    updateLayout();
}

void EnhancedMainWindow::updateRecentFiles(const QString& filePath, bool remove) {
    if (!m_navigationPanel) return;
    
    QStringList recentFiles = m_navigationPanel->recentFiles();
    
    if (remove) {
        recentFiles.removeAll(filePath);
    } else {
        recentFiles.removeAll(filePath); // Remove if already exists
        recentFiles.prepend(filePath);   // Add to front
        
        // Limit to maximum number of recent files
        while (recentFiles.size() > MAX_RECENT_FILES) {
            recentFiles.removeLast();
        }
    }
    
    m_navigationPanel->setRecentFiles(recentFiles);
    
    // Save to settings
    m_settings->setValue("files/recent", recentFiles);
}

void EnhancedMainWindow::updateLayout() {
    // Update layout based on current window size and settings
    if (m_mainSplitter) {
        // Adjust splitter sizes based on window size
        int totalWidth = width();
        int navWidth = qBound(NAVIGATION_PANEL_MIN_WIDTH, 
                             totalWidth / 4, 
                             NAVIGATION_PANEL_MAX_WIDTH);
        
        m_mainSplitter->setSizes({navWidth, totalWidth - navWidth});
    }
}

void EnhancedMainWindow::updateIconTheme() {
    // Update icons based on current theme
    QString iconTheme = (m_currentTheme == Theme::Dark || m_currentTheme == Theme::HighContrast) ? "dark" : "light";
    
    // Update action icons
    m_newArchiveAction->setIcon(QIcon(QString(":/icons/%1/add-circle.svg").arg(iconTheme)));
    m_openArchiveAction->setIcon(QIcon(QString(":/icons/%1/folder-open.svg").arg(iconTheme)));
    m_extractArchiveAction->setIcon(QIcon(QString(":/icons/%1/download.svg").arg(iconTheme)));
    m_addFilesAction->setIcon(QIcon(QString(":/icons/%1/plus.svg").arg(iconTheme)));
    m_removeFilesAction->setIcon(QIcon(QString(":/icons/%1/minus.svg").arg(iconTheme)));
    m_batchOperationsAction->setIcon(QIcon(QString(":/icons/%1/layers.svg").arg(iconTheme)));
    m_preferencesAction->setIcon(QIcon(QString(":/icons/%1/settings.svg").arg(iconTheme)));
    m_exitAction->setIcon(QIcon(QString(":/icons/%1/x.svg").arg(iconTheme)));
    m_aboutAction->setIcon(QIcon(QString(":/icons/%1/info.svg").arg(iconTheme)));
}

void EnhancedMainWindow::setupContextMenus() {
    if (!m_contextMenuManager) return;
    
    // Register context menu providers
    m_contextMenuManager->registerContextMenuProvider("file", [this](const QString& filePath) -> QMenu* {
        QMenu* menu = new QMenu(this);
        
        if (isArchiveFile(filePath)) {
            menu->addAction(QIcon(":/icons/folder-open.svg"), tr("Open Archive"), [this, filePath]() {
                openArchive(filePath);
            });
            menu->addAction(QIcon(":/icons/download.svg"), tr("Extract Archive"), [this, filePath]() {
                extractArchive(filePath);
            });
        } else {
            menu->addAction(QIcon(":/icons/eye.svg"), tr("Preview"), [this, filePath]() {
                // This would show a file preview
                showNotification(tr("Previewing: %1").arg(QFileInfo(filePath).fileName()),
                                Components::VisualFeedbackManager::FeedbackType::Information);
            });
        }
        
        menu->addSeparator();
        menu->addAction(QIcon(":/icons/trash.svg"), tr("Remove from Archive"), [this, filePath]() {
            // This would remove the file from the current archive
            showNotification(tr("Removing: %1").arg(QFileInfo(filePath).fileName()),
                            Components::VisualFeedbackManager::FeedbackType::Information);
        });
        
        return menu;
    });
}

void EnhancedMainWindow::onAboutAction() {
    QMessageBox::about(this, tr("About Flux Archive Manager"),
        tr("<h3>Flux Archive Manager</h3>"
           "<p>A modern, accessible archive management application.</p>"
           "<p>Features:</p>"
           "<ul>"
           "<li>Smart navigation with contextual actions</li>"
           "<li>Rich file display with thumbnails and metadata</li>"
           "<li>Comprehensive accessibility support</li>"
           "<li>Batch operations for multiple archives</li>"
           "<li>Modern, responsive user interface</li>"
           "</ul>"
           "<p>Built with Qt and modern C++.</p>"));
}

} // namespace FluxGUI::UI

#include "modern_main_window.h"
#include "components/unified_drop_zone.h"
#include "components/modern_toolbar.h"
#include "components/smart_status_bar.h"
#include "views/modern_welcome_view.h"
#include "managers/keyboard_shortcut_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QApplication>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDebug>

namespace FluxGUI::UI {

ModernMainWindow::ModernMainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_mainLayout(nullptr)
    , m_viewStack(nullptr)
    , m_currentView(ViewMode::Welcome)
{
    setObjectName("ModernMainWindow");
    setWindowTitle("Flux Archive Manager");
    setMinimumSize(900, 600);
    resize(1200, 800);
    
    // Enable drag and drop
    setAcceptDrops(true);
    
    // Initialize UI components
    initializeUI();
    setupKeyboardShortcuts();
    setupAnimations();
    createSystemTray();
    applyModernStyling();
    
    // Load settings
    restoreWindowState();
    
    // Show welcome view initially
    switchToView(ViewMode::Welcome);
    
    qDebug() << "ModernMainWindow initialized successfully";
}

ModernMainWindow::~ModernMainWindow() {
    saveWindowState();
}

void ModernMainWindow::openArchive(const QString& filePath) {
    if (filePath.isEmpty()) {
        QString fileName = QFileDialog::getOpenFileName(
            this,
            "Open Archive",
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            "Archive Files (*.zip *.7z *.tar *.tar.gz *.tar.bz2 *.tar.xz);;All Files (*)"
        );
        
        if (!fileName.isEmpty()) {
            openArchive(fileName);
        }
        return;
    }
    
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        QMessageBox::warning(this, "Error", "Archive file does not exist: " + filePath);
        return;
    }
    
    m_currentArchivePath = filePath;
    updateRecentFiles(filePath);
    
    // Switch to archive view
    switchToView(ViewMode::Archive);
    
    // Update toolbar title
    m_toolbar->setTitle(fileInfo.baseName());
    m_toolbar->setSubtitle(fileInfo.absolutePath());
    
    // Update status bar
    m_statusBar->setArchiveInfo(filePath, 0, 0); // Will be updated when archive is loaded
    
    emit archiveOpened(filePath);
    
    qDebug() << "Opened archive:" << filePath;
}

void ModernMainWindow::createArchive(const QStringList& inputFiles) {
    if (inputFiles.isEmpty()) {
        QMessageBox::information(this, "Information", "No files selected for archive creation.");
        return;
    }
    
    // Switch to archive view in creation mode
    switchToView(ViewMode::Archive);
    m_toolbar->setMode(Components::ModernToolbar::ToolbarMode::Creation);
    
    // Update status
    m_statusBar->setStatus(QString("Ready to create archive from %1 items").arg(inputFiles.size()));
    
    emit archiveCreated(QString()); // Will be updated with actual path when created
    
    qDebug() << "Creating archive from" << inputFiles.size() << "files";
}

void ModernMainWindow::extractArchive(const QString& archivePath, const QString& outputPath) {
    QString actualOutputPath = outputPath;
    
    if (actualOutputPath.isEmpty()) {
        actualOutputPath = QFileDialog::getExistingDirectory(
            this,
            "Select Extraction Directory",
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
        );
        
        if (actualOutputPath.isEmpty()) {
            return; // User cancelled
        }
    }
    
    // Update toolbar for extraction mode
    m_toolbar->setMode(Components::ModernToolbar::ToolbarMode::Extraction);
    
    // Start extraction operation
    m_statusBar->startOperation("Extracting Archive", true);
    
    emit extractionRequested(archivePath, actualOutputPath);
    
    qDebug() << "Extracting" << archivePath << "to" << actualOutputPath;
}

void ModernMainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void ModernMainWindow::dropEvent(QDropEvent* event) {
    QStringList filePaths;
    
    for (const QUrl& url : event->mimeData()->urls()) {
        if (url.isLocalFile()) {
            filePaths.append(url.toLocalFile());
        }
    }
    
    if (!filePaths.isEmpty()) {
        handleDroppedFiles(filePaths);
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void ModernMainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    
    // Update layout if needed
    if (m_dropZone) {
        // Adjust drop zone visibility based on window size
        bool showDropZone = width() > 800 && height() > 600;
        // Implementation depends on specific layout requirements
    }
}

void ModernMainWindow::keyPressEvent(QKeyEvent* event) {
    // Let the shortcut manager handle key events first
    if (m_shortcuts) {
        // Shortcut manager will emit signals if shortcuts are matched
    }
    
    QMainWindow::keyPressEvent(event);
}

void ModernMainWindow::closeEvent(QCloseEvent* event) {
    if (m_operationInProgress) {
        int result = QMessageBox::question(
            this,
            "Operation in Progress",
            "An operation is currently in progress. Do you want to cancel it and exit?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        
        if (result == QMessageBox::No) {
            event->ignore();
            return;
        }
    }
    
    saveWindowState();
    
    // Hide to system tray if enabled
    if (m_trayIcon && m_trayIcon->isVisible()) {
        hide();
        event->ignore();
        return;
    }
    
    event->accept();
}

void ModernMainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized() && m_trayIcon && m_trayIcon->isVisible()) {
            hide();
            return;
        }
    }
    
    QMainWindow::changeEvent(event);
}

void ModernMainWindow::switchToView(ViewMode mode) {
    if (m_currentView == mode) return;
    
    ViewMode oldMode = m_currentView;
    m_currentView = mode;
    
    // Update view stack
    switch (mode) {
        case ViewMode::Welcome:
            if (m_welcomeView) {
                m_viewStack->setCurrentWidget(m_welcomeView.get());
                m_toolbar->setMode(Components::ModernToolbar::ToolbarMode::Welcome);
                m_toolbar->setTitle("Welcome");
                m_toolbar->setSubtitle("Flux Archive Manager");
                m_toolbar->setBackButtonVisible(false);
            }
            break;
            
        case ViewMode::Archive:
            if (m_archiveView) {
                m_viewStack->setCurrentWidget(m_archiveView.get());
                m_toolbar->setMode(Components::ModernToolbar::ToolbarMode::Archive);
                m_toolbar->setBackButtonVisible(true);
            }
            break;
            
        case ViewMode::Settings:
            if (m_settingsView) {
                m_viewStack->setCurrentWidget(m_settingsView.get());
                m_toolbar->setMode(Components::ModernToolbar::ToolbarMode::Settings);
                m_toolbar->setTitle("Settings");
                m_toolbar->setSubtitle("Application Configuration");
                m_toolbar->setBackButtonVisible(true);
            }
            break;
    }
    
    // Update keyboard shortcut context
    if (m_shortcuts) {
        using Context = Managers::KeyboardShortcutManager::ShortcutContext;
        Context newContext = Context::Global;
        
        switch (mode) {
            case ViewMode::Welcome:
                newContext = Context::Welcome;
                break;
            case ViewMode::Archive:
                newContext = Context::Archive;
                break;
            case ViewMode::Settings:
                newContext = Context::Settings;
                break;
        }
        
        m_shortcuts->setCurrentContext(newContext);
    }
    
    updateNavigationState();
    
    qDebug() << "Switched from view" << static_cast<int>(oldMode) 
             << "to view" << static_cast<int>(mode);
}

void ModernMainWindow::onBackRequested() {
    // Navigate back to previous view or welcome
    switchToView(ViewMode::Welcome);
}

void ModernMainWindow::onHomeRequested() {
    switchToView(ViewMode::Welcome);
}

void ModernMainWindow::onFilesDropped(const QStringList& files) {
    handleDroppedFiles(files);
}

void ModernMainWindow::onArchiveFileDropped(const QString& archivePath) {
    openArchive(archivePath);
}

void ModernMainWindow::onRegularFilesDropped(const QStringList& files) {
    createArchive(files);
}

void ModernMainWindow::onOperationStarted(const QString& operation, const QString& details) {
    m_operationInProgress = true;
    m_statusBar->startOperation(operation, true);
    
    qDebug() << "Operation started:" << operation << "-" << details;
}

void ModernMainWindow::onOperationProgress(int percentage, const QString& currentItem) {
    Components::SmartStatusBar::OperationStatus status;
    status.operationName = "Processing";
    status.currentItem = currentItem;
    status.percentage = percentage;
    
    m_statusBar->updateProgress(status);
}

void ModernMainWindow::onOperationFinished(bool success, const QString& message) {
    m_operationInProgress = false;
    m_statusBar->finishOperation(success, message);
    
    qDebug() << "Operation finished. Success:" << success << "Message:" << message;
}

void ModernMainWindow::onOperationCancelled() {
    m_operationInProgress = false;
    m_statusBar->cancelOperation();
    
    qDebug() << "Operation cancelled by user";
}

void ModernMainWindow::onThemeChanged() {
    applyModernStyling();
}

void ModernMainWindow::onLanguageChanged() {
    // Update UI text for new language
    // Implementation depends on internationalization system
}

void ModernMainWindow::onFirstRunCompleted() {
    m_isFirstRun = false;
    
    // Save first run completion
    QSettings settings;
    settings.setValue("ui/firstRunCompleted", true);
}

void ModernMainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    switch (reason) {
        case QSystemTrayIcon::DoubleClick:
            show();
            raise();
            activateWindow();
            break;
        default:
            break;
    }
}

void ModernMainWindow::onMinimizeToTray() {
    if (m_trayIcon && m_trayIcon->isVisible()) {
        hide();
    }
}

void ModernMainWindow::initializeUI() {
    // Create central widget
    m_centralWidget = new QWidget();
    setCentralWidget(m_centralWidget);
    
    // Create main layout
    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    createToolbar();
    createCentralWidget();
    createStatusBar();
}

void ModernMainWindow::createCentralWidget() {
    // Create view stack
    m_viewStack = new QStackedWidget();
    
    // Create drop zone overlay
    m_dropZone = std::make_unique<Components::UnifiedDropZone>();
    
    // Create views
    m_welcomeView = std::make_unique<Views::ModernWelcomeView>();
    // m_archiveView and m_settingsView would be created here
    
    // Add views to stack
    m_viewStack->addWidget(m_welcomeView.get());
    
    // Connect drop zone signals
    connect(m_dropZone.get(), &Components::UnifiedDropZone::filesDropped,
            this, &ModernMainWindow::onFilesDropped);
    connect(m_dropZone.get(), &Components::UnifiedDropZone::archiveFilesDropped,
            this, &ModernMainWindow::onArchiveFileDropped);
    connect(m_dropZone.get(), &Components::UnifiedDropZone::regularFilesDropped,
            this, &ModernMainWindow::onRegularFilesDropped);
    
    // Connect welcome view signals
    connect(m_welcomeView.get(), &Views::ModernWelcomeView::createArchiveRequested,
            this, [this]() { createArchive(QStringList()); });
    connect(m_welcomeView.get(), &Views::ModernWelcomeView::openArchiveRequested,
            this, QOverload<>::of(&ModernMainWindow::openArchive));
    connect(m_welcomeView.get(), &Views::ModernWelcomeView::openArchiveRequested,
            this, QOverload<const QString&>::of(&ModernMainWindow::openArchive));
    
    m_mainLayout->addWidget(m_viewStack);
}

void ModernMainWindow::createToolbar() {
    m_toolbar = std::make_unique<Components::ModernToolbar>();
    
    // Connect toolbar signals
    connect(m_toolbar.get(), &Components::ModernToolbar::backRequested,
            this, &ModernMainWindow::onBackRequested);
    connect(m_toolbar.get(), &Components::ModernToolbar::homeRequested,
            this, &ModernMainWindow::onHomeRequested);
    connect(m_toolbar.get(), &Components::ModernToolbar::settingsRequested,
            this, [this]() { switchToView(ViewMode::Settings); });
    
    m_mainLayout->addWidget(m_toolbar.get());
}

void ModernMainWindow::createStatusBar() {
    m_statusBar = std::make_unique<Components::SmartStatusBar>();
    
    // Connect status bar signals
    connect(m_statusBar.get(), &Components::SmartStatusBar::operationCancelRequested,
            this, &ModernMainWindow::onOperationCancelled);
    
    m_mainLayout->addWidget(m_statusBar.get());
}

void ModernMainWindow::createSystemTray() {
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        m_trayIcon = std::make_unique<QSystemTrayIcon>(this);
        m_trayIcon->setIcon(QApplication::windowIcon());
        
        // Create tray menu
        m_trayMenu = std::make_unique<QMenu>(this);
        m_trayMenu->addAction("Show", this, [this]() { show(); raise(); activateWindow(); });
        m_trayMenu->addSeparator();
        m_trayMenu->addAction("Quit", this, &QWidget::close);
        
        m_trayIcon->setContextMenu(m_trayMenu.get());
        
        connect(m_trayIcon.get(), &QSystemTrayIcon::activated,
                this, &ModernMainWindow::onTrayIconActivated);
        
        m_trayIcon->show();
    }
}

void ModernMainWindow::setupKeyboardShortcuts() {
    m_shortcuts = std::make_unique<Managers::KeyboardShortcutManager>(this);
    
    // Connect shortcut signals
    connect(m_shortcuts.get(), &Managers::KeyboardShortcutManager::shortcutActivated,
            this, [this](const QString& id) {
                // Handle shortcut activation
                if (id == "file.new") {
                    createArchive(QStringList());
                } else if (id == "file.open") {
                    openArchive();
                } else if (id == "view.home") {
                    onHomeRequested();
                } else if (id == "view.settings") {
                    switchToView(ViewMode::Settings);
                }
            });
}

void ModernMainWindow::setupAnimations() {
    // Setup fade animation for view transitions
    m_opacityEffect = std::make_unique<QGraphicsOpacityEffect>();
    m_fadeAnimation = std::make_unique<QPropertyAnimation>(m_opacityEffect.get(), "opacity");
    m_fadeAnimation->setDuration(ANIMATION_DURATION);
    
    if (m_viewStack) {
        m_viewStack->setGraphicsEffect(m_opacityEffect.get());
    }
}

void ModernMainWindow::applyModernStyling() {
    setStyleSheet(R"(
        ModernMainWindow {
            background-color: #f8f9fa;
            color: #2c3e50;
        }
        
        QWidget {
            font-family: "Segoe UI", "Roboto", "Helvetica Neue", Arial, sans-serif;
        }
        
        QStackedWidget {
            background-color: transparent;
            border: none;
        }
    )");
}

void ModernMainWindow::showWelcomeView() {
    switchToView(ViewMode::Welcome);
}

void ModernMainWindow::showArchiveView(const QString& archivePath) {
    if (!archivePath.isEmpty()) {
        m_currentArchivePath = archivePath;
    }
    switchToView(ViewMode::Archive);
}

void ModernMainWindow::showSettingsView() {
    switchToView(ViewMode::Settings);
}

void ModernMainWindow::updateNavigationState() {
    // Update navigation state based on current view
    // This could include updating breadcrumbs, back button state, etc.
}

void ModernMainWindow::handleDroppedFiles(const QStringList& filePaths) {
    QStringList archives;
    QStringList regular;
    
    categorizeFiles(filePaths, archives, regular);
    
    if (!archives.isEmpty() && regular.isEmpty()) {
        // Only archives dropped - open the first one
        openArchive(archives.first());
    } else if (archives.isEmpty() && !regular.isEmpty()) {
        // Only regular files - create archive
        createArchive(regular);
    } else if (!archives.isEmpty() && !regular.isEmpty()) {
        // Mixed files - ask user what to do
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Mixed File Types");
        msgBox.setText("You dropped both archive files and regular files. What would you like to do?");
        
        QPushButton* openBtn = msgBox.addButton("Open Archive", QMessageBox::ActionRole);
        QPushButton* createBtn = msgBox.addButton("Create Archive", QMessageBox::ActionRole);
        msgBox.addButton(QMessageBox::Cancel);
        
        msgBox.exec();
        
        if (msgBox.clickedButton() == openBtn) {
            openArchive(archives.first());
        } else if (msgBox.clickedButton() == createBtn) {
            createArchive(regular);
        }
    }
}

QStringList ModernMainWindow::categorizeFiles(const QStringList& filePaths, 
                                            QStringList& archives, 
                                            QStringList& regular) {
    archives.clear();
    regular.clear();
    
    QStringList archiveExtensions = {
        "zip", "7z", "rar", "tar", "gz", "bz2", "xz", "zst",
        "tar.gz", "tar.bz2", "tar.xz", "tar.zst", "tgz", "tbz2", "txz"
    };
    
    for (const QString& filePath : filePaths) {
        QFileInfo fileInfo(filePath);
        QString suffix = fileInfo.suffix().toLower();
        QString fileName = fileInfo.fileName().toLower();
        
        bool isArchive = archiveExtensions.contains(suffix) ||
                        std::any_of(archiveExtensions.begin(), archiveExtensions.end(),
                                   [&fileName](const QString& ext) {
                                       return fileName.endsWith("." + ext);
                                   });
        
        if (isArchive) {
            archives.append(filePath);
        } else {
            regular.append(filePath);
        }
    }
    
    return filePaths;
}

void ModernMainWindow::showDropFeedback(bool show, const QString& message) {
    if (m_dropZone) {
        if (show) {
            m_dropZone->showFeedback(message);
        } else {
            m_dropZone->hideFeedback();
        }
    }
}

void ModernMainWindow::showOnboardingIfNeeded() {
    QSettings settings;
    m_isFirstRun = !settings.value("ui/firstRunCompleted", false).toBool();
    
    if (m_isFirstRun && m_welcomeView) {
        m_welcomeView->showOnboardingTip("Welcome to Flux Archive Manager! Drag files here to create archives, or drag archives to open them.");
    }
}

void ModernMainWindow::showContextualHelp(const QString& topic) {
    // Implementation would show help for specific topics
    Q_UNUSED(topic)
}

void ModernMainWindow::updateRecentFiles(const QString& filePath) {
    m_recentFiles.removeAll(filePath);
    m_recentFiles.prepend(filePath);
    
    while (m_recentFiles.size() > MAX_RECENT_FILES) {
        m_recentFiles.removeLast();
    }
    
    // Save to settings
    QSettings settings;
    settings.setValue("ui/recentFiles", m_recentFiles);
    
    // Update welcome view
    if (m_welcomeView) {
        m_welcomeView->refreshRecentFiles();
    }
}

void ModernMainWindow::saveWindowState() {
    QSettings settings;
    settings.setValue("ui/geometry", saveGeometry());
    settings.setValue("ui/windowState", saveState());
}

void ModernMainWindow::restoreWindowState() {
    QSettings settings;
    
    QByteArray geometry = settings.value("ui/geometry").toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    
    QByteArray windowState = settings.value("ui/windowState").toByteArray();
    if (!windowState.isEmpty()) {
        restoreState(windowState);
    }
    
    m_recentFiles = settings.value("ui/recentFiles").toStringList();
}

} // namespace FluxGUI::UI

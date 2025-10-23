#include "welcome_widget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QScrollArea>
#include <QListWidget>
#include <QListWidgetItem>
#include <QFileInfo>
#include <QDir>
#include <QPixmap>
#include <QMovie>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QDebug>

#include "../../core/config/settings_manager.h"
#include "../../core/archive/archive_manager.h"

namespace FluxGUI::UI::Widgets {

WelcomeWidget::WelcomeWidget(QWidget* parent)
    : QWidget(parent)
    , m_heroLabel(nullptr)
    , m_subtitleLabel(nullptr)
    , m_createArchiveButton(nullptr)
    , m_openArchiveButton(nullptr)
    , m_extractArchiveButton(nullptr)
    , m_recentFilesList(nullptr)
    , m_quickActionsFrame(nullptr)
    , m_statsFrame(nullptr)
    , m_fadeAnimation(nullptr)
{
    // Set object name for styling
    setObjectName("WelcomeWidget");
    
    // Enable drag and drop
    setAcceptDrops(true);
    
    // Initialize UI
    initializeUI();
    
    // Connect signals
    connectSignals();
    
    // Load recent files
    loadRecentFiles();
    
    // Start animations
    startAnimations();
    
    qDebug() << "WelcomeWidget initialized";
}

WelcomeWidget::~WelcomeWidget() = default;

void WelcomeWidget::refreshRecentFiles() {
    loadRecentFiles();
}

void WelcomeWidget::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void WelcomeWidget::dropEvent(QDropEvent* event) {
    QStringList archiveFiles;
    QStringList regularFiles;
    
    // Process dropped files
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
        emit openArchiveRequested(archiveFiles.first());
    } else if (!regularFiles.isEmpty()) {
        emit createArchiveRequested(regularFiles);
    }
    
    event->acceptProposedAction();
}

void WelcomeWidget::initializeUI() {
    // Create main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 40, 40, 40);
    mainLayout->setSpacing(30);
    
    // Create hero section
    createHeroSection(mainLayout);
    
    // Create content area with scroll
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    QWidget* contentWidget = new QWidget();
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(30);
    
    // Create main content sections
    createQuickActionsSection(contentLayout);
    createRecentFilesSection(contentLayout);
    createStatsSection(contentLayout);
    
    // Add stretch to push content to top
    contentLayout->addStretch();
    
    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea, 1);
    
    // Apply styling
    applyStyles();
}

void WelcomeWidget::createHeroSection(QVBoxLayout* layout) {
    // Hero container
    QFrame* heroFrame = new QFrame(this);
    heroFrame->setObjectName("heroFrame");
    heroFrame->setFixedHeight(200);
    
    QVBoxLayout* heroLayout = new QVBoxLayout(heroFrame);
    heroLayout->setAlignment(Qt::AlignCenter);
    heroLayout->setSpacing(16);
    
    // Main title
    m_heroLabel = new QLabel("Welcome to Flux Archive Manager", heroFrame);
    m_heroLabel->setObjectName("heroTitle");
    m_heroLabel->setAlignment(Qt::AlignCenter);
    heroLayout->addWidget(m_heroLabel);
    
    // Subtitle
    m_subtitleLabel = new QLabel("Create, extract, and manage archives with ease", heroFrame);
    m_subtitleLabel->setObjectName("heroSubtitle");
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    heroLayout->addWidget(m_subtitleLabel);
    
    layout->addWidget(heroFrame);
}

void WelcomeWidget::createQuickActionsSection(QVBoxLayout* layout) {
    // Section title
    QLabel* titleLabel = new QLabel("Quick Actions", this);
    titleLabel->setObjectName("sectionTitle");
    layout->addWidget(titleLabel);
    
    // Actions frame
    m_quickActionsFrame = new QFrame(this);
    m_quickActionsFrame->setObjectName("quickActionsFrame");
    m_quickActionsFrame->setProperty("class", "card");
    
    QGridLayout* actionsLayout = new QGridLayout(m_quickActionsFrame);
    actionsLayout->setSpacing(20);
    actionsLayout->setContentsMargins(30, 30, 30, 30);
    
    // Create Archive button
    m_createArchiveButton = createActionButton(
        ":/icons/compress.svg",
        "Create Archive",
        "Create a new archive from files and folders",
        m_quickActionsFrame
    );
    actionsLayout->addWidget(m_createArchiveButton, 0, 0);
    
    // Open Archive button
    m_openArchiveButton = createActionButton(
        ":/icons/folder-open.svg",
        "Open Archive",
        "Browse and extract files from an existing archive",
        m_quickActionsFrame
    );
    actionsLayout->addWidget(m_openArchiveButton, 0, 1);
    
    // Extract Archive button
    m_extractArchiveButton = createActionButton(
        ":/icons/extract.svg",
        "Extract Archive",
        "Extract all files from an archive to a folder",
        m_quickActionsFrame
    );
    actionsLayout->addWidget(m_extractArchiveButton, 1, 0);
    
    // Settings button
    QPushButton* settingsButton = createActionButton(
        ":/icons/settings.svg",
        "Settings",
        "Configure application preferences and options",
        m_quickActionsFrame
    );
    actionsLayout->addWidget(settingsButton, 1, 1);
    
    // Connect settings button
    connect(settingsButton, &QPushButton::clicked, this, &WelcomeWidget::settingsRequested);
    
    layout->addWidget(m_quickActionsFrame);
}

void WelcomeWidget::createRecentFilesSection(QVBoxLayout* layout) {
    // Section title
    QLabel* titleLabel = new QLabel("Recent Files", this);
    titleLabel->setObjectName("sectionTitle");
    layout->addWidget(titleLabel);
    
    // Recent files frame
    QFrame* recentFrame = new QFrame(this);
    recentFrame->setObjectName("recentFilesFrame");
    recentFrame->setProperty("class", "card");
    recentFrame->setMinimumHeight(200);
    
    QVBoxLayout* recentLayout = new QVBoxLayout(recentFrame);
    recentLayout->setContentsMargins(20, 20, 20, 20);
    recentLayout->setSpacing(10);
    
    // Recent files list
    m_recentFilesList = new QListWidget(recentFrame);
    m_recentFilesList->setObjectName("recentFilesList");
    m_recentFilesList->setFrameShape(QFrame::NoFrame);
    m_recentFilesList->setAlternatingRowColors(true);
    m_recentFilesList->setSelectionMode(QAbstractItemView::SingleSelection);
    
    recentLayout->addWidget(m_recentFilesList);
    
    layout->addWidget(recentFrame);
}

void WelcomeWidget::createStatsSection(QVBoxLayout* layout) {
    // Section title
    QLabel* titleLabel = new QLabel("Statistics", this);
    titleLabel->setObjectName("sectionTitle");
    layout->addWidget(titleLabel);
    
    // Stats frame
    m_statsFrame = new QFrame(this);
    m_statsFrame->setObjectName("statsFrame");
    m_statsFrame->setProperty("class", "card");
    
    QHBoxLayout* statsLayout = new QHBoxLayout(m_statsFrame);
    statsLayout->setContentsMargins(30, 20, 30, 20);
    statsLayout->setSpacing(40);
    
    // Create stat items
    createStatItem(statsLayout, "Archives Opened", "0", ":/icons/folder-open.svg");
    createStatItem(statsLayout, "Files Compressed", "0", ":/icons/compress.svg");
    createStatItem(statsLayout, "Files Extracted", "0", ":/icons/extract.svg");
    createStatItem(statsLayout, "Space Saved", "0 MB", ":/icons/success.svg");
    
    layout->addWidget(m_statsFrame);
}

QPushButton* WelcomeWidget::createActionButton(const QString& iconPath, const QString& title, 
                                              const QString& description, QWidget* parent) {
    QPushButton* button = new QPushButton(parent);
    button->setObjectName("actionButton");
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    button->setMinimumHeight(120);
    button->setCursor(Qt::PointingHandCursor);
    
    // Create button layout
    QVBoxLayout* buttonLayout = new QVBoxLayout(button);
    buttonLayout->setAlignment(Qt::AlignCenter);
    buttonLayout->setSpacing(8);
    
    // Icon label
    QLabel* iconLabel = new QLabel(button);
    iconLabel->setPixmap(QIcon(iconPath).pixmap(48, 48));
    iconLabel->setAlignment(Qt::AlignCenter);
    buttonLayout->addWidget(iconLabel);
    
    // Title label
    QLabel* titleLabel = new QLabel(title, button);
    titleLabel->setObjectName("actionButtonTitle");
    titleLabel->setAlignment(Qt::AlignCenter);
    buttonLayout->addWidget(titleLabel);
    
    // Description label
    QLabel* descLabel = new QLabel(description, button);
    descLabel->setObjectName("actionButtonDescription");
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setWordWrap(true);
    buttonLayout->addWidget(descLabel);
    
    return button;
}

void WelcomeWidget::createStatItem(QHBoxLayout* layout, const QString& title, 
                                  const QString& value, const QString& iconPath) {
    QFrame* statFrame = new QFrame(this);
    statFrame->setObjectName("statItem");
    
    QVBoxLayout* statLayout = new QVBoxLayout(statFrame);
    statLayout->setAlignment(Qt::AlignCenter);
    statLayout->setSpacing(8);
    
    // Icon
    QLabel* iconLabel = new QLabel(statFrame);
    iconLabel->setPixmap(QIcon(iconPath).pixmap(32, 32));
    iconLabel->setAlignment(Qt::AlignCenter);
    statLayout->addWidget(iconLabel);
    
    // Value
    QLabel* valueLabel = new QLabel(value, statFrame);
    valueLabel->setObjectName("statValue");
    valueLabel->setAlignment(Qt::AlignCenter);
    statLayout->addWidget(valueLabel);
    
    // Title
    QLabel* titleLabel = new QLabel(title, statFrame);
    titleLabel->setObjectName("statTitle");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setWordWrap(true);
    statLayout->addWidget(titleLabel);
    
    layout->addWidget(statFrame);
}

void WelcomeWidget::connectSignals() {
    // Quick action buttons
    connect(m_createArchiveButton, &QPushButton::clicked, 
            this, &WelcomeWidget::createArchiveRequested);
    
    connect(m_openArchiveButton, &QPushButton::clicked, this, [this]() {
        QString filePath = QFileDialog::getOpenFileName(this,
            "Open Archive", QString(),
            "Archive Files (*.zip *.7z *.rar *.tar *.gz *.bz2 *.xz);;All Files (*)");
        
        if (!filePath.isEmpty()) {
            emit openArchiveRequested(filePath);
        }
    });
    
    connect(m_extractArchiveButton, &QPushButton::clicked, 
            this, &WelcomeWidget::extractArchiveRequested);
    
    // Recent files list
    connect(m_recentFilesList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        QString filePath = item->data(Qt::UserRole).toString();
        if (!filePath.isEmpty() && QFileInfo::exists(filePath)) {
            emit openArchiveRequested(filePath);
        }
    });
}

void WelcomeWidget::loadRecentFiles() {
    if (!m_recentFilesList) {
        return;
    }
    
    m_recentFilesList->clear();
    
    auto& settingsManager = Core::Config::SettingsManager::instance();
    QStringList recentFiles = settingsManager.value("paths/recentFiles", QStringList()).toStringList();
    
    if (recentFiles.isEmpty()) {
        QListWidgetItem* item = new QListWidgetItem("No recent files");
        item->setFlags(Qt::NoItemFlags);
        item->setForeground(QColor("#888888"));
        m_recentFilesList->addItem(item);
        return;
    }
    
    for (const QString& filePath : recentFiles) {
        QFileInfo fileInfo(filePath);
        
        if (!fileInfo.exists()) {
            continue; // Skip non-existent files
        }
        
        QListWidgetItem* item = new QListWidgetItem();
        item->setText(fileInfo.fileName());
        item->setToolTip(filePath);
        item->setData(Qt::UserRole, filePath);
        
        // Set icon based on file type
        auto& archiveManager = Core::Archive::ArchiveManager::instance();
        QString format = archiveManager.detectFormat(filePath);
        if (!format.isEmpty()) {
            Core::Archive::FormatInfo formatInfo = archiveManager.getFormatInfo(format);
            // Set appropriate icon (would need icon mapping)
            item->setIcon(QIcon(":/icons/file-archive.svg"));
        }
        
        m_recentFilesList->addItem(item);
    }
}

void WelcomeWidget::startAnimations() {
    // Create fade-in animation for the hero section
    if (m_heroLabel) {
        QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(m_heroLabel);
        m_heroLabel->setGraphicsEffect(effect);
        
        m_fadeAnimation = new QPropertyAnimation(effect, "opacity", this);
        m_fadeAnimation->setDuration(1000);
        m_fadeAnimation->setStartValue(0.0);
        m_fadeAnimation->setEndValue(1.0);
        m_fadeAnimation->setEasingCurve(QEasingCurve::InOutQuad);
        
        // Start animation after a short delay
        QTimer::singleShot(100, [this]() {
            if (m_fadeAnimation) {
                m_fadeAnimation->start();
            }
        });
    }
}

void WelcomeWidget::applyStyles() {
    // Apply custom styling through object names and properties
    // The actual styling is handled by the QSS files
    
    // Set properties for different elements
    if (m_quickActionsFrame) {
        m_quickActionsFrame->setProperty("class", "card");
    }
    
    if (m_statsFrame) {
        m_statsFrame->setProperty("class", "card");
    }
    
    // Force style update
    style()->unpolish(this);
    style()->polish(this);
}

} // namespace FluxGUI::UI::Widgets

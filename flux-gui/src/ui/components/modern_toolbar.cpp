#include "modern_toolbar.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QButtonGroup>
#include <QFrame>
#include <QSpacerItem>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QTimer>
#include <QDebug>

namespace FluxGUI::UI::Components {

ModernToolbar::ModernToolbar(QWidget* parent)
    : QWidget(parent)
    , m_currentMode(Mode::Welcome)
    , m_welcomeButton(nullptr)
    , m_explorerButton(nullptr)
    , m_compressionButton(nullptr)
    , m_extractionButton(nullptr)
    , m_settingsButton(nullptr)
    , m_modeButtonGroup(nullptr)
    , m_titleLabel(nullptr)
    , m_breadcrumbLabel(nullptr)
    , m_fadeAnimation(nullptr)
{
    // Set object name for styling
    setObjectName("ModernToolbar");
    setFixedHeight(64);
    
    // Initialize UI
    initializeUI();
    
    // Connect signals
    connectSignals();
    
    // Set initial mode
    setMode(Mode::Welcome);
    
    qDebug() << "ModernToolbar initialized";
}

ModernToolbar::~ModernToolbar() = default;

void ModernToolbar::setMode(Mode mode) {
    if (m_currentMode == mode) {
        return;
    }
    
    Mode previousMode = m_currentMode;
    m_currentMode = mode;
    
    // Update button states
    updateButtonStates();
    
    // Update title and breadcrumb
    updateTitleAndBreadcrumb();
    
    // Animate transition
    animateTransition();
    
    emit modeChanged(static_cast<int>(mode), static_cast<int>(previousMode));
    
    qDebug() << "Toolbar mode changed to:" << static_cast<int>(mode);
}

ModernToolbar::Mode ModernToolbar::currentMode() const {
    return m_currentMode;
}

void ModernToolbar::setTitle(const QString& title) {
    if (m_titleLabel) {
        m_titleLabel->setText(title);
    }
}

void ModernToolbar::setBreadcrumb(const QString& breadcrumb) {
    if (m_breadcrumbLabel) {
        m_breadcrumbLabel->setText(breadcrumb);
        m_breadcrumbLabel->setVisible(!breadcrumb.isEmpty());
    }
}

void ModernToolbar::initializeUI() {
    // Create main layout
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(16, 8, 16, 8);
    mainLayout->setSpacing(16);
    
    // Create navigation section
    createNavigationSection(mainLayout);
    
    // Add spacer
    mainLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
    
    // Create title section
    createTitleSection(mainLayout);
    
    // Add spacer
    mainLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
    
    // Create action section
    createActionSection(mainLayout);
    
    // Apply styling
    applyStyles();
}

void ModernToolbar::createNavigationSection(QHBoxLayout* layout) {
    // Navigation frame
    QFrame* navFrame = new QFrame(this);
    navFrame->setObjectName("navigationFrame");
    
    QHBoxLayout* navLayout = new QHBoxLayout(navFrame);
    navLayout->setContentsMargins(0, 0, 0, 0);
    navLayout->setSpacing(4);
    
    // Create mode buttons
    m_welcomeButton = createModeButton(":/icons/home.svg", "Welcome", Mode::Welcome, navFrame);
    m_explorerButton = createModeButton(":/icons/folder.svg", "Explorer", Mode::Explorer, navFrame);
    m_compressionButton = createModeButton(":/icons/compress.svg", "Create", Mode::Compression, navFrame);
    m_extractionButton = createModeButton(":/icons/extract.svg", "Extract", Mode::Extraction, navFrame);
    
    navLayout->addWidget(m_welcomeButton);
    navLayout->addWidget(m_explorerButton);
    navLayout->addWidget(m_compressionButton);
    navLayout->addWidget(m_extractionButton);
    
    // Create button group for exclusive selection
    m_modeButtonGroup = new QButtonGroup(this);
    m_modeButtonGroup->addButton(m_welcomeButton, static_cast<int>(Mode::Welcome));
    m_modeButtonGroup->addButton(m_explorerButton, static_cast<int>(Mode::Explorer));
    m_modeButtonGroup->addButton(m_compressionButton, static_cast<int>(Mode::Compression));
    m_modeButtonGroup->addButton(m_extractionButton, static_cast<int>(Mode::Extraction));
    
    layout->addWidget(navFrame);
}

void ModernToolbar::createTitleSection(QHBoxLayout* layout) {
    // Title frame
    QFrame* titleFrame = new QFrame(this);
    titleFrame->setObjectName("titleFrame");
    
    QVBoxLayout* titleLayout = new QVBoxLayout(titleFrame);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(2);
    titleLayout->setAlignment(Qt::AlignCenter);
    
    // Main title
    m_titleLabel = new QLabel("Welcome", titleFrame);
    m_titleLabel->setObjectName("toolbarTitle");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    titleLayout->addWidget(m_titleLabel);
    
    // Breadcrumb
    m_breadcrumbLabel = new QLabel(titleFrame);
    m_breadcrumbLabel->setObjectName("toolbarBreadcrumb");
    m_breadcrumbLabel->setAlignment(Qt::AlignCenter);
    m_breadcrumbLabel->setVisible(false);
    titleLayout->addWidget(m_breadcrumbLabel);
    
    layout->addWidget(titleFrame);
}

void ModernToolbar::createActionSection(QHBoxLayout* layout) {
    // Action frame
    QFrame* actionFrame = new QFrame(this);
    actionFrame->setObjectName("actionFrame");
    
    QHBoxLayout* actionLayout = new QHBoxLayout(actionFrame);
    actionLayout->setContentsMargins(0, 0, 0, 0);
    actionLayout->setSpacing(8);
    
    // Settings button
    m_settingsButton = createActionButton(":/icons/settings.svg", "Settings", actionFrame);
    actionLayout->addWidget(m_settingsButton);
    
    layout->addWidget(actionFrame);
}

QPushButton* ModernToolbar::createModeButton(const QString& iconPath, const QString& text, 
                                           Mode mode, QWidget* parent) {
    QPushButton* button = new QPushButton(parent);
    button->setObjectName("modeButton");
    button->setCheckable(true);
    button->setToolTip(text);
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumSize(80, 48);
    button->setMaximumSize(80, 48);
    
    // Create button layout
    QVBoxLayout* buttonLayout = new QVBoxLayout(button);
    buttonLayout->setContentsMargins(8, 6, 8, 6);
    buttonLayout->setSpacing(2);
    buttonLayout->setAlignment(Qt::AlignCenter);
    
    // Icon label
    QLabel* iconLabel = new QLabel(button);
    iconLabel->setPixmap(QIcon(iconPath).pixmap(20, 20));
    iconLabel->setAlignment(Qt::AlignCenter);
    buttonLayout->addWidget(iconLabel);
    
    // Text label
    QLabel* textLabel = new QLabel(text, button);
    textLabel->setObjectName("modeButtonText");
    textLabel->setAlignment(Qt::AlignCenter);
    buttonLayout->addWidget(textLabel);
    
    // Store mode in button data
    button->setProperty("mode", static_cast<int>(mode));
    
    return button;
}

QPushButton* ModernToolbar::createActionButton(const QString& iconPath, const QString& text, 
                                             QWidget* parent) {
    QPushButton* button = new QPushButton(parent);
    button->setObjectName("actionButton");
    button->setToolTip(text);
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumSize(40, 40);
    button->setMaximumSize(40, 40);
    
    // Set icon
    button->setIcon(QIcon(iconPath));
    button->setIconSize(QSize(24, 24));
    
    return button;
}

void ModernToolbar::connectSignals() {
    // Mode button group
    connect(m_modeButtonGroup, QOverload<int>::of(&QButtonGroup::buttonClicked),
            this, [this](int mode) {
                emit modeChangeRequested(mode);
            });
    
    // Settings button
    connect(m_settingsButton, &QPushButton::clicked, this, [this]() {
        emit modeChangeRequested(static_cast<int>(Mode::Settings));
    });
}

void ModernToolbar::updateButtonStates() {
    // Update checked state of mode buttons
    QPushButton* currentButton = nullptr;
    
    switch (m_currentMode) {
        case Mode::Welcome:
            currentButton = m_welcomeButton;
            break;
        case Mode::Explorer:
            currentButton = m_explorerButton;
            break;
        case Mode::Compression:
            currentButton = m_compressionButton;
            break;
        case Mode::Extraction:
            currentButton = m_extractionButton;
            break;
        case Mode::Settings:
            // Settings doesn't have a mode button
            break;
    }
    
    if (currentButton) {
        currentButton->setChecked(true);
    }
}

void ModernToolbar::updateTitleAndBreadcrumb() {
    QString title;
    QString breadcrumb;
    
    switch (m_currentMode) {
        case Mode::Welcome:
            title = "Welcome";
            breadcrumb = "";
            break;
        case Mode::Explorer:
            title = "Archive Explorer";
            breadcrumb = "Browse archive contents";
            break;
        case Mode::Compression:
            title = "Create Archive";
            breadcrumb = "Compress files and folders";
            break;
        case Mode::Extraction:
            title = "Extract Archive";
            breadcrumb = "Extract files from archive";
            break;
        case Mode::Settings:
            title = "Settings";
            breadcrumb = "Configure application preferences";
            break;
    }
    
    setTitle(title);
    setBreadcrumb(breadcrumb);
}

void ModernToolbar::animateTransition() {
    // Create fade animation for title changes
    if (m_titleLabel) {
        QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(m_titleLabel);
        m_titleLabel->setGraphicsEffect(effect);
        
        m_fadeAnimation = new QPropertyAnimation(effect, "opacity", this);
        m_fadeAnimation->setDuration(200);
        m_fadeAnimation->setStartValue(0.7);
        m_fadeAnimation->setEndValue(1.0);
        m_fadeAnimation->setEasingCurve(QEasingCurve::InOutQuad);
        
        m_fadeAnimation->start();
    }
}

void ModernToolbar::applyStyles() {
    // Apply custom styling through object names and properties
    // The actual styling is handled by the QSS files
    
    // Force style update
    style()->unpolish(this);
    style()->polish(this);
}

} // namespace FluxGUI::UI::Components

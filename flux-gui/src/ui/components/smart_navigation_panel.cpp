#include "smart_navigation_panel.h"
#include <QApplication>
#include <QStyle>
#include <QStyleOption>
#include <QPainter>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QAccessible>
#include <QSvgRenderer>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QSettings>

namespace FluxGUI::UI::Components {

SmartNavigationPanel::SmartNavigationPanel(QWidget* parent)
    : QWidget(parent)
    , m_hoverTimer(new QTimer(this))
{
    setObjectName("SmartNavigationPanel");
    setAttribute(Qt::WA_StyledBackground, true);
    setFocusPolicy(Qt::StrongFocus);
    
    // Initialize UI components
    initializeUI();
    
    // Setup animations
    m_collapseAnimation = std::make_unique<QPropertyAnimation>(this, "minimumWidth");
    m_collapseAnimation->setDuration(ANIMATION_DURATION);
    m_collapseAnimation->setEasingCurve(QEasingCurve::OutCubic);
    
    m_opacityEffect = std::make_unique<QGraphicsOpacityEffect>(this);
    m_fadeAnimation = std::make_unique<QPropertyAnimation>(m_opacityEffect.get(), "opacity");
    m_fadeAnimation->setDuration(ANIMATION_DURATION);
    
    // Setup hover timer
    m_hoverTimer->setSingleShot(true);
    m_hoverTimer->setInterval(HOVER_DELAY);
    
    // Connect signals
    connect(m_collapseAnimation.get(), &QPropertyAnimation::finished,
            this, &SmartNavigationPanel::onAnimationFinished);
    connect(m_hoverTimer, &QTimer::timeout,
            this, [this]() { setCollapsed(false); });
    
    // Setup accessibility
    setupAccessibility();
    
    // Set initial state
    setMode(NavigationMode::Welcome);
    updateVisualState();
}

SmartNavigationPanel::~SmartNavigationPanel() = default;

void SmartNavigationPanel::setMode(NavigationMode mode) {
    if (m_currentMode == mode) return;
    
    NavigationMode oldMode = m_currentMode;
    m_currentMode = mode;
    
    updateNavigationItems();
    updateVisualState();
    
    if (m_accessibilityEnabled) {
        announceStateChange(tr("Navigation mode changed to %1").arg(static_cast<int>(mode)));
    }
    
    emit modeChanged(mode);
}

void SmartNavigationPanel::setCurrentItem(NavigationItem item) {
    if (m_currentItem == item) return;
    
    m_currentItem = item;
    updateNavigationItems();
    
    if (m_accessibilityEnabled) {
        announceStateChange(tr("Selected %1").arg(static_cast<int>(item)));
    }
}

void SmartNavigationPanel::setBreadcrumb(const QList<BreadcrumbItem>& items) {
    m_breadcrumbItems = items;
    updateBreadcrumbDisplay();
}

void SmartNavigationPanel::addBreadcrumbItem(const BreadcrumbItem& item) {
    m_breadcrumbItems.append(item);
    updateBreadcrumbDisplay();
}

void SmartNavigationPanel::clearBreadcrumb() {
    m_breadcrumbItems.clear();
    updateBreadcrumbDisplay();
}

void SmartNavigationPanel::setRecentFiles(const QStringList& files) {
    m_recentFiles = files;
    if (m_recentFiles.size() > MAX_RECENT_FILES) {
        m_recentFiles = m_recentFiles.mid(0, MAX_RECENT_FILES);
    }
    updateRecentFilesDisplay();
}

void SmartNavigationPanel::addRecentFile(const QString& filePath) {
    m_recentFiles.removeAll(filePath); // Remove if already exists
    m_recentFiles.prepend(filePath);
    
    if (m_recentFiles.size() > MAX_RECENT_FILES) {
        m_recentFiles.removeLast();
    }
    
    updateRecentFilesDisplay();
}

void SmartNavigationPanel::setQuickActions(const QList<QAction*>& actions) {
    m_quickActions = actions;
    if (m_quickActions.size() > MAX_QUICK_ACTIONS) {
        m_quickActions = m_quickActions.mid(0, MAX_QUICK_ACTIONS);
    }
    updateQuickActionsDisplay();
}

void SmartNavigationPanel::addQuickAction(QAction* action) {
    if (!action || m_quickActions.contains(action)) return;
    
    m_quickActions.append(action);
    if (m_quickActions.size() > MAX_QUICK_ACTIONS) {
        m_quickActions.removeFirst();
    }
    
    updateQuickActionsDisplay();
}

void SmartNavigationPanel::setCollapsed(bool collapsed) {
    if (m_collapsed == collapsed) return;
    
    m_collapsed = collapsed;
    animateCollapse(collapsed);
    
    emit collapsedStateChanged(collapsed);
}

void SmartNavigationPanel::setAccessibilityEnabled(bool enabled) {
    m_accessibilityEnabled = enabled;
    updateAccessibilityInfo();
}

void SmartNavigationPanel::paintEvent(QPaintEvent* event) {
    QStyleOption opt;
    opt.initFrom(this);
    QPainter painter(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);
    
    QWidget::paintEvent(event);
}

void SmartNavigationPanel::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updateLayout();
}

void SmartNavigationPanel::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        setFocus();
    }
    QWidget::mousePressEvent(event);
}

void SmartNavigationPanel::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
    case Qt::Key_Space:
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (m_navigationList && m_navigationList->hasFocus()) {
            onNavigationItemClicked();
            return;
        }
        break;
    case Qt::Key_Up:
        if (m_navigationList && m_navigationList->currentRow() > 0) {
            m_navigationList->setCurrentRow(m_navigationList->currentRow() - 1);
            return;
        }
        break;
    case Qt::Key_Down:
        if (m_navigationList && m_navigationList->currentRow() < m_navigationList->count() - 1) {
            m_navigationList->setCurrentRow(m_navigationList->currentRow() + 1);
            return;
        }
        break;
    case Qt::Key_Escape:
        if (m_collapsed) {
            setCollapsed(false);
            return;
        }
        break;
    }
    
    QWidget::keyPressEvent(event);
}

void SmartNavigationPanel::focusInEvent(QFocusEvent* event) {
    QWidget::focusInEvent(event);
    
    if (m_navigationList && !m_navigationList->currentItem()) {
        m_navigationList->setCurrentRow(0);
    }
}

void SmartNavigationPanel::initializeUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    createHeaderSection();
    createNavigationSection();
    createBreadcrumbSection();
    createRecentFilesSection();
    createQuickActionsSection();
    createFooterSection();
    
    setLayout(m_mainLayout);
}

void SmartNavigationPanel::createHeaderSection() {
    m_headerSection = new QWidget(this);
    m_headerSection->setObjectName("headerSection");
    m_headerLayout = new QHBoxLayout(m_headerSection);
    m_headerLayout->setContentsMargins(16, 16, 16, 8);
    
    m_titleLabel = new QLabel(tr("Navigation"), m_headerSection);
    m_titleLabel->setObjectName("titleLabel");
    m_titleLabel->setStyleSheet("font-weight: 600; font-size: 16px;");
    
    m_collapseButton = new QPushButton(m_headerSection);
    m_collapseButton->setObjectName("collapseButton");
    m_collapseButton->setFixedSize(32, 32);
    m_collapseButton->setIcon(createIcon("menu")->grab().scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_collapseButton->setToolTip(tr("Toggle navigation panel"));
    
    connect(m_collapseButton, &QPushButton::clicked,
            this, &SmartNavigationPanel::onCollapseToggled);
    
    m_headerLayout->addWidget(m_titleLabel);
    m_headerLayout->addStretch();
    m_headerLayout->addWidget(m_collapseButton);
    
    m_mainLayout->addWidget(m_headerSection);
}

void SmartNavigationPanel::createNavigationSection() {
    m_navigationSection = new QWidget(this);
    m_navigationSection->setObjectName("navigationSection");
    m_navigationLayout = new QVBoxLayout(m_navigationSection);
    m_navigationLayout->setContentsMargins(8, 0, 8, 0);
    
    m_navigationList = new QListWidget(m_navigationSection);
    m_navigationList->setObjectName("navigationList");
    m_navigationList->setFrameStyle(QFrame::NoFrame);
    m_navigationList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_navigationList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_navigationList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    connect(m_navigationList, &QListWidget::itemClicked,
            this, &SmartNavigationPanel::onNavigationItemClicked);
    connect(m_navigationList, &QListWidget::itemActivated,
            this, &SmartNavigationPanel::onNavigationItemClicked);
    
    m_navigationLayout->addWidget(m_navigationList);
    m_mainLayout->addWidget(m_navigationSection);
}

void SmartNavigationPanel::createBreadcrumbSection() {
    m_breadcrumbSection = new QWidget(this);
    m_breadcrumbSection->setObjectName("breadcrumbSection");
    m_breadcrumbSection->setVisible(false); // Initially hidden
    
    m_breadcrumbLayout = new QHBoxLayout(m_breadcrumbSection);
    m_breadcrumbLayout->setContentsMargins(16, 8, 16, 8);
    
    m_mainLayout->addWidget(m_breadcrumbSection);
}

void SmartNavigationPanel::createRecentFilesSection() {
    m_recentFilesSection = new QWidget(this);
    m_recentFilesSection->setObjectName("recentFilesSection");
    m_recentFilesLayout = new QVBoxLayout(m_recentFilesSection);
    m_recentFilesLayout->setContentsMargins(16, 8, 16, 8);
    
    m_recentFilesTitle = new QLabel(tr("Recent Files"), m_recentFilesSection);
    m_recentFilesTitle->setObjectName("recentFilesTitle");
    m_recentFilesTitle->setStyleSheet("font-weight: 600; font-size: 14px; color: rgba(0,0,0,0.6);");
    
    m_recentFilesList = new QListWidget(m_recentFilesSection);
    m_recentFilesList->setObjectName("recentFilesList");
    m_recentFilesList->setFrameStyle(QFrame::NoFrame);
    m_recentFilesList->setMaximumHeight(120);
    
    connect(m_recentFilesList, &QListWidget::itemClicked,
            this, &SmartNavigationPanel::onRecentFileClicked);
    
    m_recentFilesLayout->addWidget(m_recentFilesTitle);
    m_recentFilesLayout->addWidget(m_recentFilesList);
    
    m_mainLayout->addWidget(m_recentFilesSection);
}

void SmartNavigationPanel::createQuickActionsSection() {
    m_quickActionsSection = new QWidget(this);
    m_quickActionsSection->setObjectName("quickActionsSection");
    m_quickActionsLayout = new QVBoxLayout(m_quickActionsSection);
    m_quickActionsLayout->setContentsMargins(16, 8, 16, 8);
    
    m_quickActionsTitle = new QLabel(tr("Quick Actions"), m_quickActionsSection);
    m_quickActionsTitle->setObjectName("quickActionsTitle");
    m_quickActionsTitle->setStyleSheet("font-weight: 600; font-size: 14px; color: rgba(0,0,0,0.6);");
    
    m_quickActionsLayout->addWidget(m_quickActionsTitle);
    
    m_mainLayout->addWidget(m_quickActionsSection);
}

void SmartNavigationPanel::createFooterSection() {
    m_footerSection = new QWidget(this);
    m_footerSection->setObjectName("footerSection");
    m_footerLayout = new QHBoxLayout(m_footerSection);
    m_footerLayout->setContentsMargins(16, 8, 16, 16);
    
    m_statusLabel = new QLabel(m_footerSection);
    m_statusLabel->setObjectName("statusLabel");
    m_statusLabel->setStyleSheet("font-size: 12px; color: rgba(0,0,0,0.6);");
    m_statusLabel->setWordWrap(true);
    
    m_footerLayout->addWidget(m_statusLabel);
    
    m_mainLayout->addStretch();
    m_mainLayout->addWidget(m_footerSection);
}

void SmartNavigationPanel::updateNavigationItems() {
    if (!m_navigationList) return;
    
    m_navigationList->clear();
    
    // Add navigation items based on current mode
    switch (m_currentMode) {
    case NavigationMode::Welcome:
        addNavigationItem(NavigationItem::CreateArchive, tr("Create Archive"), "add-circle");
        addNavigationItem(NavigationItem::OpenArchive, tr("Open Archive"), "folder-open");
        addNavigationItem(NavigationItem::ExtractArchive, tr("Extract Archive"), "download");
        addNavigationItem(NavigationItem::Settings, tr("Settings"), "settings");
        addNavigationItem(NavigationItem::Help, tr("Help"), "help-circle");
        break;
        
    case NavigationMode::Archive:
        addNavigationItem(NavigationItem::Home, tr("Home"), "home");
        addNavigationItem(NavigationItem::BrowseArchive, tr("Browse"), "folder");
        addNavigationItem(NavigationItem::ExtractArchive, tr("Extract"), "download");
        addNavigationItem(NavigationItem::Settings, tr("Settings"), "settings");
        break;
        
    case NavigationMode::Creation:
        addNavigationItem(NavigationItem::Home, tr("Home"), "home");
        addNavigationItem(NavigationItem::CreateArchive, tr("Create"), "add-circle");
        addNavigationItem(NavigationItem::Settings, tr("Settings"), "settings");
        break;
        
    case NavigationMode::Extraction:
        addNavigationItem(NavigationItem::Home, tr("Home"), "home");
        addNavigationItem(NavigationItem::ExtractArchive, tr("Extract"), "download");
        addNavigationItem(NavigationItem::Settings, tr("Settings"), "settings");
        break;
        
    case NavigationMode::Settings:
        addNavigationItem(NavigationItem::Home, tr("Home"), "home");
        addNavigationItem(NavigationItem::Settings, tr("Settings"), "settings");
        break;
    }
    
    // Set current selection
    for (int i = 0; i < m_navigationList->count(); ++i) {
        QListWidgetItem* item = m_navigationList->item(i);
        if (item && item->data(Qt::UserRole).toInt() == static_cast<int>(m_currentItem)) {
            m_navigationList->setCurrentItem(item);
            break;
        }
    }
}

void SmartNavigationPanel::addNavigationItem(NavigationItem item, const QString& text, const QString& iconName) {
    QListWidgetItem* listItem = new QListWidgetItem(m_navigationList);
    listItem->setText(text);
    listItem->setData(Qt::UserRole, static_cast<int>(item));
    
    // Create icon
    QSvgWidget* iconWidget = createIcon(iconName);
    if (iconWidget) {
        QPixmap pixmap = iconWidget->grab();
        listItem->setIcon(QIcon(pixmap));
        delete iconWidget;
    }
    
    // Set accessibility properties
    if (m_accessibilityEnabled) {
        listItem->setToolTip(text);
        listItem->setWhatsThis(tr("Navigate to %1").arg(text));
    }
}

void SmartNavigationPanel::updateBreadcrumbDisplay() {
    if (!m_breadcrumbLayout) return;
    
    // Clear existing breadcrumb items
    QLayoutItem* item;
    while ((item = m_breadcrumbLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    
    if (m_breadcrumbItems.isEmpty()) {
        m_breadcrumbSection->setVisible(false);
        return;
    }
    
    m_breadcrumbSection->setVisible(true);
    
    for (int i = 0; i < m_breadcrumbItems.size(); ++i) {
        const BreadcrumbItem& breadcrumb = m_breadcrumbItems[i];
        
        if (i > 0) {
            // Add separator
            QLabel* separator = new QLabel("›", m_breadcrumbSection);
            separator->setStyleSheet("color: rgba(0,0,0,0.4); font-size: 12px;");
            m_breadcrumbLayout->addWidget(separator);
        }
        
        QPushButton* button = new QPushButton(breadcrumb.text, m_breadcrumbSection);
        button->setObjectName("breadcrumbButton");
        button->setFlat(true);
        button->setProperty("path", breadcrumb.path);
        button->setEnabled(breadcrumb.clickable);
        button->setStyleSheet("QPushButton { border: none; padding: 4px 8px; font-size: 12px; } "
                             "QPushButton:hover { background-color: rgba(0,0,0,0.04); border-radius: 4px; }");
        
        if (breadcrumb.clickable) {
            connect(button, &QPushButton::clicked,
                    this, &SmartNavigationPanel::onBreadcrumbItemClicked);
        }
        
        m_breadcrumbLayout->addWidget(button);
    }
    
    m_breadcrumbLayout->addStretch();
}

void SmartNavigationPanel::updateRecentFilesDisplay() {
    if (!m_recentFilesList) return;
    
    m_recentFilesList->clear();
    
    for (const QString& filePath : m_recentFiles) {
        QFileInfo fileInfo(filePath);
        QListWidgetItem* item = new QListWidgetItem(m_recentFilesList);
        item->setText(fileInfo.fileName());
        item->setToolTip(filePath);
        item->setData(Qt::UserRole, filePath);
        
        // Add file type icon
        QSvgWidget* iconWidget = createIcon("file");
        if (iconWidget) {
            QPixmap pixmap = iconWidget->grab().scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            item->setIcon(QIcon(pixmap));
            delete iconWidget;
        }
    }
    
    m_recentFilesSection->setVisible(!m_recentFiles.isEmpty() && !m_collapsed);
}

void SmartNavigationPanel::updateQuickActionsDisplay() {
    if (!m_quickActionsLayout) return;
    
    // Remove existing action buttons
    for (QPushButton* button : m_quickActionButtons) {
        m_quickActionsLayout->removeWidget(button);
        delete button;
    }
    m_quickActionButtons.clear();
    
    for (QAction* action : m_quickActions) {
        QPushButton* button = new QPushButton(action->text(), m_quickActionsSection);
        button->setObjectName("quickActionButton");
        button->setIcon(action->icon());
        button->setToolTip(action->toolTip());
        button->setEnabled(action->isEnabled());
        button->setProperty("action", QVariant::fromValue(action));
        
        connect(button, &QPushButton::clicked,
                this, &SmartNavigationPanel::onQuickActionClicked);
        connect(action, &QAction::changed, [button, action]() {
            button->setText(action->text());
            button->setIcon(action->icon());
            button->setToolTip(action->toolTip());
            button->setEnabled(action->isEnabled());
        });
        
        m_quickActionButtons.append(button);
        m_quickActionsLayout->addWidget(button);
    }
    
    m_quickActionsSection->setVisible(!m_quickActions.isEmpty() && !m_collapsed);
}

void SmartNavigationPanel::updateVisualState() {
    if (!m_titleLabel || !m_statusLabel) return;
    
    // Update title based on mode
    switch (m_currentMode) {
    case NavigationMode::Welcome:
        m_titleLabel->setText(tr("Welcome"));
        m_statusLabel->setText(tr("Ready to manage archives"));
        break;
    case NavigationMode::Archive:
        m_titleLabel->setText(tr("Archive"));
        m_statusLabel->setText(tr("Browsing archive contents"));
        break;
    case NavigationMode::Creation:
        m_titleLabel->setText(tr("Create"));
        m_statusLabel->setText(tr("Creating new archive"));
        break;
    case NavigationMode::Extraction:
        m_titleLabel->setText(tr("Extract"));
        m_statusLabel->setText(tr("Extracting archive"));
        break;
    case NavigationMode::Settings:
        m_titleLabel->setText(tr("Settings"));
        m_statusLabel->setText(tr("Configure application"));
        break;
    }
    
    // Update visibility based on collapsed state
    m_titleLabel->setVisible(!m_collapsed);
    m_recentFilesSection->setVisible(!m_recentFiles.isEmpty() && !m_collapsed);
    m_quickActionsSection->setVisible(!m_quickActions.isEmpty() && !m_collapsed);
    m_statusLabel->setVisible(!m_collapsed);
    
    update();
}

QSvgWidget* SmartNavigationPanel::createIcon(const QString& iconName, const QSize& size) {
    QString iconPath = QString(":/icons/%1.svg").arg(iconName);
    
    QSvgWidget* svgWidget = new QSvgWidget(iconPath);
    svgWidget->setFixedSize(size);
    
    return svgWidget;
}

void SmartNavigationPanel::animateCollapse(bool collapse) {
    int targetWidth = collapse ? COLLAPSED_WIDTH : EXPANDED_WIDTH;
    
    m_collapseAnimation->setStartValue(width());
    m_collapseAnimation->setEndValue(targetWidth);
    m_collapseAnimation->start();
    
    updateVisualState();
}

void SmartNavigationPanel::setupAccessibility() {
    setAccessibleName(tr("Navigation Panel"));
    setAccessibleDescription(tr("Main navigation panel for archive operations"));
    
    if (m_navigationList) {
        m_navigationList->setAccessibleName(tr("Navigation Items"));
        m_navigationList->setAccessibleDescription(tr("List of available navigation options"));
    }
}

void SmartNavigationPanel::updateAccessibilityInfo() {
    if (!m_accessibilityEnabled) return;
    
    // Update accessible properties for all interactive elements
    if (m_collapseButton) {
        m_collapseButton->setAccessibleName(m_collapsed ? tr("Expand navigation") : tr("Collapse navigation"));
    }
}

void SmartNavigationPanel::announceStateChange(const QString& message) {
    if (!m_accessibilityEnabled) return;
    
    // Use QAccessible to announce state changes to screen readers
    QAccessible::updateAccessibility(new QAccessibleEvent(this, QAccessible::NameChanged));
}

void SmartNavigationPanel::onNavigationItemClicked() {
    QListWidgetItem* item = m_navigationList->currentItem();
    if (!item) return;
    
    NavigationItem navItem = static_cast<NavigationItem>(item->data(Qt::UserRole).toInt());
    setCurrentItem(navItem);
    
    emit navigationItemClicked(navItem);
}

void SmartNavigationPanel::onBreadcrumbItemClicked() {
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (!button) return;
    
    QString path = button->property("path").toString();
    emit breadcrumbItemClicked(path);
}

void SmartNavigationPanel::onRecentFileClicked() {
    QListWidgetItem* item = m_recentFilesList->currentItem();
    if (!item) return;
    
    QString filePath = item->data(Qt::UserRole).toString();
    emit recentFileRequested(filePath);
}

void SmartNavigationPanel::onQuickActionClicked() {
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (!button) return;
    
    QAction* action = button->property("action").value<QAction*>();
    if (action) {
        emit quickActionTriggered(action);
        action->trigger();
    }
}

void SmartNavigationPanel::onCollapseToggled() {
    setCollapsed(!m_collapsed);
}

void SmartNavigationPanel::onAnimationFinished() {
    updateLayout();
}

void SmartNavigationPanel::updateLayout() {
    adjustSizeForContent();
}

void SmartNavigationPanel::adjustSizeForContent() {
    int width = m_collapsed ? COLLAPSED_WIDTH : EXPANDED_WIDTH;
    setFixedWidth(width);
    setMinimumWidth(width);
    setMaximumWidth(width);
}

} // namespace FluxGUI::UI::Components

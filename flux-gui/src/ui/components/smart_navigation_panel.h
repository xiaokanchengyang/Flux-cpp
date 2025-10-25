#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QStackedWidget>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QTimer>
#include <QSvgWidget>
#include <memory>

QT_BEGIN_NAMESPACE
class QResizeEvent;
class QPaintEvent;
class QMouseEvent;
QT_END_NAMESPACE

namespace FluxGUI::UI::Components {

/**
 * Smart Navigation Panel
 * 
 * Modern navigation component with:
 * - SVG icon support with proper theming
 * - Contextual navigation based on current operation
 * - Breadcrumb trail for archive exploration
 * - Quick action shortcuts and recent files
 * - Smooth animations and hover effects
 * - Accessibility support with keyboard navigation
 */
class SmartNavigationPanel : public QWidget {
    Q_OBJECT

public:
    enum class NavigationMode {
        Welcome,        // Initial state with main actions
        Archive,        // Archive browsing with file operations
        Creation,       // Archive creation workflow
        Extraction,     // Archive extraction workflow
        Settings        // Application settings
    };

    enum class NavigationItem {
        Home = 0,
        CreateArchive,
        OpenArchive,
        ExtractArchive,
        BrowseArchive,
        RecentFiles,
        Settings,
        Help
    };

    struct BreadcrumbItem {
        QString text;
        QString path;
        QString icon;
        bool clickable{true};
    };

    explicit SmartNavigationPanel(QWidget* parent = nullptr);
    ~SmartNavigationPanel() override;

    // Mode management
    void setMode(NavigationMode mode);
    NavigationMode currentMode() const { return m_currentMode; }

    // Navigation state
    void setCurrentItem(NavigationItem item);
    NavigationItem currentItem() const { return m_currentItem; }

    // Breadcrumb management
    void setBreadcrumb(const QList<BreadcrumbItem>& items);
    void addBreadcrumbItem(const BreadcrumbItem& item);
    void clearBreadcrumb();

    // Recent files management
    void setRecentFiles(const QStringList& files);
    void addRecentFile(const QString& filePath);

    // Quick actions
    void setQuickActions(const QList<QAction*>& actions);
    void addQuickAction(QAction* action);

    // Visual state
    void setCollapsed(bool collapsed);
    bool isCollapsed() const { return m_collapsed; }

    // Accessibility
    void setAccessibilityEnabled(bool enabled);
    bool isAccessibilityEnabled() const { return m_accessibilityEnabled; }

signals:
    // Navigation signals
    void navigationItemClicked(NavigationItem item);
    void breadcrumbItemClicked(const QString& path);
    void recentFileRequested(const QString& filePath);
    void quickActionTriggered(QAction* action);

    // State change signals
    void modeChanged(NavigationMode newMode);
    void collapsedStateChanged(bool collapsed);

protected:
    // Event handlers
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;

private slots:
    void onNavigationItemClicked();
    void onBreadcrumbItemClicked();
    void onRecentFileClicked();
    void onQuickActionClicked();
    void onCollapseToggled();
    void onAnimationFinished();

private:
    // UI initialization
    void initializeUI();
    void createHeaderSection();
    void createNavigationSection();
    void createBreadcrumbSection();
    void createRecentFilesSection();
    void createQuickActionsSection();
    void createFooterSection();

    // Visual updates
    void updateNavigationItems();
    void updateBreadcrumbDisplay();
    void updateRecentFilesDisplay();
    void updateQuickActionsDisplay();
    void updateVisualState();

    // Animation management
    void animateTransition();
    void animateCollapse(bool collapse);
    void animateHover(QWidget* widget, bool hover);

    // Icon management
    QSvgWidget* createIcon(const QString& iconName, const QSize& size = QSize(24, 24));
    void updateIconTheme();

    // Accessibility helpers
    void setupAccessibility();
    void updateAccessibilityInfo();
    void announceStateChange(const QString& message);

    // Layout management
    void updateLayout();
    void adjustSizeForContent();

private:
    // Current state
    NavigationMode m_currentMode{NavigationMode::Welcome};
    NavigationItem m_currentItem{NavigationItem::Home};
    bool m_collapsed{false};
    bool m_accessibilityEnabled{true};

    // UI components
    QVBoxLayout* m_mainLayout{nullptr};
    
    // Header section
    QWidget* m_headerSection{nullptr};
    QHBoxLayout* m_headerLayout{nullptr};
    QLabel* m_titleLabel{nullptr};
    QPushButton* m_collapseButton{nullptr};
    
    // Navigation section
    QWidget* m_navigationSection{nullptr};
    QVBoxLayout* m_navigationLayout{nullptr};
    QListWidget* m_navigationList{nullptr};
    
    // Breadcrumb section
    QWidget* m_breadcrumbSection{nullptr};
    QHBoxLayout* m_breadcrumbLayout{nullptr};
    QList<BreadcrumbItem> m_breadcrumbItems;
    
    // Recent files section
    QWidget* m_recentFilesSection{nullptr};
    QVBoxLayout* m_recentFilesLayout{nullptr};
    QLabel* m_recentFilesTitle{nullptr};
    QListWidget* m_recentFilesList{nullptr};
    QStringList m_recentFiles;
    
    // Quick actions section
    QWidget* m_quickActionsSection{nullptr};
    QVBoxLayout* m_quickActionsLayout{nullptr};
    QLabel* m_quickActionsTitle{nullptr};
    QList<QAction*> m_quickActions;
    QList<QPushButton*> m_quickActionButtons;
    
    // Footer section
    QWidget* m_footerSection{nullptr};
    QHBoxLayout* m_footerLayout{nullptr};
    QLabel* m_statusLabel{nullptr};

    // Animation components
    std::unique_ptr<QPropertyAnimation> m_collapseAnimation;
    std::unique_ptr<QPropertyAnimation> m_fadeAnimation;
    std::unique_ptr<QGraphicsOpacityEffect> m_opacityEffect;
    QTimer* m_hoverTimer{nullptr};

    // Constants
    static constexpr int EXPANDED_WIDTH = 280;
    static constexpr int COLLAPSED_WIDTH = 64;
    static constexpr int ANIMATION_DURATION = 250;
    static constexpr int HOVER_DELAY = 100;
    static constexpr int MAX_RECENT_FILES = 10;
    static constexpr int MAX_QUICK_ACTIONS = 6;
};

} // namespace FluxGUI::UI::Components

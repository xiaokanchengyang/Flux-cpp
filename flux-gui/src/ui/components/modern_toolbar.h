#pragma once

#include <QWidget>
#include <QHBoxLayout>
#include <QToolButton>
#include <QLabel>
#include <QPropertyAnimation>
#include <QButtonGroup>
#include <QMenu>
#include <QAction>
#include <memory>

QT_BEGIN_NAMESPACE
class QResizeEvent;
class QPaintEvent;
QT_END_NAMESPACE

namespace FluxGUI::UI::Components {

/**
 * Modern Toolbar Component
 * 
 * Provides a clean, context-aware toolbar with smooth animations
 * and adaptive layout that changes based on current application state.
 */
class ModernToolbar : public QWidget {
    Q_OBJECT

public:
    enum class ToolbarMode {
        Welcome,        // Initial state with basic actions
        Archive,        // Archive browsing with archive-specific actions
        Creation,       // Archive creation mode
        Extraction,     // Archive extraction mode
        Settings        // Settings view
    };

    explicit ModernToolbar(QWidget* parent = nullptr);
    ~ModernToolbar() override;

    // Mode management
    void setMode(ToolbarMode mode);
    ToolbarMode currentMode() const { return m_currentMode; }

    // Action management
    void addAction(QAction* action, const QString& tooltip = QString());
    void addSeparator();
    void addWidget(QWidget* widget);
    void clearActions();

    // State management
    void setBackButtonVisible(bool visible);
    void setTitle(const QString& title);
    void setSubtitle(const QString& subtitle);

signals:
    // Navigation signals
    void backRequested();
    void homeRequested();
    void settingsRequested();
    
    // Action signals
    void newArchiveRequested();
    void openArchiveRequested();
    void extractAllRequested();
    void addFilesRequested();
    void removeFilesRequested();
    
    // View signals
    void viewModeChanged(int mode);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onBackClicked();
    void onHomeClicked();
    void onSettingsClicked();
    void onModeAnimationFinished();

private:
    // UI setup
    void initializeUI();
    void createNavigationSection();
    void createTitleSection();
    void createActionSection();
    void createModeButtons();
    void setupAnimations();
    void applyStyles();
    
    // Mode management
    void updateModeActions();
    void animateModeChange();
    void showModeSpecificActions();
    void hideModeSpecificActions();
    
    // Layout management
    void updateLayout();
    void adjustButtonSizes();

private:
    // Layout components
    QHBoxLayout* m_mainLayout{nullptr};
    QWidget* m_navigationSection{nullptr};
    QWidget* m_titleSection{nullptr};
    QWidget* m_actionSection{nullptr};
    
    // Navigation buttons
    QToolButton* m_backButton{nullptr};
    QToolButton* m_homeButton{nullptr};
    QToolButton* m_settingsButton{nullptr};
    
    // Title display
    QLabel* m_titleLabel{nullptr};
    QLabel* m_subtitleLabel{nullptr};
    
    // Action buttons
    QToolButton* m_newArchiveButton{nullptr};
    QToolButton* m_openArchiveButton{nullptr};
    QToolButton* m_extractAllButton{nullptr};
    QToolButton* m_addFilesButton{nullptr};
    QToolButton* m_removeFilesButton{nullptr};
    
    // Mode selection
    QButtonGroup* m_modeButtonGroup{nullptr};
    QWidget* m_modeButtonContainer{nullptr};
    
    // State management
    ToolbarMode m_currentMode{ToolbarMode::Welcome};
    QString m_currentTitle;
    QString m_currentSubtitle;
    bool m_backButtonVisible{false};
    
    // Animations
    std::unique_ptr<QPropertyAnimation> m_modeAnimation;
    std::unique_ptr<QPropertyAnimation> m_titleAnimation;
    
    // Constants
    static constexpr int TOOLBAR_HEIGHT = 64;
    static constexpr int BUTTON_SIZE = 40;
    static constexpr int SPACING = 12;
    static constexpr int ANIMATION_DURATION = 250;
};

} // namespace FluxGUI::UI::Components
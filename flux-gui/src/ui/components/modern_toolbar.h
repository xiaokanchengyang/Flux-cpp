#pragma once

#include <QWidget>

class QHBoxLayout;
class QPushButton;
class QButtonGroup;
class QLabel;
class QPropertyAnimation;

namespace FluxGUI::UI::Components {

/**
 * @brief Modern navigation toolbar with mode switching
 * 
 * The ModernToolbar provides:
 * - Mode-based navigation (Welcome, Explorer, Compression, Extraction)
 * - Dynamic title and breadcrumb display
 * - Smooth animations and transitions
 * - Action buttons for common operations
 */
class ModernToolbar : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Toolbar modes corresponding to application states
     */
    enum class Mode {
        Welcome = 0,        // Welcome/home screen
        Explorer = 1,       // Archive explorer
        Compression = 2,    // Archive creation
        Extraction = 3,     // Archive extraction
        Settings = 4        // Application settings
    };

    /**
     * @brief Construct the modern toolbar
     * @param parent Parent widget
     */
    explicit ModernToolbar(QWidget* parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~ModernToolbar() override;

public slots:
    /**
     * @brief Set the current mode
     * @param mode New mode
     */
    void setMode(Mode mode);
    
    /**
     * @brief Set the toolbar title
     * @param title New title
     */
    void setTitle(const QString& title);
    
    /**
     * @brief Set the breadcrumb text
     * @param breadcrumb Breadcrumb text
     */
    void setBreadcrumb(const QString& breadcrumb);

public:
    /**
     * @brief Get the current mode
     * @return Current mode
     */
    Mode currentMode() const;

signals:
    /**
     * @brief Emitted when mode changes
     * @param newMode New mode
     * @param previousMode Previous mode
     */
    void modeChanged(int newMode, int previousMode);
    
    /**
     * @brief Emitted when mode change is requested
     * @param mode Requested mode
     */
    void modeChangeRequested(int mode);

private:
    /**
     * @brief Initialize the user interface
     */
    void initializeUI();
    
    /**
     * @brief Create navigation section
     * @param layout Target layout
     */
    void createNavigationSection(QHBoxLayout* layout);
    
    /**
     * @brief Create title section
     * @param layout Target layout
     */
    void createTitleSection(QHBoxLayout* layout);
    
    /**
     * @brief Create action section
     * @param layout Target layout
     */
    void createActionSection(QHBoxLayout* layout);
    
    /**
     * @brief Create a mode button
     * @param iconPath Icon file path
     * @param text Button text
     * @param mode Associated mode
     * @param parent Parent widget
     * @return Created button
     */
    QPushButton* createModeButton(const QString& iconPath, const QString& text, 
                                Mode mode, QWidget* parent);
    
    /**
     * @brief Create an action button
     * @param iconPath Icon file path
     * @param text Tooltip text
     * @param parent Parent widget
     * @return Created button
     */
    QPushButton* createActionButton(const QString& iconPath, const QString& text, 
                                  QWidget* parent);
    
    /**
     * @brief Connect signals and slots
     */
    void connectSignals();
    
    /**
     * @brief Update button states
     */
    void updateButtonStates();
    
    /**
     * @brief Update title and breadcrumb
     */
    void updateTitleAndBreadcrumb();
    
    /**
     * @brief Animate mode transition
     */
    void animateTransition();
    
    /**
     * @brief Apply styling
     */
    void applyStyles();

private:
    Mode m_currentMode;
    
    // Navigation buttons
    QPushButton* m_welcomeButton;
    QPushButton* m_explorerButton;
    QPushButton* m_compressionButton;
    QPushButton* m_extractionButton;
    QPushButton* m_settingsButton;
    QButtonGroup* m_modeButtonGroup;
    
    // Title section
    QLabel* m_titleLabel;
    QLabel* m_breadcrumbLabel;
    
    // Animation
    QPropertyAnimation* m_fadeAnimation;
};

} // namespace FluxGUI::UI::Components

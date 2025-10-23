#pragma once

#include <QStatusBar>

class QLabel;
class QProgressBar;
class QPushButton;
class QTimer;
class QPropertyAnimation;

namespace FluxGUI::UI::Components {

/**
 * @brief Enhanced status bar with progress tracking and theme controls
 * 
 * The StatusBar provides:
 * - Status message display with animations
 * - Progress tracking for long operations
 * - Operation cancellation support
 * - Statistics display
 * - Theme toggle button
 */
class StatusBar : public QStatusBar {
    Q_OBJECT

public:
    /**
     * @brief Construct the status bar
     * @param parent Parent widget
     */
    explicit StatusBar(QWidget* parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~StatusBar() override;

public slots:
    /**
     * @brief Show a status message
     * @param message Message text
     * @param timeout Auto-clear timeout (0 = no timeout)
     */
    void showMessage(const QString& message, int timeout = 0);
    
    /**
     * @brief Clear the status message
     */
    void clearMessage();
    
    /**
     * @brief Set operation active state
     * @param active True if operation is active
     */
    void setOperationActive(bool active);
    
    /**
     * @brief Set progress value
     * @param value Current progress value
     * @param maximum Maximum progress value
     */
    void setProgress(int value, int maximum = 100);
    
    /**
     * @brief Set progress text
     * @param text Progress description
     */
    void setProgressText(const QString& text);
    
    /**
     * @brief Set statistics text
     * @param stats Statistics information
     */
    void setStats(const QString& stats);

signals:
    /**
     * @brief Emitted when theme toggle is requested
     */
    void themeToggleRequested();
    
    /**
     * @brief Emitted when operation cancellation is requested
     */
    void operationCancelRequested();

private:
    /**
     * @brief Initialize the user interface
     */
    void initializeUI();
    
    /**
     * @brief Create progress section
     * @param layout Target layout
     */
    void createProgressSection(class QHBoxLayout* layout);
    
    /**
     * @brief Connect signals and slots
     */
    void connectSignals();
    
    /**
     * @brief Update layout
     */
    void updateLayout();
    
    /**
     * @brief Animate message display
     */
    void animateMessage();
    
    /**
     * @brief Apply styling
     */
    void applyStyles();

private:
    // UI elements
    QLabel* m_messageLabel;
    QProgressBar* m_progressBar;
    QPushButton* m_cancelButton;
    QLabel* m_statsLabel;
    QPushButton* m_themeToggleButton;
    
    // State
    QTimer* m_messageTimer;
    QPropertyAnimation* m_fadeAnimation;
    bool m_isOperationActive;
};

} // namespace FluxGUI::UI::Components

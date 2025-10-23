#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QFrame>
#include <QScrollArea>
#include <QPropertyAnimation>
#include <QGraphicsEffect>
#include <QTimer>
#include <memory>

namespace FluxGUI::UI::Widgets {

/**
 * @brief Modern welcome widget with Material Design aesthetics
 * 
 * The WelcomeWidget provides an attractive landing page featuring:
 * - Hero section with application branding
 * - Quick action buttons for common tasks
 * - Recent files list with thumbnails
 * - Feature highlights and tips
 * - Smooth animations and hover effects
 */
class WelcomeWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Construct the welcome widget
     * @param parent Parent widget
     */
    explicit WelcomeWidget(QWidget* parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~WelcomeWidget() override = default;

public slots:
    /**
     * @brief Update recent files list
     * @param recentFiles List of recent file paths
     */
    void updateRecentFiles(const QStringList& recentFiles);
    
    /**
     * @brief Refresh the welcome content
     */
    void refreshContent();

signals:
    /**
     * @brief Emitted when user wants to create new archive
     */
    void createNewArchive();
    
    /**
     * @brief Emitted when user wants to open archive
     */
    void openArchive();
    
    /**
     * @brief Emitted when user wants to extract archive
     */
    void extractArchive();
    
    /**
     * @brief Emitted when user wants to open settings
     */
    void openSettings();
    
    /**
     * @brief Emitted when user selects a recent file
     * @param filePath Path to the selected file
     */
    void openRecentFile(const QString& filePath);

protected:
    /**
     * @brief Handle paint events for custom styling
     * @param event Paint event
     */
    void paintEvent(QPaintEvent* event) override;
    
    /**
     * @brief Handle resize events
     * @param event Resize event
     */
    void resizeEvent(QResizeEvent* event) override;

private slots:
    /**
     * @brief Handle quick action button clicks
     */
    void onQuickActionClicked();
    
    /**
     * @brief Handle recent file item clicks
     */
    void onRecentFileClicked();
    
    /**
     * @brief Update animation frame
     */
    void updateAnimation();

private:
    /**
     * @brief Initialize the user interface
     */
    void initializeUI();
    
    /**
     * @brief Create hero section
     * @return Hero section widget
     */
    QWidget* createHeroSection();
    
    /**
     * @brief Create quick actions section
     * @return Quick actions widget
     */
    QWidget* createQuickActionsSection();
    
    /**
     * @brief Create recent files section
     * @return Recent files widget
     */
    QWidget* createRecentFilesSection();
    
    /**
     * @brief Create features section
     * @return Features widget
     */
    QWidget* createFeaturesSection();
    
    /**
     * @brief Create a modern card widget
     * @param title Card title
     * @param description Card description
     * @param iconName Icon resource name
     * @param clickable Whether the card is clickable
     * @return Card widget
     */
    QWidget* createCard(const QString& title, 
                       const QString& description,
                       const QString& iconName,
                       bool clickable = false);
    
    /**
     * @brief Create a quick action button
     * @param text Button text
     * @param iconName Icon resource name
     * @param description Button description
     * @param actionType Action type identifier
     * @return Action button widget
     */
    QWidget* createQuickActionButton(const QString& text,
                                   const QString& iconName,
                                   const QString& description,
                                   const QString& actionType);
    
    /**
     * @brief Create recent file item
     * @param filePath Path to the file
     * @return Recent file item widget
     */
    QWidget* createRecentFileItem(const QString& filePath);
    
    /**
     * @brief Apply hover effects to widget
     * @param widget Target widget
     */
    void applyHoverEffect(QWidget* widget);
    
    /**
     * @brief Setup animations
     */
    void setupAnimations();
    
    /**
     * @brief Get file type icon for extension
     * @param extension File extension
     * @return Icon resource name
     */
    QString getFileTypeIcon(const QString& extension);

private:
    // Layout components
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_contentWidget = nullptr;
    QVBoxLayout* m_mainLayout = nullptr;
    
    // Section widgets
    QWidget* m_heroSection = nullptr;
    QWidget* m_quickActionsSection = nullptr;
    QWidget* m_recentFilesSection = nullptr;
    QWidget* m_featuresSection = nullptr;
    
    // Hero section components
    QLabel* m_logoLabel = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_subtitleLabel = nullptr;
    QLabel* m_versionLabel = nullptr;
    
    // Quick actions
    QGridLayout* m_quickActionsLayout = nullptr;
    QList<QWidget*> m_quickActionButtons;
    
    // Recent files
    QVBoxLayout* m_recentFilesLayout = nullptr;
    QLabel* m_recentFilesTitle = nullptr;
    QWidget* m_recentFilesContainer = nullptr;
    QList<QWidget*> m_recentFileItems;
    
    // Animation
    QTimer* m_animationTimer = nullptr;
    QPropertyAnimation* m_fadeAnimation = nullptr;
    int m_animationFrame = 0;
    
    // State
    QStringList m_recentFiles;
    
    // Constants
    static constexpr int MaxRecentFilesDisplay = 8;
    static constexpr int CardSpacing = 16;
    static constexpr int SectionSpacing = 32;
};

} // namespace FluxGUI::UI::Widgets

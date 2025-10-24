#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QFrame>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QTimer>
#include <memory>

QT_BEGIN_NAMESPACE
class QDragEnterEvent;
class QDropEvent;
class QPaintEvent;
class QResizeEvent;
QT_END_NAMESPACE

namespace FluxGUI::UI::Views {

/**
 * Modern Welcome View
 * 
 * Redesigned welcome screen with improved user experience:
 * - Clear visual hierarchy with prominent actions
 * - Smart recent files with contextual information
 * - Integrated drag-and-drop with visual feedback
 * - Quick action cards for common tasks
 * - Usage statistics and tips
 */
class ModernWelcomeView : public QWidget {
    Q_OBJECT

public:
    explicit ModernWelcomeView(QWidget* parent = nullptr);
    ~ModernWelcomeView() override;

    // Public interface
    void refreshRecentFiles();
    void showOnboardingTip(const QString& tip);
    void updateStatistics();

signals:
    // Action signals
    void createArchiveRequested();
    void openArchiveRequested();
    void openArchiveRequested(const QString& filePath);
    void extractArchiveRequested();
    void settingsRequested();
    void helpRequested();
    
    // File signals
    void filesDropped(const QStringList& filePaths);
    void recentFileSelected(const QString& filePath);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void onCreateArchiveClicked();
    void onOpenArchiveClicked();
    void onExtractArchiveClicked();
    void onSettingsClicked();
    void onHelpClicked();
    void onRecentFileClicked();
    void onRecentFileDoubleClicked();
    void onClearRecentFilesClicked();
    void onAnimationFinished();

private:
    // UI setup
    void initializeUI();
    void createHeroSection();
    void createQuickActionsSection();
    void createRecentFilesSection();
    void createStatisticsSection();
    void createTipsSection();
    void setupAnimations();
    void applyStyles();
    
    // Content management
    void loadRecentFiles();
    void updateRecentFilesList();
    void loadStatistics();
    void loadRandomTip();
    
    // Visual effects
    void startWelcomeAnimation();
    void animateCardHover(QWidget* card, bool hover);
    void showDropFeedback(bool show);

private:
    // Main layout
    QVBoxLayout* m_mainLayout{nullptr};
    QWidget* m_contentWidget{nullptr};
    
    // Hero section
    QWidget* m_heroSection{nullptr};
    QLabel* m_logoLabel{nullptr};
    QLabel* m_titleLabel{nullptr};
    QLabel* m_subtitleLabel{nullptr};
    
    // Quick actions section
    QWidget* m_quickActionsSection{nullptr};
    QGridLayout* m_actionsLayout{nullptr};
    QPushButton* m_createArchiveCard{nullptr};
    QPushButton* m_openArchiveCard{nullptr};
    QPushButton* m_extractArchiveCard{nullptr};
    QPushButton* m_settingsCard{nullptr};
    
    // Recent files section
    QWidget* m_recentFilesSection{nullptr};
    QLabel* m_recentFilesTitle{nullptr};
    QListWidget* m_recentFilesList{nullptr};
    QPushButton* m_clearRecentButton{nullptr};
    
    // Statistics section
    QWidget* m_statisticsSection{nullptr};
    QLabel* m_statsTitle{nullptr};
    QLabel* m_totalArchivesLabel{nullptr};
    QLabel* m_totalSizeLabel{nullptr};
    QLabel* m_lastUsedLabel{nullptr};
    
    // Tips section
    QWidget* m_tipsSection{nullptr};
    QLabel* m_tipsTitle{nullptr};
    QLabel* m_tipLabel{nullptr};
    QPushButton* m_nextTipButton{nullptr};
    QPushButton* m_helpButton{nullptr};
    
    // Animations and effects
    std::unique_ptr<QPropertyAnimation> m_fadeInAnimation;
    std::unique_ptr<QPropertyAnimation> m_slideInAnimation;
    std::unique_ptr<QGraphicsOpacityEffect> m_opacityEffect;
    
    // State management
    QStringList m_recentFiles;
    QStringList m_tips;
    int m_currentTipIndex{0};
    bool m_isFirstShow{true};
    
    // Statistics
    struct Statistics {
        int totalArchivesCreated{0};
        int totalArchivesOpened{0};
        qint64 totalDataProcessed{0};
        QDateTime lastUsed;
    } m_statistics;
    
    // Timers
    std::unique_ptr<QTimer> m_tipRotationTimer;
    
    // Constants
    static constexpr int HERO_HEIGHT = 200;
    static constexpr int CARD_WIDTH = 180;
    static constexpr int CARD_HEIGHT = 120;
    static constexpr int ANIMATION_DURATION = 300;
    static constexpr int TIP_ROTATION_INTERVAL = 10000; // 10 seconds
};

} // namespace FluxGUI::UI::Views

#pragma once

#include <QObject>
#include <QWidget>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QParallelAnimationGroup>
#include <QGraphicsOpacityEffect>
#include <QGraphicsColorizeEffect>
#include <QGraphicsDropShadowEffect>
#include <QTimer>
#include <QQueue>
#include <QHash>
#include <QVariantAnimation>
#include <memory>

QT_BEGIN_NAMESPACE
class QLabel;
class QProgressBar;
class QSystemTrayIcon;
QT_END_NAMESPACE

namespace FluxGUI::UI::Components {

/**
 * Visual Feedback Manager
 * 
 * Centralized system for managing visual feedback throughout the application:
 * - Smooth animations for state changes
 * - Rich progress indicators with contextual information
 * - Toast notifications for user actions
 * - Loading states and skeleton screens
 * - Success/error state animations
 * - Hover effects and micro-interactions
 */
class VisualFeedbackManager : public QObject {
    Q_OBJECT

public:
    enum class FeedbackType {
        Success,        // Green checkmark, positive feedback
        Warning,        // Yellow/orange warning indicator
        Error,          // Red error indicator
        Information,    // Blue information indicator
        Progress,       // Progress indicator with percentage
        Loading         // Indeterminate loading spinner
    };

    enum class AnimationType {
        FadeIn,         // Fade in animation
        FadeOut,        // Fade out animation
        SlideIn,        // Slide in from direction
        SlideOut,       // Slide out to direction
        Bounce,         // Bounce effect
        Pulse,          // Pulsing effect
        Shake,          // Shake animation for errors
        Glow,           // Glow effect for highlights
        Scale,          // Scale up/down animation
        Rotate          // Rotation animation
    };

    enum class ToastPosition {
        TopLeft,
        TopCenter,
        TopRight,
        BottomLeft,
        BottomCenter,
        BottomRight,
        Center
    };

    struct ToastNotification {
        QString message;
        QString details;
        FeedbackType type{FeedbackType::Information};
        int durationMs{3000};
        bool dismissible{true};
        QString actionText;
        std::function<void()> actionCallback;
    };

    struct ProgressInfo {
        QString operationName;
        QString currentItem;
        int percentage{0};
        qint64 processedBytes{0};
        qint64 totalBytes{0};
        int processedFiles{0};
        int totalFiles{0};
        QString estimatedTimeRemaining;
        bool cancellable{true};
        QString statusMessage;
    };

    explicit VisualFeedbackManager(QObject* parent = nullptr);
    ~VisualFeedbackManager() override;

    // Toast notifications
    void showToast(const ToastNotification& notification);
    void showToast(const QString& message, FeedbackType type = FeedbackType::Information, int durationMs = 3000);
    void showActionToast(const QString& message, const QString& actionText, std::function<void()> callback, FeedbackType type = FeedbackType::Information);
    void dismissAllToasts();

    // Progress feedback
    void startProgress(const QString& operationName, bool cancellable = true);
    void updateProgress(const ProgressInfo& info);
    void finishProgress(bool success, const QString& message = QString());
    void cancelProgress();

    // Loading states
    void showLoadingState(QWidget* widget, const QString& message = QString());
    void hideLoadingState(QWidget* widget);
    void showSkeletonLoader(QWidget* widget);
    void hideSkeletonLoader(QWidget* widget);

    // Widget animations
    void animateWidget(QWidget* widget, AnimationType type, int durationMs = 250);
    void animateWidgetProperty(QWidget* widget, const QByteArray& property, const QVariant& startValue, const QVariant& endValue, int durationMs = 250);
    void animateSequence(const QList<QPair<QWidget*, AnimationType>>& sequence, int durationMs = 250);

    // Hover effects
    void enableHoverEffect(QWidget* widget, AnimationType hoverType = AnimationType::Glow);
    void disableHoverEffect(QWidget* widget);

    // State feedback
    void showSuccessState(QWidget* widget, const QString& message = QString());
    void showErrorState(QWidget* widget, const QString& message = QString());
    void showWarningState(QWidget* widget, const QString& message = QString());
    void clearState(QWidget* widget);

    // System tray notifications
    void showSystemNotification(const QString& title, const QString& message, FeedbackType type = FeedbackType::Information);

    // Configuration
    void setToastPosition(ToastPosition position);
    void setAnimationEnabled(bool enabled);
    void setReducedMotion(bool reduced);
    void setToastParent(QWidget* parent);

signals:
    // Progress signals
    void progressStarted(const QString& operationName);
    void progressUpdated(int percentage, const QString& status);
    void progressFinished(bool success, const QString& message);
    void progressCancelled();

    // Toast signals
    void toastShown(const QString& message, FeedbackType type);
    void toastDismissed(const QString& message);
    void toastActionTriggered(const QString& message);

    // Animation signals
    void animationStarted(QWidget* widget, AnimationType type);
    void animationFinished(QWidget* widget, AnimationType type);

private slots:
    void onToastTimerTimeout();
    void onAnimationFinished();
    void onHoverEntered();
    void onHoverLeft();
    void onProgressCancelRequested();

private:
    // Toast management
    void createToastWidget(const ToastNotification& notification);
    void positionToastWidget(QWidget* toast);
    void animateToastIn(QWidget* toast);
    void animateToastOut(QWidget* toast);
    void cleanupToast(QWidget* toast);

    // Progress UI
    void createProgressWidget();
    void updateProgressWidget(const ProgressInfo& info);
    void hideProgressWidget();

    // Loading UI
    void createLoadingOverlay(QWidget* parent, const QString& message);
    void createSkeletonLoader(QWidget* parent);

    // Animation helpers
    QPropertyAnimation* createFadeAnimation(QWidget* widget, bool fadeIn, int duration);
    QPropertyAnimation* createSlideAnimation(QWidget* widget, bool slideIn, int duration);
    QPropertyAnimation* createScaleAnimation(QWidget* widget, bool scaleUp, int duration);
    QPropertyAnimation* createRotateAnimation(QWidget* widget, int duration);
    QSequentialAnimationGroup* createBounceAnimation(QWidget* widget, int duration);
    QSequentialAnimationGroup* createPulseAnimation(QWidget* widget, int duration);
    QSequentialAnimationGroup* createShakeAnimation(QWidget* widget, int duration);

    // Effect management
    void applyGlowEffect(QWidget* widget, const QColor& color);
    void removeGlowEffect(QWidget* widget);
    void applyDropShadowEffect(QWidget* widget);
    void removeDropShadowEffect(QWidget* widget);

    // Utility functions
    QColor getTypeColor(FeedbackType type) const;
    QString getTypeIcon(FeedbackType type) const;
    QPoint calculateToastPosition(QWidget* toast) const;
    bool shouldUseReducedMotion() const;

private:
    // Configuration
    ToastPosition m_toastPosition{ToastPosition::TopRight};
    bool m_animationEnabled{true};
    bool m_reducedMotion{false};
    QWidget* m_toastParent{nullptr};

    // Toast management
    QQueue<ToastNotification> m_toastQueue;
    QList<QWidget*> m_activeToasts;
    QTimer* m_toastTimer{nullptr};
    static constexpr int MAX_CONCURRENT_TOASTS = 5;
    static constexpr int TOAST_SPACING = 8;

    // Progress tracking
    QWidget* m_progressWidget{nullptr};
    QLabel* m_progressLabel{nullptr};
    QProgressBar* m_progressBar{nullptr};
    QPushButton* m_cancelButton{nullptr};
    bool m_progressActive{false};

    // Animation tracking
    QHash<QWidget*, QPropertyAnimation*> m_activeAnimations;
    QHash<QWidget*, QGraphicsEffect*> m_appliedEffects;
    QHash<QWidget*, QWidget*> m_loadingOverlays;
    QHash<QWidget*, QWidget*> m_skeletonLoaders;

    // Hover effect tracking
    QHash<QWidget*, AnimationType> m_hoverEffects;
    QHash<QWidget*, bool> m_hoverStates;

    // System tray
    QSystemTrayIcon* m_systemTray{nullptr};

    // Constants
    static constexpr int DEFAULT_ANIMATION_DURATION = 250;
    static constexpr int TOAST_DEFAULT_DURATION = 3000;
    static constexpr int PROGRESS_UPDATE_INTERVAL = 100;
    static constexpr int HOVER_ANIMATION_DURATION = 150;
};

} // namespace FluxGUI::UI::Components

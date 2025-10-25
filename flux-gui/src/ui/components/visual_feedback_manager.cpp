#include "visual_feedback_manager.h"
#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSystemTrayIcon>
#include <QGraphicsBlurEffect>
#include <QSoundEffect>
#include <QStandardPaths>
#include <QDir>
#include <QScreen>
#include <QDesktopWidget>
#include <QPainter>
#include <QStyleOption>
#include <QAccessible>
#include <QSettings>

namespace FluxGUI::UI::Components {

VisualFeedbackManager::VisualFeedbackManager(QObject* parent)
    : QObject(parent)
    , m_toastTimer(new QTimer(this))
    , m_systemTray(nullptr)
{
    // Initialize toast timer
    m_toastTimer->setSingleShot(false);
    m_toastTimer->setInterval(100);
    connect(m_toastTimer, &QTimer::timeout,
            this, &VisualFeedbackManager::onToastTimerTimeout);
    
    // Initialize system tray if available
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        m_systemTray = new QSystemTrayIcon(this);
        m_systemTray->setIcon(QIcon(":/icons/app.svg"));
    }
    
    // Load settings
    QSettings settings;
    m_animationEnabled = settings.value("ui/animations_enabled", true).toBool();
    m_reducedMotion = settings.value("accessibility/reduced_motion", false).toBool();
    m_toastPosition = static_cast<ToastPosition>(
        settings.value("ui/toast_position", static_cast<int>(ToastPosition::TopRight)).toInt());
}

VisualFeedbackManager::~VisualFeedbackManager() {
    dismissAllToasts();
}

void VisualFeedbackManager::showToast(const ToastNotification& notification) {
    // Add to queue if we have too many active toasts
    if (m_activeToasts.size() >= MAX_CONCURRENT_TOASTS) {
        m_toastQueue.enqueue(notification);
        return;
    }
    
    createToastWidget(notification);
    
    if (!m_toastTimer->isActive()) {
        m_toastTimer->start();
    }
    
    emit toastShown(notification.message, notification.type);
}

void VisualFeedbackManager::showToast(const QString& message, FeedbackType type, int durationMs) {
    ToastNotification notification;
    notification.message = message;
    notification.type = type;
    notification.durationMs = durationMs;
    showToast(notification);
}

void VisualFeedbackManager::showActionToast(const QString& message, const QString& actionText, 
                                          std::function<void()> callback, FeedbackType type) {
    ToastNotification notification;
    notification.message = message;
    notification.type = type;
    notification.actionText = actionText;
    notification.actionCallback = callback;
    notification.durationMs = 5000; // Longer duration for action toasts
    showToast(notification);
}

void VisualFeedbackManager::dismissAllToasts() {
    for (QWidget* toast : m_activeToasts) {
        animateToastOut(toast);
    }
    m_activeToasts.clear();
    m_toastQueue.clear();
    
    if (m_toastTimer->isActive()) {
        m_toastTimer->stop();
    }
}

void VisualFeedbackManager::startProgress(const QString& operationName, bool cancellable) {
    if (m_progressActive) {
        finishProgress(false, tr("Previous operation cancelled"));
    }
    
    m_progressActive = true;
    createProgressWidget();
    
    ProgressInfo info;
    info.operationName = operationName;
    info.cancellable = cancellable;
    updateProgressWidget(info);
    
    emit progressStarted(operationName);
}

void VisualFeedbackManager::updateProgress(const ProgressInfo& info) {
    if (!m_progressActive) return;
    
    updateProgressWidget(info);
    emit progressUpdated(info.percentage, info.statusMessage);
}

void VisualFeedbackManager::finishProgress(bool success, const QString& message) {
    if (!m_progressActive) return;
    
    m_progressActive = false;
    
    // Show completion feedback
    if (success) {
        showToast(message.isEmpty() ? tr("Operation completed successfully") : message, 
                 FeedbackType::Success);
    } else {
        showToast(message.isEmpty() ? tr("Operation failed") : message, 
                 FeedbackType::Error);
    }
    
    hideProgressWidget();
    emit progressFinished(success, message);
}

void VisualFeedbackManager::cancelProgress() {
    if (!m_progressActive) return;
    
    m_progressActive = false;
    hideProgressWidget();
    
    showToast(tr("Operation cancelled"), FeedbackType::Warning);
    emit progressCancelled();
}

void VisualFeedbackManager::showLoadingState(QWidget* widget, const QString& message) {
    if (!widget || m_loadingOverlays.contains(widget)) return;
    
    createLoadingOverlay(widget, message);
}

void VisualFeedbackManager::hideLoadingState(QWidget* widget) {
    if (!widget || !m_loadingOverlays.contains(widget)) return;
    
    QWidget* overlay = m_loadingOverlays.take(widget);
    
    if (m_animationEnabled && !shouldUseReducedMotion()) {
        QPropertyAnimation* fadeOut = createFadeAnimation(overlay, false, DEFAULT_ANIMATION_DURATION);
        connect(fadeOut, &QPropertyAnimation::finished, [overlay]() {
            overlay->deleteLater();
        });
        fadeOut->start(QPropertyAnimation::DeleteWhenStopped);
    } else {
        overlay->deleteLater();
    }
}

void VisualFeedbackManager::showSkeletonLoader(QWidget* widget) {
    if (!widget || m_skeletonLoaders.contains(widget)) return;
    
    createSkeletonLoader(widget);
}

void VisualFeedbackManager::hideSkeletonLoader(QWidget* widget) {
    if (!widget || !m_skeletonLoaders.contains(widget)) return;
    
    QWidget* skeleton = m_skeletonLoaders.take(widget);
    skeleton->deleteLater();
}

void VisualFeedbackManager::animateWidget(QWidget* widget, AnimationType type, int durationMs) {
    if (!widget || !m_animationEnabled || shouldUseReducedMotion()) return;
    
    QPropertyAnimation* animation = nullptr;
    
    switch (type) {
    case AnimationType::FadeIn:
        animation = createFadeAnimation(widget, true, durationMs);
        break;
    case AnimationType::FadeOut:
        animation = createFadeAnimation(widget, false, durationMs);
        break;
    case AnimationType::SlideIn:
        animation = createSlideAnimation(widget, true, durationMs);
        break;
    case AnimationType::SlideOut:
        animation = createSlideAnimation(widget, false, durationMs);
        break;
    case AnimationType::Scale:
        animation = createScaleAnimation(widget, true, durationMs);
        break;
    case AnimationType::Rotate:
        animation = createRotateAnimation(widget, durationMs);
        break;
    default:
        return;
    }
    
    if (animation) {
        // Store animation to prevent deletion
        m_activeAnimations[widget] = animation;
        
        connect(animation, &QPropertyAnimation::finished, [this, widget, type]() {
            m_activeAnimations.remove(widget);
            emit animationFinished(widget, type);
        });
        
        animation->start(QPropertyAnimation::DeleteWhenStopped);
        emit animationStarted(widget, type);
    }
}

void VisualFeedbackManager::animateWidgetProperty(QWidget* widget, const QByteArray& property, 
                                                const QVariant& startValue, const QVariant& endValue, 
                                                int durationMs) {
    if (!widget || !m_animationEnabled || shouldUseReducedMotion()) return;
    
    QPropertyAnimation* animation = new QPropertyAnimation(widget, property, this);
    animation->setDuration(durationMs);
    animation->setStartValue(startValue);
    animation->setEndValue(endValue);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    
    m_activeAnimations[widget] = animation;
    
    connect(animation, &QPropertyAnimation::finished, [this, widget]() {
        m_activeAnimations.remove(widget);
    });
    
    animation->start(QPropertyAnimation::DeleteWhenStopped);
}

void VisualFeedbackManager::enableHoverEffect(QWidget* widget, AnimationType hoverType) {
    if (!widget) return;
    
    m_hoverEffects[widget] = hoverType;
    widget->installEventFilter(this);
}

void VisualFeedbackManager::disableHoverEffect(QWidget* widget) {
    if (!widget) return;
    
    m_hoverEffects.remove(widget);
    m_hoverStates.remove(widget);
    widget->removeEventFilter(this);
}

void VisualFeedbackManager::showSuccessState(QWidget* widget, const QString& message) {
    if (!widget) return;
    
    applyGlowEffect(widget, getTypeColor(FeedbackType::Success));
    
    if (!message.isEmpty()) {
        showToast(message, FeedbackType::Success);
    }
    
    // Auto-clear after delay
    QTimer::singleShot(2000, [this, widget]() {
        clearState(widget);
    });
}

void VisualFeedbackManager::showErrorState(QWidget* widget, const QString& message) {
    if (!widget) return;
    
    applyGlowEffect(widget, getTypeColor(FeedbackType::Error));
    animateWidget(widget, AnimationType::Shake, HOVER_ANIMATION_DURATION);
    
    if (!message.isEmpty()) {
        showToast(message, FeedbackType::Error);
    }
    
    // Auto-clear after delay
    QTimer::singleShot(3000, [this, widget]() {
        clearState(widget);
    });
}

void VisualFeedbackManager::showWarningState(QWidget* widget, const QString& message) {
    if (!widget) return;
    
    applyGlowEffect(widget, getTypeColor(FeedbackType::Warning));
    
    if (!message.isEmpty()) {
        showToast(message, FeedbackType::Warning);
    }
    
    // Auto-clear after delay
    QTimer::singleShot(2500, [this, widget]() {
        clearState(widget);
    });
}

void VisualFeedbackManager::clearState(QWidget* widget) {
    if (!widget) return;
    
    removeGlowEffect(widget);
    removeDropShadowEffect(widget);
}

void VisualFeedbackManager::showSystemNotification(const QString& title, const QString& message, 
                                                  FeedbackType type) {
    if (!m_systemTray || !m_systemTray->isVisible()) return;
    
    QSystemTrayIcon::MessageIcon icon;
    switch (type) {
    case FeedbackType::Success:
    case FeedbackType::Information:
        icon = QSystemTrayIcon::Information;
        break;
    case FeedbackType::Warning:
        icon = QSystemTrayIcon::Warning;
        break;
    case FeedbackType::Error:
        icon = QSystemTrayIcon::Critical;
        break;
    default:
        icon = QSystemTrayIcon::NoIcon;
        break;
    }
    
    m_systemTray->showMessage(title, message, icon, 5000);
}

void VisualFeedbackManager::setToastPosition(ToastPosition position) {
    m_toastPosition = position;
    
    QSettings settings;
    settings.setValue("ui/toast_position", static_cast<int>(position));
}

void VisualFeedbackManager::setAnimationEnabled(bool enabled) {
    m_animationEnabled = enabled;
    
    QSettings settings;
    settings.setValue("ui/animations_enabled", enabled);
}

void VisualFeedbackManager::setReducedMotion(bool reduced) {
    m_reducedMotion = reduced;
    
    QSettings settings;
    settings.setValue("accessibility/reduced_motion", reduced);
}

void VisualFeedbackManager::setToastParent(QWidget* parent) {
    m_toastParent = parent;
}

void VisualFeedbackManager::createToastWidget(const ToastNotification& notification) {
    QWidget* toast = new QWidget(m_toastParent);
    toast->setObjectName("toast");
    toast->setAttribute(Qt::WA_DeleteOnClose);
    toast->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    
    // Set style class based on type
    QString styleClass;
    switch (notification.type) {
    case FeedbackType::Success:
        styleClass = "success";
        break;
    case FeedbackType::Warning:
        styleClass = "warning";
        break;
    case FeedbackType::Error:
        styleClass = "error";
        break;
    default:
        styleClass = "info";
        break;
    }
    
    toast->setProperty("class", styleClass);
    
    // Create layout
    QHBoxLayout* layout = new QHBoxLayout(toast);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(12);
    
    // Add icon
    QLabel* iconLabel = new QLabel(toast);
    iconLabel->setText(getTypeIcon(notification.type));
    iconLabel->setStyleSheet("font-size: 18px;");
    layout->addWidget(iconLabel);
    
    // Add content
    QVBoxLayout* contentLayout = new QVBoxLayout();
    contentLayout->setSpacing(4);
    
    QLabel* messageLabel = new QLabel(notification.message, toast);
    messageLabel->setWordWrap(true);
    messageLabel->setStyleSheet("font-weight: 500;");
    contentLayout->addWidget(messageLabel);
    
    if (!notification.details.isEmpty()) {
        QLabel* detailsLabel = new QLabel(notification.details, toast);
        detailsLabel->setWordWrap(true);
        detailsLabel->setStyleSheet("font-size: 12px; color: rgba(0,0,0,0.6);");
        contentLayout->addWidget(detailsLabel);
    }
    
    layout->addLayout(contentLayout);
    
    // Add action button if specified
    if (!notification.actionText.isEmpty() && notification.actionCallback) {
        QPushButton* actionButton = new QPushButton(notification.actionText, toast);
        actionButton->setObjectName("toastActionButton");
        connect(actionButton, &QPushButton::clicked, [notification, this]() {
            if (notification.actionCallback) {
                notification.actionCallback();
            }
            emit toastActionTriggered(notification.message);
        });
        layout->addWidget(actionButton);
    }
    
    // Add dismiss button if dismissible
    if (notification.dismissible) {
        QPushButton* dismissButton = new QPushButton("×", toast);
        dismissButton->setObjectName("toastDismissButton");
        dismissButton->setFixedSize(24, 24);
        dismissButton->setStyleSheet("border: none; font-size: 16px; font-weight: bold;");
        connect(dismissButton, &QPushButton::clicked, [this, toast]() {
            animateToastOut(toast);
        });
        layout->addWidget(dismissButton);
    }
    
    // Position and show toast
    positionToastWidget(toast);
    toast->show();
    
    // Animate in
    animateToastIn(toast);
    
    // Store toast info
    m_activeToasts.append(toast);
    toast->setProperty("duration", notification.durationMs);
    toast->setProperty("startTime", QDateTime::currentMSecsSinceEpoch());
    
    // Setup auto-dismiss timer
    if (notification.durationMs > 0) {
        QTimer::singleShot(notification.durationMs, [this, toast]() {
            if (m_activeToasts.contains(toast)) {
                animateToastOut(toast);
            }
        });
    }
}

void VisualFeedbackManager::positionToastWidget(QWidget* toast) {
    if (!toast) return;
    
    QPoint position = calculateToastPosition(toast);
    toast->move(position);
}

void VisualFeedbackManager::animateToastIn(QWidget* toast) {
    if (!toast || !m_animationEnabled || shouldUseReducedMotion()) {
        return;
    }
    
    // Start with opacity 0 and slide in
    toast->setWindowOpacity(0.0);
    
    QPropertyAnimation* opacityAnim = new QPropertyAnimation(toast, "windowOpacity", this);
    opacityAnim->setDuration(DEFAULT_ANIMATION_DURATION);
    opacityAnim->setStartValue(0.0);
    opacityAnim->setEndValue(1.0);
    opacityAnim->setEasingCurve(QEasingCurve::OutCubic);
    
    QPropertyAnimation* slideAnim = createSlideAnimation(toast, true, DEFAULT_ANIMATION_DURATION);
    
    QParallelAnimationGroup* group = new QParallelAnimationGroup(this);
    group->addAnimation(opacityAnim);
    if (slideAnim) {
        group->addAnimation(slideAnim);
    }
    
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void VisualFeedbackManager::animateToastOut(QWidget* toast) {
    if (!toast) return;
    
    m_activeToasts.removeAll(toast);
    
    if (!m_animationEnabled || shouldUseReducedMotion()) {
        toast->close();
        return;
    }
    
    QPropertyAnimation* opacityAnim = new QPropertyAnimation(toast, "windowOpacity", this);
    opacityAnim->setDuration(DEFAULT_ANIMATION_DURATION);
    opacityAnim->setStartValue(toast->windowOpacity());
    opacityAnim->setEndValue(0.0);
    opacityAnim->setEasingCurve(QEasingCurve::InCubic);
    
    connect(opacityAnim, &QPropertyAnimation::finished, [toast]() {
        toast->close();
    });
    
    opacityAnim->start(QAbstractAnimation::DeleteWhenStopped);
    
    emit toastDismissed(toast->property("message").toString());
}

void VisualFeedbackManager::createProgressWidget() {
    if (m_progressWidget) {
        hideProgressWidget();
    }
    
    m_progressWidget = new QWidget(m_toastParent);
    m_progressWidget->setObjectName("progressWidget");
    m_progressWidget->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    m_progressWidget->setAttribute(Qt::WA_DeleteOnClose);
    
    QVBoxLayout* layout = new QVBoxLayout(m_progressWidget);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(12);
    
    m_progressLabel = new QLabel(m_progressWidget);
    m_progressLabel->setStyleSheet("font-weight: 600; font-size: 14px;");
    layout->addWidget(m_progressLabel);
    
    m_progressBar = new QProgressBar(m_progressWidget);
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(100);
    m_progressBar->setValue(0);
    layout->addWidget(m_progressBar);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_cancelButton = new QPushButton(tr("Cancel"), m_progressWidget);
    connect(m_cancelButton, &QPushButton::clicked,
            this, &VisualFeedbackManager::onProgressCancelRequested);
    buttonLayout->addWidget(m_cancelButton);
    
    layout->addLayout(buttonLayout);
    
    // Position and show
    QPoint position = calculateToastPosition(m_progressWidget);
    m_progressWidget->move(position);
    m_progressWidget->show();
    
    if (m_animationEnabled && !shouldUseReducedMotion()) {
        animateToastIn(m_progressWidget);
    }
}

void VisualFeedbackManager::updateProgressWidget(const ProgressInfo& info) {
    if (!m_progressWidget || !m_progressLabel || !m_progressBar || !m_cancelButton) return;
    
    m_progressLabel->setText(info.operationName);
    m_progressBar->setValue(info.percentage);
    m_cancelButton->setVisible(info.cancellable);
    
    if (!info.currentItem.isEmpty()) {
        m_progressBar->setFormat(QString("%1 - %p%").arg(info.currentItem));
    } else {
        m_progressBar->setFormat("%p%");
    }
    
    // Update tooltip with detailed info
    QString tooltip = QString("%1\n%2/%3 files processed")
                     .arg(info.operationName)
                     .arg(info.processedFiles)
                     .arg(info.totalFiles);
    
    if (info.totalBytes > 0) {
        tooltip += QString("\n%1/%2 bytes")
                  .arg(info.processedBytes)
                  .arg(info.totalBytes);
    }
    
    if (!info.estimatedTimeRemaining.isEmpty()) {
        tooltip += QString("\nETA: %1").arg(info.estimatedTimeRemaining);
    }
    
    m_progressWidget->setToolTip(tooltip);
}

void VisualFeedbackManager::hideProgressWidget() {
    if (!m_progressWidget) return;
    
    if (m_animationEnabled && !shouldUseReducedMotion()) {
        animateToastOut(m_progressWidget);
    } else {
        m_progressWidget->close();
    }
    
    m_progressWidget = nullptr;
    m_progressLabel = nullptr;
    m_progressBar = nullptr;
    m_cancelButton = nullptr;
}

QColor VisualFeedbackManager::getTypeColor(FeedbackType type) const {
    switch (type) {
    case FeedbackType::Success:
        return QColor("#4CAF50");
    case FeedbackType::Warning:
        return QColor("#FF9800");
    case FeedbackType::Error:
        return QColor("#F44336");
    case FeedbackType::Information:
        return QColor("#2196F3");
    case FeedbackType::Progress:
        return QColor("#6200EE");
    default:
        return QColor("#757575");
    }
}

QString VisualFeedbackManager::getTypeIcon(FeedbackType type) const {
    switch (type) {
    case FeedbackType::Success:
        return "✓";
    case FeedbackType::Warning:
        return "⚠";
    case FeedbackType::Error:
        return "✗";
    case FeedbackType::Information:
        return "ℹ";
    case FeedbackType::Progress:
        return "⟳";
    default:
        return "•";
    }
}

QPoint VisualFeedbackManager::calculateToastPosition(QWidget* toast) const {
    if (!toast) return QPoint();
    
    QWidget* parent = m_toastParent ? m_toastParent : QApplication::activeWindow();
    if (!parent) {
        parent = QApplication::desktop();
    }
    
    QRect parentRect = parent->geometry();
    QSize toastSize = toast->sizeHint();
    
    int x, y;
    const int margin = 16;
    
    switch (m_toastPosition) {
    case ToastPosition::TopLeft:
        x = parentRect.left() + margin;
        y = parentRect.top() + margin;
        break;
    case ToastPosition::TopCenter:
        x = parentRect.center().x() - toastSize.width() / 2;
        y = parentRect.top() + margin;
        break;
    case ToastPosition::TopRight:
        x = parentRect.right() - toastSize.width() - margin;
        y = parentRect.top() + margin;
        break;
    case ToastPosition::BottomLeft:
        x = parentRect.left() + margin;
        y = parentRect.bottom() - toastSize.height() - margin;
        break;
    case ToastPosition::BottomCenter:
        x = parentRect.center().x() - toastSize.width() / 2;
        y = parentRect.bottom() - toastSize.height() - margin;
        break;
    case ToastPosition::BottomRight:
        x = parentRect.right() - toastSize.width() - margin;
        y = parentRect.bottom() - toastSize.height() - margin;
        break;
    case ToastPosition::Center:
        x = parentRect.center().x() - toastSize.width() / 2;
        y = parentRect.center().y() - toastSize.height() / 2;
        break;
    }
    
    // Adjust for multiple toasts
    int toastIndex = m_activeToasts.size();
    if (m_toastPosition == ToastPosition::TopRight || m_toastPosition == ToastPosition::TopLeft || m_toastPosition == ToastPosition::TopCenter) {
        y += toastIndex * (toastSize.height() + TOAST_SPACING);
    } else if (m_toastPosition == ToastPosition::BottomRight || m_toastPosition == ToastPosition::BottomLeft || m_toastPosition == ToastPosition::BottomCenter) {
        y -= toastIndex * (toastSize.height() + TOAST_SPACING);
    }
    
    return QPoint(x, y);
}

bool VisualFeedbackManager::shouldUseReducedMotion() const {
    return m_reducedMotion || !m_animationEnabled;
}

QPropertyAnimation* VisualFeedbackManager::createFadeAnimation(QWidget* widget, bool fadeIn, int duration) {
    if (!widget) return nullptr;
    
    QPropertyAnimation* animation = new QPropertyAnimation(widget, "windowOpacity", this);
    animation->setDuration(duration);
    animation->setStartValue(fadeIn ? 0.0 : 1.0);
    animation->setEndValue(fadeIn ? 1.0 : 0.0);
    animation->setEasingCurve(fadeIn ? QEasingCurve::OutCubic : QEasingCurve::InCubic);
    
    return animation;
}

QPropertyAnimation* VisualFeedbackManager::createSlideAnimation(QWidget* widget, bool slideIn, int duration) {
    if (!widget) return nullptr;
    
    QPropertyAnimation* animation = new QPropertyAnimation(widget, "pos", this);
    animation->setDuration(duration);
    animation->setEasingCurve(slideIn ? QEasingCurve::OutCubic : QEasingCurve::InCubic);
    
    QPoint currentPos = widget->pos();
    QPoint targetPos = currentPos;
    
    // Determine slide direction based on toast position
    switch (m_toastPosition) {
    case ToastPosition::TopRight:
    case ToastPosition::BottomRight:
        if (slideIn) {
            animation->setStartValue(QPoint(currentPos.x() + 300, currentPos.y()));
            animation->setEndValue(currentPos);
        } else {
            animation->setStartValue(currentPos);
            animation->setEndValue(QPoint(currentPos.x() + 300, currentPos.y()));
        }
        break;
    case ToastPosition::TopLeft:
    case ToastPosition::BottomLeft:
        if (slideIn) {
            animation->setStartValue(QPoint(currentPos.x() - 300, currentPos.y()));
            animation->setEndValue(currentPos);
        } else {
            animation->setStartValue(currentPos);
            animation->setEndValue(QPoint(currentPos.x() - 300, currentPos.y()));
        }
        break;
    default:
        // For center positions, slide from top or bottom
        if (slideIn) {
            animation->setStartValue(QPoint(currentPos.x(), currentPos.y() - 100));
            animation->setEndValue(currentPos);
        } else {
            animation->setStartValue(currentPos);
            animation->setEndValue(QPoint(currentPos.x(), currentPos.y() - 100));
        }
        break;
    }
    
    return animation;
}

QPropertyAnimation* VisualFeedbackManager::createScaleAnimation(QWidget* widget, bool scaleUp, int duration) {
    if (!widget) return nullptr;
    
    // Qt doesn't have a direct scale property, so we'll use size animation as approximation
    QPropertyAnimation* animation = new QPropertyAnimation(widget, "size", this);
    animation->setDuration(duration);
    animation->setEasingCurve(scaleUp ? QEasingCurve::OutBack : QEasingCurve::InBack);
    
    QSize currentSize = widget->size();
    if (scaleUp) {
        animation->setStartValue(QSize(0, 0));
        animation->setEndValue(currentSize);
    } else {
        animation->setStartValue(currentSize);
        animation->setEndValue(QSize(0, 0));
    }
    
    return animation;
}

QPropertyAnimation* VisualFeedbackManager::createRotateAnimation(QWidget* widget, int duration) {
    // Qt doesn't have built-in rotation animation for widgets
    // This would require custom implementation with QGraphicsEffect
    Q_UNUSED(widget)
    Q_UNUSED(duration)
    return nullptr;
}

void VisualFeedbackManager::applyGlowEffect(QWidget* widget, const QColor& color) {
    if (!widget) return;
    
    removeGlowEffect(widget); // Remove existing effect
    
    QGraphicsDropShadowEffect* glowEffect = new QGraphicsDropShadowEffect(widget);
    glowEffect->setBlurRadius(20);
    glowEffect->setColor(color);
    glowEffect->setOffset(0, 0);
    
    widget->setGraphicsEffect(glowEffect);
    m_appliedEffects[widget] = glowEffect;
}

void VisualFeedbackManager::removeGlowEffect(QWidget* widget) {
    if (!widget) return;
    
    if (m_appliedEffects.contains(widget)) {
        QGraphicsEffect* effect = m_appliedEffects.take(widget);
        widget->setGraphicsEffect(nullptr);
        delete effect;
    }
}

void VisualFeedbackManager::onToastTimerTimeout() {
    // Process toast queue
    while (!m_toastQueue.isEmpty() && m_activeToasts.size() < MAX_CONCURRENT_TOASTS) {
        ToastNotification notification = m_toastQueue.dequeue();
        createToastWidget(notification);
    }
    
    // Stop timer if no more toasts in queue
    if (m_toastQueue.isEmpty()) {
        m_toastTimer->stop();
    }
}

void VisualFeedbackManager::onProgressCancelRequested() {
    cancelProgress();
}

} // namespace FluxGUI::UI::Components

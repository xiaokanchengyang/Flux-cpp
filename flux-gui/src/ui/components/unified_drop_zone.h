#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QDragEnterEvent;
class QDragMoveEvent;
class QDragLeaveEvent;
class QDropEvent;
class QPaintEvent;
QT_END_NAMESPACE

namespace FluxGUI::UI::Components {

/**
 * Unified Drop Zone Component
 * 
 * Provides a consistent, visually appealing drag-and-drop experience
 * across the entire application with smart file type detection and
 * contextual feedback.
 */
class UnifiedDropZone : public QWidget {
    Q_OBJECT

public:
    enum class DropState {
        Inactive,      // Normal state, no drag operation
        DragEnter,     // Files being dragged over the zone
        ValidDrop,     // Valid files detected
        InvalidDrop,   // Invalid files or unsupported operation
        Processing     // Drop accepted, processing files
    };

    explicit UnifiedDropZone(QWidget* parent = nullptr);
    ~UnifiedDropZone() override;

    // Configuration
    void setAcceptedFileTypes(const QStringList& extensions);
    void setMaxFileCount(int maxFiles);
    void setMaxFileSize(qint64 maxSizeBytes);
    void setDropMessage(const QString& message);
    void setEnabled(bool enabled);

    // State management
    DropState currentState() const { return m_currentState; }
    void setState(DropState state);
    void showFeedback(const QString& message, bool isError = false);
    void hideFeedback();

signals:
    // File drop signals
    void filesDropped(const QStringList& filePaths);
    void archiveFilesDropped(const QStringList& archivePaths);
    void regularFilesDropped(const QStringList& filePaths);
    
    // State change signals
    void dropStateChanged(DropState newState);
    void validationFailed(const QString& reason);

protected:
    // Event handlers
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onAnimationFinished();
    void onFeedbackTimeout();

private:
    // UI setup
    void initializeUI();
    void createLayout();
    void setupAnimations();
    void applyStyles();
    
    // Drag and drop logic
    bool validateDropData(const QMimeData* mimeData);
    QStringList extractFilePaths(const QMimeData* mimeData);
    bool isValidFileType(const QString& filePath);
    bool isArchiveFile(const QString& filePath);
    QString getFileValidationError(const QStringList& files);
    
    // Visual feedback
    void updateVisualState();
    void animateStateChange();
    void drawDropIndicator(QPainter& painter);
    void drawBackground(QPainter& painter);
    void updateDropMessage();

private:
    // UI components
    QVBoxLayout* m_layout{nullptr};
    QLabel* m_iconLabel{nullptr};
    QLabel* m_messageLabel{nullptr};
    QLabel* m_detailLabel{nullptr};
    
    // Visual effects
    std::unique_ptr<QPropertyAnimation> m_fadeAnimation;
    std::unique_ptr<QPropertyAnimation> m_scaleAnimation;
    std::unique_ptr<QGraphicsOpacityEffect> m_opacityEffect;
    std::unique_ptr<QGraphicsDropShadowEffect> m_shadowEffect;
    
    // State management
    DropState m_currentState{DropState::Inactive};
    bool m_isEnabled{true};
    bool m_isDragActive{false};
    
    // Configuration
    QStringList m_acceptedExtensions;
    int m_maxFileCount{100};
    qint64 m_maxFileSize{1024 * 1024 * 1024}; // 1GB default
    QString m_defaultMessage{"Drag files or archives here"};
    QString m_currentMessage;
    
    // Feedback system
    std::unique_ptr<QTimer> m_feedbackTimer;
    QString m_feedbackMessage;
    bool m_feedbackIsError{false};
    
    // Visual constants
    static constexpr int BORDER_RADIUS = 12;
    static constexpr int BORDER_WIDTH = 2;
    static constexpr int ANIMATION_DURATION = 200;
    static constexpr int FEEDBACK_TIMEOUT = 3000;
    static constexpr qreal HOVER_OPACITY = 0.8;
    static constexpr qreal ACTIVE_OPACITY = 0.9;
};

} // namespace FluxGUI::UI::Components

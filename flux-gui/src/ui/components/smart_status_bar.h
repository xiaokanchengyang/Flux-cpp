#pragma once

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QToolButton>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <memory>

QT_BEGIN_NAMESPACE
class QPaintEvent;
class QResizeEvent;
QT_END_NAMESPACE

namespace FluxGUI::UI::Components {

/**
 * Smart Status Bar Component
 * 
 * Provides intelligent status reporting with contextual information,
 * progress tracking, and user-friendly notifications.
 */
class SmartStatusBar : public QWidget {
    Q_OBJECT

public:
    enum class StatusType {
        Ready,          // Application ready for user input
        Working,        // Operation in progress
        Success,        // Operation completed successfully
        Warning,        // Non-critical issue occurred
        Error,          // Error occurred
        Information     // General information message
    };

    struct OperationStatus {
        QString operationName;
        QString currentItem;
        int percentage{0};
        qint64 processedBytes{0};
        qint64 totalBytes{0};
        int processedFiles{0};
        int totalFiles{0};
        QString estimatedTimeRemaining;
        bool cancellable{true};
    };

    explicit SmartStatusBar(QWidget* parent = nullptr);
    ~SmartStatusBar() override;

    // Status management
    void setStatus(const QString& message, StatusType type = StatusType::Ready);
    void setTemporaryStatus(const QString& message, StatusType type, int durationMs = 3000);
    void clearStatus();

    // Operation progress
    void startOperation(const QString& operationName, bool cancellable = true);
    void updateProgress(const OperationStatus& status);
    void finishOperation(bool success, const QString& message = QString());
    void cancelOperation();

    // Information display
    void setArchiveInfo(const QString& archivePath, int fileCount, qint64 totalSize);
    void setSelectionInfo(int selectedFiles, qint64 selectedSize);
    void clearArchiveInfo();

signals:
    // User interaction signals
    void operationCancelRequested();
    void statusClicked();
    void detailsRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private slots:
    void onTemporaryStatusTimeout();
    void onProgressAnimationFinished();
    void onCancelClicked();
    void onDetailsClicked();

private:
    // UI setup
    void initializeUI();
    void createStatusSection();
    void createProgressSection();
    void createInfoSection();
    void createActionSection();
    void setupAnimations();
    void applyStyles();
    
    // Status management
    void updateStatusDisplay();
    void showProgressSection(bool show);
    void updateProgressText();
    void calculateTimeEstimate();
    
    // Visual updates
    void animateStatusChange();
    void updateStatusIcon();
    void updateColors();

private:
    // Layout components
    QHBoxLayout* m_mainLayout{nullptr};
    QWidget* m_statusSection{nullptr};
    QWidget* m_progressSection{nullptr};
    QWidget* m_infoSection{nullptr};
    QWidget* m_actionSection{nullptr};
    
    // Status display
    QLabel* m_statusIcon{nullptr};
    QLabel* m_statusLabel{nullptr};
    QLabel* m_timestampLabel{nullptr};
    
    // Progress display
    QProgressBar* m_progressBar{nullptr};
    QLabel* m_progressLabel{nullptr};
    QLabel* m_speedLabel{nullptr};
    QLabel* m_etaLabel{nullptr};
    QToolButton* m_cancelButton{nullptr};
    
    // Information display
    QLabel* m_archiveInfoLabel{nullptr};
    QLabel* m_selectionInfoLabel{nullptr};
    
    // Action buttons
    QToolButton* m_detailsButton{nullptr};
    
    // State management
    StatusType m_currentStatusType{StatusType::Ready};
    QString m_currentStatusMessage;
    QString m_permanentStatusMessage;
    bool m_operationInProgress{false};
    bool m_hasTemporaryStatus{false};
    
    // Operation tracking
    OperationStatus m_currentOperation;
    QDateTime m_operationStartTime;
    qint64 m_lastBytesProcessed{0};
    QDateTime m_lastProgressUpdate;
    double m_currentSpeed{0.0}; // bytes per second
    
    // Archive information
    QString m_currentArchivePath;
    int m_archiveFileCount{0};
    qint64 m_archiveTotalSize{0};
    int m_selectedFileCount{0};
    qint64 m_selectedTotalSize{0};
    
    // Timers and animations
    std::unique_ptr<QTimer> m_temporaryStatusTimer;
    std::unique_ptr<QPropertyAnimation> m_statusAnimation;
    std::unique_ptr<QPropertyAnimation> m_progressAnimation;
    std::unique_ptr<QGraphicsOpacityEffect> m_opacityEffect;
    
    // Constants
    static constexpr int STATUS_BAR_HEIGHT = 32;
    static constexpr int ANIMATION_DURATION = 200;
    static constexpr int PROGRESS_UPDATE_INTERVAL = 100;
    static constexpr int SPEED_CALCULATION_WINDOW = 5000; // 5 seconds
};

} // namespace FluxGUI::UI::Components

#include "smart_status_bar.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QToolButton>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QDateTime>
#include <QApplication>
#include <QStyle>
#include <QStyleOption>

namespace FluxGUI::UI::Components {

SmartStatusBar::SmartStatusBar(QWidget* parent)
    : QWidget(parent)
    , m_temporaryStatusTimer(std::make_unique<QTimer>(this))
    , m_statusAnimation(std::make_unique<QPropertyAnimation>(this, "windowOpacity"))
    , m_progressAnimation(std::make_unique<QPropertyAnimation>(this, "windowOpacity"))
    , m_opacityEffect(std::make_unique<QGraphicsOpacityEffect>())
{
    setFixedHeight(STATUS_BAR_HEIGHT);
    setObjectName("SmartStatusBar");
    
    initializeUI();
    setupAnimations();
    applyStyles();
    
    // Connect timer
    connect(m_temporaryStatusTimer.get(), &QTimer::timeout,
            this, &SmartStatusBar::onTemporaryStatusTimeout);
    
    // Set initial status
    setStatus(tr("Ready"), StatusType::Ready);
}

SmartStatusBar::~SmartStatusBar() = default;

void SmartStatusBar::initializeUI()
{
    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setContentsMargins(8, 4, 8, 4);
    m_mainLayout->setSpacing(12);
    
    createStatusSection();
    createProgressSection();
    createInfoSection();
    createActionSection();
    
    // Add stretch to push action section to the right
    m_mainLayout->addStretch();
    m_mainLayout->addWidget(m_actionSection);
}

void SmartStatusBar::createStatusSection()
{
    m_statusSection = new QWidget(this);
    auto* layout = new QHBoxLayout(m_statusSection);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    
    m_statusIcon = new QLabel(m_statusSection);
    m_statusIcon->setFixedSize(16, 16);
    m_statusIcon->setScaledContents(true);
    
    m_statusLabel = new QLabel(m_statusSection);
    m_statusLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    
    m_timestampLabel = new QLabel(m_statusSection);
    m_timestampLabel->setObjectName("timestampLabel");
    
    layout->addWidget(m_statusIcon);
    layout->addWidget(m_statusLabel);
    layout->addWidget(m_timestampLabel);
    
    m_mainLayout->addWidget(m_statusSection);
}

void SmartStatusBar::createProgressSection()
{
    m_progressSection = new QWidget(this);
    auto* layout = new QHBoxLayout(m_progressSection);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    
    m_progressBar = new QProgressBar(m_progressSection);
    m_progressBar->setFixedWidth(200);
    m_progressBar->setFixedHeight(16);
    
    m_progressLabel = new QLabel(m_progressSection);
    m_progressLabel->setMinimumWidth(80);
    
    m_speedLabel = new QLabel(m_progressSection);
    m_speedLabel->setMinimumWidth(60);
    m_speedLabel->setObjectName("speedLabel");
    
    m_etaLabel = new QLabel(m_progressSection);
    m_etaLabel->setMinimumWidth(60);
    m_etaLabel->setObjectName("etaLabel");
    
    m_cancelButton = new QToolButton(m_progressSection);
    m_cancelButton->setText(tr("Cancel"));
    m_cancelButton->setToolTip(tr("Cancel current operation"));
    connect(m_cancelButton, &QToolButton::clicked,
            this, &SmartStatusBar::onCancelClicked);
    
    layout->addWidget(m_progressBar);
    layout->addWidget(m_progressLabel);
    layout->addWidget(m_speedLabel);
    layout->addWidget(m_etaLabel);
    layout->addWidget(m_cancelButton);
    
    m_progressSection->setVisible(false);
    m_mainLayout->addWidget(m_progressSection);
}

void SmartStatusBar::createInfoSection()
{
    m_infoSection = new QWidget(this);
    auto* layout = new QVBoxLayout(m_infoSection);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    
    m_archiveInfoLabel = new QLabel(m_infoSection);
    m_archiveInfoLabel->setObjectName("archiveInfoLabel");
    
    m_selectionInfoLabel = new QLabel(m_infoSection);
    m_selectionInfoLabel->setObjectName("selectionInfoLabel");
    
    layout->addWidget(m_archiveInfoLabel);
    layout->addWidget(m_selectionInfoLabel);
    
    m_mainLayout->addWidget(m_infoSection);
}

void SmartStatusBar::createActionSection()
{
    m_actionSection = new QWidget(this);
    auto* layout = new QHBoxLayout(m_actionSection);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    
    m_detailsButton = new QToolButton(m_actionSection);
    m_detailsButton->setText(tr("Details"));
    m_detailsButton->setToolTip(tr("Show operation details"));
    connect(m_detailsButton, &QToolButton::clicked,
            this, &SmartStatusBar::onDetailsClicked);
    
    layout->addWidget(m_detailsButton);
}

void SmartStatusBar::setupAnimations()
{
    m_statusAnimation->setDuration(ANIMATION_DURATION);
    m_statusAnimation->setEasingCurve(QEasingCurve::OutCubic);
    
    m_progressAnimation->setDuration(ANIMATION_DURATION);
    m_progressAnimation->setEasingCurve(QEasingCurve::InOutCubic);
    
    connect(m_progressAnimation.get(), &QPropertyAnimation::finished,
            this, &SmartStatusBar::onProgressAnimationFinished);
}

void SmartStatusBar::applyStyles()
{
    setStyleSheet(R"(
        SmartStatusBar {
            background-color: #f0f0f0;
            border-top: 1px solid #d0d0d0;
        }
        
        QLabel#timestampLabel {
            color: #666666;
            font-size: 11px;
        }
        
        QLabel#speedLabel, QLabel#etaLabel {
            color: #555555;
            font-size: 11px;
        }
        
        QLabel#archiveInfoLabel, QLabel#selectionInfoLabel {
            color: #444444;
            font-size: 11px;
        }
        
        QProgressBar {
            border: 1px solid #c0c0c0;
            border-radius: 3px;
            text-align: center;
        }
        
        QProgressBar::chunk {
            background-color: #4a90e2;
            border-radius: 2px;
        }
        
        QToolButton {
            border: 1px solid #c0c0c0;
            border-radius: 3px;
            padding: 2px 8px;
            background-color: #ffffff;
        }
        
        QToolButton:hover {
            background-color: #e8f4fd;
            border-color: #4a90e2;
        }
        
        QToolButton:pressed {
            background-color: #d0e8ff;
        }
    )");
}

void SmartStatusBar::setStatus(const QString& message, StatusType type)
{
    m_currentStatusType = type;
    m_currentStatusMessage = message;
    m_permanentStatusMessage = message;
    m_hasTemporaryStatus = false;
    
    updateStatusDisplay();
    animateStatusChange();
}

void SmartStatusBar::setTemporaryStatus(const QString& message, StatusType type, int durationMs)
{
    m_currentStatusType = type;
    m_currentStatusMessage = message;
    m_hasTemporaryStatus = true;
    
    updateStatusDisplay();
    animateStatusChange();
    
    m_temporaryStatusTimer->start(durationMs);
}

void SmartStatusBar::clearStatus()
{
    setStatus(tr("Ready"), StatusType::Ready);
}

void SmartStatusBar::startOperation(const QString& operationName, bool cancellable)
{
    m_operationInProgress = true;
    m_currentOperation = OperationStatus{};
    m_currentOperation.operationName = operationName;
    m_currentOperation.cancellable = cancellable;
    m_operationStartTime = QDateTime::currentDateTime();
    m_lastProgressUpdate = m_operationStartTime;
    m_lastBytesProcessed = 0;
    m_currentSpeed = 0.0;
    
    m_cancelButton->setVisible(cancellable);
    showProgressSection(true);
    
    setStatus(tr("Starting %1...").arg(operationName), StatusType::Working);
}

void SmartStatusBar::updateProgress(const OperationStatus& status)
{
    m_currentOperation = status;
    
    // Update progress bar
    m_progressBar->setValue(status.percentage);
    
    // Update progress text
    updateProgressText();
    
    // Calculate speed and ETA
    calculateTimeEstimate();
    
    // Update status message
    QString statusMsg = tr("%1: %2").arg(status.operationName, status.currentItem);
    setStatus(statusMsg, StatusType::Working);
}

void SmartStatusBar::finishOperation(bool success, const QString& message)
{
    m_operationInProgress = false;
    showProgressSection(false);
    
    StatusType type = success ? StatusType::Success : StatusType::Error;
    QString statusMsg = message.isEmpty() ? 
        (success ? tr("Operation completed successfully") : tr("Operation failed")) : 
        message;
    
    setTemporaryStatus(statusMsg, type, 3000);
}

void SmartStatusBar::cancelOperation()
{
    if (m_operationInProgress) {
        emit operationCancelRequested();
        setTemporaryStatus(tr("Cancelling operation..."), StatusType::Warning, 2000);
    }
}

void SmartStatusBar::setArchiveInfo(const QString& archivePath, int fileCount, qint64 totalSize)
{
    m_currentArchivePath = archivePath;
    m_archiveFileCount = fileCount;
    m_archiveTotalSize = totalSize;
    
    QString sizeText = formatBytes(totalSize);
    m_archiveInfoLabel->setText(tr("Archive: %1 files, %2")
                               .arg(fileCount).arg(sizeText));
}

void SmartStatusBar::setSelectionInfo(int selectedFiles, qint64 selectedSize)
{
    m_selectedFileCount = selectedFiles;
    m_selectedTotalSize = selectedSize;
    
    if (selectedFiles > 0) {
        QString sizeText = formatBytes(selectedSize);
        m_selectionInfoLabel->setText(tr("Selected: %1 files, %2")
                                     .arg(selectedFiles).arg(sizeText));
    } else {
        m_selectionInfoLabel->clear();
    }
}

void SmartStatusBar::clearArchiveInfo()
{
    m_archiveInfoLabel->clear();
    m_selectionInfoLabel->clear();
    m_currentArchivePath.clear();
    m_archiveFileCount = 0;
    m_archiveTotalSize = 0;
    m_selectedFileCount = 0;
    m_selectedTotalSize = 0;
}

void SmartStatusBar::updateStatusDisplay()
{
    updateStatusIcon();
    m_statusLabel->setText(m_currentStatusMessage);
    m_timestampLabel->setText(QDateTime::currentDateTime().toString("hh:mm:ss"));
    updateColors();
}

void SmartStatusBar::showProgressSection(bool show)
{
    if (show) {
        m_progressSection->setVisible(true);
        m_progressAnimation->setStartValue(0.0);
        m_progressAnimation->setEndValue(1.0);
    } else {
        m_progressAnimation->setStartValue(1.0);
        m_progressAnimation->setEndValue(0.0);
    }
    m_progressAnimation->start();
}

void SmartStatusBar::updateProgressText()
{
    const auto& op = m_currentOperation;
    
    if (op.totalFiles > 0) {
        m_progressLabel->setText(tr("%1/%2 files")
                               .arg(op.processedFiles)
                               .arg(op.totalFiles));
    } else if (op.totalBytes > 0) {
        QString processed = formatBytes(op.processedBytes);
        QString total = formatBytes(op.totalBytes);
        m_progressLabel->setText(tr("%1/%2").arg(processed, total));
    } else {
        m_progressLabel->setText(tr("%1%").arg(op.percentage));
    }
}

void SmartStatusBar::calculateTimeEstimate()
{
    const auto& op = m_currentOperation;
    QDateTime now = QDateTime::currentDateTime();
    
    // Calculate speed (bytes per second)
    qint64 timeDiff = m_lastProgressUpdate.msecsTo(now);
    if (timeDiff >= PROGRESS_UPDATE_INTERVAL) {
        qint64 bytesDiff = op.processedBytes - m_lastBytesProcessed;
        if (timeDiff > 0) {
            double speed = (bytesDiff * 1000.0) / timeDiff; // bytes per second
            
            // Smooth the speed calculation
            if (m_currentSpeed == 0.0) {
                m_currentSpeed = speed;
            } else {
                m_currentSpeed = (m_currentSpeed * 0.7) + (speed * 0.3);
            }
        }
        
        m_lastBytesProcessed = op.processedBytes;
        m_lastProgressUpdate = now;
    }
    
    // Update speed display
    if (m_currentSpeed > 0) {
        m_speedLabel->setText(formatSpeed(m_currentSpeed));
        
        // Calculate ETA
        if (op.totalBytes > op.processedBytes) {
            qint64 remainingBytes = op.totalBytes - op.processedBytes;
            int etaSeconds = static_cast<int>(remainingBytes / m_currentSpeed);
            m_etaLabel->setText(formatDuration(etaSeconds));
        } else {
            m_etaLabel->clear();
        }
    } else {
        m_speedLabel->clear();
        m_etaLabel->clear();
    }
}

void SmartStatusBar::animateStatusChange()
{
    // Simple fade effect for status changes
    setGraphicsEffect(m_opacityEffect.get());
    m_statusAnimation->setStartValue(0.7);
    m_statusAnimation->setEndValue(1.0);
    m_statusAnimation->start();
}

void SmartStatusBar::updateStatusIcon()
{
    QString iconName;
    switch (m_currentStatusType) {
        case StatusType::Ready:
            iconName = ":/icons/status-ready.png";
            break;
        case StatusType::Working:
            iconName = ":/icons/status-working.png";
            break;
        case StatusType::Success:
            iconName = ":/icons/status-success.png";
            break;
        case StatusType::Warning:
            iconName = ":/icons/status-warning.png";
            break;
        case StatusType::Error:
            iconName = ":/icons/status-error.png";
            break;
        case StatusType::Information:
            iconName = ":/icons/status-info.png";
            break;
    }
    
    // For now, use standard icons from the system
    QStyle* style = QApplication::style();
    QIcon icon;
    
    switch (m_currentStatusType) {
        case StatusType::Ready:
        case StatusType::Information:
            icon = style->standardIcon(QStyle::SP_MessageBoxInformation);
            break;
        case StatusType::Working:
            icon = style->standardIcon(QStyle::SP_BrowserReload);
            break;
        case StatusType::Success:
            icon = style->standardIcon(QStyle::SP_DialogApplyButton);
            break;
        case StatusType::Warning:
            icon = style->standardIcon(QStyle::SP_MessageBoxWarning);
            break;
        case StatusType::Error:
            icon = style->standardIcon(QStyle::SP_MessageBoxCritical);
            break;
    }
    
    m_statusIcon->setPixmap(icon.pixmap(16, 16));
}

void SmartStatusBar::updateColors()
{
    QString color;
    switch (m_currentStatusType) {
        case StatusType::Ready:
        case StatusType::Information:
            color = "#333333";
            break;
        case StatusType::Working:
            color = "#0066cc";
            break;
        case StatusType::Success:
            color = "#008800";
            break;
        case StatusType::Warning:
            color = "#ff8800";
            break;
        case StatusType::Error:
            color = "#cc0000";
            break;
    }
    
    m_statusLabel->setStyleSheet(QString("color: %1;").arg(color));
}

void SmartStatusBar::paintEvent(QPaintEvent* event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter painter(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);
    
    QWidget::paintEvent(event);
}

void SmartStatusBar::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
}

void SmartStatusBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit statusClicked();
    }
    QWidget::mousePressEvent(event);
}

void SmartStatusBar::onTemporaryStatusTimeout()
{
    m_temporaryStatusTimer->stop();
    
    if (m_hasTemporaryStatus) {
        m_hasTemporaryStatus = false;
        m_currentStatusMessage = m_permanentStatusMessage;
        m_currentStatusType = StatusType::Ready;
        updateStatusDisplay();
        animateStatusChange();
    }
}

void SmartStatusBar::onProgressAnimationFinished()
{
    if (m_progressAnimation->endValue().toDouble() == 0.0) {
        m_progressSection->setVisible(false);
    }
}

void SmartStatusBar::onCancelClicked()
{
    cancelOperation();
}

void SmartStatusBar::onDetailsClicked()
{
    emit detailsRequested();
}

QString SmartStatusBar::formatBytes(qint64 bytes)
{
    const QStringList units = {"B", "KB", "MB", "GB", "TB"};
    double size = bytes;
    int unitIndex = 0;
    
    while (size >= 1024.0 && unitIndex < units.size() - 1) {
        size /= 1024.0;
        ++unitIndex;
    }
    
    return QString("%1 %2").arg(QString::number(size, 'f', unitIndex > 0 ? 1 : 0), units[unitIndex]);
}

QString SmartStatusBar::formatSpeed(double bytesPerSecond)
{
    return formatBytes(static_cast<qint64>(bytesPerSecond)) + "/s";
}

QString SmartStatusBar::formatDuration(int seconds)
{
    if (seconds < 60) {
        return tr("%1s").arg(seconds);
    } else if (seconds < 3600) {
        return tr("%1m %2s").arg(seconds / 60).arg(seconds % 60);
    } else {
        int hours = seconds / 3600;
        int minutes = (seconds % 3600) / 60;
        return tr("%1h %2m").arg(hours).arg(minutes);
    }
}

} // namespace FluxGUI::UI::Components

// #include "smart_status_bar.moc"  // Removed as class has Q_OBJECT in header

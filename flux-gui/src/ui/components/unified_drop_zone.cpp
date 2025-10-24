#include "unified_drop_zone.h"

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QStyle>
#include <QDebug>

namespace FluxGUI::UI::Components {

UnifiedDropZone::UnifiedDropZone(QWidget* parent)
    : QWidget(parent)
    , m_layout(nullptr)
    , m_iconLabel(nullptr)
    , m_messageLabel(nullptr)
    , m_detailLabel(nullptr)
    , m_feedbackTimer(std::make_unique<QTimer>(this))
{
    setObjectName("UnifiedDropZone");
    setAcceptDrops(true);
    setMinimumHeight(200);
    
    // Initialize default accepted file types
    m_acceptedExtensions = {
        "zip", "7z", "rar", "tar", "gz", "bz2", "xz", "zst",
        "tar.gz", "tar.bz2", "tar.xz", "tar.zst", "tgz", "tbz2", "txz"
    };
    
    initializeUI();
    setupAnimations();
    applyStyles();
    
    // Setup feedback timer
    m_feedbackTimer->setSingleShot(true);
    connect(m_feedbackTimer.get(), &QTimer::timeout, this, &UnifiedDropZone::onFeedbackTimeout);
    
    qDebug() << "UnifiedDropZone initialized";
}

UnifiedDropZone::~UnifiedDropZone() = default;

void UnifiedDropZone::setAcceptedFileTypes(const QStringList& extensions) {
    m_acceptedExtensions = extensions;
    qDebug() << "Updated accepted file types:" << extensions;
}

void UnifiedDropZone::setMaxFileCount(int maxFiles) {
    m_maxFileCount = maxFiles;
}

void UnifiedDropZone::setMaxFileSize(qint64 maxSizeBytes) {
    m_maxFileSize = maxSizeBytes;
}

void UnifiedDropZone::setDropMessage(const QString& message) {
    m_defaultMessage = message;
    if (m_currentState == DropState::Inactive) {
        m_messageLabel->setText(message);
    }
}

void UnifiedDropZone::setEnabled(bool enabled) {
    m_isEnabled = enabled;
    QWidget::setEnabled(enabled);
    updateVisualState();
}

void UnifiedDropZone::setState(DropState state) {
    if (m_currentState == state) return;
    
    DropState oldState = m_currentState;
    m_currentState = state;
    
    updateVisualState();
    animateStateChange();
    
    emit dropStateChanged(state);
    
    qDebug() << "Drop zone state changed from" << static_cast<int>(oldState) 
             << "to" << static_cast<int>(state);
}

void UnifiedDropZone::showFeedback(const QString& message, bool isError) {
    m_feedbackMessage = message;
    m_feedbackIsError = isError;
    
    if (m_detailLabel) {
        m_detailLabel->setText(message);
        m_detailLabel->setStyleSheet(isError ? 
            "color: #e74c3c; font-weight: bold;" : 
            "color: #27ae60; font-weight: bold;");
        m_detailLabel->setVisible(true);
    }
    
    // Auto-hide feedback after timeout
    m_feedbackTimer->start(FEEDBACK_TIMEOUT);
}

void UnifiedDropZone::hideFeedback() {
    if (m_detailLabel) {
        m_detailLabel->setVisible(false);
    }
    m_feedbackTimer->stop();
}

void UnifiedDropZone::dragEnterEvent(QDragEnterEvent* event) {
    if (!m_isEnabled) {
        event->ignore();
        return;
    }
    
    m_isDragActive = true;
    
    if (validateDropData(event->mimeData())) {
        setState(DropState::ValidDrop);
        event->acceptProposedAction();
    } else {
        setState(DropState::InvalidDrop);
        event->ignore();
    }
}

void UnifiedDropZone::dragMoveEvent(QDragMoveEvent* event) {
    if (!m_isEnabled || !m_isDragActive) {
        event->ignore();
        return;
    }
    
    event->acceptProposedAction();
}

void UnifiedDropZone::dragLeaveEvent(QDragLeaveEvent* event) {
    m_isDragActive = false;
    setState(DropState::Inactive);
    event->accept();
}

void UnifiedDropZone::dropEvent(QDropEvent* event) {
    if (!m_isEnabled) {
        event->ignore();
        return;
    }
    
    m_isDragActive = false;
    setState(DropState::Processing);
    
    QStringList filePaths = extractFilePaths(event->mimeData());
    
    if (filePaths.isEmpty()) {
        showFeedback("No valid files found", true);
        setState(DropState::Inactive);
        event->ignore();
        return;
    }
    
    // Validate files
    QString validationError = getFileValidationError(filePaths);
    if (!validationError.isEmpty()) {
        showFeedback(validationError, true);
        setState(DropState::Inactive);
        emit validationFailed(validationError);
        event->ignore();
        return;
    }
    
    // Categorize files
    QStringList archiveFiles;
    QStringList regularFiles;
    
    for (const QString& filePath : filePaths) {
        if (isArchiveFile(filePath)) {
            archiveFiles.append(filePath);
        } else {
            regularFiles.append(filePath);
        }
    }
    
    // Emit appropriate signals
    emit filesDropped(filePaths);
    
    if (!archiveFiles.isEmpty()) {
        emit archiveFilesDropped(archiveFiles);
    }
    
    if (!regularFiles.isEmpty()) {
        emit regularFilesDropped(regularFiles);
    }
    
    showFeedback(QString("Successfully processed %1 file(s)").arg(filePaths.size()), false);
    
    // Return to inactive state after a short delay
    QTimer::singleShot(1000, this, [this]() {
        setState(DropState::Inactive);
    });
    
    event->acceptProposedAction();
}

void UnifiedDropZone::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    drawBackground(painter);
    drawDropIndicator(painter);
    
    QWidget::paintEvent(event);
}

void UnifiedDropZone::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updateVisualState();
}

void UnifiedDropZone::onAnimationFinished() {
    // Animation completed, update final state if needed
}

void UnifiedDropZone::onFeedbackTimeout() {
    hideFeedback();
}

void UnifiedDropZone::initializeUI() {
    m_layout = new QVBoxLayout(this);
    m_layout->setAlignment(Qt::AlignCenter);
    m_layout->setSpacing(16);
    
    // Icon label
    m_iconLabel = new QLabel();
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->setPixmap(QApplication::style()->standardIcon(QStyle::SP_FileDialogDetailedView)
                          .pixmap(64, 64));
    
    // Message label
    m_messageLabel = new QLabel(m_defaultMessage);
    m_messageLabel->setAlignment(Qt::AlignCenter);
    m_messageLabel->setWordWrap(true);
    
    // Detail label (hidden by default)
    m_detailLabel = new QLabel();
    m_detailLabel->setAlignment(Qt::AlignCenter);
    m_detailLabel->setWordWrap(true);
    m_detailLabel->setVisible(false);
    
    m_layout->addWidget(m_iconLabel);
    m_layout->addWidget(m_messageLabel);
    m_layout->addWidget(m_detailLabel);
}

void UnifiedDropZone::setupAnimations() {
    // Fade animation
    m_opacityEffect = std::make_unique<QGraphicsOpacityEffect>();
    setGraphicsEffect(m_opacityEffect.get());
    
    m_fadeAnimation = std::make_unique<QPropertyAnimation>(m_opacityEffect.get(), "opacity");
    m_fadeAnimation->setDuration(ANIMATION_DURATION);
    connect(m_fadeAnimation.get(), &QPropertyAnimation::finished, 
            this, &UnifiedDropZone::onAnimationFinished);
    
    // Scale animation for icon
    m_scaleAnimation = std::make_unique<QPropertyAnimation>(m_iconLabel, "geometry");
    m_scaleAnimation->setDuration(ANIMATION_DURATION);
}

void UnifiedDropZone::applyStyles() {
    setStyleSheet(R"(
        UnifiedDropZone {
            background-color: transparent;
            border: 2px dashed #bdc3c7;
            border-radius: 12px;
        }
        
        UnifiedDropZone[state="inactive"] {
            border-color: #bdc3c7;
            background-color: rgba(236, 240, 241, 0.3);
        }
        
        UnifiedDropZone[state="valid"] {
            border-color: #27ae60;
            background-color: rgba(39, 174, 96, 0.1);
        }
        
        UnifiedDropZone[state="invalid"] {
            border-color: #e74c3c;
            background-color: rgba(231, 76, 60, 0.1);
        }
        
        UnifiedDropZone[state="processing"] {
            border-color: #3498db;
            background-color: rgba(52, 152, 219, 0.1);
        }
        
        QLabel {
            background-color: transparent;
            color: #2c3e50;
        }
    )");
}

bool UnifiedDropZone::validateDropData(const QMimeData* mimeData) {
    if (!mimeData->hasUrls()) {
        return false;
    }
    
    QStringList filePaths = extractFilePaths(mimeData);
    return !filePaths.isEmpty() && getFileValidationError(filePaths).isEmpty();
}

QStringList UnifiedDropZone::extractFilePaths(const QMimeData* mimeData) {
    QStringList filePaths;
    
    for (const QUrl& url : mimeData->urls()) {
        if (url.isLocalFile()) {
            QString filePath = url.toLocalFile();
            QFileInfo fileInfo(filePath);
            
            if (fileInfo.exists()) {
                filePaths.append(filePath);
            }
        }
    }
    
    return filePaths;
}

bool UnifiedDropZone::isValidFileType(const QString& filePath) {
    QFileInfo fileInfo(filePath);
    
    // Check if it's a directory (always valid for creation)
    if (fileInfo.isDir()) {
        return true;
    }
    
    // Check file extension
    QString suffix = fileInfo.suffix().toLower();
    QString fileName = fileInfo.fileName().toLower();
    
    // Check single extensions
    if (m_acceptedExtensions.contains(suffix)) {
        return true;
    }
    
    // Check compound extensions (e.g., tar.gz)
    for (const QString& ext : m_acceptedExtensions) {
        if (fileName.endsWith("." + ext)) {
            return true;
        }
    }
    
    return false;
}

bool UnifiedDropZone::isArchiveFile(const QString& filePath) {
    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();
    QString fileName = fileInfo.fileName().toLower();
    
    QStringList archiveExtensions = {
        "zip", "7z", "rar", "tar", "gz", "bz2", "xz", "zst",
        "tar.gz", "tar.bz2", "tar.xz", "tar.zst", "tgz", "tbz2", "txz"
    };
    
    return archiveExtensions.contains(suffix) || 
           std::any_of(archiveExtensions.begin(), archiveExtensions.end(),
                      [&fileName](const QString& ext) {
                          return fileName.endsWith("." + ext);
                      });
}

QString UnifiedDropZone::getFileValidationError(const QStringList& files) {
    if (files.size() > m_maxFileCount) {
        return QString("Too many files. Maximum allowed: %1").arg(m_maxFileCount);
    }
    
    qint64 totalSize = 0;
    for (const QString& filePath : files) {
        QFileInfo fileInfo(filePath);
        
        if (fileInfo.isFile()) {
            qint64 fileSize = fileInfo.size();
            if (fileSize > m_maxFileSize) {
                return QString("File too large: %1. Maximum size: %2 MB")
                       .arg(fileInfo.fileName())
                       .arg(m_maxFileSize / (1024 * 1024));
            }
            totalSize += fileSize;
        }
        
        if (!isValidFileType(filePath)) {
            return QString("Unsupported file type: %1").arg(fileInfo.fileName());
        }
    }
    
    return QString(); // No errors
}

void UnifiedDropZone::updateVisualState() {
    QString stateProperty;
    QString message = m_defaultMessage;
    
    switch (m_currentState) {
        case DropState::Inactive:
            stateProperty = "inactive";
            message = m_defaultMessage;
            break;
        case DropState::DragEnter:
            stateProperty = "drag";
            message = "Drop files here";
            break;
        case DropState::ValidDrop:
            stateProperty = "valid";
            message = "Release to add files";
            break;
        case DropState::InvalidDrop:
            stateProperty = "invalid";
            message = "Invalid files or operation";
            break;
        case DropState::Processing:
            stateProperty = "processing";
            message = "Processing files...";
            break;
    }
    
    setProperty("state", stateProperty);
    style()->unpolish(this);
    style()->polish(this);
    
    if (m_messageLabel) {
        m_messageLabel->setText(message);
    }
    
    update();
}

void UnifiedDropZone::animateStateChange() {
    if (!m_fadeAnimation) return;
    
    qreal targetOpacity = m_isEnabled ? 1.0 : 0.5;
    
    switch (m_currentState) {
        case DropState::ValidDrop:
            targetOpacity = ACTIVE_OPACITY;
            break;
        case DropState::DragEnter:
            targetOpacity = HOVER_OPACITY;
            break;
        default:
            break;
    }
    
    m_fadeAnimation->setStartValue(m_opacityEffect->opacity());
    m_fadeAnimation->setEndValue(targetOpacity);
    m_fadeAnimation->start();
}

void UnifiedDropZone::drawDropIndicator(QPainter& painter) {
    if (m_currentState == DropState::Inactive) return;
    
    painter.save();
    
    QColor indicatorColor;
    switch (m_currentState) {
        case DropState::ValidDrop:
            indicatorColor = QColor(39, 174, 96, 100);
            break;
        case DropState::InvalidDrop:
            indicatorColor = QColor(231, 76, 60, 100);
            break;
        case DropState::Processing:
            indicatorColor = QColor(52, 152, 219, 100);
            break;
        default:
            indicatorColor = QColor(189, 195, 199, 100);
            break;
    }
    
    painter.setBrush(indicatorColor);
    painter.setPen(Qt::NoPen);
    
    QPainterPath path;
    path.addRoundedRect(rect().adjusted(4, 4, -4, -4), BORDER_RADIUS - 2, BORDER_RADIUS - 2);
    painter.drawPath(path);
    
    painter.restore();
}

void UnifiedDropZone::drawBackground(QPainter& painter) {
    painter.save();
    
    QColor backgroundColor = palette().color(QPalette::Base);
    backgroundColor.setAlpha(200);
    
    painter.setBrush(backgroundColor);
    painter.setPen(QPen(palette().color(QPalette::Mid), BORDER_WIDTH, Qt::DashLine));
    
    QPainterPath path;
    path.addRoundedRect(rect().adjusted(1, 1, -1, -1), BORDER_RADIUS, BORDER_RADIUS);
    painter.drawPath(path);
    
    painter.restore();
}

void UnifiedDropZone::updateDropMessage() {
    // This method can be used to update the drop message based on context
    // Implementation depends on specific requirements
}

} // namespace FluxGUI::UI::Components

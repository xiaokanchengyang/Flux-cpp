#include "status_bar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QFrame>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QDebug>

namespace FluxGUI::UI::Components {

StatusBar::StatusBar(QWidget* parent)
    : QStatusBar(parent)
    , m_messageLabel(nullptr)
    , m_progressBar(nullptr)
    , m_cancelButton(nullptr)
    , m_statsLabel(nullptr)
    , m_themeToggleButton(nullptr)
    , m_messageTimer(nullptr)
    , m_fadeAnimation(nullptr)
    , m_isOperationActive(false)
{
    // Set object name for styling
    setObjectName("ModernStatusBar");
    setSizeGripEnabled(false);
    
    // Initialize UI
    initializeUI();
    
    // Connect signals
    connectSignals();
    
    // Set initial state
    setOperationActive(false);
    
    qDebug() << "StatusBar initialized";
}

StatusBar::~StatusBar() = default;

void StatusBar::showMessage(const QString& message, int timeout) {
    if (m_messageLabel) {
        m_messageLabel->setText(message);
        
        // Animate message appearance
        animateMessage();
        
        // Set timer for auto-clear
        if (timeout > 0) {
            if (!m_messageTimer) {
                m_messageTimer = new QTimer(this);
                m_messageTimer->setSingleShot(true);
                connect(m_messageTimer, &QTimer::timeout, this, &StatusBar::clearMessage);
            }
            m_messageTimer->start(timeout);
        }
    }
    
    qDebug() << "Status message:" << message;
}

void StatusBar::clearMessage() {
    if (m_messageLabel) {
        m_messageLabel->clear();
    }
    
    if (m_messageTimer) {
        m_messageTimer->stop();
    }
}

void StatusBar::setOperationActive(bool active) {
    if (m_isOperationActive == active) {
        return;
    }
    
    m_isOperationActive = active;
    
    // Update progress bar visibility
    if (m_progressBar) {
        m_progressBar->setVisible(active);
    }
    
    // Update cancel button visibility
    if (m_cancelButton) {
        m_cancelButton->setVisible(active);
    }
    
    // Update layout
    updateLayout();
    
    qDebug() << "Operation active state:" << active;
}

void StatusBar::setProgress(int value, int maximum) {
    if (m_progressBar) {
        m_progressBar->setMaximum(maximum);
        m_progressBar->setValue(value);
        
        // Show progress bar if not visible
        if (!m_progressBar->isVisible() && maximum > 0) {
            setOperationActive(true);
        }
    }
}

void StatusBar::setProgressText(const QString& text) {
    if (m_progressBar) {
        m_progressBar->setFormat(text + " (%p%)");
    }
}

void StatusBar::setStats(const QString& stats) {
    if (m_statsLabel) {
        m_statsLabel->setText(stats);
    }
}

void StatusBar::initializeUI() {
    // Create main container widget
    QWidget* container = new QWidget(this);
    container->setObjectName("statusBarContainer");
    
    QHBoxLayout* layout = new QHBoxLayout(container);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(12);
    
    // Message label (left side)
    m_messageLabel = new QLabel(container);
    m_messageLabel->setObjectName("statusMessage");
    m_messageLabel->setMinimumWidth(200);
    layout->addWidget(m_messageLabel);
    
    // Progress section (center)
    createProgressSection(layout);
    
    // Add stretch to push stats and theme toggle to the right
    layout->addStretch();
    
    // Stats label (right side)
    m_statsLabel = new QLabel(container);
    m_statsLabel->setObjectName("statusStats");
    m_statsLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(m_statsLabel);
    
    // Separator
    QFrame* separator = new QFrame(container);
    separator->setFrameShape(QFrame::VLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setObjectName("statusSeparator");
    layout->addWidget(separator);
    
    // Theme toggle button (far right)
    m_themeToggleButton = new QPushButton(container);
    m_themeToggleButton->setObjectName("themeToggleButton");
    m_themeToggleButton->setIcon(QIcon(":/icons/theme-dark.svg"));
    m_themeToggleButton->setToolTip("Toggle theme");
    m_themeToggleButton->setFlat(true);
    m_themeToggleButton->setFixedSize(24, 24);
    layout->addWidget(m_themeToggleButton);
    
    // Add container to status bar
    addWidget(container, 1);
    
    // Apply styling
    applyStyles();
}

void StatusBar::createProgressSection(QHBoxLayout* layout) {
    // Progress frame
    QFrame* progressFrame = new QFrame(this);
    progressFrame->setObjectName("progressFrame");
    
    QHBoxLayout* progressLayout = new QHBoxLayout(progressFrame);
    progressLayout->setContentsMargins(0, 0, 0, 0);
    progressLayout->setSpacing(8);
    
    // Progress bar
    m_progressBar = new QProgressBar(progressFrame);
    m_progressBar->setObjectName("statusProgressBar");
    m_progressBar->setMinimumWidth(200);
    m_progressBar->setMaximumHeight(16);
    m_progressBar->setTextVisible(true);
    m_progressBar->setVisible(false);
    progressLayout->addWidget(m_progressBar);
    
    // Cancel button
    m_cancelButton = new QPushButton(progressFrame);
    m_cancelButton->setObjectName("cancelButton");
    m_cancelButton->setIcon(QIcon(":/icons/close.svg"));
    m_cancelButton->setToolTip("Cancel operation");
    m_cancelButton->setFlat(true);
    m_cancelButton->setFixedSize(20, 20);
    m_cancelButton->setVisible(false);
    progressLayout->addWidget(m_cancelButton);
    
    layout->addWidget(progressFrame);
}

void StatusBar::connectSignals() {
    // Theme toggle button
    connect(m_themeToggleButton, &QPushButton::clicked, this, &StatusBar::themeToggleRequested);
    
    // Cancel button
    connect(m_cancelButton, &QPushButton::clicked, this, &StatusBar::operationCancelRequested);
}

void StatusBar::updateLayout() {
    // Force layout update
    if (parentWidget()) {
        parentWidget()->updateGeometry();
    }
}

void StatusBar::animateMessage() {
    if (!m_messageLabel) {
        return;
    }
    
    // Create fade-in animation for messages
    QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(m_messageLabel);
    m_messageLabel->setGraphicsEffect(effect);
    
    m_fadeAnimation = new QPropertyAnimation(effect, "opacity", this);
    m_fadeAnimation->setDuration(300);
    m_fadeAnimation->setStartValue(0.0);
    m_fadeAnimation->setEndValue(1.0);
    m_fadeAnimation->setEasingCurve(QEasingCurve::InOutQuad);
    
    m_fadeAnimation->start();
}

void StatusBar::applyStyles() {
    // Apply custom styling through object names and properties
    // The actual styling is handled by the QSS files
    
    // Force style update
    style()->unpolish(this);
    style()->polish(this);
}

} // namespace FluxGUI::UI::Components

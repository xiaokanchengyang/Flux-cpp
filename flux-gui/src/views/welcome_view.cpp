#include "welcome_view.h"
#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <QDesktopServices>
#include <ranges>
#include <algorithm>

WelcomeView::WelcomeView(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_dropZone(nullptr)
    , m_dropLabel(nullptr)
    , m_quickActionsFrame(nullptr)
    , m_createButton(nullptr)
    , m_openButton(nullptr)
    , m_extractButton(nullptr)
    , m_recentFilesFrame(nullptr)
    , m_recentFilesLayout(nullptr)
    , m_infoFrame(nullptr)
    , m_versionLabel(nullptr)
    , m_supportedFormatsLabel(nullptr)
{
    setupUI();
    updateRecentFiles();
}

void WelcomeView::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(20);
    m_mainLayout->setContentsMargins(40, 40, 40, 40);
    
    setupDropZone();
    setupQuickActions();
    setupRecentFiles();
    setupInfoSection();
    
    // Add flexible space
    m_mainLayout->addStretch();
}

void WelcomeView::setupDropZone() {
    m_dropZone = new QFrame();
    m_dropZone->setFrameStyle(QFrame::Box);
    m_dropZone->setLineWidth(2);
    m_dropZone->setMinimumHeight(200);
    m_dropZone->setAcceptDrops(true);
    
    // Set styles
    m_dropZone->setStyleSheet(R"(
        QFrame {
            border: 2px dashed #cccccc;
            border-radius: 10px;
            background-color: #fafafa;
        }
        QFrame:hover {
            border-color: #007acc;
            background-color: #f0f8ff;
        }
    )");
    
    QVBoxLayout* dropLayout = new QVBoxLayout(m_dropZone);
    dropLayout->setAlignment(Qt::AlignCenter);
    
    // Drag and drop icon
    QLabel* dropIcon = new QLabel("📁");
    dropIcon->setAlignment(Qt::AlignCenter);
    dropIcon->setStyleSheet("font-size: 48px; margin-bottom: 10px;");
    
    m_dropLabel = new QLabel("Drag files or archives here");
    m_dropLabel->setAlignment(Qt::AlignCenter);
    m_dropLabel->setStyleSheet("font-size: 16px; color: #666666;");
    
    QLabel* dropHint = new QLabel("Supports ZIP, 7Z, TAR.GZ, TAR.XZ, TAR.ZSTD formats");
    dropHint->setAlignment(Qt::AlignCenter);
    dropHint->setStyleSheet("font-size: 12px; color: #999999; margin-top: 5px;");
    
    dropLayout->addWidget(dropIcon);
    dropLayout->addWidget(m_dropLabel);
    dropLayout->addWidget(dropHint);
    
    m_mainLayout->addWidget(m_dropZone);
}

void WelcomeView::setupQuickActions() {
    m_quickActionsFrame = new QFrame();
    
    QHBoxLayout* actionsLayout = new QHBoxLayout(m_quickActionsFrame);
    actionsLayout->setSpacing(20);
    
    // Create archive button
    m_createButton = new QPushButton("📦 Create Archive");
    m_createButton->setMinimumSize(150, 60);
    m_createButton->setStyleSheet(R"(
        QPushButton {
            font-size: 14px;
            padding: 10px;
            border: 1px solid #007acc;
            border-radius: 5px;
            background-color: #007acc;
            color: white;
        }
        QPushButton:hover {
            background-color: #005a9e;
        }
        QPushButton:pressed {
            background-color: #004080;
        }
    )");
    
    // Open archive button
    m_openButton = new QPushButton("📂 Open Archive");
    m_openButton->setMinimumSize(150, 60);
    m_openButton->setStyleSheet(R"(
        QPushButton {
            font-size: 14px;
            padding: 10px;
            border: 1px solid #28a745;
            border-radius: 5px;
            background-color: #28a745;
            color: white;
        }
        QPushButton:hover {
            background-color: #218838;
        }
        QPushButton:pressed {
            background-color: #1e7e34;
        }
    )");
    
    // Extract archive button
    m_extractButton = new QPushButton("📤 Extract Archive");
    m_extractButton->setMinimumSize(150, 60);
    m_extractButton->setStyleSheet(R"(
        QPushButton {
            font-size: 14px;
            padding: 10px;
            border: 1px solid #ffc107;
            border-radius: 5px;
            background-color: #ffc107;
            color: #212529;
        }
        QPushButton:hover {
            background-color: #e0a800;
        }
        QPushButton:pressed {
            background-color: #d39e00;
        }
    )");
    
    actionsLayout->addStretch();
    actionsLayout->addWidget(m_createButton);
    actionsLayout->addWidget(m_openButton);
    actionsLayout->addWidget(m_extractButton);
    actionsLayout->addStretch();
    
    // Connect signals
    connect(m_createButton, &QPushButton::clicked, this, &WelcomeView::onCreateArchive);
    connect(m_openButton, &QPushButton::clicked, this, &WelcomeView::onOpenArchive);
    connect(m_extractButton, &QPushButton::clicked, this, &WelcomeView::onExtractArchive);
    
    m_mainLayout->addWidget(m_quickActionsFrame);
}

void WelcomeView::setupRecentFiles() {
    m_recentFilesFrame = new QFrame();
    
    QVBoxLayout* recentLayout = new QVBoxLayout(m_recentFilesFrame);
    
    QLabel* recentTitle = new QLabel("Recent Files");
    recentTitle->setStyleSheet("font-size: 16px; font-weight: bold; margin-bottom: 10px;");
    recentLayout->addWidget(recentTitle);
    
    m_recentFilesLayout = new QVBoxLayout();
    recentLayout->addLayout(m_recentFilesLayout);
    
    m_mainLayout->addWidget(m_recentFilesFrame);
}

void WelcomeView::setupInfoSection() {
    m_infoFrame = new QFrame();
    
    QHBoxLayout* infoLayout = new QHBoxLayout(m_infoFrame);
    
    // Version information
    m_versionLabel = new QLabel(QString("Flux v%1").arg(QApplication::applicationVersion()));
    m_versionLabel->setStyleSheet("font-size: 12px; color: #666666;");
    
    // Supported formats
    m_supportedFormatsLabel = new QLabel("Supported formats: ZIP, 7Z, TAR.GZ, TAR.XZ, TAR.ZSTD");
    m_supportedFormatsLabel->setStyleSheet("font-size: 12px; color: #666666;");
    
    infoLayout->addWidget(m_versionLabel);
    infoLayout->addStretch();
    infoLayout->addWidget(m_supportedFormatsLabel);
    
    m_mainLayout->addWidget(m_infoFrame);
}

void WelcomeView::updateRecentFiles() {
    // Clear existing recent file items using modern C++ approach
    while (auto* item = m_recentFilesLayout->takeAt(0)) {
        if (auto* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
    
    // Read recent files from settings
    QSettings settings;
    QStringList recentFiles = settings.value("recentFiles").toStringList();
    
    if (recentFiles.isEmpty()) {
        QLabel* noFilesLabel = new QLabel("No recent files");
        noFilesLabel->setStyleSheet("color: #999999; font-style: italic;");
        m_recentFilesLayout->addWidget(noFilesLabel);
        return;
    }
    
    // Display up to 5 recent files using modern approach
    constexpr int MAX_RECENT_FILES = 5;
    const int filesToShow = std::min(MAX_RECENT_FILES, recentFiles.size());
    
    for (int i = 0; i < filesToShow; ++i) {
        const auto& filePath = recentFiles[i];
        const QFileInfo fileInfo(filePath);
        
        if (!fileInfo.exists()) {
            continue;
        }
        
        QPushButton* fileButton = new QPushButton();
        fileButton->setText(QString("%1 (%2)")
                           .arg(fileInfo.fileName())
                           .arg(fileInfo.dir().dirName()));
        fileButton->setToolTip(filePath);
        fileButton->setStyleSheet(R"(
            QPushButton {
                text-align: left;
                padding: 8px;
                border: 1px solid #e0e0e0;
                border-radius: 3px;
                background-color: white;
            }
            QPushButton:hover {
                background-color: #f5f5f5;
                border-color: #007acc;
            }
        )");
        
        connect(fileButton, &QPushButton::clicked, [this, filePath]() {
            emit recentFileRequested(filePath);
        });
        
        m_recentFilesLayout->addWidget(fileButton);
    }
}

void WelcomeView::onCreateArchive() {
    emit createArchiveRequested();
}

void WelcomeView::onOpenArchive() {
    emit openArchiveRequested();
}

void WelcomeView::onExtractArchive() {
    emit extractArchiveRequested();
}

void WelcomeView::onRecentFileClicked() {
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (button) {
        QString filePath = button->toolTip();
        emit recentFileRequested(filePath);
    }
}

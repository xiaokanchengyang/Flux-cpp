#include "extract_view.h"
#include "../ui/components/unified_drop_zone.h"
#include "../ui/components/context_menu_manager.h"
#include "../ui/dialogs/smart_extraction_dialog.h"
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QSplitter>
#include <QGridLayout>
#include <QSpacerItem>
#include <QApplication>
#include <QStyle>

using namespace FluxGUI::UI::Components;

ExtractView::ExtractView(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    connectSignals();
    setAcceptDrops(true);
    
    // Setup context menu manager
    m_contextMenuManager = new FluxGUI::UI::Components::ContextMenuManager(this);
    setupContextMenu();
}

void ExtractView::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(16);
    m_mainLayout->setContentsMargins(20, 20, 20, 20);
    
    setupDropZone();
    setupControlPanel();
    setupProgressArea();
    
    // Initially hide progress area
    showExtractionProgress(false);
}

void ExtractView::setupDropZone() {
    m_dropZone = new UnifiedDropZone(this);
    m_dropZone->setDropMessage("Drop archive files here to extract");
    m_dropZone->setAcceptedFileTypes({
        "*.zip", "*.7z", "*.rar", "*.tar", "*.gz", "*.bz2", 
        "*.xz", "*.tar.gz", "*.tar.bz2", "*.tar.xz"
    });
    m_dropZone->setMaxFileCount(1); // Only one archive at a time for extraction
    m_dropZone->setMinimumHeight(200);
    
    m_mainLayout->addWidget(m_dropZone);
}

void ExtractView::setupControlPanel() {
    m_controlGroup = new QGroupBox("Extraction Settings", this);
    auto* controlLayout = new QGridLayout(m_controlGroup);
    controlLayout->setSpacing(12);
    
    // Archive path selection
    auto* archiveLabel = new QLabel("Archive File:", this);
    m_archivePathEdit = new QLineEdit(this);
    m_archivePathEdit->setPlaceholderText("Select or drop an archive file...");
    m_archivePathEdit->setReadOnly(true);
    m_selectArchiveBtn = new QPushButton("Browse...", this);
    m_selectArchiveBtn->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    
    controlLayout->addWidget(archiveLabel, 0, 0);
    controlLayout->addWidget(m_archivePathEdit, 0, 1);
    controlLayout->addWidget(m_selectArchiveBtn, 0, 2);
    
    // Output path selection
    auto* outputLabel = new QLabel("Extract To:", this);
    m_outputPathEdit = new QLineEdit(this);
    m_outputPathEdit->setPlaceholderText("Choose extraction destination...");
    m_selectOutputBtn = new QPushButton("Browse...", this);
    m_selectOutputBtn->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    
    controlLayout->addWidget(outputLabel, 1, 0);
    controlLayout->addWidget(m_outputPathEdit, 1, 1);
    controlLayout->addWidget(m_selectOutputBtn, 1, 2);
    
    // Options
    m_overwriteCheck = new QCheckBox("Overwrite existing files", this);
    m_createFolderCheck = new QCheckBox("Create folder for archive contents", this);
    m_createFolderCheck->setChecked(true);
    
    controlLayout->addWidget(m_overwriteCheck, 2, 0, 1, 2);
    controlLayout->addWidget(m_createFolderCheck, 3, 0, 1, 2);
    
    // Action buttons
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_extractHereBtn = new QPushButton("Extract Here", this);
    m_extractHereBtn->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    m_extractHereBtn->setEnabled(false);
    
    m_extractBtn = new QPushButton("Extract", this);
    m_extractBtn->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    m_extractBtn->setEnabled(false);
    
    m_extractWithOptionsBtn = new QPushButton("Smart Extract...", this);
    m_extractWithOptionsBtn->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
    m_extractWithOptionsBtn->setEnabled(false);
    m_extractWithOptionsBtn->setDefault(true);
    
    buttonLayout->addWidget(m_extractHereBtn);
    buttonLayout->addWidget(m_extractBtn);
    buttonLayout->addWidget(m_extractWithOptionsBtn);
    
    controlLayout->addLayout(buttonLayout, 4, 0, 1, 3);
    
    m_mainLayout->addWidget(m_controlGroup);
}

void ExtractView::setupProgressArea() {
    m_progressGroup = new QGroupBox("Extraction Progress", this);
    auto* progressLayout = new QVBoxLayout(m_progressGroup);
    
    m_progressLabel = new QLabel("Ready to extract...", this);
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    
    auto* progressButtonLayout = new QHBoxLayout();
    progressButtonLayout->addStretch();
    m_cancelBtn = new QPushButton("Cancel", this);
    m_cancelBtn->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
    progressButtonLayout->addWidget(m_cancelBtn);
    
    progressLayout->addWidget(m_progressLabel);
    progressLayout->addWidget(m_progressBar);
    progressLayout->addLayout(progressButtonLayout);
    
    m_mainLayout->addWidget(m_progressGroup);
}

void ExtractView::connectSignals() {
    // Drop zone signals
    connect(m_dropZone, &UnifiedDropZone::filesDropped,
            this, &ExtractView::onDropZoneFilesDropped);
    connect(m_dropZone, &UnifiedDropZone::archiveFilesDropped,
            this, &ExtractView::onDropZoneArchiveFilesDropped);
    
    // Button signals
    connect(m_selectArchiveBtn, &QPushButton::clicked,
            this, &ExtractView::onSelectArchiveClicked);
    connect(m_selectOutputBtn, &QPushButton::clicked,
            this, &ExtractView::onSelectOutputClicked);
    connect(m_extractBtn, &QPushButton::clicked,
            this, &ExtractView::onExtractClicked);
    connect(m_extractHereBtn, &QPushButton::clicked,
            this, &ExtractView::onExtractHereClicked);
    connect(m_extractWithOptionsBtn, &QPushButton::clicked,
            this, &ExtractView::onExtractWithOptionsClicked);
    
    // Options signals
    connect(m_overwriteCheck, &QCheckBox::toggled,
            this, &ExtractView::onOverwriteModeChanged);
    
    // Context menu for archive path field
    m_archivePathEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_archivePathEdit, &QLineEdit::customContextMenuRequested,
            this, &ExtractView::onShowContextMenu);
}

void ExtractView::setupContextMenu() {
    // Connect context menu signals
    connect(m_contextMenuManager, &FluxGUI::UI::Components::ContextMenuManager::extractHere,
            this, &ExtractView::onContextExtractHere);
    connect(m_contextMenuManager, &FluxGUI::UI::Components::ContextMenuManager::extractToFolder,
            this, &ExtractView::onContextExtractToFolder);
    connect(m_contextMenuManager, &FluxGUI::UI::Components::ContextMenuManager::extractWithOptions,
            this, &ExtractView::onContextExtractWithOptions);
    connect(m_contextMenuManager, &FluxGUI::UI::Components::ContextMenuManager::previewArchive,
            this, &ExtractView::onContextPreviewArchive);
    connect(m_contextMenuManager, &FluxGUI::UI::Components::ContextMenuManager::showArchiveProperties,
            this, &ExtractView::onContextShowProperties);
}

void ExtractView::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        bool hasArchive = false;
        for (const QUrl& url : event->mimeData()->urls()) {
            if (url.isLocalFile() && isArchiveFile(url.toLocalFile())) {
                hasArchive = true;
                break;
            }
        }
        
        if (hasArchive) {
            event->acceptProposedAction();
            return;
        }
    }
    event->ignore();
}

void ExtractView::dragMoveEvent(QDragMoveEvent* event) {
    event->acceptProposedAction();
}

void ExtractView::dragLeaveEvent(QDragLeaveEvent* event) {
    event->accept();
}

void ExtractView::dropEvent(QDropEvent* event) {
    QStringList archiveFiles;
    
    for (const QUrl& url : event->mimeData()->urls()) {
        if (url.isLocalFile()) {
            QString filePath = url.toLocalFile();
            if (isArchiveFile(filePath)) {
                archiveFiles.append(filePath);
            }
        }
    }
    
    if (!archiveFiles.isEmpty()) {
        setArchivePath(archiveFiles.first()); // Take the first archive
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void ExtractView::onDropZoneFilesDropped(const QStringList& filePaths) {
    for (const QString& filePath : filePaths) {
        if (isArchiveFile(filePath)) {
            setArchivePath(filePath);
            break; // Take the first archive file
        }
    }
}

void ExtractView::onDropZoneArchiveFilesDropped(const QStringList& archivePaths) {
    if (!archivePaths.isEmpty()) {
        setArchivePath(archivePaths.first());
    }
}

void ExtractView::onSelectArchiveClicked() {
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Select Archive File",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "Archive Files (*.zip *.7z *.rar *.tar *.gz *.bz2 *.xz *.tar.gz *.tar.bz2 *.tar.xz);;All Files (*)"
    );
    
    if (!fileName.isEmpty()) {
        setArchivePath(fileName);
    }
}

void ExtractView::onSelectOutputClicked() {
    QString dirName = QFileDialog::getExistingDirectory(
        this,
        "Select Extraction Directory",
        m_currentOutputPath.isEmpty() ? 
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) :
            m_currentOutputPath
    );
    
    if (!dirName.isEmpty()) {
        setOutputPath(dirName);
    }
}

void ExtractView::onExtractClicked() {
    if (m_currentArchivePath.isEmpty() || m_currentOutputPath.isEmpty()) {
        QMessageBox::warning(this, "Missing Information", 
                           "Please select both archive file and output directory.");
        return;
    }
    
    showExtractionProgress(true);
    m_isExtracting = true;
    
    emit extractionRequested(m_currentArchivePath, m_currentOutputPath);
}

void ExtractView::onExtractHereClicked() {
    if (m_currentArchivePath.isEmpty()) {
        QMessageBox::warning(this, "Missing Information", 
                           "Please select an archive file first.");
        return;
    }
    
    QString outputPath = getDefaultOutputPath(m_currentArchivePath);
    setOutputPath(outputPath);
    onExtractClicked();
}

void ExtractView::onOverwriteModeChanged() {
    // Update extraction options based on overwrite mode
    // This can be used to show/hide additional options
}

void ExtractView::setArchivePath(const QString& path) {
    m_currentArchivePath = path;
    m_archivePathEdit->setText(path);
    
    // Auto-set output path if not already set
    if (m_currentOutputPath.isEmpty()) {
        setOutputPath(getDefaultOutputPath(path));
    }
    
    updateExtractButton();
    emit archiveSelected(path);
}

void ExtractView::setOutputPath(const QString& path) {
    m_currentOutputPath = path;
    m_outputPathEdit->setText(path);
    updateExtractButton();
}

void ExtractView::updateExtractButton() {
    bool canExtract = !m_currentArchivePath.isEmpty() && 
                     !m_currentOutputPath.isEmpty() && 
                     !m_isExtracting;
    
    m_extractBtn->setEnabled(canExtract);
    m_extractHereBtn->setEnabled(!m_currentArchivePath.isEmpty() && !m_isExtracting);
    m_extractWithOptionsBtn->setEnabled(!m_currentArchivePath.isEmpty() && !m_isExtracting);
}

void ExtractView::showExtractionProgress(bool show) {
    m_progressGroup->setVisible(show);
    
    // Update UI state
    m_controlGroup->setEnabled(!show);
    m_dropZone->setEnabled(!show);
}

bool ExtractView::isArchiveFile(const QString& filePath) const {
    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();
    QString completeSuffix = fileInfo.completeSuffix().toLower();
    
    QStringList archiveExtensions = {
        "zip", "7z", "rar", "tar", "gz", "bz2", "xz",
        "tar.gz", "tar.bz2", "tar.xz", "tgz", "tbz2", "txz"
    };
    
    return archiveExtensions.contains(suffix) || 
           archiveExtensions.contains(completeSuffix);
}

QString ExtractView::getDefaultOutputPath(const QString& archivePath) const {
    QFileInfo fileInfo(archivePath);
    QString baseName = fileInfo.completeBaseName();
    QString parentDir = fileInfo.absolutePath();
    
    // If "create folder" is checked, create a subfolder
    if (m_createFolderCheck && m_createFolderCheck->isChecked()) {
        return QDir(parentDir).absoluteFilePath(baseName);
    } else {
        return parentDir;
    }
}

void ExtractView::onExtractWithOptionsClicked() {
    if (m_currentArchivePath.isEmpty()) {
        QMessageBox::warning(this, "Missing Information", 
                           "Please select an archive file first.");
        return;
    }
    
    auto* dialog = new FluxGUI::UI::Dialogs::SmartExtractionDialog(m_currentArchivePath, this);
    dialog->setDefaultOutputPath(m_currentOutputPath.isEmpty() ? 
                                getDefaultOutputPath(m_currentArchivePath) : 
                                m_currentOutputPath);
    
    connect(dialog, &FluxGUI::UI::Dialogs::SmartExtractionDialog::extractionRequested,
            [this](const FluxGUI::UI::Dialogs::SmartExtractionDialog::ExtractionSettings& settings) {
                // Update UI with selected settings
                setOutputPath(settings.createSubfolder ? 
                             QDir(settings.outputPath).absoluteFilePath(settings.subfolderName) :
                             settings.outputPath);
                
                // Trigger extraction with advanced settings
                showExtractionProgress(true);
                m_isExtracting = true;
                emit extractionRequested(m_currentArchivePath, settings.outputPath);
            });
    
    dialog->exec();
    dialog->deleteLater();
}

void ExtractView::onShowContextMenu(const QPoint& pos) {
    if (!m_currentArchivePath.isEmpty() && isArchiveFile(m_currentArchivePath)) {
        QPoint globalPos = m_archivePathEdit->mapToGlobal(pos);
        m_contextMenuManager->showArchiveContextMenu(m_currentArchivePath, globalPos);
    }
}

void ExtractView::onContextExtractHere(const QString& archivePath) {
    setArchivePath(archivePath);
    onExtractHereClicked();
}

void ExtractView::onContextExtractToFolder(const QString& archivePath, const QString& outputPath) {
    setArchivePath(archivePath);
    setOutputPath(outputPath);
    onExtractClicked();
}

void ExtractView::onContextExtractWithOptions(const QString& archivePath) {
    setArchivePath(archivePath);
    onExtractWithOptionsClicked();
}

void ExtractView::onContextPreviewArchive(const QString& archivePath) {
    // Switch to browse view or open preview dialog
    emit archiveSelected(archivePath);
    QMessageBox::information(this, "Archive Preview", 
                           QString("Preview functionality for %1 would be implemented here.")
                           .arg(QFileInfo(archivePath).fileName()));
}

void ExtractView::onContextShowProperties(const QString& archivePath) {
    QFileInfo info(archivePath);
    QString properties = QString("Archive Properties:\n\n"
                               "Name: %1\n"
                               "Size: %2\n"
                               "Path: %3\n"
                               "Modified: %4")
                        .arg(info.fileName())
                        .arg(formatFileSize(info.size()))
                        .arg(info.absolutePath())
                        .arg(info.lastModified().toString());
    
    QMessageBox::information(this, "Archive Properties", properties);
}

QString ExtractView::formatFileSize(qint64 size) const {
    if (size < 1024) {
        return QString("%1 B").arg(size);
    } else if (size < 1024 * 1024) {
        return QString("%1 KB").arg(size / 1024.0, 0, 'f', 1);
    } else if (size < 1024 * 1024 * 1024) {
        return QString("%1 MB").arg(size / (1024.0 * 1024.0), 0, 'f', 1);
    } else {
        return QString("%1 GB").arg(size / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
    }
}


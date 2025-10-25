#include "smart_extraction_dialog.h"
#include <QApplication>
#include <QStyle>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>
#include <QDir>
#include <QTimer>
#include <QHeaderView>

namespace FluxGUI::UI::Dialogs {

SmartExtractionDialog::SmartExtractionDialog(const QString& archivePath, QWidget* parent)
    : QDialog(parent)
    , m_archivePath(archivePath)
{
    setWindowTitle("Smart Extraction");
    setWindowIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    setModal(true);
    resize(700, 600);
    
    setupUI();
    connectSignals();
    
    // Set default output path
    QFileInfo archiveInfo(archivePath);
    setDefaultOutputPath(archiveInfo.absolutePath());
    
    // Start analysis automatically
    QTimer::singleShot(100, this, &SmartExtractionDialog::onAnalyzeArchive);
}

SmartExtractionDialog::~SmartExtractionDialog() = default;

void SmartExtractionDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // Archive info header
    QFileInfo archiveInfo(m_archivePath);
    auto* headerLabel = new QLabel(QString("Extracting: %1").arg(archiveInfo.fileName()), this);
    headerLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; }");
    mainLayout->addWidget(headerLabel);
    
    // Create tab widget
    m_tabWidget = new QTabWidget(this);
    
    setupBasicTab();
    setupFilesTab();
    setupAdvancedTab();
    
    mainLayout->addWidget(m_tabWidget);
    
    // Analysis and recommendations section
    m_analysisGroup = new QGroupBox("Archive Analysis", this);
    auto* analysisLayout = new QGridLayout(m_analysisGroup);
    
    m_archiveSizeLabel = new QLabel("Size: Analyzing...", this);
    m_fileCountLabel = new QLabel("Files: Analyzing...", this);
    m_structureLabel = new QLabel("Structure: Analyzing...", this);
    
    m_recommendationLabel = new QLabel("Analyzing archive to provide recommendations...", this);
    m_recommendationLabel->setWordWrap(true);
    m_recommendationLabel->setStyleSheet("QLabel { color: #0066cc; }");
    
    m_analysisProgress = new QProgressBar(this);
    m_analysisProgress->setRange(0, 0); // Indeterminate
    
    auto* analysisButtonLayout = new QHBoxLayout();
    m_analyzeBtn = new QPushButton("Re-analyze", this);
    m_analyzeBtn->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    
    m_previewBtn = new QPushButton("Preview", this);
    m_previewBtn->setIcon(style()->standardIcon(QStyle::SP_FileDialogListView));
    m_previewBtn->setEnabled(false);
    
    analysisButtonLayout->addWidget(m_analyzeBtn);
    analysisButtonLayout->addWidget(m_previewBtn);
    analysisButtonLayout->addStretch();
    
    analysisLayout->addWidget(m_archiveSizeLabel, 0, 0);
    analysisLayout->addWidget(m_fileCountLabel, 0, 1);
    analysisLayout->addWidget(m_structureLabel, 1, 0, 1, 2);
    analysisLayout->addWidget(m_recommendationLabel, 2, 0, 1, 2);
    analysisLayout->addWidget(m_analysisProgress, 3, 0, 1, 2);
    analysisLayout->addLayout(analysisButtonLayout, 4, 0, 1, 2);
    
    mainLayout->addWidget(m_analysisGroup);
    
    // Dialog buttons
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_helpBtn = new QPushButton("Help", this);
    m_helpBtn->setIcon(style()->standardIcon(QStyle::SP_DialogHelpButton));
    
    m_cancelBtn = new QPushButton("Cancel", this);
    m_cancelBtn->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
    
    m_okBtn = new QPushButton("Extract", this);
    m_okBtn->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    m_okBtn->setDefault(true);
    
    buttonLayout->addWidget(m_helpBtn);
    buttonLayout->addWidget(m_cancelBtn);
    buttonLayout->addWidget(m_okBtn);
    
    mainLayout->addLayout(buttonLayout);
}

void SmartExtractionDialog::setupBasicTab() {
    m_basicTab = new QWidget();
    auto* layout = new QVBoxLayout(m_basicTab);
    layout->setSpacing(16);
    
    // Output location group
    auto* outputGroup = new QGroupBox("Output Location", this);
    auto* outputLayout = new QGridLayout(outputGroup);
    
    auto* pathLabel = new QLabel("Extract to:", this);
    m_outputPathEdit = new QLineEdit(this);
    m_outputPathEdit->setPlaceholderText("Choose extraction destination...");
    m_browseBtn = new QPushButton("Browse...", this);
    m_browseBtn->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    
    outputLayout->addWidget(pathLabel, 0, 0);
    outputLayout->addWidget(m_outputPathEdit, 0, 1);
    outputLayout->addWidget(m_browseBtn, 0, 2);
    
    layout->addWidget(outputGroup);
    
    // Folder options group
    auto* folderGroup = new QGroupBox("Folder Options", this);
    auto* folderLayout = new QVBoxLayout(folderGroup);
    
    m_createSubfolderCheck = new QCheckBox("Create subfolder for archive contents", this);
    m_createSubfolderCheck->setChecked(true);
    
    auto* subfolderLayout = new QHBoxLayout();
    auto* subfolderLabel = new QLabel("Subfolder name:", this);
    m_subfolderNameEdit = new QLineEdit(this);
    m_subfolderNameEdit->setPlaceholderText("Auto-generated name");
    
    subfolderLayout->addWidget(subfolderLabel);
    subfolderLayout->addWidget(m_subfolderNameEdit);
    
    m_previewPathLabel = new QLabel(this);
    m_previewPathLabel->setStyleSheet("QLabel { color: #666; font-size: 11px; }");
    m_previewPathLabel->setWordWrap(true);
    
    folderLayout->addWidget(m_createSubfolderCheck);
    folderLayout->addLayout(subfolderLayout);
    folderLayout->addWidget(m_previewPathLabel);
    
    layout->addWidget(folderGroup);
    
    // Basic options group
    auto* optionsGroup = new QGroupBox("Options", this);
    auto* optionsLayout = new QVBoxLayout(optionsGroup);
    
    m_overwriteCheck = new QCheckBox("Overwrite existing files", this);
    m_openDestinationCheck = new QCheckBox("Open destination folder after extraction", this);
    m_openDestinationCheck->setChecked(true);
    
    optionsLayout->addWidget(m_overwriteCheck);
    optionsLayout->addWidget(m_openDestinationCheck);
    
    layout->addWidget(optionsGroup);
    
    layout->addStretch();
    
    m_tabWidget->addTab(m_basicTab, "Basic");
}

void SmartExtractionDialog::setupFilesTab() {
    m_filesTab = new QWidget();
    auto* layout = new QVBoxLayout(m_filesTab);
    layout->setSpacing(12);
    
    // Selection controls
    auto* controlsLayout = new QHBoxLayout();
    m_extractSelectedOnlyCheck = new QCheckBox("Extract selected files only", this);
    
    controlsLayout->addWidget(m_extractSelectedOnlyCheck);
    controlsLayout->addStretch();
    
    m_selectAllBtn = new QPushButton("Select All", this);
    m_selectAllBtn->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    m_selectNoneBtn = new QPushButton("Select None", this);
    m_selectNoneBtn->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
    
    controlsLayout->addWidget(m_selectAllBtn);
    controlsLayout->addWidget(m_selectNoneBtn);
    
    layout->addLayout(controlsLayout);
    
    // File tree and info splitter
    m_filesSplitter = new QSplitter(Qt::Horizontal, this);
    
    // File tree
    m_fileTree = new QTreeWidget(this);
    m_fileTree->setHeaderLabels({"Name", "Size", "Type", "Modified"});
    m_fileTree->setRootIsDecorated(true);
    m_fileTree->setAlternatingRowColors(true);
    m_fileTree->setSortingEnabled(true);
    m_fileTree->header()->setStretchLastSection(false);
    m_fileTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    
    // File info panel
    m_fileInfoEdit = new QTextEdit(this);
    m_fileInfoEdit->setMaximumWidth(250);
    m_fileInfoEdit->setReadOnly(true);
    m_fileInfoEdit->setPlaceholderText("Select a file to view details...");
    
    m_filesSplitter->addWidget(m_fileTree);
    m_filesSplitter->addWidget(m_fileInfoEdit);
    m_filesSplitter->setStretchFactor(0, 1);
    m_filesSplitter->setStretchFactor(1, 0);
    
    layout->addWidget(m_filesSplitter);
    
    // Selection count
    m_selectionCountLabel = new QLabel("0 files selected", this);
    m_selectionCountLabel->setStyleSheet("QLabel { color: #666; }");
    layout->addWidget(m_selectionCountLabel);
    
    m_tabWidget->addTab(m_filesTab, "Files");
}

void SmartExtractionDialog::setupAdvancedTab() {
    m_advancedTab = new QWidget();
    auto* layout = new QVBoxLayout(m_advancedTab);
    layout->setSpacing(16);
    
    // File attributes group
    auto* attributesGroup = new QGroupBox("File Attributes", this);
    auto* attributesLayout = new QVBoxLayout(attributesGroup);
    
    m_preservePermissionsCheck = new QCheckBox("Preserve file permissions", this);
    m_preservePermissionsCheck->setChecked(true);
    
    m_preserveTimestampsCheck = new QCheckBox("Preserve file timestamps", this);
    m_preserveTimestampsCheck->setChecked(true);
    
    attributesLayout->addWidget(m_preservePermissionsCheck);
    attributesLayout->addWidget(m_preserveTimestampsCheck);
    
    layout->addWidget(attributesGroup);
    
    // Security group
    auto* securityGroup = new QGroupBox("Security", this);
    auto* securityLayout = new QGridLayout(securityGroup);
    
    auto* passwordLabel = new QLabel("Archive password:", this);
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("Enter password if archive is encrypted");
    
    securityLayout->addWidget(passwordLabel, 0, 0);
    securityLayout->addWidget(m_passwordEdit, 0, 1);
    
    layout->addWidget(securityGroup);
    
    // Exclude patterns group
    auto* excludeGroup = new QGroupBox("Exclude Patterns", this);
    auto* excludeLayout = new QVBoxLayout(excludeGroup);
    
    auto* excludeLabel = new QLabel("Skip files matching these patterns (one per line):", this);
    m_excludePatternsEdit = new QTextEdit(this);
    m_excludePatternsEdit->setMaximumHeight(80);
    m_excludePatternsEdit->setPlaceholderText("*.tmp\n*.log\n.DS_Store\nThumbs.db");
    
    excludeLayout->addWidget(excludeLabel);
    excludeLayout->addWidget(m_excludePatternsEdit);
    
    layout->addWidget(excludeGroup);
    
    // Post-extraction actions group
    auto* actionsGroup = new QGroupBox("Post-Extraction Actions", this);
    auto* actionsLayout = new QVBoxLayout(actionsGroup);
    
    m_deleteArchiveCheck = new QCheckBox("Delete archive file after successful extraction", this);
    
    actionsLayout->addWidget(m_deleteArchiveCheck);
    
    layout->addWidget(actionsGroup);
    
    layout->addStretch();
    
    m_tabWidget->addTab(m_advancedTab, "Advanced");
}

void SmartExtractionDialog::connectSignals() {
    // Basic tab signals
    connect(m_browseBtn, &QPushButton::clicked, this, &SmartExtractionDialog::onOutputPathBrowse);
    connect(m_createSubfolderCheck, &QCheckBox::toggled, this, &SmartExtractionDialog::onCreateSubfolderToggled);
    connect(m_subfolderNameEdit, &QLineEdit::textChanged, this, &SmartExtractionDialog::onSubfolderNameChanged);
    connect(m_outputPathEdit, &QLineEdit::textChanged, this, &SmartExtractionDialog::onSubfolderNameChanged);
    
    // Files tab signals
    connect(m_selectAllBtn, &QPushButton::clicked, this, &SmartExtractionDialog::onSelectAll);
    connect(m_selectNoneBtn, &QPushButton::clicked, this, &SmartExtractionDialog::onSelectNone);
    connect(m_fileTree, &QTreeWidget::itemChanged, this, &SmartExtractionDialog::onTreeItemChanged);
    
    // Analysis signals
    connect(m_analyzeBtn, &QPushButton::clicked, this, &SmartExtractionDialog::onAnalyzeArchive);
    connect(m_previewBtn, &QPushButton::clicked, this, &SmartExtractionDialog::onPreviewExtraction);
    
    // Dialog buttons
    connect(m_okBtn, &QPushButton::clicked, this, &SmartExtractionDialog::onAccept);
    connect(m_cancelBtn, &QPushButton::clicked, this, &SmartExtractionDialog::onReject);
}

void SmartExtractionDialog::onOutputPathBrowse() {
    QString defaultPath = m_outputPathEdit->text();
    if (defaultPath.isEmpty()) {
        defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    
    QString dirName = QFileDialog::getExistingDirectory(
        this,
        "Select Extraction Directory",
        defaultPath
    );
    
    if (!dirName.isEmpty()) {
        m_outputPathEdit->setText(dirName);
    }
}

void SmartExtractionDialog::onCreateSubfolderToggled(bool enabled) {
    m_subfolderNameEdit->setEnabled(enabled);
    onSubfolderNameChanged(); // Update preview
}

void SmartExtractionDialog::onSubfolderNameChanged() {
    QString basePath = m_outputPathEdit->text();
    QString previewPath = basePath;
    
    if (m_createSubfolderCheck->isChecked()) {
        QString subfolderName = m_subfolderNameEdit->text();
        if (subfolderName.isEmpty() && m_analysisComplete) {
            subfolderName = m_analysis.recommendedSubfolderName;
        }
        if (!subfolderName.isEmpty()) {
            previewPath = QDir(basePath).absoluteFilePath(subfolderName);
        }
    }
    
    m_previewPathLabel->setText(QString("Files will be extracted to: %1").arg(previewPath));
}

void SmartExtractionDialog::onAnalyzeArchive() {
    m_analysisProgress->setVisible(true);
    m_analyzeBtn->setEnabled(false);
    
    analyzeArchiveStructure();
    updateRecommendations();
    populateFileTree();
    
    m_analysisProgress->setVisible(false);
    m_analyzeBtn->setEnabled(true);
    m_previewBtn->setEnabled(true);
    m_analysisComplete = true;
    
    // Update subfolder name if not manually set
    if (m_subfolderNameEdit->text().isEmpty()) {
        m_subfolderNameEdit->setText(m_analysis.recommendedSubfolderName);
    }
    
    onSubfolderNameChanged(); // Update preview path
}

void SmartExtractionDialog::onSelectAll() {
    for (int i = 0; i < m_fileTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_fileTree->topLevelItem(i);
        item->setCheckState(0, Qt::Checked);
    }
    updateSelectionCount();
}

void SmartExtractionDialog::onSelectNone() {
    for (int i = 0; i < m_fileTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_fileTree->topLevelItem(i);
        item->setCheckState(0, Qt::Unchecked);
    }
    updateSelectionCount();
}

void SmartExtractionDialog::onTreeItemChanged(QTreeWidgetItem* item, int column) {
    Q_UNUSED(item)
    Q_UNUSED(column)
    updateSelectionCount();
}

void SmartExtractionDialog::onPreviewExtraction() {
    validateSettings();
    
    QString message = QString("Extraction Preview:\n\n");
    message += QString("Archive: %1\n").arg(QFileInfo(m_archivePath).fileName());
    message += QString("Destination: %1\n").arg(m_previewPathLabel->text().mid(26)); // Remove "Files will be extracted to: "
    message += QString("Files to extract: %1\n").arg(m_extractSelectedOnlyCheck->isChecked() ? 
                                                     QString::number(m_selectedFiles.size()) : "All files");
    message += QString("Overwrite existing: %1\n").arg(m_overwriteCheck->isChecked() ? "Yes" : "No");
    
    QMessageBox::information(this, "Extraction Preview", message);
}

void SmartExtractionDialog::onAccept() {
    validateSettings();
    
    if (m_outputPathEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Missing Output Path", 
                           "Please specify an output path for extraction.");
        m_tabWidget->setCurrentIndex(0); // Switch to basic tab
        m_outputPathEdit->setFocus();
        return;
    }
    
    // Collect settings
    m_currentSettings.archivePath = m_archivePath;
    m_currentSettings.outputPath = m_outputPathEdit->text();
    m_currentSettings.createSubfolder = m_createSubfolderCheck->isChecked();
    m_currentSettings.subfolderName = m_subfolderNameEdit->text();
    m_currentSettings.overwriteExisting = m_overwriteCheck->isChecked();
    m_currentSettings.preservePermissions = m_preservePermissionsCheck->isChecked();
    m_currentSettings.preserveTimestamps = m_preserveTimestampsCheck->isChecked();
    m_currentSettings.extractSelectedOnly = m_extractSelectedOnlyCheck->isChecked();
    m_currentSettings.selectedFiles = m_selectedFiles;
    m_currentSettings.openDestinationAfter = m_openDestinationCheck->isChecked();
    m_currentSettings.deleteArchiveAfter = m_deleteArchiveCheck->isChecked();
    m_currentSettings.password = m_passwordEdit->text();
    
    // Parse exclude patterns
    QString excludeText = m_excludePatternsEdit->toPlainText();
    m_currentSettings.excludePatterns = excludeText.split('\n', Qt::SkipEmptyParts);
    
    emit extractionRequested(m_currentSettings);
    accept();
}

void SmartExtractionDialog::onReject() {
    reject();
}

void SmartExtractionDialog::analyzeArchiveStructure() {
    m_analysis = analyzeArchive(m_archivePath);
    
    // Update UI with analysis results
    QFileInfo archiveInfo(m_archivePath);
    m_archiveSizeLabel->setText(QString("Size: %1").arg(formatFileSize(archiveInfo.size())));
    m_fileCountLabel->setText(QString("Files: %1 files, %2 folders")
                             .arg(m_analysis.totalFiles)
                             .arg(m_analysis.totalFolders));
    
    QString structureText = "Structure: ";
    if (m_analysis.hasRootFolder) {
        structureText += QString("Single root folder (%1)").arg(m_analysis.suggestedRootFolder);
    } else if (m_analysis.hasMultipleRootItems) {
        structureText += QString("Multiple root items (%1 items)").arg(m_analysis.rootItems.size());
    } else {
        structureText += "Mixed structure";
    }
    m_structureLabel->setText(structureText);
}

void SmartExtractionDialog::updateRecommendations() {
    QString recommendation;
    
    if (m_analysis.needsSubfolder) {
        recommendation = QString("Recommended: Create subfolder '%1' to organize extracted files. ")
                        .arg(m_analysis.recommendedSubfolderName);
        m_createSubfolderCheck->setChecked(true);
    } else {
        recommendation = "Recommended: Extract directly to destination folder. ";
        m_createSubfolderCheck->setChecked(false);
    }
    
    if (m_analysis.hasExecutables) {
        recommendation += "Archive contains executable files - be cautious. ";
    }
    
    if (m_analysis.hasHiddenFiles) {
        recommendation += "Archive contains hidden files. ";
    }
    
    m_recommendationLabel->setText(recommendation);
}

void SmartExtractionDialog::populateFileTree() {
    m_fileTree->clear();
    
    // Simulate populating file tree with archive contents
    // In real implementation, this would read the actual archive
    
    QTreeWidgetItem* rootItem = new QTreeWidgetItem(m_fileTree);
    rootItem->setText(0, "documents");
    rootItem->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
    rootItem->setCheckState(0, Qt::Checked);
    rootItem->setFlags(rootItem->flags() | Qt::ItemIsUserCheckable);
    
    QTreeWidgetItem* fileItem1 = new QTreeWidgetItem(rootItem);
    fileItem1->setText(0, "readme.txt");
    fileItem1->setText(1, "1.2 KB");
    fileItem1->setText(2, "Text file");
    fileItem1->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
    fileItem1->setCheckState(0, Qt::Checked);
    fileItem1->setFlags(fileItem1->flags() | Qt::ItemIsUserCheckable);
    
    QTreeWidgetItem* fileItem2 = new QTreeWidgetItem(rootItem);
    fileItem2->setText(0, "data.csv");
    fileItem2->setText(1, "45.6 KB");
    fileItem2->setText(2, "CSV file");
    fileItem2->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
    fileItem2->setCheckState(0, Qt::Checked);
    fileItem2->setFlags(fileItem2->flags() | Qt::ItemIsUserCheckable);
    
    m_fileTree->expandAll();
    m_fileTree->resizeColumnToContents(0);
    m_fileTree->resizeColumnToContents(1);
    m_fileTree->resizeColumnToContents(2);
    
    updateSelectionCount();
}

void SmartExtractionDialog::updateSelectionCount() {
    int selectedCount = 0;
    m_selectedFiles.clear();
    
    // Count selected items (simplified)
    for (int i = 0; i < m_fileTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_fileTree->topLevelItem(i);
        if (item->checkState(0) == Qt::Checked) {
            selectedCount++;
            m_selectedFiles.append(item->text(0));
        }
        
        // Check children
        for (int j = 0; j < item->childCount(); ++j) {
            QTreeWidgetItem* child = item->child(j);
            if (child->checkState(0) == Qt::Checked) {
                selectedCount++;
                m_selectedFiles.append(child->text(0));
            }
        }
    }
    
    m_selectionCountLabel->setText(QString("%1 files selected").arg(selectedCount));
}

void SmartExtractionDialog::validateSettings() {
    // Validation logic would go here
}

SmartExtractionDialog::ArchiveAnalysis SmartExtractionDialog::analyzeArchive(const QString& archivePath) {
    ArchiveAnalysis analysis;
    
    // Simulate archive analysis
    // In real implementation, this would use the archive library
    QFileInfo archiveInfo(archivePath);
    
    analysis.totalSize = archiveInfo.size();
    analysis.totalFiles = 15; // Simulated
    analysis.totalFolders = 3; // Simulated
    analysis.hasRootFolder = true;
    analysis.suggestedRootFolder = "documents";
    analysis.hasMultipleRootItems = false;
    analysis.rootItems = QStringList{"documents"};
    analysis.fileTypes = QStringList{"text", "csv", "image"};
    analysis.hasExecutables = false;
    analysis.hasHiddenFiles = false;
    
    // Determine if subfolder is needed
    analysis.needsSubfolder = shouldCreateSubfolder(analysis);
    analysis.recommendedSubfolderName = suggestSubfolderName(analysis, archivePath);
    
    return analysis;
}

bool SmartExtractionDialog::shouldCreateSubfolder(const ArchiveAnalysis& analysis) {
    // Create subfolder if:
    // 1. Archive has multiple root items
    // 2. Archive doesn't have a single root folder
    // 3. Archive has many files at root level
    
    return analysis.hasMultipleRootItems || !analysis.hasRootFolder;
}

QString SmartExtractionDialog::suggestSubfolderName(const ArchiveAnalysis& analysis, const QString& archivePath) {
    if (analysis.hasRootFolder && !analysis.suggestedRootFolder.isEmpty()) {
        return analysis.suggestedRootFolder;
    } else {
        QFileInfo archiveInfo(archivePath);
        return archiveInfo.completeBaseName();
    }
}

QString SmartExtractionDialog::formatFileSize(qint64 size) const {
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

QTreeWidgetItem* SmartExtractionDialog::findOrCreateTreeItem(const QString& path, QTreeWidgetItem* parent) {
    Q_UNUSED(path)
    Q_UNUSED(parent)
    // Implementation for finding or creating tree items
    return nullptr;
}

SmartExtractionDialog::ExtractionSettings SmartExtractionDialog::getSettings() const {
    return m_currentSettings;
}

void SmartExtractionDialog::setDefaultOutputPath(const QString& path) {
    m_outputPathEdit->setText(path);
}

void SmartExtractionDialog::setSelectedFiles(const QStringList& files) {
    m_selectedFiles = files;
}

} // namespace FluxGUI::UI::Dialogs





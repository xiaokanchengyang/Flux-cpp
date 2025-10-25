#include "smart_compression_dialog.h"
#include <QApplication>
#include <QStyle>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QMimeType>
#include <QDir>
#include <QDirIterator>
#include <QThread>
#include <QTimer>
#include <QSplitter>

namespace FluxGUI::UI::Dialogs {

SmartCompressionDialog::SmartCompressionDialog(const QStringList& inputPaths, QWidget* parent)
    : QDialog(parent)
    , m_inputPaths(inputPaths)
{
    setWindowTitle("Smart Compression");
    setWindowIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    setModal(true);
    resize(600, 500);
    
    setupUI();
    connectSignals();
    
    // Start analysis automatically
    QTimer::singleShot(100, this, &SmartCompressionDialog::onAnalyzeFiles);
}

SmartCompressionDialog::~SmartCompressionDialog() = default;

void SmartCompressionDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // Create tab widget
    m_tabWidget = new QTabWidget(this);
    
    setupBasicTab();
    setupAdvancedTab();
    setupAnalysisTab();
    
    mainLayout->addWidget(m_tabWidget);
    
    // Dialog buttons
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_helpBtn = new QPushButton("Help", this);
    m_helpBtn->setIcon(style()->standardIcon(QStyle::SP_DialogHelpButton));
    
    m_cancelBtn = new QPushButton("Cancel", this);
    m_cancelBtn->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
    
    m_okBtn = new QPushButton("Compress", this);
    m_okBtn->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    m_okBtn->setDefault(true);
    
    buttonLayout->addWidget(m_helpBtn);
    buttonLayout->addWidget(m_cancelBtn);
    buttonLayout->addWidget(m_okBtn);
    
    mainLayout->addLayout(buttonLayout);
}

void SmartCompressionDialog::setupBasicTab() {
    m_basicTab = new QWidget();
    auto* layout = new QVBoxLayout(m_basicTab);
    layout->setSpacing(16);
    
    // Output settings group
    auto* outputGroup = new QGroupBox("Output Settings", this);
    auto* outputLayout = new QGridLayout(outputGroup);
    
    // Output path
    auto* pathLabel = new QLabel("Output Path:", this);
    m_outputPathEdit = new QLineEdit(this);
    m_outputPathEdit->setPlaceholderText("Choose output location...");
    m_browseBtn = new QPushButton("Browse...", this);
    m_browseBtn->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    
    outputLayout->addWidget(pathLabel, 0, 0);
    outputLayout->addWidget(m_outputPathEdit, 0, 1);
    outputLayout->addWidget(m_browseBtn, 0, 2);
    
    layout->addWidget(outputGroup);
    
    // Format settings group
    auto* formatGroup = new QGroupBox("Compression Format", this);
    auto* formatLayout = new QVBoxLayout(formatGroup);
    
    m_formatCombo = new QComboBox(this);
    m_formatCombo->addItem("ZIP - Universal compatibility", "zip");
    m_formatCombo->addItem("7Z - Best compression ratio", "7z");
    m_formatCombo->addItem("TAR.GZ - Unix/Linux standard", "tar.gz");
    m_formatCombo->addItem("TAR.BZ2 - Better compression", "tar.bz2");
    m_formatCombo->addItem("TAR.XZ - Excellent compression", "tar.xz");
    
    m_formatDescLabel = new QLabel(this);
    m_formatDescLabel->setWordWrap(true);
    m_formatDescLabel->setStyleSheet("QLabel { color: #666; font-size: 11px; }");
    
    formatLayout->addWidget(m_formatCombo);
    formatLayout->addWidget(m_formatDescLabel);
    
    layout->addWidget(formatGroup);
    
    // Compression level group
    auto* levelGroup = new QGroupBox("Compression Level", this);
    auto* levelLayout = new QGridLayout(levelGroup);
    
    auto* levelLabel = new QLabel("Level:", this);
    m_compressionSlider = new QSlider(Qt::Horizontal, this);
    m_compressionSlider->setRange(0, 9);
    m_compressionSlider->setValue(6);
    m_compressionSlider->setTickPosition(QSlider::TicksBelow);
    m_compressionSlider->setTickInterval(1);
    
    m_compressionSpin = new QSpinBox(this);
    m_compressionSpin->setRange(0, 9);
    m_compressionSpin->setValue(6);
    
    m_compressionDescLabel = new QLabel("Balanced (recommended)", this);
    m_compressionDescLabel->setStyleSheet("QLabel { color: #666; font-size: 11px; }");
    
    // Level descriptions
    auto* fastLabel = new QLabel("Fast", this);
    fastLabel->setAlignment(Qt::AlignLeft);
    auto* bestLabel = new QLabel("Best", this);
    bestLabel->setAlignment(Qt::AlignRight);
    
    levelLayout->addWidget(levelLabel, 0, 0);
    levelLayout->addWidget(m_compressionSlider, 0, 1);
    levelLayout->addWidget(m_compressionSpin, 0, 2);
    levelLayout->addWidget(fastLabel, 1, 1, Qt::AlignLeft);
    levelLayout->addWidget(bestLabel, 1, 1, Qt::AlignRight);
    levelLayout->addWidget(m_compressionDescLabel, 2, 0, 1, 3);
    
    layout->addWidget(levelGroup);
    
    // Options group
    auto* optionsGroup = new QGroupBox("Options", this);
    auto* optionsLayout = new QVBoxLayout(optionsGroup);
    
    m_deleteOriginalCheck = new QCheckBox("Delete original files after compression", this);
    optionsLayout->addWidget(m_deleteOriginalCheck);
    
    layout->addWidget(optionsGroup);
    
    layout->addStretch();
    
    m_tabWidget->addTab(m_basicTab, "Basic");
}

void SmartCompressionDialog::setupAdvancedTab() {
    m_advancedTab = new QWidget();
    auto* layout = new QVBoxLayout(m_advancedTab);
    layout->setSpacing(16);
    
    // Compression method group
    auto* methodGroup = new QGroupBox("Compression Method", this);
    auto* methodLayout = new QGridLayout(methodGroup);
    
    auto* methodLabel = new QLabel("Method:", this);
    m_compressionMethodCombo = new QComboBox(this);
    m_compressionMethodCombo->addItem("LZMA2 (recommended)", "lzma2");
    m_compressionMethodCombo->addItem("LZMA", "lzma");
    m_compressionMethodCombo->addItem("Deflate", "deflate");
    m_compressionMethodCombo->addItem("BZip2", "bzip2");
    m_compressionMethodCombo->addItem("PPMd", "ppmd");
    
    methodLayout->addWidget(methodLabel, 0, 0);
    methodLayout->addWidget(m_compressionMethodCombo, 0, 1);
    
    // Dictionary size
    auto* dictLabel = new QLabel("Dictionary Size (MB):", this);
    m_dictionarySizeSpin = new QSpinBox(this);
    m_dictionarySizeSpin->setRange(1, 1024);
    m_dictionarySizeSpin->setValue(32);
    m_dictionarySizeSpin->setSuffix(" MB");
    
    methodLayout->addWidget(dictLabel, 1, 0);
    methodLayout->addWidget(m_dictionarySizeSpin, 1, 1);
    
    // Word size
    auto* wordLabel = new QLabel("Word Size:", this);
    m_wordSizeSpin = new QSpinBox(this);
    m_wordSizeSpin->setRange(8, 273);
    m_wordSizeSpin->setValue(32);
    
    methodLayout->addWidget(wordLabel, 2, 0);
    methodLayout->addWidget(m_wordSizeSpin, 2, 1);
    
    layout->addWidget(methodGroup);
    
    // Archive options group
    auto* archiveGroup = new QGroupBox("Archive Options", this);
    auto* archiveLayout = new QVBoxLayout(archiveGroup);
    
    m_solidArchiveCheck = new QCheckBox("Create solid archive (better compression)", this);
    m_solidArchiveCheck->setChecked(true);
    
    m_multiThreadCheck = new QCheckBox("Enable multi-threading", this);
    m_multiThreadCheck->setChecked(true);
    
    m_separateArchivesCheck = new QCheckBox("Create separate archives for each item", this);
    
    archiveLayout->addWidget(m_solidArchiveCheck);
    archiveLayout->addWidget(m_multiThreadCheck);
    archiveLayout->addWidget(m_separateArchivesCheck);
    
    layout->addWidget(archiveGroup);
    
    // Encryption group
    auto* encryptGroup = new QGroupBox("Encryption", this);
    auto* encryptLayout = new QGridLayout(encryptGroup);
    
    m_encryptCheck = new QCheckBox("Encrypt archive with password", this);
    
    auto* passwordLabel = new QLabel("Password:", this);
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setEnabled(false);
    
    auto* confirmLabel = new QLabel("Confirm:", this);
    m_confirmPasswordEdit = new QLineEdit(this);
    m_confirmPasswordEdit->setEchoMode(QLineEdit::Password);
    m_confirmPasswordEdit->setEnabled(false);
    
    encryptLayout->addWidget(m_encryptCheck, 0, 0, 1, 2);
    encryptLayout->addWidget(passwordLabel, 1, 0);
    encryptLayout->addWidget(m_passwordEdit, 1, 1);
    encryptLayout->addWidget(confirmLabel, 2, 0);
    encryptLayout->addWidget(m_confirmPasswordEdit, 2, 1);
    
    layout->addWidget(encryptGroup);
    
    // Exclude patterns group
    auto* excludeGroup = new QGroupBox("Exclude Patterns", this);
    auto* excludeLayout = new QVBoxLayout(excludeGroup);
    
    auto* excludeLabel = new QLabel("Exclude files matching these patterns (one per line):", this);
    m_excludePatternsEdit = new QTextEdit(this);
    m_excludePatternsEdit->setMaximumHeight(80);
    m_excludePatternsEdit->setPlaceholderText("*.tmp\n*.log\n.DS_Store\nThumbs.db");
    
    excludeLayout->addWidget(excludeLabel);
    excludeLayout->addWidget(m_excludePatternsEdit);
    
    layout->addWidget(excludeGroup);
    
    layout->addStretch();
    
    m_tabWidget->addTab(m_advancedTab, "Advanced");
}

void SmartCompressionDialog::setupAnalysisTab() {
    m_analysisTab = new QWidget();
    auto* layout = new QVBoxLayout(m_analysisTab);
    layout->setSpacing(16);
    
    // File analysis group
    auto* analysisGroup = new QGroupBox("File Analysis", this);
    auto* analysisLayout = new QGridLayout(analysisGroup);
    
    m_totalSizeLabel = new QLabel("Total Size: Analyzing...", this);
    m_fileCountLabel = new QLabel("Files: Analyzing...", this);
    m_fileTypesLabel = new QLabel("File Types: Analyzing...", this);
    
    analysisLayout->addWidget(m_totalSizeLabel, 0, 0);
    analysisLayout->addWidget(m_fileCountLabel, 1, 0);
    analysisLayout->addWidget(m_fileTypesLabel, 2, 0);
    
    layout->addWidget(analysisGroup);
    
    // Recommendations group
    auto* recommendGroup = new QGroupBox("Smart Recommendations", this);
    auto* recommendLayout = new QVBoxLayout(recommendGroup);
    
    m_recommendationLabel = new QLabel("Analyzing files to provide recommendations...", this);
    m_recommendationLabel->setWordWrap(true);
    m_recommendationLabel->setStyleSheet("QLabel { color: #0066cc; }");
    
    recommendLayout->addWidget(m_recommendationLabel);
    
    layout->addWidget(recommendGroup);
    
    // Estimation group
    auto* estimateGroup = new QGroupBox("Size Estimation", this);
    auto* estimateLayout = new QGridLayout(estimateGroup);
    
    m_estimatedSizeLabel = new QLabel("Estimated Size: Calculating...", this);
    m_estimatedTimeLabel = new QLabel("Estimated Time: Calculating...", this);
    
    estimateLayout->addWidget(m_estimatedSizeLabel, 0, 0);
    estimateLayout->addWidget(m_estimatedTimeLabel, 1, 0);
    
    layout->addWidget(estimateGroup);
    
    // Progress and actions
    m_analysisProgress = new QProgressBar(this);
    m_analysisProgress->setRange(0, 0); // Indeterminate
    
    auto* actionLayout = new QHBoxLayout();
    m_analyzeBtn = new QPushButton("Re-analyze", this);
    m_analyzeBtn->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    
    m_previewBtn = new QPushButton("Preview Size", this);
    m_previewBtn->setIcon(style()->standardIcon(QStyle::SP_FileDialogListView));
    m_previewBtn->setEnabled(false);
    
    actionLayout->addWidget(m_analyzeBtn);
    actionLayout->addWidget(m_previewBtn);
    actionLayout->addStretch();
    
    layout->addWidget(m_analysisProgress);
    layout->addLayout(actionLayout);
    layout->addStretch();
    
    m_tabWidget->addTab(m_analysisTab, "Analysis");
}

void SmartCompressionDialog::connectSignals() {
    // Basic tab signals
    connect(m_browseBtn, &QPushButton::clicked, this, &SmartCompressionDialog::onOutputPathBrowse);
    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SmartCompressionDialog::onFormatChanged);
    connect(m_compressionSlider, &QSlider::valueChanged, this, &SmartCompressionDialog::onCompressionLevelChanged);
    connect(m_compressionSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            m_compressionSlider, &QSlider::setValue);
    connect(m_compressionSlider, &QSlider::valueChanged, m_compressionSpin, &QSpinBox::setValue);
    
    // Advanced tab signals
    connect(m_encryptCheck, &QCheckBox::toggled, m_passwordEdit, &QLineEdit::setEnabled);
    connect(m_encryptCheck, &QCheckBox::toggled, m_confirmPasswordEdit, &QLineEdit::setEnabled);
    
    // Analysis tab signals
    connect(m_analyzeBtn, &QPushButton::clicked, this, &SmartCompressionDialog::onAnalyzeFiles);
    connect(m_previewBtn, &QPushButton::clicked, this, &SmartCompressionDialog::onPreviewSize);
    
    // Dialog buttons
    connect(m_okBtn, &QPushButton::clicked, this, &SmartCompressionDialog::onAccept);
    connect(m_cancelBtn, &QPushButton::clicked, this, &SmartCompressionDialog::onReject);
    
    // Update format-specific options when format changes
    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SmartCompressionDialog::updateFormatSpecificOptions);
}

void SmartCompressionDialog::onFormatChanged() {
    QString format = m_formatCombo->currentData().toString();
    m_formatDescLabel->setText(getFormatDescription(format));
    updateFormatSpecificOptions();
}

void SmartCompressionDialog::onCompressionLevelChanged(int level) {
    QStringList descriptions = {
        "Store only (no compression)",
        "Fastest compression",
        "Fast compression", 
        "Good compression",
        "Better compression",
        "Good compression",
        "Balanced (recommended)",
        "Better compression",
        "High compression",
        "Maximum compression"
    };
    
    if (level >= 0 && level < descriptions.size()) {
        m_compressionDescLabel->setText(descriptions[level]);
    }
}

void SmartCompressionDialog::onOutputPathBrowse() {
    QString defaultPath = m_outputPathEdit->text();
    if (defaultPath.isEmpty()) {
        defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Choose Output Location",
        defaultPath,
        "Archive Files (*.zip *.7z *.tar.gz *.tar.bz2 *.tar.xz);;All Files (*)"
    );
    
    if (!fileName.isEmpty()) {
        m_outputPathEdit->setText(fileName);
    }
}

void SmartCompressionDialog::onAnalyzeFiles() {
    m_analysisProgress->setVisible(true);
    m_analyzeBtn->setEnabled(false);
    
    // Perform analysis in a separate thread (simplified here)
    analyzeInputFiles();
    updateRecommendations();
    
    m_analysisProgress->setVisible(false);
    m_analyzeBtn->setEnabled(true);
    m_previewBtn->setEnabled(true);
    m_analysisComplete = true;
}

void SmartCompressionDialog::onPreviewSize() {
    updateCompressionPreview();
}

void SmartCompressionDialog::onAccept() {
    // Validate settings
    if (m_outputPathEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Missing Output Path", 
                           "Please specify an output path for the archive.");
        m_tabWidget->setCurrentIndex(0); // Switch to basic tab
        m_outputPathEdit->setFocus();
        return;
    }
    
    if (m_encryptCheck->isChecked()) {
        if (m_passwordEdit->text().isEmpty()) {
            QMessageBox::warning(this, "Missing Password", 
                               "Please enter a password for encryption.");
            m_tabWidget->setCurrentIndex(1); // Switch to advanced tab
            m_passwordEdit->setFocus();
            return;
        }
        
        if (m_passwordEdit->text() != m_confirmPasswordEdit->text()) {
            QMessageBox::warning(this, "Password Mismatch", 
                               "The passwords do not match.");
            m_tabWidget->setCurrentIndex(1); // Switch to advanced tab
            m_confirmPasswordEdit->setFocus();
            return;
        }
    }
    
    // Collect settings
    m_currentSettings.format = m_formatCombo->currentData().toString();
    m_currentSettings.outputPath = m_outputPathEdit->text();
    m_currentSettings.compressionLevel = m_compressionSlider->value();
    m_currentSettings.createSolidArchive = m_solidArchiveCheck->isChecked();
    m_currentSettings.encryptArchive = m_encryptCheck->isChecked();
    m_currentSettings.password = m_passwordEdit->text();
    m_currentSettings.deleteOriginalFiles = m_deleteOriginalCheck->isChecked();
    m_currentSettings.createSeparateArchives = m_separateArchivesCheck->isChecked();
    m_currentSettings.compressionMethod = m_compressionMethodCombo->currentData().toString();
    m_currentSettings.dictionarySize = m_dictionarySizeSpin->value();
    m_currentSettings.wordSize = m_wordSizeSpin->value();
    m_currentSettings.enableMultiThreading = m_multiThreadCheck->isChecked();
    
    // Parse exclude patterns
    QString excludeText = m_excludePatternsEdit->toPlainText();
    m_currentSettings.excludePatterns = excludeText.split('\n', Qt::SkipEmptyParts);
    
    emit compressionRequested(m_currentSettings);
    accept();
}

void SmartCompressionDialog::onReject() {
    reject();
}

void SmartCompressionDialog::analyzeInputFiles() {
    m_analysis = analyzeFiles(m_inputPaths);
    
    // Update UI with analysis results
    m_totalSizeLabel->setText(QString("Total Size: %1").arg(formatFileSize(m_analysis.totalSize)));
    m_fileCountLabel->setText(QString("Files: %1 files, %2 folders")
                             .arg(m_analysis.totalFiles)
                             .arg(m_analysis.totalFolders));
    
    QString fileTypesText = "File Types: ";
    if (m_analysis.fileTypes.size() <= 5) {
        fileTypesText += m_analysis.fileTypes.join(", ");
    } else {
        fileTypesText += m_analysis.fileTypes.mid(0, 5).join(", ") + 
                        QString(" and %1 more").arg(m_analysis.fileTypes.size() - 5);
    }
    m_fileTypesLabel->setText(fileTypesText);
}

void SmartCompressionDialog::updateRecommendations() {
    QString optimalFormat = suggestOptimalFormat(m_analysis);
    int optimalLevel = suggestCompressionLevel(m_analysis, optimalFormat);
    
    // Update UI with recommendations
    for (int i = 0; i < m_formatCombo->count(); ++i) {
        if (m_formatCombo->itemData(i).toString() == optimalFormat) {
            m_formatCombo->setCurrentIndex(i);
            break;
        }
    }
    
    m_compressionSlider->setValue(optimalLevel);
    
    // Generate recommendation text
    QString recommendation = QString("Recommended: %1 format with compression level %2. ")
                           .arg(optimalFormat.toUpper())
                           .arg(optimalLevel);
    
    if (m_analysis.hasCompressedFiles) {
        recommendation += "Note: Some files are already compressed and may not benefit from additional compression. ";
    }
    
    if (m_analysis.totalSize > 1024 * 1024 * 1024) { // > 1GB
        recommendation += "Consider using 7Z format for better compression of large files. ";
    }
    
    if (m_analysis.hasTextFiles) {
        recommendation += "Text files detected - good compression ratio expected. ";
    }
    
    m_recommendationLabel->setText(recommendation);
    
    // Update estimated size
    qint64 estimatedSize = static_cast<qint64>(m_analysis.totalSize * (1.0 - m_analysis.estimatedCompressionRatio));
    m_estimatedSizeLabel->setText(QString("Estimated Size: %1 (%.1f%% reduction)")
                                 .arg(formatFileSize(estimatedSize))
                                 .arg(m_analysis.estimatedCompressionRatio * 100));
    
    // Estimate time (very rough)
    int estimatedSeconds = static_cast<int>(m_analysis.totalSize / (10 * 1024 * 1024)); // ~10MB/s
    if (estimatedSeconds < 60) {
        m_estimatedTimeLabel->setText(QString("Estimated Time: %1 seconds").arg(estimatedSeconds));
    } else {
        m_estimatedTimeLabel->setText(QString("Estimated Time: %1 minutes").arg(estimatedSeconds / 60));
    }
}

void SmartCompressionDialog::updateCompressionPreview() {
    // This would show a detailed preview of compression results
    QMessageBox::information(this, "Compression Preview", 
                           "Preview functionality would show detailed compression statistics here.");
}

void SmartCompressionDialog::updateFormatSpecificOptions() {
    QString format = m_formatCombo->currentData().toString();
    
    // Enable/disable options based on format
    bool supports7zFeatures = (format == "7z");
    bool supportsEncryption = (format == "7z" || format == "zip");
    
    m_solidArchiveCheck->setEnabled(supports7zFeatures);
    m_compressionMethodCombo->setEnabled(supports7zFeatures);
    m_dictionarySizeSpin->setEnabled(supports7zFeatures);
    m_wordSizeSpin->setEnabled(supports7zFeatures);
    m_encryptCheck->setEnabled(supportsEncryption);
    
    if (!supportsEncryption) {
        m_encryptCheck->setChecked(false);
    }
}

SmartCompressionDialog::FileAnalysis SmartCompressionDialog::analyzeFiles(const QStringList& paths) {
    FileAnalysis analysis;
    analysis.totalSize = 0;
    analysis.totalFiles = 0;
    analysis.totalFolders = 0;
    analysis.hasCompressedFiles = false;
    analysis.hasTextFiles = false;
    analysis.hasMediaFiles = false;
    analysis.estimatedCompressionRatio = 0.5; // Default 50%
    
    QMimeDatabase mimeDb;
    QSet<QString> uniqueTypes;
    
    for (const QString& path : paths) {
        QFileInfo info(path);
        
        if (info.isFile()) {
            analysis.totalFiles++;
            analysis.totalSize += info.size();
            
            // Analyze file type
            QMimeType mimeType = mimeDb.mimeTypeForFile(path);
            QString category = mimeType.name().split('/').first();
            uniqueTypes.insert(category);
            
            if (mimeType.name().startsWith("text/") || 
                mimeType.name().contains("xml") ||
                mimeType.name().contains("json")) {
                analysis.hasTextFiles = true;
            }
            
            if (mimeType.name().startsWith("image/") ||
                mimeType.name().startsWith("video/") ||
                mimeType.name().startsWith("audio/")) {
                analysis.hasMediaFiles = true;
            }
            
            QString suffix = info.suffix().toLower();
            if (suffix == "zip" || suffix == "7z" || suffix == "rar" ||
                suffix == "gz" || suffix == "bz2" || suffix == "xz") {
                analysis.hasCompressedFiles = true;
            }
            
            if (info.size() > 100 * 1024 * 1024) { // > 100MB
                analysis.largeFiles.append(path);
            }
        } else if (info.isDir()) {
            analysis.totalFolders++;
            
            // Recursively analyze directory
            QDirIterator it(path, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, 
                           QDirIterator::Subdirectories);
            while (it.hasNext()) {
                it.next();
                QFileInfo subInfo = it.fileInfo();
                if (subInfo.isFile()) {
                    analysis.totalFiles++;
                    analysis.totalSize += subInfo.size();
                } else if (subInfo.isDir()) {
                    analysis.totalFolders++;
                }
            }
        }
    }
    
    analysis.fileTypes = uniqueTypes.values();
    
    // Estimate compression ratio based on file types
    if (analysis.hasTextFiles && !analysis.hasMediaFiles) {
        analysis.estimatedCompressionRatio = 0.7; // 70% for text files
    } else if (analysis.hasMediaFiles && !analysis.hasTextFiles) {
        analysis.estimatedCompressionRatio = 0.1; // 10% for media files
    } else if (analysis.hasCompressedFiles) {
        analysis.estimatedCompressionRatio = 0.2; // 20% for already compressed
    }
    
    return analysis;
}

QString SmartCompressionDialog::suggestOptimalFormat(const FileAnalysis& analysis) {
    if (analysis.totalSize > 1024 * 1024 * 1024) { // > 1GB
        return "7z"; // Best compression for large files
    } else if (analysis.hasTextFiles && !analysis.hasMediaFiles) {
        return "tar.xz"; // Excellent for text files
    } else if (analysis.hasCompressedFiles) {
        return "zip"; // Don't over-compress
    } else {
        return "zip"; // Universal compatibility
    }
}

int SmartCompressionDialog::suggestCompressionLevel(const FileAnalysis& analysis, const QString& format) {
    Q_UNUSED(format)
    
    if (analysis.totalSize > 1024 * 1024 * 1024) { // > 1GB
        return 5; // Balanced for large files
    } else if (analysis.hasTextFiles) {
        return 7; // Higher compression for text
    } else if (analysis.hasMediaFiles) {
        return 3; // Lower compression for media
    } else {
        return 6; // Default balanced
    }
}

QString SmartCompressionDialog::formatFileSize(qint64 size) const {
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

QString SmartCompressionDialog::getFormatDescription(const QString& format) const {
    if (format == "zip") {
        return "Universal format supported by all operating systems. Good compression and fast.";
    } else if (format == "7z") {
        return "Excellent compression ratio with advanced features. Best for archiving.";
    } else if (format == "tar.gz") {
        return "Standard Unix/Linux format. Good compression, widely supported.";
    } else if (format == "tar.bz2") {
        return "Better compression than gzip, slower but smaller files.";
    } else if (format == "tar.xz") {
        return "Excellent compression ratio, modern and efficient.";
    } else {
        return "Archive format for compressed storage.";
    }
}

SmartCompressionDialog::CompressionSettings SmartCompressionDialog::getSettings() const {
    return m_currentSettings;
}

void SmartCompressionDialog::setDefaultOutputPath(const QString& path) {
    m_outputPathEdit->setText(path);
}

} // namespace FluxGUI::UI::Dialogs





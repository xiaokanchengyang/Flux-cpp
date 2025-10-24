#include "compression_widget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QSlider>
#include <QListWidget>
#include <QTreeWidget>
#include <QProgressBar>
#include <QFileDialog>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUrl>
#include <QDebug>

#include "../../core/archive/archive_manager.h"
#include "../../core/config/settings_manager.h"

namespace FluxGUI::UI::Widgets {

CompressionWidget::CompressionWidget(QWidget* parent)
    : QWidget(parent)
    , m_inputFiles()
    , m_outputPath()
    , m_fileListWidget(nullptr)
    , m_outputPathEdit(nullptr)
    , m_formatCombo(nullptr)
    , m_presetCombo(nullptr)
    , m_levelSlider(nullptr)
    , m_levelLabel(nullptr)
    , m_passwordEdit(nullptr)
    , m_solidArchiveCheckBox(nullptr)
    , m_deleteSourceCheckBox(nullptr)
    , m_progressBar(nullptr)
    , m_createButton(nullptr)
    , m_clearButton(nullptr)
    , m_currentOperation(nullptr)
{
    // Set object name for styling
    setObjectName("CompressionWidget");
    
    // Enable drag and drop
    setAcceptDrops(true);
    
    // Initialize UI
    initializeUI();
    
    // Connect signals
    connectSignals();
    
    // Load settings
    loadSettings();
    
    qDebug() << "CompressionWidget initialized";
}

CompressionWidget::~CompressionWidget() = default;

void CompressionWidget::setInputFiles(const QStringList& files) {
    m_inputFiles = files;
    updateFileList();
    updateOutputPath();
    updateUIState();
    
    qDebug() << "Set input files:" << files.size() << "items";
}

void CompressionWidget::addInputFiles(const QStringList& files) {
    for (const QString& file : files) {
        if (!m_inputFiles.contains(file)) {
            m_inputFiles.append(file);
        }
    }
    
    updateFileList();
    updateOutputPath();
    updateUIState();
    
    qDebug() << "Added input files, total:" << m_inputFiles.size() << "items";
}

void CompressionWidget::clearInputFiles() {
    m_inputFiles.clear();
    updateFileList();
    updateOutputPath();
    updateUIState();
    
    qDebug() << "Cleared input files";
}

QStringList CompressionWidget::inputFiles() const {
    return m_inputFiles;
}

void CompressionWidget::setOutputPath(const QString& path) {
    m_outputPath = path;
    if (m_outputPathEdit) {
        m_outputPathEdit->setText(path);
    }
    updateUIState();
}

QString CompressionWidget::outputPath() const {
    return m_outputPath;
}

void CompressionWidget::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void CompressionWidget::dropEvent(QDropEvent* event) {
    QStringList files;
    
    for (const QUrl& url : event->mimeData()->urls()) {
        if (url.isLocalFile()) {
            files.append(url.toLocalFile());
        }
    }
    
    if (!files.isEmpty()) {
        addInputFiles(files);
    }
    
    event->acceptProposedAction();
}

void CompressionWidget::initializeUI() {
    // Create main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);
    
    // Create sections
    createFileSelectionSection(mainLayout);
    createOutputSection(mainLayout);
    createCompressionSettingsSection(mainLayout);
    createAdvancedSettingsSection(mainLayout);
    createProgressSection(mainLayout);
    createActionSection(mainLayout);
    
    // Apply styling
    applyStyles();
}

void CompressionWidget::createFileSelectionSection(QVBoxLayout* layout) {
    // File selection group
    QGroupBox* fileGroup = new QGroupBox("Files to Compress", this);
    fileGroup->setObjectName("fileSelectionGroup");
    
    QVBoxLayout* fileLayout = new QVBoxLayout(fileGroup);
    fileLayout->setSpacing(10);
    
    // File list widget
    m_fileListWidget = new QListWidget(fileGroup);
    m_fileListWidget->setObjectName("fileListWidget");
    m_fileListWidget->setMinimumHeight(150);
    m_fileListWidget->setAlternatingRowColors(true);
    m_fileListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_fileListWidget->setDragDropMode(QAbstractItemView::DropOnly);
    fileLayout->addWidget(m_fileListWidget);
    
    // File action buttons
    QHBoxLayout* fileButtonLayout = new QHBoxLayout();
    
    QPushButton* addFilesButton = new QPushButton("Add Files...", fileGroup);
    addFilesButton->setIcon(QIcon(":/icons/add.svg"));
    connect(addFilesButton, &QPushButton::clicked, this, &CompressionWidget::addFiles);
    fileButtonLayout->addWidget(addFilesButton);
    
    QPushButton* addFolderButton = new QPushButton("Add Folder...", fileGroup);
    addFolderButton->setIcon(QIcon(":/icons/folder.svg"));
    connect(addFolderButton, &QPushButton::clicked, this, &CompressionWidget::addFolder);
    fileButtonLayout->addWidget(addFolderButton);
    
    QPushButton* removeButton = new QPushButton("Remove Selected", fileGroup);
    removeButton->setIcon(QIcon(":/icons/delete.svg"));
    connect(removeButton, &QPushButton::clicked, this, &CompressionWidget::removeSelected);
    fileButtonLayout->addWidget(removeButton);
    
    fileButtonLayout->addStretch();
    
    m_clearButton = new QPushButton("Clear All", fileGroup);
    m_clearButton->setIcon(QIcon(":/icons/close.svg"));
    connect(m_clearButton, &QPushButton::clicked, this, &CompressionWidget::clearInputFiles);
    fileButtonLayout->addWidget(m_clearButton);
    
    fileLayout->addLayout(fileButtonLayout);
    
    layout->addWidget(fileGroup);
}

void CompressionWidget::createOutputSection(QVBoxLayout* layout) {
    // Output group
    QGroupBox* outputGroup = new QGroupBox("Output Archive", this);
    outputGroup->setObjectName("outputGroup");
    
    QGridLayout* outputLayout = new QGridLayout(outputGroup);
    outputLayout->setSpacing(10);
    
    // Output path
    outputLayout->addWidget(new QLabel("Archive Path:", outputGroup), 0, 0);
    
    m_outputPathEdit = new QLineEdit(outputGroup);
    m_outputPathEdit->setObjectName("outputPathEdit");
    m_outputPathEdit->setPlaceholderText("Select output path for the archive...");
    outputLayout->addWidget(m_outputPathEdit, 0, 1);
    
    QPushButton* browseButton = new QPushButton("Browse...", outputGroup);
    browseButton->setIcon(QIcon(":/icons/folder.svg"));
    connect(browseButton, &QPushButton::clicked, this, &CompressionWidget::browseOutputPath);
    outputLayout->addWidget(browseButton, 0, 2);
    
    // Format selection
    outputLayout->addWidget(new QLabel("Format:", outputGroup), 1, 0);
    
    m_formatCombo = new QComboBox(outputGroup);
    m_formatCombo->setObjectName("formatCombo");
    populateFormatCombo();
    outputLayout->addWidget(m_formatCombo, 1, 1, 1, 2);
    
    layout->addWidget(outputGroup);
}

void CompressionWidget::createCompressionSettingsSection(QVBoxLayout* layout) {
    // Compression settings group
    QGroupBox* compressionGroup = new QGroupBox("Compression Settings", this);
    compressionGroup->setObjectName("compressionGroup");
    
    QGridLayout* compressionLayout = new QGridLayout(compressionGroup);
    compressionLayout->setSpacing(10);
    
    // Preset selection
    compressionLayout->addWidget(new QLabel("Preset:", compressionGroup), 0, 0);
    
    m_presetCombo = new QComboBox(compressionGroup);
    m_presetCombo->setObjectName("presetCombo");
    populatePresetCombo();
    compressionLayout->addWidget(m_presetCombo, 0, 1, 1, 2);
    
    // Compression level
    compressionLayout->addWidget(new QLabel("Level:", compressionGroup), 1, 0);
    
    m_levelSlider = new QSlider(Qt::Horizontal, compressionGroup);
    m_levelSlider->setObjectName("levelSlider");
    m_levelSlider->setRange(0, 9);
    m_levelSlider->setValue(6);
    m_levelSlider->setTickPosition(QSlider::TicksBelow);
    m_levelSlider->setTickInterval(1);
    compressionLayout->addWidget(m_levelSlider, 1, 1);
    
    m_levelLabel = new QLabel("6", compressionGroup);
    m_levelLabel->setObjectName("levelLabel");
    m_levelLabel->setMinimumWidth(20);
    compressionLayout->addWidget(m_levelLabel, 1, 2);
    
    layout->addWidget(compressionGroup);
}

void CompressionWidget::createAdvancedSettingsSection(QVBoxLayout* layout) {
    // Advanced settings group
    QGroupBox* advancedGroup = new QGroupBox("Advanced Settings", this);
    advancedGroup->setObjectName("advancedGroup");
    
    QVBoxLayout* advancedLayout = new QVBoxLayout(advancedGroup);
    advancedLayout->setSpacing(10);
    
    // Password protection
    QHBoxLayout* passwordLayout = new QHBoxLayout();
    passwordLayout->addWidget(new QLabel("Password:", advancedGroup));
    
    m_passwordEdit = new QLineEdit(advancedGroup);
    m_passwordEdit->setObjectName("passwordEdit");
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("Optional password protection...");
    passwordLayout->addWidget(m_passwordEdit);
    
    advancedLayout->addLayout(passwordLayout);
    
    // Options
    m_solidArchiveCheckBox = new QCheckBox("Solid archive (better compression)", advancedGroup);
    m_solidArchiveCheckBox->setObjectName("solidArchiveCheckBox");
    advancedLayout->addWidget(m_solidArchiveCheckBox);
    
    m_deleteSourceCheckBox = new QCheckBox("Delete source files after compression", advancedGroup);
    m_deleteSourceCheckBox->setObjectName("deleteSourceCheckBox");
    advancedLayout->addWidget(m_deleteSourceCheckBox);
    
    layout->addWidget(advancedGroup);
}

void CompressionWidget::createProgressSection(QVBoxLayout* layout) {
    // Progress group
    QGroupBox* progressGroup = new QGroupBox("Progress", this);
    progressGroup->setObjectName("progressGroup");
    progressGroup->setVisible(false);
    
    QVBoxLayout* progressLayout = new QVBoxLayout(progressGroup);
    progressLayout->setSpacing(10);
    
    m_progressBar = new QProgressBar(progressGroup);
    m_progressBar->setObjectName("compressionProgressBar");
    m_progressBar->setTextVisible(true);
    progressLayout->addWidget(m_progressBar);
    
    layout->addWidget(progressGroup);
}

void CompressionWidget::createActionSection(QVBoxLayout* layout) {
    // Action buttons
    QHBoxLayout* actionLayout = new QHBoxLayout();
    actionLayout->addStretch();
    
    QPushButton* cancelButton = new QPushButton("Cancel", this);
    cancelButton->setIcon(QIcon(":/icons/close.svg"));
    connect(cancelButton, &QPushButton::clicked, this, &CompressionWidget::cancelOperation);
    actionLayout->addWidget(cancelButton);
    
    m_createButton = new QPushButton("Create Archive", this);
    m_createButton->setObjectName("createButton");
    m_createButton->setProperty("class", "primary");
    m_createButton->setIcon(QIcon(":/icons/compress.svg"));
    connect(m_createButton, &QPushButton::clicked, this, &CompressionWidget::createArchive);
    actionLayout->addWidget(m_createButton);
    
    layout->addLayout(actionLayout);
}

void CompressionWidget::connectSignals() {
    // Output path edit
    connect(m_outputPathEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        m_outputPath = text;
        updateUIState();
    });
    
    // Format combo
    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CompressionWidget::onFormatChanged);
    
    // Preset combo
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CompressionWidget::onPresetChanged);
    
    // Level slider
    connect(m_levelSlider, &QSlider::valueChanged, this, [this](int value) {
        m_levelLabel->setText(QString::number(value));
    });
    
    // File list context menu
    m_fileListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_fileListWidget, &QListWidget::customContextMenuRequested,
            this, &CompressionWidget::onFileListContextMenu);
}

void CompressionWidget::loadSettings() {
    auto& settingsManager = Core::Config::SettingsManager::instance();
    
    // Load default format
    QString defaultFormat = settingsManager.value("compression/defaultFormat", "zip").toString();
    int formatIndex = m_formatCombo->findData(defaultFormat);
    if (formatIndex >= 0) {
        m_formatCombo->setCurrentIndex(formatIndex);
    }
    
    // Load default level
    int defaultLevel = settingsManager.value("compression/defaultLevel", 6).toInt();
    m_levelSlider->setValue(defaultLevel);
    
    // Load other settings
    m_solidArchiveCheckBox->setChecked(settingsManager.value("compression/solidArchive", false).toBool());
    m_deleteSourceCheckBox->setChecked(settingsManager.value("compression/deleteAfterCompression", false).toBool());
}

void CompressionWidget::saveSettings() {
    auto& settingsManager = Core::Config::SettingsManager::instance();
    
    // Save current settings
    settingsManager.setValue("compression/defaultFormat", m_formatCombo->currentData().toString());
    settingsManager.setValue("compression/defaultLevel", m_levelSlider->value());
    settingsManager.setValue("compression/solidArchive", m_solidArchiveCheckBox->isChecked());
    settingsManager.setValue("compression/deleteAfterCompression", m_deleteSourceCheckBox->isChecked());
}

void CompressionWidget::populateFormatCombo() {
    if (!m_formatCombo) {
        return;
    }
    
    m_formatCombo->clear();
    
    auto& archiveManager = Core::Archive::ArchiveManager::instance();
    QStringList formats = archiveManager.supportedFormats();
    
    for (const QString& format : formats) {
        Core::Archive::FormatInfo info = archiveManager.getFormatInfo(format);
        if (info.canCreate) {
            m_formatCombo->addItem(QString("%1 (%2)").arg(info.name, format.toUpper()), format);
        }
    }
}

void CompressionWidget::populatePresetCombo() {
    if (!m_presetCombo) {
        return;
    }
    
    m_presetCombo->clear();
    m_presetCombo->addItem("Custom", "custom");
    
    auto& archiveManager = Core::Archive::ArchiveManager::instance();
    QStringList presets = archiveManager.getCompressionPresets();
    
    for (const QString& presetId : presets) {
        Core::Archive::CompressionPreset preset = archiveManager.getCompressionPreset(presetId);
        m_presetCombo->addItem(preset.name, presetId);
    }
}

void CompressionWidget::updateFileList() {
    if (!m_fileListWidget) {
        return;
    }
    
    m_fileListWidget->clear();
    
    for (const QString& filePath : m_inputFiles) {
        QFileInfo fileInfo(filePath);
        QListWidgetItem* item = new QListWidgetItem();
        
        if (fileInfo.isDir()) {
            item->setIcon(QIcon(":/icons/folder.svg"));
            item->setText(QString("%1 (Folder)").arg(fileInfo.fileName()));
        } else {
            item->setIcon(QIcon(":/icons/file-generic.svg"));
            item->setText(QString("%1 (%2)").arg(fileInfo.fileName(), formatFileSize(fileInfo.size())));
        }
        
        item->setToolTip(filePath);
        item->setData(Qt::UserRole, filePath);
        
        m_fileListWidget->addItem(item);
    }
}

void CompressionWidget::updateOutputPath() {
    if (m_inputFiles.isEmpty() || !m_outputPathEdit) {
        return;
    }
    
    // Generate default output path
    QString basePath;
    if (m_inputFiles.size() == 1) {
        QFileInfo fileInfo(m_inputFiles.first());
        basePath = fileInfo.baseName();
    } else {
        basePath = "Archive";
    }
    
    QString format = m_formatCombo->currentData().toString();
    QString extension = getFormatExtension(format);
    
    QString outputDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString outputPath = QDir(outputDir).filePath(basePath + extension);
    
    // Make sure path is unique
    int counter = 1;
    while (QFileInfo::exists(outputPath)) {
        outputPath = QDir(outputDir).filePath(QString("%1_%2%3").arg(basePath).arg(counter).arg(extension));
        counter++;
    }
    
    setOutputPath(outputPath);
}

void CompressionWidget::updateUIState() {
    bool hasFiles = !m_inputFiles.isEmpty();
    bool hasOutputPath = !m_outputPath.isEmpty();
    bool canCreate = hasFiles && hasOutputPath && !m_currentOperation;
    
    // Update button states
    if (m_createButton) {
        m_createButton->setEnabled(canCreate);
    }
    
    if (m_clearButton) {
        m_clearButton->setEnabled(hasFiles);
    }
}

QString CompressionWidget::getFormatExtension(const QString& format) const {
    // Map format to default extension
    static QHash<QString, QString> formatExtensions = {
        {"zip", ".zip"},
        {"7z", ".7z"},
        {"tar", ".tar"},
        {"gz", ".tar.gz"},
        {"bz2", ".tar.bz2"},
        {"xz", ".tar.xz"}
    };
    
    return formatExtensions.value(format, ".zip");
}

QString CompressionWidget::formatFileSize(qint64 size) const {
    const qint64 KB = 1024;
    const qint64 MB = KB * 1024;
    const qint64 GB = MB * 1024;
    
    if (size >= GB) {
        return QString("%1 GB").arg(QString::number(size / (double)GB, 'f', 1));
    } else if (size >= MB) {
        return QString("%1 MB").arg(QString::number(size / (double)MB, 'f', 1));
    } else if (size >= KB) {
        return QString("%1 KB").arg(QString::number(size / (double)KB, 'f', 1));
    } else {
        return QString("%1 B").arg(size);
    }
}

void CompressionWidget::applyStyles() {
    // Apply custom styling through object names and properties
    // The actual styling is handled by the QSS files
    
    // Force style update
    style()->unpolish(this);
    style()->polish(this);
}

// Slot implementations
void CompressionWidget::addFiles() {
    QStringList files = QFileDialog::getOpenFileNames(this, "Add Files", QString(),
        "All Files (*)");
    
    if (!files.isEmpty()) {
        addInputFiles(files);
    }
}

void CompressionWidget::addFolder() {
    QString folder = QFileDialog::getExistingDirectory(this, "Add Folder");
    
    if (!folder.isEmpty()) {
        addInputFiles(QStringList() << folder);
    }
}

void CompressionWidget::removeSelected() {
    QList<QListWidgetItem*> selectedItems = m_fileListWidget->selectedItems();
    
    for (QListWidgetItem* item : selectedItems) {
        QString filePath = item->data(Qt::UserRole).toString();
        m_inputFiles.removeAll(filePath);
        delete item;
    }
    
    updateOutputPath();
    updateUIState();
}

void CompressionWidget::browseOutputPath() {
    QString format = m_formatCombo->currentData().toString();
    QString extension = getFormatExtension(format);
    QString filter = QString("Archive Files (*%1);;All Files (*)").arg(extension);
    
    QString filePath = QFileDialog::getSaveFileName(this, "Save Archive As", m_outputPath, filter);
    
    if (!filePath.isEmpty()) {
        setOutputPath(filePath);
    }
}

void CompressionWidget::createArchive() {
    if (m_inputFiles.isEmpty() || m_outputPath.isEmpty()) {
        return;
    }
    
    // Save current settings
    saveSettings();
    
    // Create compression options
    Core::Archive::ArchiveCreationOptions options;
    options.outputPath = m_outputPath;
    options.inputFiles = m_inputFiles;
    options.format = m_formatCombo->currentData().toString();
    options.compressionLevel = m_levelSlider->value();
    options.password = m_passwordEdit->text();
    options.solidArchive = m_solidArchiveCheckBox->isChecked();
    options.deleteAfterCompression = m_deleteSourceCheckBox->isChecked();
    
    // Create archive operation
    auto& archiveManager = Core::Archive::ArchiveManager::instance();
    m_currentOperation = archiveManager.createArchive(options);
    
    if (!m_currentOperation) {
        QMessageBox::warning(this, "Create Archive", "Failed to create compression operation");
        return;
    }
    
    // Connect operation signals
    connect(m_currentOperation, &Core::Archive::ArchiveOperation::started, this, [this]() {
        // Show progress
        QGroupBox* progressGroup = findChild<QGroupBox*>("progressGroup");
        if (progressGroup) {
            progressGroup->setVisible(true);
        }
        
        m_progressBar->setRange(0, 0); // Indeterminate progress
        updateUIState();
    });
    
    connect(m_currentOperation, &Core::Archive::ArchiveOperation::progress, this, [this](int value, int maximum) {
        m_progressBar->setRange(0, maximum);
        m_progressBar->setValue(value);
    });
    
    connect(m_currentOperation, &Core::Archive::ArchiveOperation::finished, this, [this]() {
        m_currentOperation = nullptr;
        
        // Hide progress
        QGroupBox* progressGroup = findChild<QGroupBox*>("progressGroup");
        if (progressGroup) {
            progressGroup->setVisible(false);
        }
        
        updateUIState();
        
        QMessageBox::information(this, "Create Archive", 
                               QString("Archive created successfully: %1").arg(m_outputPath));
        
        emit archiveCreated(m_outputPath);
    });
    
    connect(m_currentOperation, &Core::Archive::ArchiveOperation::error, this, [this](const QString& error) {
        m_currentOperation = nullptr;
        
        // Hide progress
        QGroupBox* progressGroup = findChild<QGroupBox*>("progressGroup");
        if (progressGroup) {
            progressGroup->setVisible(false);
        }
        
        updateUIState();
        
        QMessageBox::warning(this, "Create Archive", 
                           QString("Failed to create archive: %1").arg(error));
    });
    
    // Start compression operation
    auto operation = new Core::Archive::ArchiveOperation(this);
    operation->setOutputPath(m_outputPath);
    operation->setFilesToAdd(m_inputFiles);
    operation->setCompressionLevel(m_compressionLevel);
    
    // Show progress dialog
    QProgressDialog* progressDialog = new QProgressDialog("Creating archive...", "Cancel", 0, 100, this);
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->show();
    
    connect(operation, &Core::Archive::ArchiveOperation::progressChanged, 
            progressDialog, &QProgressDialog::setValue);
    connect(operation, &Core::Archive::ArchiveOperation::finished, this, [this, progressDialog]() {
        progressDialog->close();
        QMessageBox::information(this, "Success", "Archive created successfully!");
        // Clear the input files list
        m_inputFiles.clear();
        updateFileList();
    });
    connect(progressDialog, &QProgressDialog::canceled, operation, &Core::Archive::ArchiveOperation::cancel);
    
    // Start the operation
    operation->createArchive();
    qDebug() << "Creating archive:" << m_outputPath;
}

void CompressionWidget::cancelOperation() {
    if (m_currentOperation) {
        m_currentOperation->cancel();
    } else {
        emit cancelled();
    }
}

void CompressionWidget::onFormatChanged() {
    updateOutputPath();
    
    // Update compression level range based on format
    QString format = m_formatCombo->currentData().toString();
    auto& archiveManager = Core::Archive::ArchiveManager::instance();
    Core::Archive::FormatInfo info = archiveManager.getFormatInfo(format);
    
    if (!info.supportedLevels.isEmpty()) {
        int minLevel = *std::min_element(info.supportedLevels.begin(), info.supportedLevels.end());
        int maxLevel = *std::max_element(info.supportedLevels.begin(), info.supportedLevels.end());
        
        m_levelSlider->setRange(minLevel, maxLevel);
        m_levelSlider->setValue(info.defaultLevel);
    }
}

void CompressionWidget::onPresetChanged() {
    QString presetId = m_presetCombo->currentData().toString();
    
    if (presetId == "custom") {
        return; // Keep current settings for custom
    }
    
    auto& archiveManager = Core::Archive::ArchiveManager::instance();
    Core::Archive::CompressionPreset preset = archiveManager.getCompressionPreset(presetId);
    
    // Apply preset settings
    int formatIndex = m_formatCombo->findData(preset.format);
    if (formatIndex >= 0) {
        m_formatCombo->setCurrentIndex(formatIndex);
    }
    
    m_levelSlider->setValue(preset.level);
    m_solidArchiveCheckBox->setChecked(preset.solidArchive);
}

void CompressionWidget::onFileListContextMenu(const QPoint& position) {
    QListWidgetItem* item = m_fileListWidget->itemAt(position);
    
    QMenu contextMenu(this);
    
    if (item) {
        // Item-specific actions
        contextMenu.addAction("Remove from List", this, [this, item]() {
            int row = m_fileListWidget->row(item);
            if (row >= 0 && row < m_inputFiles.size()) {
                m_inputFiles.removeAt(row);
                updateFileList();
            }
        });
        contextMenu.addSeparator();
    }
    
    // General actions
    contextMenu.addAction("Add Files", this, &CompressionWidget::addFiles);
    contextMenu.addAction("Add Folder", this, &CompressionWidget::addFolder);
    contextMenu.addSeparator();
    contextMenu.addAction("Clear All", this, [this]() {
        m_inputFiles.clear();
        updateFileList();
    });
    
    contextMenu.exec(m_fileListWidget->mapToGlobal(position));
}

} // namespace FluxGUI::UI::Widgets

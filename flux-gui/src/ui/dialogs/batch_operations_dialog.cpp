#include "batch_operations_dialog.h"

#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUrl>
#include <QCloseEvent>
#include <QShowEvent>
#include <QHeaderView>
#include <QSplitter>
#include <QScrollArea>
#include <QButtonGroup>
#include <QElapsedTimer>
#include <QLocale>
#include <QMutexLocker>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QDebug>

namespace FluxGUI::UI::Dialogs {

BatchOperationsDialog::BatchOperationsDialog(QWidget* parent)
    : QDialog(parent)
    , m_progressTimer(new QTimer(this))
    , m_statisticsTimer(new QTimer(this))
{
    setWindowTitle("Batch Operations");
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);
    setMinimumSize(800, 600);
    resize(1000, 700);
    
    // Enable drag and drop
    setAcceptDrops(true);
    
    // Setup timers
    m_progressTimer->setInterval(PROGRESS_UPDATE_INTERVAL);
    m_progressTimer->setSingleShot(false);
    connect(m_progressTimer, &QTimer::timeout, this, &BatchOperationsDialog::onProgressTimerTimeout);
    
    m_statisticsTimer->setInterval(STATISTICS_UPDATE_INTERVAL);
    m_statisticsTimer->setSingleShot(false);
    connect(m_statisticsTimer, &QTimer::timeout, this, &BatchOperationsDialog::updateStatistics);
    
    initializeUI();
    connectSignals();
    updateButtonStates();
    
    // Load default settings
    m_settings.outputDirectory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
}

BatchOperationsDialog::~BatchOperationsDialog() {
    if (m_batchRunning) {
        cancelBatchOperation();
    }
    
    stopWorkerThread();
}

void BatchOperationsDialog::initializeUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);
    
    // Create tab widget
    m_tabWidget = new QTabWidget(this);
    
    createOperationTab();
    createSettingsTab();
    createItemsTab();
    createProgressTab();
    createLogTab();
    
    mainLayout->addWidget(m_tabWidget);
    
    // Create preset management
    createPresetManagement();
    mainLayout->addWidget(m_presetGroup);
    
    // Create control buttons
    createControlButtons();
    mainLayout->addLayout(m_buttonLayout);
    
    setupLayouts();
}

void BatchOperationsDialog::createOperationTab() {
    m_operationTab = new QWidget();
    m_tabWidget->addTab(m_operationTab, "Operation");
    
    QVBoxLayout* layout = new QVBoxLayout(m_operationTab);
    
    // Operation type selection
    QGroupBox* operationGroup = new QGroupBox("Operation Type", m_operationTab);
    QGridLayout* operationLayout = new QGridLayout(operationGroup);
    
    QLabel* typeLabel = new QLabel("Operation:", operationGroup);
    m_operationTypeCombo = new QComboBox(operationGroup);
    m_operationTypeCombo->addItems({
        "Compress", "Extract", "Validate", "Convert", 
        "Optimize", "Split", "Merge", "Encrypt", "Decrypt"
    });
    
    QLabel* formatLabel = new QLabel("Archive Format:", operationGroup);
    m_archiveFormatCombo = new QComboBox(operationGroup);
    m_archiveFormatCombo->addItems({
        "Auto-detect", "ZIP", "7-Zip", "TAR", "TAR.GZ", "TAR.BZ2", "TAR.XZ", "RAR"
    });
    
    QLabel* compressionLabel = new QLabel("Compression Level:", operationGroup);
    m_compressionLevelCombo = new QComboBox(operationGroup);
    m_compressionLevelCombo->addItems({
        "Store (0)", "Fastest (1)", "Fast (3)", "Normal (6)", "Maximum (9)", "Ultra (22)"
    });
    m_compressionLevelCombo->setCurrentIndex(3); // Normal
    
    operationLayout->addWidget(typeLabel, 0, 0);
    operationLayout->addWidget(m_operationTypeCombo, 0, 1);
    operationLayout->addWidget(formatLabel, 1, 0);
    operationLayout->addWidget(m_archiveFormatCombo, 1, 1);
    operationLayout->addWidget(compressionLabel, 2, 0);
    operationLayout->addWidget(m_compressionLevelCombo, 2, 1);
    
    layout->addWidget(operationGroup);
    layout->addStretch();
}

void BatchOperationsDialog::createSettingsTab() {
    m_settingsTab = new QWidget();
    m_tabWidget->addTab(m_settingsTab, "Settings");
    
    QScrollArea* scrollArea = new QScrollArea(m_settingsTab);
    QWidget* scrollWidget = new QWidget();
    QVBoxLayout* scrollLayout = new QVBoxLayout(scrollWidget);
    
    // Output settings
    QGroupBox* outputGroup = new QGroupBox("Output Settings", scrollWidget);
    QGridLayout* outputLayout = new QGridLayout(outputGroup);
    
    QLabel* outputDirLabel = new QLabel("Output Directory:", outputGroup);
    m_outputDirectoryEdit = new QLineEdit(outputGroup);
    m_outputDirectoryEdit->setText(m_settings.outputDirectory);
    m_browseOutputButton = new QPushButton("Browse...", outputGroup);
    
    QLabel* passwordLabel = new QLabel("Password:", outputGroup);
    m_passwordEdit = new QLineEdit(outputGroup);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    
    outputLayout->addWidget(outputDirLabel, 0, 0);
    outputLayout->addWidget(m_outputDirectoryEdit, 0, 1);
    outputLayout->addWidget(m_browseOutputButton, 0, 2);
    outputLayout->addWidget(passwordLabel, 1, 0);
    outputLayout->addWidget(m_passwordEdit, 1, 1, 1, 2);
    
    // File handling options
    QGroupBox* fileGroup = new QGroupBox("File Handling", scrollWidget);
    QVBoxLayout* fileLayout = new QVBoxLayout(fileGroup);
    
    m_createSubfoldersCheck = new QCheckBox("Create subfolders for each archive", fileGroup);
    m_createSubfoldersCheck->setChecked(m_settings.createSubfolders);
    
    m_overwriteExistingCheck = new QCheckBox("Overwrite existing files", fileGroup);
    m_overwriteExistingCheck->setChecked(m_settings.overwriteExisting);
    
    m_preserveTimestampsCheck = new QCheckBox("Preserve file timestamps", fileGroup);
    m_preserveTimestampsCheck->setChecked(m_settings.preserveTimestamps);
    
    m_preservePermissionsCheck = new QCheckBox("Preserve file permissions", fileGroup);
    m_preservePermissionsCheck->setChecked(m_settings.preservePermissions);
    
    m_deleteSourceCheck = new QCheckBox("Delete source files after operation", fileGroup);
    m_deleteSourceCheck->setChecked(m_settings.deleteSourceAfterOperation);
    
    m_validateAfterCheck = new QCheckBox("Validate archives after creation", fileGroup);
    m_validateAfterCheck->setChecked(m_settings.validateAfterOperation);
    
    fileLayout->addWidget(m_createSubfoldersCheck);
    fileLayout->addWidget(m_overwriteExistingCheck);
    fileLayout->addWidget(m_preserveTimestampsCheck);
    fileLayout->addWidget(m_preservePermissionsCheck);
    fileLayout->addWidget(m_deleteSourceCheck);
    fileLayout->addWidget(m_validateAfterCheck);
    
    // Performance settings
    QGroupBox* perfGroup = new QGroupBox("Performance", scrollWidget);
    QGridLayout* perfLayout = new QGridLayout(perfGroup);
    
    QLabel* concurrentLabel = new QLabel("Max Concurrent Operations:", perfGroup);
    m_maxConcurrentSpin = new QSpinBox(perfGroup);
    m_maxConcurrentSpin->setRange(1, 16);
    m_maxConcurrentSpin->setValue(m_settings.maxConcurrentOperations);
    
    QLabel* maxSizeLabel = new QLabel("Max Archive Size (GB):", perfGroup);
    m_maxArchiveSizeSpin = new QSpinBox(perfGroup);
    m_maxArchiveSizeSpin->setRange(0, 1000);
    m_maxArchiveSizeSpin->setValue(m_settings.maxArchiveSize / (1024 * 1024 * 1024));
    m_maxArchiveSizeSpin->setSpecialValueText("No limit");
    
    perfLayout->addWidget(concurrentLabel, 0, 0);
    perfLayout->addWidget(m_maxConcurrentSpin, 0, 1);
    perfLayout->addWidget(maxSizeLabel, 1, 0);
    perfLayout->addWidget(m_maxArchiveSizeSpin, 1, 1);
    
    // Filter patterns
    QGroupBox* filterGroup = new QGroupBox("Filter Patterns", scrollWidget);
    QGridLayout* filterLayout = new QGridLayout(filterGroup);
    
    QLabel* excludeLabel = new QLabel("Exclude Patterns:", filterGroup);
    m_excludePatternsEdit = new QTextEdit(filterGroup);
    m_excludePatternsEdit->setMaximumHeight(80);
    m_excludePatternsEdit->setPlainText("*.tmp\n*.log\n*~\n.DS_Store");
    
    QLabel* includeLabel = new QLabel("Include Patterns:", filterGroup);
    m_includePatternsEdit = new QTextEdit(filterGroup);
    m_includePatternsEdit->setMaximumHeight(80);
    m_includePatternsEdit->setPlainText("*");
    
    filterLayout->addWidget(excludeLabel, 0, 0);
    filterLayout->addWidget(m_excludePatternsEdit, 1, 0);
    filterLayout->addWidget(includeLabel, 0, 1);
    filterLayout->addWidget(m_includePatternsEdit, 1, 1);
    
    scrollLayout->addWidget(outputGroup);
    scrollLayout->addWidget(fileGroup);
    scrollLayout->addWidget(perfGroup);
    scrollLayout->addWidget(filterGroup);
    scrollLayout->addStretch();
    
    scrollArea->setWidget(scrollWidget);
    scrollArea->setWidgetResizable(true);
    
    QVBoxLayout* settingsLayout = new QVBoxLayout(m_settingsTab);
    settingsLayout->addWidget(scrollArea);
}

void BatchOperationsDialog::createItemsTab() {
    m_itemsTab = new QWidget();
    m_tabWidget->addTab(m_itemsTab, "Items");
    
    QVBoxLayout* layout = new QVBoxLayout(m_itemsTab);
    
    // Toolbar
    QHBoxLayout* toolbarLayout = new QHBoxLayout();
    
    m_addFilesButton = new QPushButton("Add Files...", m_itemsTab);
    m_addFoldersButton = new QPushButton("Add Folders...", m_itemsTab);
    m_removeItemsButton = new QPushButton("Remove Selected", m_itemsTab);
    m_clearAllButton = new QPushButton("Clear All", m_itemsTab);
    
    toolbarLayout->addWidget(m_addFilesButton);
    toolbarLayout->addWidget(m_addFoldersButton);
    toolbarLayout->addWidget(m_removeItemsButton);
    toolbarLayout->addWidget(m_clearAllButton);
    toolbarLayout->addStretch();
    
    // Status labels
    m_itemsCountLabel = new QLabel("Items: 0", m_itemsTab);
    m_totalSizeLabel = new QLabel("Total Size: 0 bytes", m_itemsTab);
    
    toolbarLayout->addWidget(m_itemsCountLabel);
    toolbarLayout->addWidget(m_totalSizeLabel);
    
    layout->addLayout(toolbarLayout);
    
    // Items tree
    m_itemsTree = new QTreeWidget(m_itemsTab);
    m_itemsTree->setHeaderLabels({
        "Source Path", "Target Path", "Archive Name", "Format", 
        "Source Size", "Estimated Output", "Status", "Progress"
    });
    m_itemsTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_itemsTree->setAlternatingRowColors(true);
    m_itemsTree->setSortingEnabled(true);
    
    // Set column widths
    QHeaderView* header = m_itemsTree->header();
    header->setStretchLastSection(false);
    header->resizeSection(0, 200); // Source Path
    header->resizeSection(1, 200); // Target Path
    header->resizeSection(2, 150); // Archive Name
    header->resizeSection(3, 80);  // Format
    header->resizeSection(4, 100); // Source Size
    header->resizeSection(5, 120); // Estimated Output
    header->resizeSection(6, 100); // Status
    header->resizeSection(7, 100); // Progress
    
    layout->addWidget(m_itemsTree);
}

void BatchOperationsDialog::createProgressTab() {
    m_progressTab = new QWidget();
    m_tabWidget->addTab(m_progressTab, "Progress");
    
    QVBoxLayout* layout = new QVBoxLayout(m_progressTab);
    
    // Overall progress
    QGroupBox* overallGroup = new QGroupBox("Overall Progress", m_progressTab);
    QVBoxLayout* overallLayout = new QVBoxLayout(overallGroup);
    
    m_overallProgressBar = new QProgressBar(overallGroup);
    m_overallProgressBar->setTextVisible(true);
    overallLayout->addWidget(m_overallProgressBar);
    
    // Current item progress
    QGroupBox* currentGroup = new QGroupBox("Current Item", m_progressTab);
    QVBoxLayout* currentLayout = new QVBoxLayout(currentGroup);
    
    m_currentItemLabel = new QLabel("No operation in progress", currentGroup);
    m_currentItemProgressBar = new QProgressBar(currentGroup);
    m_currentItemProgressBar->setTextVisible(true);
    
    currentLayout->addWidget(m_currentItemLabel);
    currentLayout->addWidget(m_currentItemProgressBar);
    
    // Statistics
    QGroupBox* statsGroup = new QGroupBox("Statistics", m_progressTab);
    QGridLayout* statsLayout = new QGridLayout(statsGroup);
    
    m_statisticsLabel = new QLabel("Ready to start", statsGroup);
    m_timeRemainingLabel = new QLabel("Time Remaining: --", statsGroup);
    m_speedLabel = new QLabel("Speed: --", statsGroup);
    
    statsLayout->addWidget(m_statisticsLabel, 0, 0, 1, 2);
    statsLayout->addWidget(m_timeRemainingLabel, 1, 0);
    statsLayout->addWidget(m_speedLabel, 1, 1);
    
    layout->addWidget(overallGroup);
    layout->addWidget(currentGroup);
    layout->addWidget(statsGroup);
    layout->addStretch();
}

void BatchOperationsDialog::createLogTab() {
    m_logTab = new QWidget();
    m_tabWidget->addTab(m_logTab, "Log");
    
    QVBoxLayout* layout = new QVBoxLayout(m_logTab);
    
    // Log toolbar
    QHBoxLayout* logToolbarLayout = new QHBoxLayout();
    
    m_clearLogButton = new QPushButton("Clear Log", m_logTab);
    m_saveLogButton = new QPushButton("Save Log...", m_logTab);
    
    logToolbarLayout->addWidget(m_clearLogButton);
    logToolbarLayout->addWidget(m_saveLogButton);
    logToolbarLayout->addStretch();
    
    layout->addLayout(logToolbarLayout);
    
    // Log text
    m_logTextEdit = new QTextEdit(m_logTab);
    m_logTextEdit->setReadOnly(true);
    m_logTextEdit->setFont(QFont("Consolas", 9));
    
    layout->addWidget(m_logTextEdit);
}

void BatchOperationsDialog::createPresetManagement() {
    m_presetGroup = new QGroupBox("Presets", this);
    QHBoxLayout* presetLayout = new QHBoxLayout(m_presetGroup);
    
    m_presetCombo = new QComboBox(m_presetGroup);
    m_presetCombo->setEditable(true);
    
    m_savePresetButton = new QPushButton("Save", m_presetGroup);
    m_loadPresetButton = new QPushButton("Load", m_presetGroup);
    m_deletePresetButton = new QPushButton("Delete", m_presetGroup);
    
    presetLayout->addWidget(new QLabel("Preset:", m_presetGroup));
    presetLayout->addWidget(m_presetCombo);
    presetLayout->addWidget(m_savePresetButton);
    presetLayout->addWidget(m_loadPresetButton);
    presetLayout->addWidget(m_deletePresetButton);
    presetLayout->addStretch();
    
    // Load available presets
    updatePresetList();
}

void BatchOperationsDialog::createControlButtons() {
    m_buttonLayout = new QHBoxLayout();
    
    m_startButton = new QPushButton("Start", this);
    m_startButton->setDefault(true);
    m_startButton->setMinimumWidth(80);
    
    m_pauseButton = new QPushButton("Pause", this);
    m_pauseButton->setMinimumWidth(80);
    m_pauseButton->setEnabled(false);
    
    m_cancelButton = new QPushButton("Cancel", this);
    m_cancelButton->setMinimumWidth(80);
    m_cancelButton->setEnabled(false);
    
    m_closeButton = new QPushButton("Close", this);
    m_closeButton->setMinimumWidth(80);
    
    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_startButton);
    m_buttonLayout->addWidget(m_pauseButton);
    m_buttonLayout->addWidget(m_cancelButton);
    m_buttonLayout->addWidget(m_closeButton);
}

void BatchOperationsDialog::setupLayouts() {
    // Additional layout setup if needed
}

void BatchOperationsDialog::connectSignals() {
    // Operation type changes
    connect(m_operationTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BatchOperationsDialog::onOperationTypeChanged);
    
    // Settings changes
    connect(m_outputDirectoryEdit, &QLineEdit::textChanged, this, &BatchOperationsDialog::onSettingsChanged);
    connect(m_browseOutputButton, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Output Directory", m_outputDirectoryEdit->text());
        if (!dir.isEmpty()) {
            m_outputDirectoryEdit->setText(dir);
        }
    });
    
    // Item management
    connect(m_addFilesButton, &QPushButton::clicked, this, &BatchOperationsDialog::onAddFilesClicked);
    connect(m_addFoldersButton, &QPushButton::clicked, this, &BatchOperationsDialog::onAddFoldersClicked);
    connect(m_removeItemsButton, &QPushButton::clicked, this, &BatchOperationsDialog::onRemoveItemsClicked);
    connect(m_clearAllButton, &QPushButton::clicked, this, &BatchOperationsDialog::onClearAllClicked);
    
    connect(m_itemsTree, &QTreeWidget::itemSelectionChanged, this, &BatchOperationsDialog::onItemSelectionChanged);
    connect(m_itemsTree, &QTreeWidget::itemDoubleClicked, this, &BatchOperationsDialog::onItemDoubleClicked);
    
    // Control buttons
    connect(m_startButton, &QPushButton::clicked, this, &BatchOperationsDialog::onStartClicked);
    connect(m_pauseButton, &QPushButton::clicked, this, &BatchOperationsDialog::onPauseClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &BatchOperationsDialog::onCancelClicked);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::close);
    
    // Preset management
    connect(m_savePresetButton, &QPushButton::clicked, this, &BatchOperationsDialog::onPresetSaveClicked);
    connect(m_loadPresetButton, &QPushButton::clicked, this, &BatchOperationsDialog::onPresetLoadClicked);
    connect(m_deletePresetButton, &QPushButton::clicked, this, &BatchOperationsDialog::onPresetDeleteClicked);
    
    // Log management
    connect(m_clearLogButton, &QPushButton::clicked, m_logTextEdit, &QTextEdit::clear);
    connect(m_saveLogButton, &QPushButton::clicked, this, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this, "Save Log", "batch_operations.log", "Log Files (*.log)");
        if (!fileName.isEmpty()) {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream stream(&file);
                stream << m_logTextEdit->toPlainText();
            }
        }
    });
}

void BatchOperationsDialog::setOperationType(OperationType type) {
    m_settings.operation = type;
    m_operationTypeCombo->setCurrentIndex(static_cast<int>(type));
    updateOperationUI();
}

void BatchOperationsDialog::setOperationSettings(const OperationSettings& settings) {
    m_settings = settings;
    updateSettingsUI();
}

void BatchOperationsDialog::addBatchItem(const BatchItem& item) {
    m_batchItems.append(item);
    updateItemsList();
    updateItemEstimates();
}

void BatchOperationsDialog::addBatchItems(const QList<BatchItem>& items) {
    m_batchItems.append(items);
    updateItemsList();
    updateItemEstimates();
}

void BatchOperationsDialog::removeBatchItem(int index) {
    if (index >= 0 && index < m_batchItems.size()) {
        m_batchItems.removeAt(index);
        updateItemsList();
        updateItemEstimates();
    }
}

void BatchOperationsDialog::clearBatchItems() {
    m_batchItems.clear();
    updateItemsList();
    updateItemEstimates();
}

void BatchOperationsDialog::startBatchOperation() {
    if (m_batchRunning || m_batchItems.isEmpty()) {
        return;
    }
    
    // Validate settings and items
    if (!validateSettings() || !validateBatchItems()) {
        return;
    }
    
    // Update settings from UI
    updateSettingsFromUI();
    
    // Initialize statistics
    m_statistics = BatchStatistics();
    m_statistics.totalItems = m_batchItems.size();
    m_statistics.startTime = QDateTime::currentDateTime();
    
    for (const auto& item : m_batchItems) {
        m_statistics.totalSourceSize += item.sourceSize;
    }
    
    // Start worker thread
    startWorkerThread();
    
    // Update UI state
    m_batchRunning = true;
    m_batchPaused = false;
    m_currentItemIndex = 0;
    
    updateButtonStates();
    m_progressTimer->start();
    m_statisticsTimer->start();
    
    // Switch to progress tab
    m_tabWidget->setCurrentWidget(m_progressTab);
    
    logMessage("Batch operation started", "INFO");
    emit batchStarted();
}

void BatchOperationsDialog::pauseBatchOperation() {
    if (!m_batchRunning || m_batchPaused) {
        return;
    }
    
    m_batchPaused = true;
    
    if (m_worker) {
        m_worker->pauseBatch();
    }
    
    updateButtonStates();
    logMessage("Batch operation paused", "INFO");
    emit batchPaused();
}

void BatchOperationsDialog::resumeBatchOperation() {
    if (!m_batchRunning || !m_batchPaused) {
        return;
    }
    
    m_batchPaused = false;
    
    if (m_worker) {
        m_worker->resumeBatch();
    }
    
    updateButtonStates();
    logMessage("Batch operation resumed", "INFO");
    emit batchResumed();
}

void BatchOperationsDialog::cancelBatchOperation() {
    if (!m_batchRunning) {
        return;
    }
    
    // Stop worker
    if (m_worker) {
        m_worker->cancelBatch();
    }
    
    stopWorkerThread();
    
    // Update UI state
    m_batchRunning = false;
    m_batchPaused = false;
    
    m_progressTimer->stop();
    m_statisticsTimer->stop();
    
    updateButtonStates();
    updateProgressDisplay();
    
    logMessage("Batch operation cancelled", "WARNING");
    emit batchCancelled();
}

bool BatchOperationsDialog::isBatchRunning() const {
    return m_batchRunning;
}

BatchOperationsDialog::BatchStatistics BatchOperationsDialog::batchStatistics() const {
    return m_statistics;
}

void BatchOperationsDialog::savePreset(const QString& name) {
    if (name.isEmpty()) {
        return;
    }
    
    saveSettingsToPreset(name);
    updatePresetList();
    m_presetCombo->setCurrentText(name);
    
    logMessage(QString("Preset '%1' saved").arg(name), "INFO");
}

void BatchOperationsDialog::loadPreset(const QString& name) {
    if (name.isEmpty()) {
        return;
    }
    
    loadSettingsFromPreset(name);
    updateSettingsUI();
    
    logMessage(QString("Preset '%1' loaded").arg(name), "INFO");
}

QStringList BatchOperationsDialog::availablePresets() const {
    QDir presetDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/presets");
    QStringList presets;
    
    if (presetDir.exists()) {
        QStringList files = presetDir.entryList({"*.json"}, QDir::Files);
        for (const QString& file : files) {
            presets.append(QFileInfo(file).baseName());
        }
    }
    
    return presets;
}

void BatchOperationsDialog::deletePreset(const QString& name) {
    if (name.isEmpty()) {
        return;
    }
    
    QString filePath = getPresetFilePath(name);
    if (QFile::exists(filePath)) {
        QFile::remove(filePath);
        updatePresetList();
        logMessage(QString("Preset '%1' deleted").arg(name), "INFO");
    }
}

// Protected event handlers
void BatchOperationsDialog::closeEvent(QCloseEvent* event) {
    if (m_batchRunning) {
        int ret = QMessageBox::question(this, "Batch Operation Running",
                                      "A batch operation is currently running. Do you want to cancel it and close?",
                                      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        
        if (ret == QMessageBox::Yes) {
            cancelBatchOperation();
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        event->accept();
    }
}

void BatchOperationsDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    updatePresetList();
}

// Private slot implementations
void BatchOperationsDialog::onOperationTypeChanged() {
    m_settings.operation = static_cast<OperationType>(m_operationTypeCombo->currentIndex());
    updateOperationUI();
}

void BatchOperationsDialog::onSettingsChanged() {
    updateSettingsFromUI();
}

void BatchOperationsDialog::onAddFilesClicked() {
    QStringList files = QFileDialog::getOpenFileNames(this, "Select Files to Add");
    if (!files.isEmpty()) {
        addFilesToBatch(files);
    }
}

void BatchOperationsDialog::onAddFoldersClicked() {
    QString folder = QFileDialog::getExistingDirectory(this, "Select Folder to Add");
    if (!folder.isEmpty()) {
        addFoldersToBatch({folder});
    }
}

void BatchOperationsDialog::onRemoveItemsClicked() {
    QList<QTreeWidgetItem*> selected = m_itemsTree->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    
    // Remove items in reverse order to maintain indices
    QList<int> indices;
    for (QTreeWidgetItem* item : selected) {
        int index = m_itemsTree->indexOfTopLevelItem(item);
        if (index >= 0) {
            indices.append(index);
        }
    }
    
    std::sort(indices.rbegin(), indices.rend());
    
    for (int index : indices) {
        removeBatchItem(index);
    }
}

void BatchOperationsDialog::onClearAllClicked() {
    if (m_batchItems.isEmpty()) {
        return;
    }
    
    int ret = QMessageBox::question(this, "Clear All Items",
                                  "Are you sure you want to remove all items from the batch?",
                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        clearBatchItems();
    }
}

void BatchOperationsDialog::onStartClicked() {
    if (m_batchPaused) {
        resumeBatchOperation();
    } else {
        startBatchOperation();
    }
}

void BatchOperationsDialog::onPauseClicked() {
    pauseBatchOperation();
}

void BatchOperationsDialog::onCancelClicked() {
    cancelBatchOperation();
}

void BatchOperationsDialog::onPresetSaveClicked() {
    QString name = m_presetCombo->currentText().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Save Preset", "Please enter a preset name.");
        return;
    }
    
    savePreset(name);
}

void BatchOperationsDialog::onPresetLoadClicked() {
    QString name = m_presetCombo->currentText();
    if (name.isEmpty()) {
        return;
    }
    
    loadPreset(name);
}

void BatchOperationsDialog::onPresetDeleteClicked() {
    QString name = m_presetCombo->currentText();
    if (name.isEmpty()) {
        return;
    }
    
    int ret = QMessageBox::question(this, "Delete Preset",
                                  QString("Are you sure you want to delete the preset '%1'?").arg(name),
                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        deletePreset(name);
    }
}

void BatchOperationsDialog::onItemSelectionChanged() {
    bool hasSelection = !m_itemsTree->selectedItems().isEmpty();
    m_removeItemsButton->setEnabled(hasSelection && !m_batchRunning);
}

void BatchOperationsDialog::onItemDoubleClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column)
    
    if (!item) {
        return;
    }
    
    int index = m_itemsTree->indexOfTopLevelItem(item);
    if (index >= 0 && index < m_batchItems.size()) {
        const BatchItem& batchItem = m_batchItems[index];
        
        // Show item details or open file location
        QString message = QString("Source: %1\nTarget: %2\nSize: %3\nStatus: %4")
                         .arg(batchItem.sourcePath)
                         .arg(batchItem.targetPath)
                         .arg(formatSize(batchItem.sourceSize))
                         .arg(batchItem.status);
        
        QMessageBox::information(this, "Item Details", message);
    }
}

void BatchOperationsDialog::onProgressTimerTimeout() {
    updateProgressDisplay();
}

void BatchOperationsDialog::onWorkerFinished() {
    stopWorkerThread();
    
    m_batchRunning = false;
    m_batchPaused = false;
    
    m_progressTimer->stop();
    m_statisticsTimer->stop();
    
    updateButtonStates();
    updateProgressDisplay();
    
    // Calculate final statistics
    calculateStatistics();
    
    bool success = m_statistics.failedItems == 0;
    
    QString message = QString("Batch operation completed.\n\nProcessed: %1/%2 items\nFailed: %3\nTotal time: %4")
                     .arg(m_statistics.completedItems)
                     .arg(m_statistics.totalItems)
                     .arg(m_statistics.failedItems)
                     .arg(formatTime(m_statistics.elapsedTime));
    
    if (success) {
        logMessage("Batch operation completed successfully", "INFO");
        QMessageBox::information(this, "Batch Complete", message);
    } else {
        logMessage("Batch operation completed with errors", "WARNING");
        QMessageBox::warning(this, "Batch Complete", message);
    }
    
    emit batchCompleted(success);
}

void BatchOperationsDialog::onWorkerError(const QString& error) {
    logError(error);
    emit criticalError(error);
}

void BatchOperationsDialog::onWorkerProgress(int index, double progress) {
    if (index >= 0 && index < m_batchItems.size()) {
        m_batchItems[index].progress = progress;
        
        // Update tree item
        QTreeWidgetItem* item = m_itemsTree->topLevelItem(index);
        if (item) {
            item->setText(7, QString("%1%").arg(QString::number(progress, 'f', 1)));
        }
        
        emit itemProgress(index, progress);
    }
}

// Private implementation methods
void BatchOperationsDialog::updateOperationUI() {
    // Update UI based on operation type
    bool needsFormat = (m_settings.operation == OperationType::Compress || 
                       m_settings.operation == OperationType::Convert);
    m_archiveFormatCombo->setEnabled(needsFormat);
    
    bool needsCompression = (m_settings.operation == OperationType::Compress ||
                           m_settings.operation == OperationType::Optimize);
    m_compressionLevelCombo->setEnabled(needsCompression);
}

void BatchOperationsDialog::updateSettingsUI() {
    m_operationTypeCombo->setCurrentIndex(static_cast<int>(m_settings.operation));
    m_archiveFormatCombo->setCurrentIndex(static_cast<int>(m_settings.format));
    m_compressionLevelCombo->setCurrentIndex(static_cast<int>(m_settings.compressionLevel));
    
    m_outputDirectoryEdit->setText(m_settings.outputDirectory);
    m_passwordEdit->setText(m_settings.archivePassword);
    
    m_createSubfoldersCheck->setChecked(m_settings.createSubfolders);
    m_overwriteExistingCheck->setChecked(m_settings.overwriteExisting);
    m_preserveTimestampsCheck->setChecked(m_settings.preserveTimestamps);
    m_preservePermissionsCheck->setChecked(m_settings.preservePermissions);
    m_deleteSourceCheck->setChecked(m_settings.deleteSourceAfterOperation);
    m_validateAfterCheck->setChecked(m_settings.validateAfterOperation);
    
    m_maxConcurrentSpin->setValue(m_settings.maxConcurrentOperations);
    m_maxArchiveSizeSpin->setValue(m_settings.maxArchiveSize / (1024 * 1024 * 1024));
    
    m_excludePatternsEdit->setPlainText(m_settings.excludePatterns.join('\n'));
    m_includePatternsEdit->setPlainText(m_settings.includePatterns.join('\n'));
}

void BatchOperationsDialog::updateSettingsFromUI() {
    m_settings.operation = static_cast<OperationType>(m_operationTypeCombo->currentIndex());
    m_settings.format = static_cast<ArchiveFormat>(m_archiveFormatCombo->currentIndex());
    m_settings.compressionLevel = static_cast<CompressionLevel>(m_compressionLevelCombo->currentIndex());
    
    m_settings.outputDirectory = m_outputDirectoryEdit->text();
    m_settings.archivePassword = m_passwordEdit->text();
    
    m_settings.createSubfolders = m_createSubfoldersCheck->isChecked();
    m_settings.overwriteExisting = m_overwriteExistingCheck->isChecked();
    m_settings.preserveTimestamps = m_preserveTimestampsCheck->isChecked();
    m_settings.preservePermissions = m_preservePermissionsCheck->isChecked();
    m_settings.deleteSourceAfterOperation = m_deleteSourceCheck->isChecked();
    m_settings.validateAfterOperation = m_validateAfterCheck->isChecked();
    
    m_settings.maxConcurrentOperations = m_maxConcurrentSpin->value();
    m_settings.maxArchiveSize = static_cast<qint64>(m_maxArchiveSizeSpin->value()) * 1024 * 1024 * 1024;
    
    m_settings.excludePatterns = m_excludePatternsEdit->toPlainText().split('\n', Qt::SkipEmptyParts);
    m_settings.includePatterns = m_includePatternsEdit->toPlainText().split('\n', Qt::SkipEmptyParts);
}

void BatchOperationsDialog::updateItemsList() {
    m_itemsTree->clear();
    
    for (int i = 0; i < m_batchItems.size(); ++i) {
        const BatchItem& item = m_batchItems[i];
        
        QTreeWidgetItem* treeItem = new QTreeWidgetItem(m_itemsTree);
        treeItem->setText(0, item.sourcePath);
        treeItem->setText(1, item.targetPath);
        treeItem->setText(2, item.archiveName);
        treeItem->setText(3, formatToString(item.format));
        treeItem->setText(4, formatSize(item.sourceSize));
        treeItem->setText(5, formatSize(item.estimatedOutputSize));
        treeItem->setText(6, item.status);
        treeItem->setText(7, QString("%1%").arg(QString::number(item.progress, 'f', 1)));
        
        // Set item colors based on status
        if (item.hasError) {
            treeItem->setForeground(6, QBrush(Qt::red));
        } else if (item.completed) {
            treeItem->setForeground(6, QBrush(Qt::darkGreen));
        }
    }
    
    // Update counts
    m_itemsCountLabel->setText(QString("Items: %1").arg(m_batchItems.size()));
}

void BatchOperationsDialog::updateProgressDisplay() {
    if (!m_batchRunning) {
        m_overallProgressBar->setValue(0);
        m_currentItemProgressBar->setValue(0);
        m_currentItemLabel->setText("No operation in progress");
        return;
    }
    
    // Update overall progress
    int completedItems = 0;
    for (const auto& item : m_batchItems) {
        if (item.completed) {
            completedItems++;
        }
    }
    
    int overallProgress = m_batchItems.isEmpty() ? 0 : (completedItems * 100) / m_batchItems.size();
    m_overallProgressBar->setValue(overallProgress);
    m_overallProgressBar->setFormat(QString("%1/%2 items (%p%)").arg(completedItems).arg(m_batchItems.size()));
    
    // Update current item progress
    if (m_currentItemIndex >= 0 && m_currentItemIndex < m_batchItems.size()) {
        const BatchItem& currentItem = m_batchItems[m_currentItemIndex];
        m_currentItemLabel->setText(QString("Processing: %1").arg(QFileInfo(currentItem.sourcePath).fileName()));
        m_currentItemProgressBar->setValue(static_cast<int>(currentItem.progress));
    }
}

void BatchOperationsDialog::updateStatistics() {
    calculateStatistics();
    
    QString statsText = QString("Completed: %1/%2 | Failed: %3 | Processed: %4")
                       .arg(m_statistics.completedItems)
                       .arg(m_statistics.totalItems)
                       .arg(m_statistics.failedItems)
                       .arg(formatSize(m_statistics.processedSize));
    
    m_statisticsLabel->setText(statsText);
    m_timeRemainingLabel->setText(QString("Time Remaining: %1").arg(formatTime(m_statistics.remainingTime)));
    m_speedLabel->setText(QString("Speed: %1/s").arg(formatSpeed(m_statistics.averageSpeed)));
}

void BatchOperationsDialog::updateButtonStates() {
    bool canStart = !m_batchRunning && !m_batchItems.isEmpty();
    bool canPause = m_batchRunning && !m_batchPaused;
    bool canResume = m_batchRunning && m_batchPaused;
    bool canCancel = m_batchRunning;
    
    m_startButton->setEnabled(canStart || canResume);
    m_startButton->setText(canResume ? "Resume" : "Start");
    
    m_pauseButton->setEnabled(canPause);
    m_cancelButton->setEnabled(canCancel);
    
    // Disable item management during operation
    m_addFilesButton->setEnabled(!m_batchRunning);
    m_addFoldersButton->setEnabled(!m_batchRunning);
    m_removeItemsButton->setEnabled(!m_batchRunning && !m_itemsTree->selectedItems().isEmpty());
    m_clearAllButton->setEnabled(!m_batchRunning && !m_batchItems.isEmpty());
}

void BatchOperationsDialog::validateSettings() {
    // Validate output directory
    if (!validateOutputDirectory()) {
        throw std::runtime_error("Invalid output directory");
    }
    
    // Validate other settings
    if (!validateSettings()) {
        throw std::runtime_error("Invalid settings");
    }
}

void BatchOperationsDialog::prepareOperation() {
    // Prepare batch items and settings for operation
    updateItemEstimates();
}

void BatchOperationsDialog::startWorkerThread() {
    if (m_workerThread && m_workerThread->isRunning()) {
        stopWorkerThread();
    }
    
    m_workerThread = new QThread(this);
    m_worker = new BatchWorker();
    m_worker->moveToThread(m_workerThread);
    
    m_worker->setBatchItems(m_batchItems);
    m_worker->setOperationSettings(m_settings);
    
    // Connect worker signals
    connect(m_worker, &BatchWorker::itemStarted, this, [this](int index) {
        m_currentItemIndex = index;
        if (index >= 0 && index < m_batchItems.size()) {
            m_batchItems[index].status = "Processing...";
            updateItemsList();
            emit itemStarted(index, m_batchItems[index]);
        }
    });
    
    connect(m_worker, &BatchWorker::itemProgress, this, &BatchOperationsDialog::onWorkerProgress);
    
    connect(m_worker, &BatchWorker::itemCompleted, this, [this](int index, bool success, const QString& message) {
        if (index >= 0 && index < m_batchItems.size()) {
            m_batchItems[index].completed = true;
            m_batchItems[index].hasError = !success;
            m_batchItems[index].status = success ? "Completed" : "Failed";
            m_batchItems[index].errorMessage = success ? QString() : message;
            m_batchItems[index].progress = 100.0;
            
            updateItemsList();
            
            if (success) {
                m_statistics.completedItems++;
            } else {
                m_statistics.failedItems++;
                logError(QString("Item %1 failed: %2").arg(index).arg(message));
            }
            
            emit itemCompleted(index, success, message);
        }
    });
    
    connect(m_worker, &BatchWorker::batchCompleted, this, &BatchOperationsDialog::onWorkerFinished);
    connect(m_worker, &BatchWorker::error, this, &BatchOperationsDialog::onWorkerError);
    
    connect(m_workerThread, &QThread::started, m_worker, &BatchWorker::startBatch);
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    
    m_workerThread->start();
}

void BatchOperationsDialog::stopWorkerThread() {
    if (m_worker) {
        m_worker->cancelBatch();
    }
    
    if (m_workerThread && m_workerThread->isRunning()) {
        m_workerThread->quit();
        if (!m_workerThread->wait(5000)) {
            m_workerThread->terminate();
            m_workerThread->wait();
        }
    }
    
    m_worker = nullptr;
    m_workerThread = nullptr;
}

void BatchOperationsDialog::addFilesToBatch(const QStringList& files) {
    for (const QString& file : files) {
        BatchItem item = createBatchItem(file);
        m_batchItems.append(item);
    }
    
    updateItemsList();
    updateItemEstimates();
}

void BatchOperationsDialog::addFoldersToBatch(const QStringList& folders) {
    for (const QString& folder : folders) {
        BatchItem item = createBatchItem(folder);
        m_batchItems.append(item);
    }
    
    updateItemsList();
    updateItemEstimates();
}

void BatchOperationsDialog::updateItemEstimates() {
    qint64 totalSize = 0;
    qint64 totalEstimated = 0;
    
    for (auto& item : m_batchItems) {
        // Calculate source size if not set
        if (item.sourceSize == 0) {
            QFileInfo info(item.sourcePath);
            if (info.isFile()) {
                item.sourceSize = info.size();
            } else if (info.isDir()) {
                // Calculate directory size (simplified)
                QDir dir(item.sourcePath);
                item.sourceSize = calculateDirectorySize(dir);
            }
        }
        
        // Estimate output size based on compression
        if (item.estimatedOutputSize == 0) {
            double compressionRatio = 0.7; // Default estimate
            switch (m_settings.compressionLevel) {
            case CompressionLevel::Store:
                compressionRatio = 1.0;
                break;
            case CompressionLevel::Fastest:
                compressionRatio = 0.9;
                break;
            case CompressionLevel::Fast:
                compressionRatio = 0.8;
                break;
            case CompressionLevel::Normal:
                compressionRatio = 0.7;
                break;
            case CompressionLevel::Maximum:
                compressionRatio = 0.6;
                break;
            case CompressionLevel::Ultra:
                compressionRatio = 0.5;
                break;
            }
            
            item.estimatedOutputSize = static_cast<qint64>(item.sourceSize * compressionRatio);
        }
        
        totalSize += item.sourceSize;
        totalEstimated += item.estimatedOutputSize;
    }
    
    m_totalSizeLabel->setText(QString("Total Size: %1 (estimated output: %2)")
                             .arg(formatSize(totalSize))
                             .arg(formatSize(totalEstimated)));
}

BatchOperationsDialog::BatchItem BatchOperationsDialog::createBatchItem(const QString& sourcePath) const {
    BatchItem item;
    item.sourcePath = sourcePath;
    item.createdTime = QDateTime::currentDateTime();
    item.status = "Ready";
    
    QFileInfo info(sourcePath);
    
    // Generate target path
    QString baseName = info.baseName();
    QString extension = getArchiveExtension(m_settings.format);
    
    if (m_settings.createSubfolders) {
        item.targetPath = QDir(m_settings.outputDirectory).filePath(baseName);
        QDir().mkpath(item.targetPath);
    } else {
        item.targetPath = m_settings.outputDirectory;
    }
    
    // Generate archive name
    item.archiveName = baseName + extension;
    item.format = m_settings.format;
    
    return item;
}

void BatchOperationsDialog::calculateStatistics() {
    m_statistics.completedItems = 0;
    m_statistics.failedItems = 0;
    m_statistics.processedSize = 0;
    
    for (const auto& item : m_batchItems) {
        if (item.completed) {
            if (item.hasError) {
                m_statistics.failedItems++;
            } else {
                m_statistics.completedItems++;
                m_statistics.processedSize += item.sourceSize;
            }
        }
    }
    
    // Calculate time estimates
    updateTimeEstimates();
}

void BatchOperationsDialog::updateTimeEstimates() {
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    m_statistics.elapsedTime = currentTime - m_statistics.startTime.toMSecsSinceEpoch();
    
    if (m_statistics.completedItems > 0 && m_statistics.elapsedTime > 0) {
        // Calculate average speed
        m_statistics.averageSpeed = (double)m_statistics.processedSize / (m_statistics.elapsedTime / 1000.0);
        
        // Estimate remaining time
        qint64 remainingSize = m_statistics.totalSourceSize - m_statistics.processedSize;
        if (m_statistics.averageSpeed > 0) {
            m_statistics.remainingTime = static_cast<qint64>((remainingSize / m_statistics.averageSpeed) * 1000);
        }
        
        // Estimate end time
        m_statistics.estimatedEndTime = QDateTime::currentDateTime().addMSecs(m_statistics.remainingTime);
    }
}

QString BatchOperationsDialog::formatTime(qint64 milliseconds) const {
    if (milliseconds <= 0) {
        return "--";
    }
    
    qint64 seconds = milliseconds / 1000;
    qint64 minutes = seconds / 60;
    qint64 hours = minutes / 60;
    
    seconds %= 60;
    minutes %= 60;
    
    if (hours > 0) {
        return QString("%1:%2:%3").arg(hours).arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
    }
}

QString BatchOperationsDialog::formatSize(qint64 bytes) const {
    if (bytes < 0) return "Unknown";
    if (bytes == 0) return "0 bytes";
    
    const QStringList units = {"bytes", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = bytes;
    
    while (size >= 1024.0 && unitIndex < units.size() - 1) {
        size /= 1024.0;
        ++unitIndex;
    }
    
    return QString("%1 %2").arg(QString::number(size, 'f', unitIndex > 0 ? 1 : 0), units[unitIndex]);
}

QString BatchOperationsDialog::formatSpeed(double bytesPerSecond) const {
    if (bytesPerSecond <= 0) {
        return "--";
    }
    
    const QStringList units = {"B", "KB", "MB", "GB"};
    int unitIndex = 0;
    double speed = bytesPerSecond;
    
    while (speed >= 1024.0 && unitIndex < units.size() - 1) {
        speed /= 1024.0;
        ++unitIndex;
    }
    
    return QString("%1 %2").arg(QString::number(speed, 'f', 1), units[unitIndex]);
}

void BatchOperationsDialog::saveSettingsToPreset(const QString& name) {
    QDir presetDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/presets");
    if (!presetDir.exists()) {
        presetDir.mkpath(".");
    }
    
    QString filePath = getPresetFilePath(name);
    
    QJsonObject json;
    json["operation"] = static_cast<int>(m_settings.operation);
    json["format"] = static_cast<int>(m_settings.format);
    json["compressionLevel"] = static_cast<int>(m_settings.compressionLevel);
    json["outputDirectory"] = m_settings.outputDirectory;
    json["createSubfolders"] = m_settings.createSubfolders;
    json["overwriteExisting"] = m_settings.overwriteExisting;
    json["preserveTimestamps"] = m_settings.preserveTimestamps;
    json["preservePermissions"] = m_settings.preservePermissions;
    json["deleteSourceAfterOperation"] = m_settings.deleteSourceAfterOperation;
    json["validateAfterOperation"] = m_settings.validateAfterOperation;
    json["maxConcurrentOperations"] = m_settings.maxConcurrentOperations;
    json["maxArchiveSize"] = static_cast<double>(m_settings.maxArchiveSize);
    
    QJsonArray excludeArray;
    for (const QString& pattern : m_settings.excludePatterns) {
        excludeArray.append(pattern);
    }
    json["excludePatterns"] = excludeArray;
    
    QJsonArray includeArray;
    for (const QString& pattern : m_settings.includePatterns) {
        includeArray.append(pattern);
    }
    json["includePatterns"] = includeArray;
    
    QJsonDocument doc(json);
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
    }
}

void BatchOperationsDialog::loadSettingsFromPreset(const QString& name) {
    QString filePath = getPresetFilePath(name);
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject json = doc.object();
    
    m_settings.operation = static_cast<OperationType>(json["operation"].toInt());
    m_settings.format = static_cast<ArchiveFormat>(json["format"].toInt());
    m_settings.compressionLevel = static_cast<CompressionLevel>(json["compressionLevel"].toInt());
    m_settings.outputDirectory = json["outputDirectory"].toString();
    m_settings.createSubfolders = json["createSubfolders"].toBool();
    m_settings.overwriteExisting = json["overwriteExisting"].toBool();
    m_settings.preserveTimestamps = json["preserveTimestamps"].toBool();
    m_settings.preservePermissions = json["preservePermissions"].toBool();
    m_settings.deleteSourceAfterOperation = json["deleteSourceAfterOperation"].toBool();
    m_settings.validateAfterOperation = json["validateAfterOperation"].toBool();
    m_settings.maxConcurrentOperations = json["maxConcurrentOperations"].toInt();
    m_settings.maxArchiveSize = static_cast<qint64>(json["maxArchiveSize"].toDouble());
    
    m_settings.excludePatterns.clear();
    QJsonArray excludeArray = json["excludePatterns"].toArray();
    for (const QJsonValue& value : excludeArray) {
        m_settings.excludePatterns.append(value.toString());
    }
    
    m_settings.includePatterns.clear();
    QJsonArray includeArray = json["includePatterns"].toArray();
    for (const QJsonValue& value : includeArray) {
        m_settings.includePatterns.append(value.toString());
    }
}

QString BatchOperationsDialog::getPresetFilePath(const QString& name) const {
    QDir presetDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/presets");
    return presetDir.filePath(name + ".json");
}

void BatchOperationsDialog::logMessage(const QString& message, const QString& level) {
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString logEntry = QString("[%1] %2: %3").arg(timestamp, level, message);
    
    m_logTextEdit->append(logEntry);
    
    // Limit log size
    if (m_logTextEdit->document()->blockCount() > MAX_LOG_ENTRIES) {
        QTextCursor cursor = m_logTextEdit->textCursor();
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, 100);
        cursor.removeSelectedText();
    }
    
    // Auto-scroll to bottom
    QScrollBar* scrollBar = m_logTextEdit->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void BatchOperationsDialog::logError(const QString& error) {
    logMessage(error, "ERROR");
}

void BatchOperationsDialog::logWarning(const QString& warning) {
    logMessage(warning, "WARNING");
}

bool BatchOperationsDialog::validateBatchItems() const {
    if (m_batchItems.isEmpty()) {
        QMessageBox::warning(const_cast<BatchOperationsDialog*>(this), "No Items", 
                           "Please add items to the batch before starting.");
        return false;
    }
    
    // Check if all source paths exist
    for (const auto& item : m_batchItems) {
        if (!QFileInfo::exists(item.sourcePath)) {
            QMessageBox::warning(const_cast<BatchOperationsDialog*>(this), "Invalid Source", 
                               QString("Source path does not exist: %1").arg(item.sourcePath));
            return false;
        }
    }
    
    return true;
}

bool BatchOperationsDialog::validateOutputDirectory() const {
    if (m_settings.outputDirectory.isEmpty()) {
        QMessageBox::warning(const_cast<BatchOperationsDialog*>(this), "No Output Directory", 
                           "Please specify an output directory.");
        return false;
    }
    
    QDir dir(m_settings.outputDirectory);
    if (!dir.exists()) {
        int ret = QMessageBox::question(const_cast<BatchOperationsDialog*>(this), "Create Directory", 
                                      QString("Output directory does not exist. Create it?\n%1").arg(m_settings.outputDirectory),
                                      QMessageBox::Yes | QMessageBox::No);
        
        if (ret == QMessageBox::Yes) {
            if (!dir.mkpath(".")) {
                QMessageBox::warning(const_cast<BatchOperationsDialog*>(this), "Cannot Create Directory", 
                                   "Failed to create output directory.");
                return false;
            }
        } else {
            return false;
        }
    }
    
    return true;
}

bool BatchOperationsDialog::validateSettings() const {
    // Additional settings validation
    if (m_settings.maxConcurrentOperations < 1 || m_settings.maxConcurrentOperations > 16) {
        QMessageBox::warning(const_cast<BatchOperationsDialog*>(this), "Invalid Settings", 
                           "Max concurrent operations must be between 1 and 16.");
        return false;
    }
    
    return true;
}

void BatchOperationsDialog::updatePresetList() {
    QString currentText = m_presetCombo->currentText();
    m_presetCombo->clear();
    
    QStringList presets = availablePresets();
    m_presetCombo->addItems(presets);
    
    // Restore selection if possible
    int index = m_presetCombo->findText(currentText);
    if (index >= 0) {
        m_presetCombo->setCurrentIndex(index);
    }
}

QString BatchOperationsDialog::formatToString(ArchiveFormat format) const {
    switch (format) {
    case ArchiveFormat::Auto: return "Auto";
    case ArchiveFormat::Zip: return "ZIP";
    case ArchiveFormat::SevenZ: return "7Z";
    case ArchiveFormat::Tar: return "TAR";
    case ArchiveFormat::TarGz: return "TAR.GZ";
    case ArchiveFormat::TarBz2: return "TAR.BZ2";
    case ArchiveFormat::TarXz: return "TAR.XZ";
    case ArchiveFormat::Rar: return "RAR";
    }
    return "Unknown";
}

QString BatchOperationsDialog::getArchiveExtension(ArchiveFormat format) const {
    switch (format) {
    case ArchiveFormat::Auto:
    case ArchiveFormat::Zip: return ".zip";
    case ArchiveFormat::SevenZ: return ".7z";
    case ArchiveFormat::Tar: return ".tar";
    case ArchiveFormat::TarGz: return ".tar.gz";
    case ArchiveFormat::TarBz2: return ".tar.bz2";
    case ArchiveFormat::TarXz: return ".tar.xz";
    case ArchiveFormat::Rar: return ".rar";
    }
    return ".zip";
}

qint64 BatchOperationsDialog::calculateDirectorySize(const QDir& dir) const {
    qint64 size = 0;
    
    QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo& info : entries) {
        if (info.isFile()) {
            size += info.size();
        } else if (info.isDir()) {
            size += calculateDirectorySize(QDir(info.filePath()));
        }
    }
    
    return size;
}

// BatchWorker implementation
BatchWorker::BatchWorker(QObject* parent)
    : QObject(parent)
{
}

void BatchWorker::setBatchItems(const QList<BatchOperationsDialog::BatchItem>& items) {
    m_batchItems = items;
}

void BatchWorker::setOperationSettings(const BatchOperationsDialog::OperationSettings& settings) {
    m_settings = settings;
}

void BatchWorker::startBatch() {
    m_currentIndex = 0;
    m_paused = false;
    m_cancelled = false;
    
    processNextItem();
}

void BatchWorker::pauseBatch() {
    QMutexLocker locker(&m_mutex);
    m_paused = true;
}

void BatchWorker::resumeBatch() {
    QMutexLocker locker(&m_mutex);
    m_paused = false;
}

void BatchWorker::cancelBatch() {
    QMutexLocker locker(&m_mutex);
    m_cancelled = true;
}

void BatchWorker::processNextItem() {
    QMutexLocker locker(&m_mutex);
    
    if (m_cancelled) {
        emit batchCompleted(false);
        return;
    }
    
    if (m_paused) {
        // Wait for resume
        QTimer::singleShot(100, this, &BatchWorker::processNextItem);
        return;
    }
    
    if (m_currentIndex >= m_batchItems.size()) {
        emit batchCompleted(true);
        return;
    }
    
    locker.unlock();
    
    emit itemStarted(m_currentIndex);
    processItem(m_currentIndex);
    
    ++m_currentIndex;
    
    // Process next item
    QTimer::singleShot(10, this, &BatchWorker::processNextItem);
}

void BatchWorker::processItem(int index) {
    if (index < 0 || index >= m_batchItems.size()) {
        return;
    }
    
    const BatchOperationsDialog::BatchItem& item = m_batchItems[index];
    
    try {
        switch (m_settings.operation) {
        case BatchOperationsDialog::OperationType::Compress:
            compressItem(item);
            break;
        case BatchOperationsDialog::OperationType::Extract:
            extractItem(item);
            break;
        case BatchOperationsDialog::OperationType::Validate:
            validateItem(item);
            break;
        case BatchOperationsDialog::OperationType::Convert:
            convertItem(item);
            break;
        default:
            throw std::runtime_error("Operation not implemented");
        }
        
        emit itemCompleted(index, true, "Completed successfully");
    } catch (const std::exception& e) {
        emit itemCompleted(index, false, QString::fromStdString(e.what()));
    }
}

void BatchWorker::compressItem(const BatchOperationsDialog::BatchItem& item) {
    // Simulate compression with progress updates
    for (int i = 0; i <= 100; i += 10) {
        QMutexLocker locker(&m_mutex);
        if (m_cancelled) {
            throw std::runtime_error("Operation cancelled");
        }
        
        while (m_paused && !m_cancelled) {
            locker.unlock();
            QThread::msleep(100);
            locker.relock();
        }
        
        locker.unlock();
        
        emit itemProgress(m_currentIndex, i);
        QThread::msleep(100); // Simulate work
    }
}

void BatchWorker::extractItem(const BatchOperationsDialog::BatchItem& item) {
    // Simulate extraction with progress updates
    for (int i = 0; i <= 100; i += 15) {
        QMutexLocker locker(&m_mutex);
        if (m_cancelled) {
            throw std::runtime_error("Operation cancelled");
        }
        
        while (m_paused && !m_cancelled) {
            locker.unlock();
            QThread::msleep(100);
            locker.relock();
        }
        
        locker.unlock();
        
        emit itemProgress(m_currentIndex, i);
        QThread::msleep(80); // Simulate work
    }
}

void BatchWorker::validateItem(const BatchOperationsDialog::BatchItem& item) {
    // Simulate validation with progress updates
    for (int i = 0; i <= 100; i += 20) {
        QMutexLocker locker(&m_mutex);
        if (m_cancelled) {
            throw std::runtime_error("Operation cancelled");
        }
        
        while (m_paused && !m_cancelled) {
            locker.unlock();
            QThread::msleep(100);
            locker.relock();
        }
        
        locker.unlock();
        
        emit itemProgress(m_currentIndex, i);
        QThread::msleep(50); // Simulate work
    }
}

void BatchWorker::convertItem(const BatchOperationsDialog::BatchItem& item) {
    // Simulate conversion with progress updates
    for (int i = 0; i <= 100; i += 12) {
        QMutexLocker locker(&m_mutex);
        if (m_cancelled) {
            throw std::runtime_error("Operation cancelled");
        }
        
        while (m_paused && !m_cancelled) {
            locker.unlock();
            QThread::msleep(100);
            locker.relock();
        }
        
        locker.unlock();
        
        emit itemProgress(m_currentIndex, i);
        QThread::msleep(120); // Simulate work
    }
}

} // namespace FluxGUI::UI::Dialogs

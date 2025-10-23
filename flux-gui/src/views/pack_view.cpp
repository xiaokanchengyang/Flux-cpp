#include "pack_view.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTimer>

PackView::PackView(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_fileListGroup(nullptr)
    , m_fileList(nullptr)
    , m_settingsGroup(nullptr)
    , m_formatCombo(nullptr)
    , m_compressionSpin(nullptr)
    , m_threadsSpin(nullptr)
    , m_passwordEdit(nullptr)
    , m_outputEdit(nullptr)
    , m_browseButton(nullptr)
    , m_packButton(nullptr)
    , m_cancelButton(nullptr)
    , m_progressBar(nullptr)
    , m_statusLabel(nullptr)
{
    setupUI();
    setAcceptDrops(true);
}

void PackView::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(15);
    m_mainLayout->setContentsMargins(20, 20, 20, 20);
    
    setupFileList();
    setupSettings();
    setupActions();
    
    // Progress display
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    m_mainLayout->addWidget(m_progressBar);
    
    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet("color: #666666; font-size: 12px;");
    m_mainLayout->addWidget(m_statusLabel);
    
    updateUI();
}

void PackView::setupFileList() {
    m_fileListGroup = new QGroupBox("File List");
    QVBoxLayout* groupLayout = new QVBoxLayout(m_fileListGroup);
    
    m_fileList = new QListWidget();
    m_fileList->setMinimumHeight(200);
    m_fileList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_fileList->setDragDropMode(QAbstractItemView::DropOnly);
    groupLayout->addWidget(m_fileList);
    
    // File operation buttons
    m_fileButtonsLayout = new QHBoxLayout();
    
    m_addFilesButton = new QPushButton("Add Files");
    m_addFolderButton = new QPushButton("Add Folder");
    m_removeButton = new QPushButton("Remove Selected");
    m_clearButton = new QPushButton("Clear List");
    
    m_fileButtonsLayout->addWidget(m_addFilesButton);
    m_fileButtonsLayout->addWidget(m_addFolderButton);
    m_fileButtonsLayout->addStretch();
    m_fileButtonsLayout->addWidget(m_removeButton);
    m_fileButtonsLayout->addWidget(m_clearButton);
    
    groupLayout->addLayout(m_fileButtonsLayout);
    
    // Connect signals
    connect(m_addFilesButton, &QPushButton::clicked, this, &PackView::onAddFiles);
    connect(m_addFolderButton, &QPushButton::clicked, this, &PackView::onAddFolder);
    connect(m_removeButton, &QPushButton::clicked, this, &PackView::onRemoveSelected);
    connect(m_clearButton, &QPushButton::clicked, this, &PackView::onClearAll);
    
    m_mainLayout->addWidget(m_fileListGroup);
}

void PackView::setupSettings() {
    m_settingsGroup = new QGroupBox("Compression Settings");
    QGridLayout* gridLayout = new QGridLayout(m_settingsGroup);
    
    // Format selection
    gridLayout->addWidget(new QLabel("Format:"), 0, 0);
    m_formatCombo = new QComboBox();
    m_formatCombo->addItems({"tar.zst", "zip", "tar.gz", "tar.xz", "7z"});
    gridLayout->addWidget(m_formatCombo, 0, 1);
    
    // Compression level
    gridLayout->addWidget(new QLabel("Compression Level:"), 1, 0);
    m_compressionSpin = new QSpinBox();
    m_compressionSpin->setRange(0, 9);
    m_compressionSpin->setValue(3);
    gridLayout->addWidget(m_compressionSpin, 1, 1);
    
    // Thread count
    gridLayout->addWidget(new QLabel("Threads:"), 2, 0);
    m_threadsSpin = new QSpinBox();
    m_threadsSpin->setRange(0, 16);
    m_threadsSpin->setValue(0);
    m_threadsSpin->setSpecialValueText("Auto");
    gridLayout->addWidget(m_threadsSpin, 2, 1);
    
    // Password
    gridLayout->addWidget(new QLabel("Password (Optional):"), 3, 0);
    m_passwordEdit = new QLineEdit();
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    gridLayout->addWidget(m_passwordEdit, 3, 1);
    
    // Output path
    gridLayout->addWidget(new QLabel("Output File:"), 4, 0);
    QHBoxLayout* outputLayout = new QHBoxLayout();
    m_outputEdit = new QLineEdit();
    m_browseButton = new QPushButton("Browse...");
    outputLayout->addWidget(m_outputEdit);
    outputLayout->addWidget(m_browseButton);
    gridLayout->addLayout(outputLayout, 4, 1);
    
    // Connect signals
    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PackView::onFormatChanged);
    connect(m_browseButton, &QPushButton::clicked, this, &PackView::onBrowseOutput);
    
    m_mainLayout->addWidget(m_settingsGroup);
}

void PackView::setupActions() {
    m_actionLayout = new QHBoxLayout();
    
    m_packButton = new QPushButton("Start Packing");
    m_packButton->setStyleSheet(R"(
        QPushButton {
            font-size: 14px;
            padding: 10px 20px;
            background-color: #007acc;
            color: white;
            border: none;
            border-radius: 5px;
        }
        QPushButton:hover {
            background-color: #005a9e;
        }
        QPushButton:disabled {
            background-color: #cccccc;
        }
    )");
    
    m_cancelButton = new QPushButton("Cancel");
    m_cancelButton->setVisible(false);
    
    m_actionLayout->addStretch();
    m_actionLayout->addWidget(m_packButton);
    m_actionLayout->addWidget(m_cancelButton);
    
    // Connect signals
    connect(m_packButton, &QPushButton::clicked, this, &PackView::onStartPacking);
    
    m_mainLayout->addLayout(m_actionLayout);
}

void PackView::addFiles(const QStringList& filePaths) {
    for (const QString& filePath : filePaths) {
        QFileInfo fileInfo(filePath);
        if (fileInfo.exists()) {
            // Check if already exists
            bool exists = false;
            for (int i = 0; i < m_fileList->count(); ++i) {
                if (m_fileList->item(i)->data(Qt::UserRole).toString() == filePath) {
                    exists = true;
                    break;
                }
            }
            
            if (!exists) {
                QListWidgetItem* item = new QListWidgetItem();
                item->setText(fileInfo.fileName());
                item->setToolTip(filePath);
                item->setData(Qt::UserRole, filePath);
                
                // Set icon
                if (fileInfo.isDir()) {
                    item->setText(item->text() + "/");
                    item->setIcon(QIcon(":/icons/folder.png"));
                } else {
                    item->setIcon(QIcon(":/icons/file.png"));
                }
                
                m_fileList->addItem(item);
            }
        }
    }
    
    updateUI();
}

void PackView::onAddFiles() {
    QStringList filePaths = QFileDialog::getOpenFileNames(
        this,
        "Select Files",
        QString(),
        "All Files (*.*)"
    );
    
    if (!filePaths.isEmpty()) {
        addFiles(filePaths);
    }
}

void PackView::onAddFolder() {
    QString folderPath = QFileDialog::getExistingDirectory(
        this,
        "Select Folder"
    );
    
    if (!folderPath.isEmpty()) {
        addFiles({folderPath});
    }
}

void PackView::onRemoveSelected() {
    QList<QListWidgetItem*> selectedItems = m_fileList->selectedItems();
    for (QListWidgetItem* item : selectedItems) {
        delete m_fileList->takeItem(m_fileList->row(item));
    }
    updateUI();
}

void PackView::onClearAll() {
    m_fileList->clear();
    updateUI();
}

void PackView::onBrowseOutput() {
    QString format = m_formatCombo->currentText();
    QString filter;
    
    if (format == "zip") {
        filter = "ZIP Files (*.zip)";
    } else if (format == "7z") {
        filter = "7-Zip Files (*.7z)";
    } else if (format == "tar.gz") {
        filter = "TAR.GZ Files (*.tar.gz)";
    } else if (format == "tar.xz") {
        filter = "TAR.XZ Files (*.tar.xz)";
    } else if (format == "tar.zst") {
        filter = "TAR.ZSTD Files (*.tar.zst)";
    }
    
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Save Archive File",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        filter
    );
    
    if (!filePath.isEmpty()) {
        m_outputEdit->setText(filePath);
        updateUI();
    }
}

void PackView::onStartPacking() {
    // TODO: Implement actual packing logic
    // This is a placeholder implementation
    
    if (m_fileList->count() == 0) {
        QMessageBox::warning(this, "Warning", "Please add files or folders to pack first.");
        return;
    }
    
    if (m_outputEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please specify output file path.");
        return;
    }
    
    // Show progress
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    m_statusLabel->setText("Preparing to pack...");
    m_packButton->setEnabled(false);
    m_cancelButton->setVisible(true);
    
    emit packingStarted();
    
    // TODO: Execute actual packing operation in background thread
    // Use timer to simulate progress here
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, [this, timer]() {
        static int progress = 0;
        progress += 10;
        m_progressBar->setValue(progress);
        m_statusLabel->setText(QString("Packing progress: %1%").arg(progress));
        
        if (progress >= 100) {
            timer->stop();
            timer->deleteLater();
            
            m_progressBar->setVisible(false);
            m_statusLabel->setText("Packing completed!");
            m_packButton->setEnabled(true);
            m_cancelButton->setVisible(false);
            
            emit packingFinished(true, "Archive file created successfully");
            
            QMessageBox::information(this, "Complete", "Archive file created successfully!");
        }
    });
    timer->start(500);
}

void PackView::onFormatChanged() {
    // Update output file extension based on format
    QString currentPath = m_outputEdit->text();
    if (!currentPath.isEmpty()) {
        QFileInfo fileInfo(currentPath);
        QString baseName = fileInfo.completeBaseName();
        QString dir = fileInfo.absolutePath();
        QString format = m_formatCombo->currentText();
        
        QString newPath = dir + "/" + baseName + "." + format;
        m_outputEdit->setText(newPath);
    }
    
    updateUI();
}

void PackView::updateUI() {
    // Update button states
    bool hasFiles = m_fileList->count() > 0;
    bool hasSelection = !m_fileList->selectedItems().isEmpty();
    bool hasOutput = !m_outputEdit->text().isEmpty();
    
    m_removeButton->setEnabled(hasSelection);
    m_clearButton->setEnabled(hasFiles);
    m_packButton->setEnabled(hasFiles && hasOutput);
    
    // Update status label
    if (hasFiles) {
        m_statusLabel->setText(QString("Added %1 items").arg(m_fileList->count()));
    } else {
        m_statusLabel->setText("Please add files or folders to pack");
    }
}

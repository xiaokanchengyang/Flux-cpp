/**
 * Example demonstrating the smart context menu system for Flux GUI
 * This shows how to integrate intelligent compression/extraction features
 */

#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>

#include "../ui/components/context_menu_manager.h"
#include "../ui/dialogs/smart_compression_dialog.h"
#include "../ui/dialogs/smart_extraction_dialog.h"

class SmartContextMenuExample : public QMainWindow {
    Q_OBJECT

public:
    SmartContextMenuExample(QWidget* parent = nullptr) : QMainWindow(parent) {
        setupUI();
        setupContextMenu();
        populateExampleFiles();
    }

private slots:
    void onAddFiles() {
        QStringList files = QFileDialog::getOpenFileNames(
            this,
            "Select Files for Context Menu Demo",
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            "All Files (*)"
        );
        
        for (const QString& file : files) {
            auto* item = new QListWidgetItem(file);
            item->setData(Qt::UserRole, file);
            m_fileList->addItem(item);
        }
    }
    
    void onAddFolder() {
        QString folder = QFileDialog::getExistingDirectory(
            this,
            "Select Folder for Context Menu Demo",
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
        );
        
        if (!folder.isEmpty()) {
            auto* item = new QListWidgetItem(QString("[FOLDER] %1").arg(folder));
            item->setData(Qt::UserRole, folder);
            item->setData(Qt::UserRole + 1, true); // Mark as folder
            m_fileList->addItem(item);
        }
    }
    
    void onShowContextMenu(const QPoint& pos) {
        QListWidgetItem* item = m_fileList->itemAt(pos);
        if (!item) return;
        
        QString path = item->data(Qt::UserRole).toString();
        bool isFolder = item->data(Qt::UserRole + 1).toBool();
        QPoint globalPos = m_fileList->mapToGlobal(pos);
        
        if (isFolder) {
            m_contextMenuManager->showFolderContextMenu(path, globalPos);
        } else if (isArchiveFile(path)) {
            m_contextMenuManager->showArchiveContextMenu(path, globalPos);
        } else {
            m_contextMenuManager->showFileContextMenu(path, globalPos);
        }
    }
    
    void onMultiSelection() {
        QList<QListWidgetItem*> selected = m_fileList->selectedItems();
        if (selected.isEmpty()) {
            QMessageBox::information(this, "No Selection", "Please select multiple files first.");
            return;
        }
        
        QStringList paths;
        for (QListWidgetItem* item : selected) {
            paths.append(item->data(Qt::UserRole).toString());
        }
        
        QPoint pos = m_fileList->rect().center();
        QPoint globalPos = m_fileList->mapToGlobal(pos);
        m_contextMenuManager->showMultiFileContextMenu(paths, globalPos);
    }
    
    // Context menu handlers
    void onCompressToZip(const QStringList& paths, const QString& outputPath) {
        QMessageBox::information(this, "Compress to ZIP", 
                                QString("Would compress %1 files to:\n%2")
                                .arg(paths.size()).arg(outputPath));
    }
    
    void onCompressTo7z(const QStringList& paths, const QString& outputPath) {
        QMessageBox::information(this, "Compress to 7Z", 
                                QString("Would compress %1 files to:\n%2")
                                .arg(paths.size()).arg(outputPath));
    }
    
    void onCompressWithOptions(const QStringList& paths, const QString& suggestedFormat) {
        auto* dialog = new FluxGUI::UI::Dialogs::SmartCompressionDialog(paths, this);
        
        connect(dialog, &FluxGUI::UI::Dialogs::SmartCompressionDialog::compressionRequested,
                [this](const FluxGUI::UI::Dialogs::SmartCompressionDialog::CompressionSettings& settings) {
                    QString message = QString("Smart Compression Settings:\n\n"
                                            "Format: %1\n"
                                            "Output: %2\n"
                                            "Level: %3\n"
                                            "Encryption: %4")
                                    .arg(settings.format)
                                    .arg(settings.outputPath)
                                    .arg(settings.compressionLevel)
                                    .arg(settings.encryptArchive ? "Yes" : "No");
                    
                    QMessageBox::information(this, "Compression Settings", message);
                });
        
        dialog->exec();
        dialog->deleteLater();
    }
    
    void onExtractHere(const QString& archivePath) {
        QMessageBox::information(this, "Extract Here", 
                                QString("Would extract %1 to current directory")
                                .arg(QFileInfo(archivePath).fileName()));
    }
    
    void onExtractWithOptions(const QString& archivePath) {
        auto* dialog = new FluxGUI::UI::Dialogs::SmartExtractionDialog(archivePath, this);
        
        connect(dialog, &FluxGUI::UI::Dialogs::SmartExtractionDialog::extractionRequested,
                [this](const FluxGUI::UI::Dialogs::SmartExtractionDialog::ExtractionSettings& settings) {
                    QString message = QString("Smart Extraction Settings:\n\n"
                                            "Archive: %1\n"
                                            "Output: %2\n"
                                            "Create Subfolder: %3\n"
                                            "Overwrite: %4")
                                    .arg(QFileInfo(settings.archivePath).fileName())
                                    .arg(settings.outputPath)
                                    .arg(settings.createSubfolder ? "Yes" : "No")
                                    .arg(settings.overwriteExisting ? "Yes" : "No");
                    
                    QMessageBox::information(this, "Extraction Settings", message);
                });
        
        dialog->exec();
        dialog->deleteLater();
    }
    
    void onPreviewArchive(const QString& archivePath) {
        QMessageBox::information(this, "Archive Preview", 
                                QString("Would show preview of %1")
                                .arg(QFileInfo(archivePath).fileName()));
    }

private:
    void setupUI() {
        setWindowTitle("Smart Context Menu Example - Flux GUI");
        setMinimumSize(800, 600);
        
        auto* centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        
        auto* layout = new QVBoxLayout(centralWidget);
        
        // Header
        auto* headerLabel = new QLabel("Smart Context Menu System Demo", this);
        headerLabel->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; margin: 10px; }");
        headerLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(headerLabel);
        
        // Instructions
        auto* instructionLabel = new QLabel(
            "Right-click on files/folders to see intelligent context menus.\n"
            "The system automatically detects file types and suggests optimal operations.", this);
        instructionLabel->setWordWrap(true);
        instructionLabel->setStyleSheet("QLabel { color: #666; margin: 10px; }");
        layout->addWidget(instructionLabel);
        
        // File list
        m_fileList = new QListWidget(this);
        m_fileList->setContextMenuPolicy(Qt::CustomContextMenu);
        m_fileList->setSelectionMode(QAbstractItemView::ExtendedSelection);
        layout->addWidget(m_fileList);
        
        // Buttons
        auto* buttonLayout = new QHBoxLayout();
        
        auto* addFilesBtn = new QPushButton("Add Files", this);
        auto* addFolderBtn = new QPushButton("Add Folder", this);
        auto* multiSelectBtn = new QPushButton("Multi-Select Context Menu", this);
        
        buttonLayout->addWidget(addFilesBtn);
        buttonLayout->addWidget(addFolderBtn);
        buttonLayout->addWidget(multiSelectBtn);
        buttonLayout->addStretch();
        
        layout->addLayout(buttonLayout);
        
        // Connect signals
        connect(addFilesBtn, &QPushButton::clicked, this, &SmartContextMenuExample::onAddFiles);
        connect(addFolderBtn, &QPushButton::clicked, this, &SmartContextMenuExample::onAddFolder);
        connect(multiSelectBtn, &QPushButton::clicked, this, &SmartContextMenuExample::onMultiSelection);
        connect(m_fileList, &QListWidget::customContextMenuRequested, 
                this, &SmartContextMenuExample::onShowContextMenu);
    }
    
    void setupContextMenu() {
        m_contextMenuManager = new FluxGUI::UI::Components::ContextMenuManager(this);
        
        // Connect compression signals
        connect(m_contextMenuManager, &FluxGUI::UI::Components::ContextMenuManager::compressToZip,
                this, &SmartContextMenuExample::onCompressToZip);
        connect(m_contextMenuManager, &FluxGUI::UI::Components::ContextMenuManager::compressTo7z,
                this, &SmartContextMenuExample::onCompressTo7z);
        connect(m_contextMenuManager, &FluxGUI::UI::Components::ContextMenuManager::compressWithOptions,
                this, &SmartContextMenuExample::onCompressWithOptions);
        
        // Connect extraction signals
        connect(m_contextMenuManager, &FluxGUI::UI::Components::ContextMenuManager::extractHere,
                this, &SmartContextMenuExample::onExtractHere);
        connect(m_contextMenuManager, &FluxGUI::UI::Components::ContextMenuManager::extractWithOptions,
                this, &SmartContextMenuExample::onExtractWithOptions);
        
        // Connect preview signals
        connect(m_contextMenuManager, &FluxGUI::UI::Components::ContextMenuManager::previewArchive,
                this, &SmartContextMenuExample::onPreviewArchive);
    }
    
    void populateExampleFiles() {
        // Add some example files for demonstration
        QStringList examples = {
            "example_document.txt",
            "sample_archive.zip",
            "data_backup.7z",
            "project_files.tar.gz",
            "photos.rar"
        };
        
        for (const QString& example : examples) {
            auto* item = new QListWidgetItem(QString("[EXAMPLE] %1").arg(example));
            item->setData(Qt::UserRole, example);
            m_fileList->addItem(item);
        }
    }
    
    bool isArchiveFile(const QString& filePath) const {
        QStringList archiveExtensions = {
            ".zip", ".7z", ".rar", ".tar", ".gz", ".bz2", ".xz",
            ".tar.gz", ".tar.bz2", ".tar.xz"
        };
        
        QString lowerPath = filePath.toLower();
        for (const QString& ext : archiveExtensions) {
            if (lowerPath.endsWith(ext)) {
                return true;
            }
        }
        return false;
    }

private:
    QListWidget* m_fileList{nullptr};
    FluxGUI::UI::Components::ContextMenuManager* m_contextMenuManager{nullptr};
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    SmartContextMenuExample window;
    window.show();
    
    return app.exec();
}

#include "smart_context_menu_example.moc"

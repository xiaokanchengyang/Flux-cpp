#include "archive_explorer_widget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTreeView>
#include <QListView>
#include <QTableView>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QToolBar>
#include <QAction>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QProgressDialog>
#include <QDebug>

#include "../../core/archive/archive_manager.h"
#include "../../models/archive_model.h"

namespace FluxGUI::UI::Widgets {

ArchiveExplorerWidget::ArchiveExplorerWidget(QWidget* parent)
    : QWidget(parent)
    , m_currentArchivePath()
    , m_archiveModel(nullptr)
    , m_treeView(nullptr)
    , m_toolbar(nullptr)
    , m_statusLabel(nullptr)
    , m_extractButton(nullptr)
    , m_addButton(nullptr)
    , m_deleteButton(nullptr)
{
    // Set object name for styling
    setObjectName("ArchiveExplorerWidget");
    
    // Initialize UI
    initializeUI();
    
    // Connect signals
    connectSignals();
    
    qDebug() << "ArchiveExplorerWidget initialized";
}

ArchiveExplorerWidget::~ArchiveExplorerWidget() = default;

void ArchiveExplorerWidget::openArchive(const QString& archivePath) {
    if (archivePath.isEmpty()) {
        return;
    }
    
    QFileInfo fileInfo(archivePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        QMessageBox::warning(this, "Archive Explorer", 
                           QString("Archive file does not exist: %1").arg(archivePath));
        return;
    }
    
    // Check if format is supported
    auto& archiveManager = Core::Archive::ArchiveManager::instance();
    QString format = archiveManager.detectFormat(archivePath);
    if (format.isEmpty()) {
        QMessageBox::warning(this, "Archive Explorer", 
                           QString("Unsupported archive format: %1").arg(archivePath));
        return;
    }
    
    m_currentArchivePath = archivePath;
    
    // Update status
    updateStatus(QString("Loading archive: %1").arg(fileInfo.fileName()));
    
    // Load archive contents
    loadArchiveContents();
    
    // Update UI state
    updateUIState();
    
    qDebug() << "Opened archive:" << archivePath;
}

void ArchiveExplorerWidget::closeArchive() {
    m_currentArchivePath.clear();
    
    // Clear model
    if (m_archiveModel) {
        m_archiveModel->clear();
    }
    
    // Update status
    updateStatus("No archive loaded");
    
    // Update UI state
    updateUIState();
    
    qDebug() << "Closed archive";
}

QString ArchiveExplorerWidget::currentArchive() const {
    return m_currentArchivePath;
}

bool ArchiveExplorerWidget::hasArchiveLoaded() const {
    return !m_currentArchivePath.isEmpty();
}

void ArchiveExplorerWidget::initializeUI() {
    // Create main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Create toolbar
    createToolbar(mainLayout);
    
    // Create content area
    createContentArea(mainLayout);
    
    // Create status area
    createStatusArea(mainLayout);
    
    // Apply styling
    applyStyles();
}

void ArchiveExplorerWidget::createToolbar(QVBoxLayout* layout) {
    m_toolbar = new QToolBar(this);
    m_toolbar->setObjectName("explorerToolbar");
    m_toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    
    // Extract action
    QAction* extractAction = m_toolbar->addAction(QIcon(":/icons/extract.svg"), "Extract");
    extractAction->setToolTip("Extract selected files");
    connect(extractAction, &QAction::triggered, this, &ArchiveExplorerWidget::extractSelected);
    
    // Extract all action
    QAction* extractAllAction = m_toolbar->addAction(QIcon(":/icons/extract.svg"), "Extract All");
    extractAllAction->setToolTip("Extract all files from archive");
    connect(extractAllAction, &QAction::triggered, this, &ArchiveExplorerWidget::extractAll);
    
    m_toolbar->addSeparator();
    
    // Add files action
    QAction* addAction = m_toolbar->addAction(QIcon(":/icons/add.svg"), "Add Files");
    addAction->setToolTip("Add files to archive");
    connect(addAction, &QAction::triggered, this, &ArchiveExplorerWidget::addFiles);
    
    // Delete action
    QAction* deleteAction = m_toolbar->addAction(QIcon(":/icons/delete.svg"), "Delete");
    deleteAction->setToolTip("Delete selected files from archive");
    connect(deleteAction, &QAction::triggered, this, &ArchiveExplorerWidget::deleteSelected);
    
    m_toolbar->addSeparator();
    
    // Test archive action
    QAction* testAction = m_toolbar->addAction(QIcon(":/icons/check.svg"), "Test");
    testAction->setToolTip("Test archive integrity");
    connect(testAction, &QAction::triggered, this, &ArchiveExplorerWidget::testArchive);
    
    // Properties action
    QAction* propertiesAction = m_toolbar->addAction(QIcon(":/icons/info.svg"), "Properties");
    propertiesAction->setToolTip("Show archive properties");
    connect(propertiesAction, &QAction::triggered, this, &ArchiveExplorerWidget::showProperties);
    
    layout->addWidget(m_toolbar);
}

void ArchiveExplorerWidget::createContentArea(QVBoxLayout* layout) {
    // Create splitter for main content
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setObjectName("explorerSplitter");
    
    // Create archive tree view
    m_treeView = new QTreeView(splitter);
    m_treeView->setObjectName("archiveTreeView");
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    
    // Create archive model
    m_archiveModel = new Models::ArchiveModel(this);
    m_treeView->setModel(m_archiveModel);
    
    // Configure tree view
    m_treeView->header()->setStretchLastSection(false);
    m_treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    
    splitter->addWidget(m_treeView);
    
    // TODO: Add archive info panel
    // QWidget* infoPanel = createInfoPanel(splitter);
    // splitter->addWidget(infoPanel);
    
    // Set splitter proportions
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    
    layout->addWidget(splitter, 1);
}

void ArchiveExplorerWidget::createStatusArea(QVBoxLayout* layout) {
    // Status label
    m_statusLabel = new QLabel("No archive loaded", this);
    m_statusLabel->setObjectName("explorerStatus");
    m_statusLabel->setMargin(8);
    
    layout->addWidget(m_statusLabel);
}

void ArchiveExplorerWidget::connectSignals() {
    // Tree view signals
    if (m_treeView) {
        connect(m_treeView, &QTreeView::doubleClicked, 
                this, &ArchiveExplorerWidget::onItemDoubleClicked);
        connect(m_treeView, &QTreeView::customContextMenuRequested,
                this, &ArchiveExplorerWidget::onContextMenuRequested);
        
        // Selection changes
        connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &ArchiveExplorerWidget::onSelectionChanged);
    }
}

void ArchiveExplorerWidget::loadArchiveContents() {
    if (m_currentArchivePath.isEmpty() || !m_archiveModel) {
        return;
    }
    
    // Create list operation
    auto& archiveManager = Core::Archive::ArchiveManager::instance();
    Core::Archive::ArchiveOperation* operation = archiveManager.listArchive(m_currentArchivePath);
    
    if (!operation) {
        updateStatus("Failed to create list operation");
        return;
    }
    
    // Connect operation signals
    connect(operation, &Core::Archive::ArchiveOperation::finished, this, [this]() {
        updateStatus("Archive loaded successfully");
        // TODO: Update model with archive contents
    });
    
    connect(operation, &Core::Archive::ArchiveOperation::error, this, [this](const QString& error) {
        updateStatus(QString("Failed to load archive: %1").arg(error));
        QMessageBox::warning(this, "Archive Explorer", 
                           QString("Failed to load archive: %1").arg(error));
    });
    
    // TODO: Start operation
    updateStatus("Loading archive contents...");
}

void ArchiveExplorerWidget::updateStatus(const QString& message) {
    if (m_statusLabel) {
        m_statusLabel->setText(message);
    }
}

void ArchiveExplorerWidget::updateUIState() {
    bool hasArchive = hasArchiveLoaded();
    
    // Update toolbar actions
    if (m_toolbar) {
        for (QAction* action : m_toolbar->actions()) {
            if (!action->isSeparator()) {
                action->setEnabled(hasArchive);
            }
        }
    }
}

void ArchiveExplorerWidget::applyStyles() {
    // Apply custom styling through object names and properties
    // The actual styling is handled by the QSS files
    
    // Force style update
    style()->unpolish(this);
    style()->polish(this);
}

// Slot implementations
void ArchiveExplorerWidget::extractSelected() {
    // TODO: Implement extract selected functionality
    QMessageBox::information(this, "Extract Selected", 
                           "Extract selected functionality will be implemented.");
}

void ArchiveExplorerWidget::extractAll() {
    if (!hasArchiveLoaded()) {
        return;
    }
    
    emit extractArchiveRequested(m_currentArchivePath);
}

void ArchiveExplorerWidget::addFiles() {
    // TODO: Implement add files functionality
    QMessageBox::information(this, "Add Files", 
                           "Add files functionality will be implemented.");
}

void ArchiveExplorerWidget::deleteSelected() {
    // TODO: Implement delete selected functionality
    QMessageBox::information(this, "Delete Selected", 
                           "Delete selected functionality will be implemented.");
}

void ArchiveExplorerWidget::testArchive() {
    if (!hasArchiveLoaded()) {
        return;
    }
    
    // Create test operation
    auto& archiveManager = Core::Archive::ArchiveManager::instance();
    Core::Archive::ArchiveOperation* operation = archiveManager.testArchive(m_currentArchivePath);
    
    if (!operation) {
        QMessageBox::warning(this, "Test Archive", "Failed to create test operation");
        return;
    }
    
    // Show progress dialog
    QProgressDialog* progressDialog = new QProgressDialog("Testing archive...", "Cancel", 0, 0, this);
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->show();
    
    // Connect operation signals
    connect(operation, &Core::Archive::ArchiveOperation::finished, this, [this, progressDialog]() {
        progressDialog->hide();
        progressDialog->deleteLater();
        QMessageBox::information(this, "Test Archive", "Archive test completed successfully.");
    });
    
    connect(operation, &Core::Archive::ArchiveOperation::error, this, [this, progressDialog](const QString& error) {
        progressDialog->hide();
        progressDialog->deleteLater();
        QMessageBox::warning(this, "Test Archive", 
                           QString("Archive test failed: %1").arg(error));
    });
    
    connect(progressDialog, &QProgressDialog::canceled, operation, &Core::Archive::ArchiveOperation::cancel);
    
    // TODO: Start operation
}

void ArchiveExplorerWidget::showProperties() {
    // TODO: Implement show properties functionality
    QMessageBox::information(this, "Properties", 
                           "Properties functionality will be implemented.");
}

void ArchiveExplorerWidget::onItemDoubleClicked(const QModelIndex& index) {
    Q_UNUSED(index)
    // TODO: Handle item double-click (extract or navigate)
}

void ArchiveExplorerWidget::onContextMenuRequested(const QPoint& position) {
    Q_UNUSED(position)
    // TODO: Show context menu
}

void ArchiveExplorerWidget::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {
    Q_UNUSED(selected)
    Q_UNUSED(deselected)
    // TODO: Update UI based on selection
}

} // namespace FluxGUI::UI::Widgets

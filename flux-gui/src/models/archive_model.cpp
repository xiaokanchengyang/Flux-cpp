#include "archive_model.h"
#include <QApplication>
#include <QStyle>
#include <QFileIconProvider>
#include <QMimeData>
#include <QUrl>
#include <QDebug>
#include <QLocale>
#include <QDateTime>
#include <algorithm>

ArchiveModel::ArchiveModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_rootNode(new TreeNode())
    , m_statsValid(false)
{
    m_rootNode->entry.name = "Root";
    m_rootNode->entry.isDirectory = true;
}

ArchiveModel::~ArchiveModel() {
    delete m_rootNode;
}

QModelIndex ArchiveModel::index(int row, int column, const QModelIndex &parent) const {
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    TreeNode* parentNode = getNode(parent);
    if (!parentNode || row >= parentNode->children.size()) {
        return QModelIndex();
    }

    TreeNode* childNode = parentNode->children.at(row);
    return createIndex(row, column, childNode);
}

QModelIndex ArchiveModel::parent(const QModelIndex &child) const {
    if (!child.isValid()) {
        return QModelIndex();
    }

    TreeNode* childNode = getNode(child);
    if (!childNode || !childNode->parent || childNode->parent == m_rootNode) {
        return QModelIndex();
    }

    TreeNode* parentNode = childNode->parent;
    TreeNode* grandParentNode = parentNode->parent;
    if (!grandParentNode) {
        return QModelIndex();
    }

    int row = grandParentNode->children.indexOf(parentNode);
    return createIndex(row, 0, parentNode);
}

int ArchiveModel::rowCount(const QModelIndex &parent) const {
    TreeNode* parentNode = getNode(parent);
    if (!parentNode) {
        return 0;
    }

    // If there's a filter, only show matching nodes
    if (!m_filterString.isEmpty()) {
        int count = 0;
        for (TreeNode* child : parentNode->children) {
            if (matchesFilter(child)) {
                count++;
            }
        }
        return count;
    }

    return parentNode->children.size();
}

int ArchiveModel::columnCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return ColumnCount;
}

QVariant ArchiveModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) {
        return QVariant();
    }

    TreeNode* node = getNode(index);
    if (!node) {
        return QVariant();
    }

    const ArchiveEntry& entry = node->entry;

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case NameColumn:
            return entry.name;
        case SizeColumn:
            return entry.isDirectory ? QString() : formatSize(entry.size);
        case CompressedSizeColumn:
            return entry.isDirectory ? QString() : formatSize(entry.compressedSize);
        case RatioColumn:
            if (entry.isDirectory || entry.size == 0) return QString();
            return QString("%1%").arg(entry.compressionRatio(), 0, 'f', 1);
        case ModifiedColumn:
            return formatDateTime(entry.modified);
        case PermissionsColumn:
            return entry.permissions;
        case CrcColumn:
            return entry.isDirectory ? QString() : QString("%1").arg(entry.crc32, 8, 16, QChar('0')).toUpper();
        }
        break;

    case Qt::DecorationRole:
        if (index.column() == NameColumn) {
            return getFileIcon(entry);
        }
        break;

    case Qt::TextAlignmentRole:
        switch (index.column()) {
        case SizeColumn:
        case CompressedSizeColumn:
        case RatioColumn:
        case CrcColumn:
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        default:
            return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        }

    case Qt::ToolTipRole:
        {
            QString tooltip = QString("<b>%1</b><br/>").arg(entry.name);
            if (!entry.isDirectory) {
                tooltip += QString("Size: %1<br/>").arg(formatSize(entry.size));
                tooltip += QString("Compressed: %1<br/>").arg(formatSize(entry.compressedSize));
                tooltip += QString("Compression: %1%<br/>").arg(entry.compressionRatio(), 0, 'f', 1);
                if (entry.crc32 != 0) {
                    tooltip += QString("CRC32: %1<br/>").arg(entry.crc32, 8, 16, QChar('0')).toUpper();
                }
            }
            tooltip += QString("Modified: %1<br/>").arg(formatDateTime(entry.modified));
            if (!entry.permissions.isEmpty()) {
                tooltip += QString("Permissions: %1<br/>").arg(entry.permissions);
            }
            if (!entry.comment.isEmpty()) {
                tooltip += QString("Comment: %1").arg(entry.comment);
            }
            return tooltip;
        }

    case EntryRole:
        return QVariant::fromValue(&entry);

    case PathRole:
        return entry.path;

    case IsDirectoryRole:
        return entry.isDirectory;

    case SizeRole:
        return entry.size;

    case CompressedSizeRole:
        return entry.compressedSize;

    case IconRole:
        return getFileIcon(entry);
    }

    return QVariant();
}

QVariant ArchiveModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal) {
        return QVariant();
    }

    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case NameColumn: return "Name";
        case SizeColumn: return "Size";
        case CompressedSizeColumn: return "Compressed Size";
        case RatioColumn: return "Ratio";
        case ModifiedColumn: return "Modified";
        case PermissionsColumn: return "Permissions";
        case CrcColumn: return "CRC32";
        }
        break;

    case Qt::TextAlignmentRole:
        switch (section) {
        case SizeColumn:
        case CompressedSizeColumn:
        case RatioColumn:
        case CrcColumn:
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        default:
            return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        }

    case Qt::ToolTipRole:
        switch (section) {
        case NameColumn: return "File or folder name";
        case SizeColumn: return "Original file size";
        case CompressedSizeColumn: return "Compressed file size";
        case RatioColumn: return "Compression ratio percentage";
        case ModifiedColumn: return "Last modified time";
        case PermissionsColumn: return "File permissions";
        case CrcColumn: return "CRC32 checksum value";
        }
        break;
    }

    return QVariant();
}

Qt::ItemFlags ArchiveModel::flags(const QModelIndex &index) const {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    
    // Support drag and drop
    flags |= Qt::ItemIsDragEnabled;
    
    return flags;
}

QStringList ArchiveModel::mimeTypes() const {
    return QStringList() << "application/x-archive-entries" << "text/uri-list";
}

QMimeData* ArchiveModel::mimeData(const QModelIndexList &indexes) const {
    QMimeData* mimeData = new QMimeData();
    
    QStringList paths;
    QList<QUrl> urls;
    
    for (const QModelIndex& index : indexes) {
        if (index.column() == 0) { // Only process first column
            TreeNode* node = getNode(index);
            if (node) {
                paths << node->entry.path;
                // Create virtual URL for dragging to external apps
                QUrl url;
                url.setScheme("archive");
                url.setPath(node->entry.path);
                urls << url;
            }
        }
    }
    
    // Set custom MIME data
    mimeData->setData("application/x-archive-entries", paths.join('\n').toUtf8());
    
    // Set URL list (for dragging to external apps)
    mimeData->setUrls(urls);
    
    return mimeData;
}

Qt::DropActions ArchiveModel::supportedDropActions() const {
    return Qt::CopyAction | Qt::MoveAction;
}

void ArchiveModel::setEntries(const QList<ArchiveEntry>& entries) {
    beginResetModel();
    
    // Clear existing data
    delete m_rootNode;
    m_rootNode = new TreeNode();
    m_rootNode->entry.name = "Root";
    m_rootNode->entry.isDirectory = true;
    m_pathToNode.clear();
    m_statsValid = false;
    
    // Build new tree structure
    buildTree(entries);
    
    endResetModel();
    emit dataUpdated();
}

void ArchiveModel::addEntry(const ArchiveEntry& entry) {
    TreeNode* parentNode = findOrCreateNode(entry.path);
    if (!parentNode) return;
    
    // Find insertion position
    QModelIndex parentIndex;
    if (parentNode != m_rootNode) {
        // Find parent node index
        TreeNode* grandParent = parentNode->parent;
        if (grandParent) {
            int parentRow = grandParent->children.indexOf(parentNode);
            parentIndex = createIndex(parentRow, 0, parentNode);
        }
    }
    
    int insertRow = parentNode->children.size();
    beginInsertRows(parentIndex, insertRow, insertRow);
    
    TreeNode* newNode = new TreeNode(parentNode);
    newNode->entry = entry;
    parentNode->addChild(newNode);
    m_pathToNode[entry.path] = newNode;
    
    endInsertRows();
    
    m_statsValid = false;
    emit dataUpdated();
}

void ArchiveModel::removeEntry(const QString& path) {
    auto it = m_pathToNode.find(path);
    if (it == m_pathToNode.end()) return;
    
    TreeNode* node = it.value();
    TreeNode* parent = node->parent;
    if (!parent) return;
    
    int row = parent->children.indexOf(node);
    if (row < 0) return;
    
    QModelIndex parentIndex;
    if (parent != m_rootNode) {
        TreeNode* grandParent = parent->parent;
        if (grandParent) {
            int parentRow = grandParent->children.indexOf(parent);
            parentIndex = createIndex(parentRow, 0, parent);
        }
    }
    
    beginRemoveRows(parentIndex, row, row);
    
    parent->children.removeAt(row);
    m_pathToNode.remove(path);
    delete node;
    
    endRemoveRows();
    
    m_statsValid = false;
    emit dataUpdated();
}

void ArchiveModel::clear() {
    beginResetModel();
    
    delete m_rootNode;
    m_rootNode = new TreeNode();
    m_rootNode->entry.name = "Root";
    m_rootNode->entry.isDirectory = true;
    m_pathToNode.clear();
    m_iconCache.clear();
    m_statsValid = false;
    
    endResetModel();
    emit dataUpdated();
}

const ArchiveEntry* ArchiveModel::getEntry(const QModelIndex& index) const {
    TreeNode* node = getNode(index);
    return node ? &node->entry : nullptr;
}

const ArchiveEntry* ArchiveModel::findEntry(const QString& path) const {
    auto it = m_pathToNode.find(path);
    return it != m_pathToNode.end() ? &it.value()->entry : nullptr;
}

QStringList ArchiveModel::getSelectedPaths(const QModelIndexList& indexes) const {
    QStringList paths;
    for (const QModelIndex& index : indexes) {
        if (index.column() == 0) { // Only process first column
            TreeNode* node = getNode(index);
            if (node) {
                paths << node->entry.path;
            }
        }
    }
    return paths;
}

void ArchiveModel::setFilter(const QString& filter) {
    if (m_filterString == filter) return;
    
    beginResetModel();
    m_filterString = filter;
    endResetModel();
}

ArchiveModel::Statistics ArchiveModel::getStatistics() const {
    if (m_statsValid) {
        return m_cachedStats;
    }
    
    Statistics stats = {};
    qint64 totalCompressionSavings = 0;
    int compressedFiles = 0;
    
    std::function<void(TreeNode*)> calculateStats = [&](TreeNode* node) {
        if (node->entry.isDirectory) {
            stats.totalDirectories++;
        } else {
            stats.totalFiles++;
            stats.totalSize += node->entry.size;
            stats.totalCompressedSize += node->entry.compressedSize;
            
            if (node->entry.size > 0) {
                totalCompressionSavings += (node->entry.size - node->entry.compressedSize);
                compressedFiles++;
            }
        }
        
        for (TreeNode* child : node->children) {
            calculateStats(child);
        }
    };
    
    calculateStats(m_rootNode);
    
    // Calculate average compression ratio
    if (stats.totalSize > 0) {
        stats.averageCompressionRatio = (static_cast<double>(totalCompressionSavings) / stats.totalSize) * 100.0;
    }
    
    m_cachedStats = stats;
    m_statsValid = true;
    
    return stats;
}

void ArchiveModel::onItemDoubleClicked(const QModelIndex& index) {
    TreeNode* node = getNode(index);
    if (node) {
        emit entryDoubleClicked(node->entry.path);
    }
}

void ArchiveModel::buildTree(const QList<ArchiveEntry>& entries) {
    // Sort by path depth to ensure parent directories are created first
    QList<ArchiveEntry> sortedEntries = entries;
    std::sort(sortedEntries.begin(), sortedEntries.end(), 
              [](const ArchiveEntry& a, const ArchiveEntry& b) {
                  int depthA = a.path.count('/');
                  int depthB = b.path.count('/');
                  if (depthA != depthB) return depthA < depthB;
                  return a.path < b.path;
              });
    
    for (const ArchiveEntry& entry : sortedEntries) {
        TreeNode* node = findOrCreateNode(entry.path);
        if (node) {
            node->entry = entry;
            m_pathToNode[entry.path] = node;
        }
    }
}

ArchiveModel::TreeNode* ArchiveModel::findOrCreateNode(const QString& path) {
    if (path.isEmpty()) return m_rootNode;
    
    // Check cache
    auto it = m_pathToNode.find(path);
    if (it != m_pathToNode.end()) {
        return it.value();
    }
    
    // Decompose path
    QStringList pathParts = path.split('/', Qt::SkipEmptyParts);
    if (pathParts.isEmpty()) return m_rootNode;
    
    TreeNode* currentNode = m_rootNode;
    QString currentPath;
    
    for (int i = 0; i < pathParts.size(); ++i) {
        const QString& part = pathParts[i];
        if (!currentPath.isEmpty()) currentPath += "/";
        currentPath += part;
        
        // Find existing child node
        TreeNode* childNode = currentNode->findChild(part);
        
        if (!childNode) {
            // Create new node
            childNode = new TreeNode(currentNode);
            childNode->entry.name = part;
            childNode->entry.path = currentPath;
            childNode->entry.isDirectory = (i < pathParts.size() - 1); // Last one might be a file
            
            currentNode->addChild(childNode);
            m_pathToNode[currentPath] = childNode;
        }
        
        currentNode = childNode;
    }
    
    return currentNode;
}

ArchiveModel::TreeNode* ArchiveModel::getNode(const QModelIndex& index) const {
    if (!index.isValid()) {
        return m_rootNode;
    }
    return static_cast<TreeNode*>(index.internalPointer());
}

QIcon ArchiveModel::getFileIcon(const ArchiveEntry& entry) const {
    // Check icon cache
    QString cacheKey = entry.isDirectory ? "folder" : entry.extension();
    auto it = m_iconCache.find(cacheKey);
    if (it != m_iconCache.end()) {
        return it.value();
    }
    
    QIcon icon;
    
    if (entry.isDirectory) {
        icon = QApplication::style()->standardIcon(QStyle::SP_DirIcon);
    } else {
        // Select icon based on file extension
        QString ext = entry.extension();
        
        if (ext.isEmpty()) {
            icon = QIcon(":/icons/file.png");
        } else if (QStringList({"jpg", "jpeg", "png", "gif", "bmp", "svg", "webp"}).contains(ext)) {
            icon = QIcon(":/icons/image.png");
        } else if (QStringList({"txt", "md", "doc", "docx", "pdf", "rtf"}).contains(ext)) {
            icon = QIcon(":/icons/document.png");
        } else if (QStringList({"cpp", "h", "c", "py", "js", "html", "css", "xml", "json"}).contains(ext)) {
            icon = QIcon(":/icons/code.png");
        } else if (QStringList({"mp3", "wav", "flac", "ogg", "m4a"}).contains(ext)) {
            icon = QIcon(":/icons/audio.png");
        } else if (QStringList({"mp4", "avi", "mkv", "mov", "wmv", "flv"}).contains(ext)) {
            icon = QIcon(":/icons/video.png");
        } else if (QStringList({"zip", "7z", "rar", "tar", "gz", "xz", "zst"}).contains(ext)) {
            icon = QIcon(":/icons/archive.png");
        } else {
            icon = QIcon(":/icons/file.png");
        }
    }
    
    // Cache icon
    m_iconCache[cacheKey] = icon;
    return icon;
}

QString ArchiveModel::formatSize(qint64 size) const {
    if (size < 0) return "N/A";
    
    const QStringList units = {"B", "KB", "MB", "GB", "TB"};
    double sizeDouble = size;
    int unitIndex = 0;
    
    while (sizeDouble >= 1024.0 && unitIndex < units.size() - 1) {
        sizeDouble /= 1024.0;
        unitIndex++;
    }
    
    if (unitIndex == 0) {
        return QString("%1 %2").arg(size).arg(units[unitIndex]);
    } else {
        return QString("%1 %2").arg(sizeDouble, 0, 'f', 1).arg(units[unitIndex]);
    }
}

QString ArchiveModel::formatDateTime(const QDateTime& dateTime) const {
    if (!dateTime.isValid()) return "N/A";
    
    QLocale locale;
    return locale.toString(dateTime, QLocale::ShortFormat);
}

bool ArchiveModel::matchesFilter(const TreeNode* node) const {
    if (m_filterString.isEmpty()) return true;
    
    // Check current node
    if (node->entry.name.contains(m_filterString, Qt::CaseInsensitive)) {
        return true;
    }
    
    // Check child nodes (recursive)
    for (TreeNode* child : node->children) {
        if (matchesFilter(child)) {
            return true;
        }
    }
    
    return false;
}
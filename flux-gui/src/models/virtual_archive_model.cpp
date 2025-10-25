// Copyright (c) 2024 Flux Archive Manager Contributors
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "virtual_archive_model.h"
#include <QApplication>
#include <QStyle>
#include <QFileIconProvider>
#include <QMimeData>
#include <QUrl>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QCollator>
#include <QLocale>
#include <QDateTime>
#include <algorithm>
#include <numeric>

namespace FluxGui {

// ArchiveNode implementation
ArchiveNode* ArchiveNode::findOrCreateChild(const QString& child_name) {
    // Look for existing child
    for (const auto& child : children) {
        if (child->name == child_name) {
            return child.get();
        }
    }
    
    // Create new child
    auto new_child = std::make_unique<ArchiveNode>(child_name, this);
    new_child->depth = depth + 1;
    ArchiveNode* child_ptr = new_child.get();
    children.push_back(std::move(new_child));
    
    return child_ptr;
}

ArchiveNode* ArchiveNode::child(int index) const {
    if (index >= 0 && index < static_cast<int>(children.size())) {
        return children[index].get();
    }
    return nullptr;
}

int ArchiveNode::childCount() const {
    return static_cast<int>(children.size());
}

int ArchiveNode::row() const {
    if (!parent) {
        return 0;
    }
    
    const auto& siblings = parent->children;
    for (int i = 0; i < static_cast<int>(siblings.size()); ++i) {
        if (siblings[i].get() == this) {
            return i;
        }
    }
    return 0;
}

quint64 ArchiveNode::totalSize() const {
    quint64 total = size;
    for (const auto& child : children) {
        total += child->totalSize();
    }
    return total;
}

bool ArchiveNode::matchesFilter(const QString& filter, Qt::CaseSensitivity cs) const {
    if (filter.isEmpty()) {
        return true;
    }
    
    return name.contains(filter, cs) || full_path.contains(filter, cs);
}

// VirtualArchiveModel implementation
VirtualArchiveModel::VirtualArchiveModel(QObject* parent)
    : QAbstractItemModel(parent)
    , root_node_(std::make_unique<ArchiveNode>())
    , task_executor_(std::make_shared<TaskExecutor>())
{
    // Connect task executor signals
    connect(task_executor_.get(), &TaskExecutor::archiveContentsReady,
            this, &VirtualArchiveModel::onArchiveContentsReady);
    connect(task_executor_.get(), &TaskExecutor::taskFinished,
            this, &VirtualArchiveModel::onTaskFinished);
}

VirtualArchiveModel::~VirtualArchiveModel() = default;

QModelIndex VirtualArchiveModel::index(int row, int column, const QModelIndex& parent) const {
    if (!hasIndex(row, column, parent)) {
        return QModelIndex{};
    }
    
    ArchiveNode* parent_node = nodeFromIndex(parent);
    if (!parent_node) {
        return QModelIndex{};
    }
    
    ArchiveNode* child_node = parent_node->child(row);
    if (child_node) {
        return createIndex(row, column, child_node);
    }
    
    return QModelIndex{};
}

QModelIndex VirtualArchiveModel::parent(const QModelIndex& child) const {
    if (!child.isValid()) {
        return QModelIndex{};
    }
    
    ArchiveNode* child_node = nodeFromIndex(child);
    if (!child_node || !child_node->parent || child_node->parent == root_node_.get()) {
        return QModelIndex{};
    }
    
    ArchiveNode* parent_node = child_node->parent;
    return createIndex(parent_node->row(), 0, parent_node);
}

int VirtualArchiveModel::rowCount(const QModelIndex& parent) const {
    ArchiveNode* parent_node = nodeFromIndex(parent);
    return parent_node ? parent_node->childCount() : 0;
}

int VirtualArchiveModel::columnCount(const QModelIndex& parent) const {
    Q_UNUSED(parent)
    return ColumnCount;
}

QVariant VirtualArchiveModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) {
        return QVariant{};
    }
    
    ArchiveNode* node = nodeFromIndex(index);
    if (!node) {
        return QVariant{};
    }
    
    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case NameColumn:
            return node->name;
        case SizeColumn:
            return node->is_directory ? QString{} : formatSize(node->size);
        case CompressedSizeColumn:
            return node->is_directory ? QString{} : formatSize(node->compressed_size);
        case RatioColumn:
            return node->is_directory ? QString{} : formatRatio(node->size, node->compressed_size);
        case ModifiedColumn:
            return node->modification_time;
        case PermissionsColumn:
            return formatPermissions(node->permissions);
        }
        break;
        
    case Qt::DecorationRole:
        if (index.column() == NameColumn) {
            return getIcon(node);
        }
        break;
        
    case Qt::ToolTipRole:
        return QString("Path: %1\nSize: %2\nCompressed: %3\nModified: %4")
               .arg(node->full_path)
               .arg(formatSize(node->size))
               .arg(formatSize(node->compressed_size))
               .arg(node->modification_time);
        
    case FullPathRole:
        return node->full_path;
        
    case IsDirectoryRole:
        return node->is_directory;
        
    case OriginalSizeRole:
        return static_cast<qulonglong>(node->size);
        
    case CompressedSizeRole:
        return static_cast<qulonglong>(node->compressed_size);
        
    case CompressionRatioRole:
        if (node->size > 0) {
            return static_cast<double>(node->compressed_size) / node->size;
        }
        return 0.0;
        
    case ModificationTimeRole:
        return node->modification_time;
        
    case PermissionsRole:
        return node->permissions;
        
    case DepthRole:
        return node->depth;
        
    case NodePointerRole:
        return QVariant::fromValue(static_cast<void*>(node));
    }
    
    return QVariant{};
}

QVariant VirtualArchiveModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant{};
    }
    
    switch (section) {
    case NameColumn:
        return tr("Name");
    case SizeColumn:
        return tr("Size");
    case CompressedSizeColumn:
        return tr("Compressed");
    case RatioColumn:
        return tr("Ratio");
    case ModifiedColumn:
        return tr("Modified");
    case PermissionsColumn:
        return tr("Permissions");
    default:
        return QVariant{};
    }
}

Qt::ItemFlags VirtualArchiveModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    
    ArchiveNode* node = nodeFromIndex(index);
    if (node && node->is_directory) {
        flags |= Qt::ItemIsDropEnabled;
    }
    
    flags |= Qt::ItemIsDragEnabled;
    
    return flags;
}

bool VirtualArchiveModel::hasChildren(const QModelIndex& parent) const {
    ArchiveNode* parent_node = nodeFromIndex(parent);
    return parent_node && (parent_node->childCount() > 0 || parent_node->is_directory);
}

bool VirtualArchiveModel::canFetchMore(const QModelIndex& parent) const {
    Q_UNUSED(parent)
    // For now, we load all data at once
    // In a more advanced implementation, this could support lazy loading
    return false;
}

void VirtualArchiveModel::fetchMore(const QModelIndex& parent) {
    Q_UNUSED(parent)
    // Implementation for lazy loading would go here
}

Qt::DropActions VirtualArchiveModel::supportedDropActions() const {
    return Qt::CopyAction | Qt::MoveAction;
}

QStringList VirtualArchiveModel::mimeTypes() const {
    return QStringList{} << "text/uri-list" << "application/x-flux-archive-entries";
}

QMimeData* VirtualArchiveModel::mimeData(const QModelIndexList& indexes) const {
    auto* mime_data = new QMimeData{};
    
    QStringList paths;
    QList<QUrl> urls;
    
    for (const QModelIndex& index : indexes) {
        if (index.column() == 0) { // Only process name column
            ArchiveNode* node = nodeFromIndex(index);
            if (node) {
                paths << node->full_path;
                urls << QUrl::fromLocalFile(node->full_path);
            }
        }
    }
    
    mime_data->setUrls(urls);
    mime_data->setData("application/x-flux-archive-entries", paths.join('\n').toUtf8());
    
    return mime_data;
}

void VirtualArchiveModel::loadArchive(const QString& archive_path, const QString& password) {
    if (is_loading_) {
        return;
    }
    
    current_archive_path_ = archive_path;
    current_password_ = password;
    
    clear();
    
    is_loading_ = true;
    emit loadingStarted();
    
    // Start async loading
    task_executor_->listArchiveContents(archive_path, password);
}

void VirtualArchiveModel::clear() {
    beginResetModel();
    
    root_node_ = std::make_unique<ArchiveNode>();
    archive_info_ = ArchiveInfo{};
    total_entries_ = 0;
    icon_cache_.clear();
    
    endResetModel();
}

ArchiveNode* VirtualArchiveModel::nodeFromIndex(const QModelIndex& index) const {
    if (index.isValid()) {
        return static_cast<ArchiveNode*>(index.internalPointer());
    }
    return root_node_.get();
}

QModelIndex VirtualArchiveModel::indexFromNode(ArchiveNode* node, int column) const {
    if (!node || node == root_node_.get()) {
        return QModelIndex{};
    }
    
    return createIndex(node->row(), column, node);
}

void VirtualArchiveModel::expandToDepth(int depth) {
    // This would be implemented by the view, not the model
    Q_UNUSED(depth)
}

QStringList VirtualArchiveModel::getSelectedPaths(const QModelIndexList& indexes) const {
    QStringList paths;
    
    for (const QModelIndex& index : indexes) {
        if (index.column() == 0) {
            ArchiveNode* node = nodeFromIndex(index);
            if (node) {
                paths << node->full_path;
            }
        }
    }
    
    return paths;
}

void VirtualArchiveModel::refresh() {
    if (!current_archive_path_.isEmpty()) {
        loadArchive(current_archive_path_, current_password_);
    }
}

void VirtualArchiveModel::onArchiveContentsReady(const ArchiveInfo& info, const QList<ArchiveEntry>& entries) {
    beginResetModel();
    
    archive_info_ = info;
    total_entries_ = entries.size();
    
    buildTree(entries);
    
    endResetModel();
}

void VirtualArchiveModel::onTaskFinished(bool success, const QString& message) {
    is_loading_ = false;
    emit loadingFinished(success, message);
}

void VirtualArchiveModel::buildTree(const QList<ArchiveEntry>& entries) {
    root_node_ = std::make_unique<ArchiveNode>();
    
    // Build tree structure
    for (const ArchiveEntry& entry : entries) {
        QStringList path_parts = entry.path.split('/', Qt::SkipEmptyParts);
        
        ArchiveNode* current_node = root_node_.get();
        QString current_path;
        
        // Navigate/create path
        for (int i = 0; i < path_parts.size(); ++i) {
            const QString& part = path_parts[i];
            current_path += (current_path.isEmpty() ? "" : "/") + part;
            
            ArchiveNode* child_node = current_node->findOrCreateChild(part);
            child_node->full_path = current_path;
            
            // Set properties for the final node (file or directory)
            if (i == path_parts.size() - 1) {
                child_node->size = entry.uncompressed_size;
                child_node->compressed_size = entry.compressed_size;
                child_node->is_directory = entry.is_directory;
                child_node->modification_time = entry.modification_time;
                child_node->permissions = entry.permissions;
            } else {
                // Intermediate directory
                child_node->is_directory = true;
            }
            
            current_node = child_node;
        }
    }
    
    // Sort all children
    sortChildren(root_node_.get());
}

std::unique_ptr<ArchiveNode> VirtualArchiveModel::createNode(const ArchiveEntry& entry, ArchiveNode* parent) {
    auto node = std::make_unique<ArchiveNode>(entry.name, parent);
    node->full_path = entry.path;
    node->size = entry.uncompressed_size;
    node->compressed_size = entry.compressed_size;
    node->is_directory = entry.is_directory;
    node->modification_time = entry.modification_time;
    node->permissions = entry.permissions;
    node->depth = parent ? parent->depth + 1 : 0;
    
    return node;
}

QIcon VirtualArchiveModel::getIcon(const ArchiveNode* node) const {
    if (!node) {
        return QIcon{};
    }
    
    QString cache_key = node->is_directory ? "folder" : QFileInfo(node->name).suffix().toLower();
    
    auto it = icon_cache_.find(cache_key);
    if (it != icon_cache_.end()) {
        return it.value();
    }
    
    QIcon icon;
    if (node->is_directory) {
        icon = QApplication::style()->standardIcon(QStyle::SP_DirIcon);
    } else {
        QFileIconProvider provider;
        icon = provider.icon(QFileInfo(node->name));
        if (icon.isNull()) {
            icon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);
        }
    }
    
    icon_cache_[cache_key] = icon;
    return icon;
}

QString VirtualArchiveModel::formatSize(quint64 size) const {
    const QStringList units = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size_double = static_cast<double>(size);
    
    while (size_double >= 1024.0 && unit_index < units.size() - 1) {
        size_double /= 1024.0;
        ++unit_index;
    }
    
    return QString("%1 %2").arg(size_double, 0, 'f', unit_index == 0 ? 0 : 1).arg(units[unit_index]);
}

QString VirtualArchiveModel::formatRatio(quint64 original, quint64 compressed) const {
    if (original == 0) {
        return "0%";
    }
    
    double ratio = (1.0 - static_cast<double>(compressed) / original) * 100.0;
    return QString("%1%").arg(ratio, 0, 'f', 1);
}

QString VirtualArchiveModel::formatPermissions(quint32 permissions) const {
    QString result;
    
    // Owner permissions
    result += (permissions & 0400) ? 'r' : '-';
    result += (permissions & 0200) ? 'w' : '-';
    result += (permissions & 0100) ? 'x' : '-';
    
    // Group permissions
    result += (permissions & 0040) ? 'r' : '-';
    result += (permissions & 0020) ? 'w' : '-';
    result += (permissions & 0010) ? 'x' : '-';
    
    // Other permissions
    result += (permissions & 0004) ? 'r' : '-';
    result += (permissions & 0002) ? 'w' : '-';
    result += (permissions & 0001) ? 'x' : '-';
    
    return result;
}

void VirtualArchiveModel::sortChildren(ArchiveNode* node) {
    if (!node || node->children.empty()) {
        return;
    }
    
    // Sort children: directories first, then files, both alphabetically
    std::sort(node->children.begin(), node->children.end(),
              [](const std::unique_ptr<ArchiveNode>& a, const std::unique_ptr<ArchiveNode>& b) {
                  if (a->is_directory != b->is_directory) {
                      return a->is_directory; // Directories first
                  }
                  
                  QCollator collator;
                  collator.setNumericMode(true);
                  return collator.compare(a->name, b->name) < 0;
              });
    
    // Recursively sort children
    for (const auto& child : node->children) {
        sortChildren(child.get());
    }
}

// ArchiveFilterProxyModel implementation
ArchiveFilterProxyModel::ArchiveFilterProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

void ArchiveFilterProxyModel::setNameFilter(const QString& pattern) {
    name_pattern_ = pattern;
    has_filters_ = !pattern.isEmpty() || !type_extensions_.isEmpty() || 
                   min_size_ > 0 || max_size_ < std::numeric_limits<quint64>::max() ||
                   directories_only_;
    invalidateFilter();
}

void ArchiveFilterProxyModel::setTypeFilter(const QStringList& extensions) {
    type_extensions_ = extensions;
    has_filters_ = !name_pattern_.isEmpty() || !extensions.isEmpty() || 
                   min_size_ > 0 || max_size_ < std::numeric_limits<quint64>::max() ||
                   directories_only_;
    invalidateFilter();
}

void ArchiveFilterProxyModel::setSizeFilter(quint64 min_size, quint64 max_size) {
    min_size_ = min_size;
    max_size_ = max_size;
    has_filters_ = !name_pattern_.isEmpty() || !type_extensions_.isEmpty() || 
                   min_size > 0 || max_size < std::numeric_limits<quint64>::max() ||
                   directories_only_;
    invalidateFilter();
}

void ArchiveFilterProxyModel::setShowDirectoriesOnly(bool directories_only) {
    directories_only_ = directories_only;
    has_filters_ = !name_pattern_.isEmpty() || !type_extensions_.isEmpty() || 
                   min_size_ > 0 || max_size_ < std::numeric_limits<quint64>::max() ||
                   directories_only;
    invalidateFilter();
}

void ArchiveFilterProxyModel::clearFilters() {
    name_pattern_.clear();
    type_extensions_.clear();
    min_size_ = 0;
    max_size_ = std::numeric_limits<quint64>::max();
    directories_only_ = false;
    has_filters_ = false;
    invalidateFilter();
}

bool ArchiveFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const {
    if (!has_filters_) {
        return true;
    }
    
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
    if (!index.isValid()) {
        return false;
    }
    
    // Check directory filter
    bool is_directory = index.data(VirtualArchiveModel::IsDirectoryRole).toBool();
    if (directories_only_ && !is_directory) {
        return false;
    }
    
    // Check name filter
    if (!name_pattern_.isEmpty()) {
        QString name = index.data(Qt::DisplayRole).toString();
        QRegularExpression regex(name_pattern_, QRegularExpression::CaseInsensitiveOption);
        if (!regex.match(name).hasMatch()) {
            return false;
        }
    }
    
    // Check type filter
    if (!type_extensions_.isEmpty() && !is_directory) {
        QString name = index.data(Qt::DisplayRole).toString();
        QString extension = QFileInfo(name).suffix().toLower();
        if (!type_extensions_.contains(extension, Qt::CaseInsensitive)) {
            return false;
        }
    }
    
    // Check size filter
    if (!is_directory) {
        quint64 size = index.data(VirtualArchiveModel::OriginalSizeRole).toULongLong();
        if (size < min_size_ || size > max_size_) {
            return false;
        }
    }
    
    return true;
}

bool ArchiveFilterProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const {
    // Custom sorting logic
    bool left_is_dir = left.data(VirtualArchiveModel::IsDirectoryRole).toBool();
    bool right_is_dir = right.data(VirtualArchiveModel::IsDirectoryRole).toBool();
    
    // Directories always come first
    if (left_is_dir != right_is_dir) {
        return left_is_dir;
    }
    
    // For size columns, use numeric comparison
    if (left.column() == VirtualArchiveModel::SizeColumn ||
        left.column() == VirtualArchiveModel::CompressedSizeColumn) {
        quint64 left_size = left.data(VirtualArchiveModel::OriginalSizeRole).toULongLong();
        quint64 right_size = right.data(VirtualArchiveModel::OriginalSizeRole).toULongLong();
        return left_size < right_size;
    }
    
    // Default string comparison
    return QSortFilterProxyModel::lessThan(left, right);
}

// ArchiveSelectionModel implementation
ArchiveSelectionModel::ArchiveSelectionModel(VirtualArchiveModel* model, QObject* parent)
    : QObject(parent), model_(model)
{
}

void ArchiveSelectionModel::selectByPattern(const QString& pattern, Qt::CaseSensitivity cs) {
    selected_paths_.clear();
    
    QRegularExpression regex(pattern, cs == Qt::CaseInsensitive ? 
                           QRegularExpression::CaseInsensitiveOption : 
                           QRegularExpression::NoPatternOption);
    
    // This would need to traverse the model to find matching entries
    // Implementation depends on how selection is managed in the view
    
    emit selectionChanged();
}

void ArchiveSelectionModel::selectAll() {
    selected_paths_.clear();
    
    // This would need to traverse the entire model
    // Implementation depends on the specific requirements
    
    emit selectionChanged();
}

void ArchiveSelectionModel::clearSelection() {
    selected_paths_.clear();
    emit selectionChanged();
}

void ArchiveSelectionModel::invertSelection() {
    // Implementation would invert the current selection
    emit selectionChanged();
}

QStringList ArchiveSelectionModel::getSelectedPaths() const {
    return selected_paths_.values();
}

ArchiveSelectionModel::SelectionStats ArchiveSelectionModel::getSelectionStats() const {
    SelectionStats stats;
    
    // Calculate statistics from selected paths
    // This would require accessing the model data
    
    return stats;
}

} // namespace FluxGui

#include "virtual_archive_model.moc"

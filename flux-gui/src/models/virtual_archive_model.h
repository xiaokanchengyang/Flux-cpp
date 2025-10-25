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

#pragma once

#include "../core/async_task_executor.h"
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QIcon>
#include <QMimeData>
#include <QStringList>
#include <QHash>
#include <QVector>
#include <QMutex>
#include <memory>
#include <unordered_map>

namespace FluxGui {

/**
 * @brief Tree node for archive entries
 */
struct ArchiveNode {
    QString name;
    QString full_path;
    quint64 size = 0;
    quint64 compressed_size = 0;
    bool is_directory = false;
    QString modification_time;
    quint32 permissions = 0;
    int depth = 0;
    
    std::vector<std::unique_ptr<ArchiveNode>> children;
    ArchiveNode* parent = nullptr;
    
    ArchiveNode() = default;
    explicit ArchiveNode(const QString& name, ArchiveNode* parent = nullptr)
        : name(name), parent(parent) {}
    
    ~ArchiveNode() = default;
    
    // Non-copyable but movable
    ArchiveNode(const ArchiveNode&) = delete;
    ArchiveNode& operator=(const ArchiveNode&) = delete;
    ArchiveNode(ArchiveNode&&) = default;
    ArchiveNode& operator=(ArchiveNode&&) = default;
    
    /**
     * @brief Find or create child node
     */
    ArchiveNode* findOrCreateChild(const QString& child_name);
    
    /**
     * @brief Get child by index
     */
    ArchiveNode* child(int index) const;
    
    /**
     * @brief Get child count
     */
    int childCount() const;
    
    /**
     * @brief Get row index of this node in parent
     */
    int row() const;
    
    /**
     * @brief Calculate total size including children
     */
    quint64 totalSize() const;
    
    /**
     * @brief Check if this node matches search criteria
     */
    bool matchesFilter(const QString& filter, Qt::CaseSensitivity cs = Qt::CaseInsensitive) const;
};

/**
 * @brief High-performance virtual model for archive contents
 * 
 * This model provides efficient handling of large archive files with
 * lazy loading, virtual scrolling, and hierarchical display.
 */
class VirtualArchiveModel : public QAbstractItemModel {
    Q_OBJECT

public:
    /**
     * @brief Column enumeration
     */
    enum Column {
        NameColumn = 0,
        SizeColumn,
        CompressedSizeColumn,
        RatioColumn,
        ModifiedColumn,
        PermissionsColumn,
        ColumnCount
    };

    /**
     * @brief Custom roles for additional data
     */
    enum Role {
        FullPathRole = Qt::UserRole + 1,
        IsDirectoryRole,
        OriginalSizeRole,
        CompressedSizeRole,
        CompressionRatioRole,
        ModificationTimeRole,
        PermissionsRole,
        DepthRole,
        NodePointerRole
    };

    explicit VirtualArchiveModel(QObject* parent = nullptr);
    ~VirtualArchiveModel() override;

    // QAbstractItemModel interface
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool hasChildren(const QModelIndex& parent = QModelIndex()) const override;
    bool canFetchMore(const QModelIndex& parent) const override;
    void fetchMore(const QModelIndex& parent) override;

    // Drag and drop support
    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;

    /**
     * @brief Load archive contents
     */
    void loadArchive(const QString& archive_path, const QString& password = QString{});

    /**
     * @brief Clear all data
     */
    void clear();

    /**
     * @brief Get archive information
     */
    const ArchiveInfo& archiveInfo() const { return archive_info_; }

    /**
     * @brief Check if model is loading
     */
    bool isLoading() const { return is_loading_; }

    /**
     * @brief Get total entry count
     */
    int totalEntryCount() const { return total_entries_; }

    /**
     * @brief Get node from model index
     */
    ArchiveNode* nodeFromIndex(const QModelIndex& index) const;

    /**
     * @brief Get model index from node
     */
    QModelIndex indexFromNode(ArchiveNode* node, int column = 0) const;

    /**
     * @brief Expand all directories up to specified depth
     */
    void expandToDepth(int depth);

    /**
     * @brief Get selected file paths
     */
    QStringList getSelectedPaths(const QModelIndexList& indexes) const;

public slots:
    /**
     * @brief Refresh the model data
     */
    void refresh();

signals:
    /**
     * @brief Emitted when loading starts
     */
    void loadingStarted();

    /**
     * @brief Emitted when loading finishes
     */
    void loadingFinished(bool success, const QString& message);

    /**
     * @brief Emitted when loading progress updates
     */
    void loadingProgress(int percentage);

private slots:
    void onArchiveContentsReady(const ArchiveInfo& info, const QList<ArchiveEntry>& entries);
    void onTaskFinished(bool success, const QString& message);

private:
    /**
     * @brief Build tree structure from flat entry list
     */
    void buildTree(const QList<ArchiveEntry>& entries);

    /**
     * @brief Create node from entry
     */
    std::unique_ptr<ArchiveNode> createNode(const ArchiveEntry& entry, ArchiveNode* parent);

    /**
     * @brief Get icon for file/directory
     */
    QIcon getIcon(const ArchiveNode* node) const;

    /**
     * @brief Format file size for display
     */
    QString formatSize(quint64 size) const;

    /**
     * @brief Format compression ratio
     */
    QString formatRatio(quint64 original, quint64 compressed) const;

    /**
     * @brief Format permissions for display
     */
    QString formatPermissions(quint32 permissions) const;

    /**
     * @brief Sort children of a node
     */
    void sortChildren(ArchiveNode* node);

private:
    std::unique_ptr<ArchiveNode> root_node_;
    ArchiveInfo archive_info_;
    QString current_archive_path_;
    QString current_password_;
    
    mutable QMutex mutex_;
    bool is_loading_ = false;
    int total_entries_ = 0;
    
    // Caching for performance
    mutable QHash<QString, QIcon> icon_cache_;
    
    // Task executor for async operations
    std::shared_ptr<TaskExecutor> task_executor_;
};

/**
 * @brief Proxy model for filtering and sorting archive contents
 */
class ArchiveFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit ArchiveFilterProxyModel(QObject* parent = nullptr);

    /**
     * @brief Set name filter pattern
     */
    void setNameFilter(const QString& pattern);

    /**
     * @brief Set file type filter
     */
    void setTypeFilter(const QStringList& extensions);

    /**
     * @brief Set size range filter
     */
    void setSizeFilter(quint64 min_size, quint64 max_size);

    /**
     * @brief Show only directories
     */
    void setShowDirectoriesOnly(bool directories_only);

    /**
     * @brief Clear all filters
     */
    void clearFilters();

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

private:
    QString name_pattern_;
    QStringList type_extensions_;
    quint64 min_size_ = 0;
    quint64 max_size_ = std::numeric_limits<quint64>::max();
    bool directories_only_ = false;
    bool has_filters_ = false;
};

/**
 * @brief Selection model for archive entries
 */
class ArchiveSelectionModel : public QObject {
    Q_OBJECT

public:
    explicit ArchiveSelectionModel(VirtualArchiveModel* model, QObject* parent = nullptr);

    /**
     * @brief Select entries by path patterns
     */
    void selectByPattern(const QString& pattern, Qt::CaseSensitivity cs = Qt::CaseInsensitive);

    /**
     * @brief Select all entries
     */
    void selectAll();

    /**
     * @brief Clear selection
     */
    void clearSelection();

    /**
     * @brief Invert selection
     */
    void invertSelection();

    /**
     * @brief Get selected paths
     */
    QStringList getSelectedPaths() const;

    /**
     * @brief Get selection statistics
     */
    struct SelectionStats {
        int file_count = 0;
        int directory_count = 0;
        quint64 total_size = 0;
        quint64 compressed_size = 0;
    };
    SelectionStats getSelectionStats() const;

signals:
    void selectionChanged();

private:
    VirtualArchiveModel* model_;
    QSet<QString> selected_paths_;
};

} // namespace FluxGui

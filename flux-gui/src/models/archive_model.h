#pragma once

#include <QAbstractItemModel>
#include <QFileInfo>
#include <QIcon>
#include <QDateTime>
#include <QMimeData>
#include <QStringList>
#include <QHash>

/**
 * 归档文件条目
 */
struct ArchiveEntry {
    QString name;           // 文件名
    QString path;           // 完整路径
    qint64 size;           // 文件大小（字节）
    qint64 compressedSize; // 压缩后大小（字节）
    QDateTime modified;     // 修改时间
    bool isDirectory;       // 是否为目录
    QString permissions;    // 权限字符串
    QString mimeType;       // MIME 类型
    QString comment;        // 文件注释
    uint32_t crc32;        // CRC32 校验值
    
    ArchiveEntry() 
        : size(0), compressedSize(0), isDirectory(false), crc32(0) {}
        
    // 计算压缩比
    double compressionRatio() const {
        if (size == 0) return 0.0;
        return (1.0 - static_cast<double>(compressedSize) / size) * 100.0;
    }
    
    // 获取文件扩展名
    QString extension() const {
        if (isDirectory) return QString();
        int dotIndex = name.lastIndexOf('.');
        return dotIndex >= 0 ? name.mid(dotIndex + 1).toLower() : QString();
    }
};

/**
 * 高性能归档模型
 * 
 * 特性：
 * - 支持大型归档文件（百万级文件）
 * - 虚拟化树形结构
 * - 拖拽支持
 * - 快速搜索和过滤
 * - 多列排序
 */
class ArchiveModel : public QAbstractItemModel {
    Q_OBJECT

public:
    enum Column {
        NameColumn = 0,
        SizeColumn,
        CompressedSizeColumn,
        RatioColumn,
        ModifiedColumn,
        PermissionsColumn,
        CrcColumn,
        ColumnCount
    };
    
    enum ItemRole {
        EntryRole = Qt::UserRole + 1,
        PathRole,
        IsDirectoryRole,
        SizeRole,
        CompressedSizeRole,
        IconRole
    };

    explicit ArchiveModel(QObject *parent = nullptr);
    ~ArchiveModel();

    // QAbstractItemModel 接口
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    
    // 拖拽支持
    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList &indexes) const override;
    Qt::DropActions supportedDropActions() const override;

    /**
     * 设置归档条目
     * @param entries 条目列表
     */
    void setEntries(const QList<ArchiveEntry>& entries);
    
    /**
     * 添加条目
     * @param entry 新条目
     */
    void addEntry(const ArchiveEntry& entry);
    
    /**
     * 移除条目
     * @param path 条目路径
     */
    void removeEntry(const QString& path);
    
    /**
     * 清空模型
     */
    void clear();
    
    /**
     * 获取指定索引的条目
     * @param index 模型索引
     * @return 归档条目指针，如果无效则返回nullptr
     */
    const ArchiveEntry* getEntry(const QModelIndex& index) const;
    
    /**
     * 根据路径查找条目
     * @param path 文件路径
     * @return 条目指针，如果不存在则返回nullptr
     */
    const ArchiveEntry* findEntry(const QString& path) const;
    
    /**
     * 获取选中条目的路径列表
     * @param indexes 选中的索引列表
     * @return 路径列表
     */
    QStringList getSelectedPaths(const QModelIndexList& indexes) const;
    
    /**
     * 设置过滤器
     * @param filter 过滤字符串
     */
    void setFilter(const QString& filter);
    
    /**
     * 获取统计信息
     */
    struct Statistics {
        int totalFiles;
        int totalDirectories;
        qint64 totalSize;
        qint64 totalCompressedSize;
        double averageCompressionRatio;
    };
    Statistics getStatistics() const;

signals:
    /**
     * 模型数据已更新
     */
    void dataUpdated();
    
    /**
     * 条目被双击
     */
    void entryDoubleClicked(const QString& path);

private slots:
    void onItemDoubleClicked(const QModelIndex& index);

private:
    struct TreeNode {
        ArchiveEntry entry;
        TreeNode* parent;
        QList<TreeNode*> children;
        bool isExpanded;
        
        TreeNode(TreeNode* p = nullptr) 
            : parent(p), isExpanded(false) {}
            
        ~TreeNode() { 
            qDeleteAll(children); 
        }
        
        // 查找子节点
        TreeNode* findChild(const QString& name) const {
            for (TreeNode* child : children) {
                if (child->entry.name == name) {
                    return child;
                }
            }
            return nullptr;
        }
        
        // 添加子节点（保持排序）
        void addChild(TreeNode* child) {
            child->parent = this;
            
            // 目录优先，然后按名称排序
            int insertPos = 0;
            for (int i = 0; i < children.size(); ++i) {
                TreeNode* existing = children[i];
                
                // 目录优先
                if (child->entry.isDirectory && !existing->entry.isDirectory) {
                    break;
                }
                if (!child->entry.isDirectory && existing->entry.isDirectory) {
                    insertPos = i + 1;
                    continue;
                }
                
                // 同类型按名称排序
                if (child->entry.name.compare(existing->entry.name, Qt::CaseInsensitive) < 0) {
                    break;
                }
                insertPos = i + 1;
            }
            
            children.insert(insertPos, child);
        }
    };

    void buildTree(const QList<ArchiveEntry>& entries);
    TreeNode* findOrCreateNode(const QString& path);
    TreeNode* getNode(const QModelIndex& index) const;
    QIcon getFileIcon(const ArchiveEntry& entry) const;
    QString formatSize(qint64 size) const;
    QString formatDateTime(const QDateTime& dateTime) const;
    bool matchesFilter(const TreeNode* node) const;

private:
    TreeNode* m_rootNode;
    QHash<QString, TreeNode*> m_pathToNode; // 路径到节点的快速查找
    QString m_filterString;
    mutable QHash<QString, QIcon> m_iconCache; // 图标缓存
    
    // 统计信息缓存
    mutable Statistics m_cachedStats;
    mutable bool m_statsValid;
};
#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTreeView>
#include <QTextEdit>
#include <QLabel>
#include <QToolBar>
#include <QAction>
#include <QLineEdit>
#include <QProgressBar>
#include <QGroupBox>
#include <QTableWidget>
#include <QTabWidget>
#include <QScrollArea>
#include <QTimer>
#include <QFileSystemWatcher>

class ArchiveModel;
class QSortFilterProxyModel;

/**
 * 归档浏览器页面
 * 
 * 功能特性：
 * - 高性能树形视图显示归档内容
 * - 实时搜索和过滤
 * - 文件预览（文本、图片）
 * - 拖拽提取支持
 * - 详细的文件信息面板
 * - 统计信息显示
 */
class BrowsePage : public QWidget {
    Q_OBJECT

public:
    explicit BrowsePage(QWidget *parent = nullptr);
    ~BrowsePage();

    /**
     * 打开归档文件
     * @param archivePath 归档文件路径
     */
    void openArchive(const QString& archivePath);
    
    /**
     * 关闭当前归档
     */
    void closeArchive();
    
    /**
     * 获取当前归档路径
     */
    QString currentArchivePath() const { return m_currentArchivePath; }
    
    /**
     * 获取选中的文件路径列表
     */
    QStringList getSelectedPaths() const;

signals:
    /**
     * 请求提取文件
     * @param archivePath 归档路径
     * @param filePaths 要提取的文件路径列表
     * @param outputDir 输出目录
     */
    void extractRequested(const QString& archivePath, const QStringList& filePaths, const QString& outputDir);
    
    /**
     * 请求添加文件到归档
     * @param archivePath 归档路径
     * @param filePaths 要添加的文件路径列表
     */
    void addFilesRequested(const QString& archivePath, const QStringList& filePaths);
    
    /**
     * 请求删除文件
     * @param archivePath 归档路径
     * @param filePaths 要删除的文件路径列表
     */
    void deleteFilesRequested(const QString& archivePath, const QStringList& filePaths);
    
    /**
     * 请求预览文件
     * @param archivePath 归档路径
     * @param filePath 文件路径
     */
    void previewRequested(const QString& archivePath, const QString& filePath);

public slots:
    /**
     * 设置归档条目数据
     * @param entries 条目列表
     */
    void setArchiveEntries(const QList<struct ArchiveEntry>& entries);
    
    /**
     * 设置预览内容
     * @param filePath 文件路径
     * @param content 文件内容
     * @param mimeType MIME 类型
     */
    void setPreviewContent(const QString& filePath, const QByteArray& content, const QString& mimeType);
    
    /**
     * 显示错误信息
     * @param message 错误消息
     */
    void showError(const QString& message);

private slots:
    void onTreeSelectionChanged();
    void onTreeDoubleClicked(const QModelIndex& index);
    void onTreeContextMenu(const QPoint& pos);
    void onSearchTextChanged(const QString& text);
    void onExtractSelected();
    void onAddFiles();
    void onDeleteSelected();
    void onRefresh();
    void onExpandAll();
    void onCollapseAll();
    void onCopyPath();
    void onProperties();
    void updateStatistics();
    void onPreviewTimer();

private:
    void setupUI();
    void setupToolBar();
    void setupTreeView();
    void setupPreviewPanel();
    void setupInfoPanel();
    void setupStatusPanel();
    void updateActions();
    void updateFileInfo(const struct ArchiveEntry* entry);
    void clearPreview();
    void showImagePreview(const QByteArray& data);
    void showTextPreview(const QByteArray& data, const QString& encoding = "UTF-8");
    void showHexPreview(const QByteArray& data);
    QString detectTextEncoding(const QByteArray& data) const;
    void resizeTreeColumns();

private:
    // 布局组件
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_toolbarLayout;
    QSplitter* m_mainSplitter;
    QSplitter* m_rightSplitter;
    
    // 工具栏
    QToolBar* m_toolBar;
    QLineEdit* m_searchEdit;
    QAction* m_extractAction;
    QAction* m_addAction;
    QAction* m_deleteAction;
    QAction* m_refreshAction;
    QAction* m_expandAllAction;
    QAction* m_collapseAllAction;
    QAction* m_propertiesAction;
    
    // 主视图
    QTreeView* m_treeView;
    ArchiveModel* m_archiveModel;
    QSortFilterProxyModel* m_proxyModel;
    
    // 预览面板
    QTabWidget* m_previewTabs;
    QTextEdit* m_textPreview;
    QLabel* m_imagePreview;
    QScrollArea* m_imageScrollArea;
    QTextEdit* m_hexPreview;
    QLabel* m_previewStatus;
    
    // 信息面板
    QGroupBox* m_infoGroup;
    QTableWidget* m_infoTable;
    
    // 统计面板
    QGroupBox* m_statsGroup;
    QLabel* m_totalFilesLabel;
    QLabel* m_totalSizeLabel;
    QLabel* m_compressionRatioLabel;
    QProgressBar* m_compressionBar;
    
    // 状态
    QString m_currentArchivePath;
    QString m_currentPreviewPath;
    QTimer* m_previewTimer;
    QTimer* m_searchTimer;
    
    // 设置
    bool m_autoPreview;
    int m_maxPreviewSize;
};


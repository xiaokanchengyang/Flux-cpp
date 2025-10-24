#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QListWidget>
#include <QSplitter>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <memory>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QDragEnterEvent;
class QDropEvent;
QT_END_NAMESPACE

// 前向声明视图类
class WelcomeView;
class PackView;
class ExtractView;
class ArchiveBrowserView;
class SettingsView;
class TaskManager;
class ThemeManager;

/**
 * 主窗口类
 * 
 * 采用现代化的侧边栏导航 + 中心内容区布局
 * 支持拖放操作、多主题、国际化等功能
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    /**
     * 打开归档文件
     * @param filePath 归档文件路径
     */
    void openArchive(const QString& filePath);

protected:
    // 拖放事件处理
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    
    // 窗口事件处理
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    // 菜单和工具栏动作
    void onNewArchive();
    void onOpenArchive();
    void onExtractArchive();
    void onSettings();
    void onAbout();
    void onExit();
    
    // 视图切换
    void onNavigationItemClicked(int index);
    
    // 主题切换
    void onThemeChanged();
    
    // 任务管理
    void onTaskStarted(const QString& taskName);
    void onTaskProgress(int percentage, const QString& status);
    void onTaskFinished(bool success, const QString& message);
    
    // 状态更新
    void updateStatusBar();

private:
    // UI 初始化
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupNavigationPanel();
    void setupCentralWidget();
    void setupConnections();
    
    // 样式设置
    void applyStyles();
    void updateNavigationStyles();
    
    // 工具方法
    void switchToView(int viewIndex);
    void showWelcomeView();
    void showPackView();
    void showExtractView();
    void showArchiveBrowserView();
    void showSettingsView();
    
    // 文件操作
    void handleDroppedFiles(const QStringList& filePaths);
    bool isArchiveFile(const QString& filePath);
    
    // 设置管理
    void loadSettings();
    void saveSettings();

private:
    // 主要布局组件
    QSplitter* m_mainSplitter;
    QListWidget* m_navigationList;
    QStackedWidget* m_contentStack;
    
    // 视图组件
    std::unique_ptr<WelcomeView> m_welcomeView;
    std::unique_ptr<PackView> m_packView;
    std::unique_ptr<ExtractView> m_extractView;
    std::unique_ptr<ArchiveBrowserView> m_archiveBrowserView;
    std::unique_ptr<SettingsView> m_settingsView;
    
    // 菜单和工具栏
    QMenu* m_fileMenu;
    QMenu* m_toolsMenu;
    QMenu* m_viewMenu;
    QMenu* m_helpMenu;
    
    QToolBar* m_mainToolBar;
    
    QAction* m_newArchiveAction;
    QAction* m_openArchiveAction;
    QAction* m_extractArchiveAction;
    QAction* m_settingsAction;
    QAction* m_exitAction;
    QAction* m_aboutAction;
    
    // 状态栏组件
    QLabel* m_statusLabel;
    QProgressBar* m_progressBar;
    QLabel* m_taskLabel;
    
    // 管理器
    std::unique_ptr<TaskManager> m_taskManager;
    
    // 状态变量
    QString m_currentArchivePath;
    int m_currentViewIndex;
    
    // 定时器
    QTimer* m_statusUpdateTimer;
    
    // 常量
    static constexpr int NAVIGATION_WIDTH = 200;
    static constexpr int MIN_WINDOW_WIDTH = 1000;
    static constexpr int MIN_WINDOW_HEIGHT = 700;
    
    // 视图索引枚举
    enum ViewIndex {
        WelcomeViewIndex = 0,
        PackViewIndex = 1,
        ExtractViewIndex = 2,
        ArchiveBrowserViewIndex = 3,
        SettingsViewIndex = 4
    };
};














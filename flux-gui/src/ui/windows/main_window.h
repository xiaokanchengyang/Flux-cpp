#pragma once

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QStackedWidget>
#include <QLabel>
#include <QListWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QSplitter>
#include <QProgressBar>
#include <QThread>
#include <QTimer>
#include <QMutex>
#include <QVariantMap>
#include <memory>
#include <functional>
#include <optional>

// Forward declarations
class HomePage;
class PackPage;
class BrowsePage;
class SettingsPage;
class WorkerThread;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    // Modern C++ features
    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
    MainWindow(MainWindow&&) = delete;
    MainWindow& operator=(MainWindow&&) = delete;

private slots:
    // Navigation slot functions
    void onNavigationItemClicked(int index);
    
    // Menu and toolbar slot functions
    void onNewArchive();
    void onOpenArchive();
    void onExtractArchive();
    void onAbout();
    void onSettings();
    
    // Background task communication slot functions
    void onProgressUpdated(const QString& currentFile, float percentage);
    void onTaskFinished(bool success, const QString& message);
    void onTaskStarted(const QString& taskName);
    
    // Other slot functions
    void onRecentFileRequested(const QString& filePath);
    void onThemeChanged();

private:
    void setupUI();
    void setupMenus();
    void setupToolBar();
    void setupStatusBar();
    void setupCentralWidget();
    void setupSidebar();
    void setupWorkerThread();
    void showView(int index);
    void updateStatusMessage(const QString& message);
    void loadStyleSheet();
    void setupWindowEffects();

private:
    // UI components using smart pointers where appropriate
    QMenuBar* m_menuBar{nullptr};
    QToolBar* m_toolBar{nullptr};
    QStatusBar* m_statusBar{nullptr};
    QWidget* m_centralWidget{nullptr};
    QHBoxLayout* m_mainLayout{nullptr};
    QSplitter* m_splitter{nullptr};
    
    // Sidebar navigation
    QWidget* m_sidebarWidget{nullptr};
    QListWidget* m_navigationList{nullptr};
    QVBoxLayout* m_sidebarLayout{nullptr};
    
    // Main view area
    QStackedWidget* m_stackedWidget{nullptr};
    
    // Status bar components
    QLabel* m_statusLabel{nullptr};
    QProgressBar* m_progressBar{nullptr};
    QLabel* m_taskLabel{nullptr};
    
    // View pages using smart pointers
    std::unique_ptr<HomePage> m_homePage;
    std::unique_ptr<PackPage> m_packPage;
    std::unique_ptr<::BrowsePage> m_browsePage;  // Use global namespace BrowsePage
    std::unique_ptr<SettingsPage> m_settingsPage;
    
    // Background task thread
    std::unique_ptr<WorkerThread> m_workerThread;
    
    // View indices using scoped enum
    enum class ViewIndex : int {
        Home = 0,
        Pack,
        Browse,
        Settings
    };
    
    // Current theme state
    bool m_isDarkTheme{false};
};

// Background task thread class using modern C++ features
class WorkerThread : public QThread {
    Q_OBJECT

public:
    explicit WorkerThread(QObject* parent = nullptr);
    ~WorkerThread() override = default;
    
    // Delete copy and move constructors/operators
    WorkerThread(const WorkerThread&) = delete;
    WorkerThread& operator=(const WorkerThread&) = delete;
    WorkerThread(WorkerThread&&) = delete;
    WorkerThread& operator=(WorkerThread&&) = delete;
    
    // Task type enumeration using scoped enum
    enum class TaskType : int {
        Extract,
        Pack,
        List,
        Benchmark
    };
    
    // Task management functions
    void startTask(TaskType type, const QVariantMap& parameters);
    void stopTask();
    [[nodiscard]] bool isRunning() const;

signals:
    // Progress update signals
    void progressUpdated(const QString& currentFile, float percentage);
    
    // Task completion signal
    void taskFinished(bool success, const QString& message);
    
    // Task start signal
    void taskStarted(const QString& taskName);
    
    // Data signals
    void archiveListReady(const QStringList& fileList);
    void benchmarkResultReady(const QVariantMap& results);

protected:
    void run() override;

private:
    // Task execution functions using functional programming patterns
    void executeExtractTask(const QVariantMap& params);
    void executePackTask(const QVariantMap& params);
    void executeListTask(const QVariantMap& params);
    void executeBenchmarkTask(const QVariantMap& params);
    
    // Task execution dispatcher using std::function
    using TaskExecutor = std::function<void(const QVariantMap&)>;
    std::optional<TaskExecutor> getTaskExecutor(TaskType type);
    
private:
    TaskType m_currentTask{TaskType::Extract};
    QVariantMap m_taskParameters{};
    std::atomic<bool> m_shouldStop{false};
    mutable QMutex m_mutex{};
};
#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QGroupBox>

/**
 * 打包视图
 * 
 * 提供创建归档文件的功能，包括：
 * - 文件列表管理
 * - 格式选择
 * - 压缩设置
 * - 输出路径设置
 * - 进度显示
 */
class PackView : public QWidget {
    Q_OBJECT

public:
    explicit PackView(QWidget *parent = nullptr);
    
    /**
     * 添加文件到打包列表
     * @param filePaths 文件路径列表
     */
    void addFiles(const QStringList& filePaths);

signals:
    void packingStarted();
    void packingFinished(bool success, const QString& message);

private slots:
    void onAddFiles();
    void onAddFolder();
    void onRemoveSelected();
    void onClearAll();
    void onBrowseOutput();
    void onStartPacking();
    void onFormatChanged();

private:
    void setupUI();
    void setupFileList();
    void setupSettings();
    void setupActions();
    void updateUI();
    void validateSettings();

private:
    QVBoxLayout* m_mainLayout;
    
    // 文件列表区域
    QGroupBox* m_fileListGroup;
    QListWidget* m_fileList;
    QHBoxLayout* m_fileButtonsLayout;
    QPushButton* m_addFilesButton;
    QPushButton* m_addFolderButton;
    QPushButton* m_removeButton;
    QPushButton* m_clearButton;
    
    // 设置区域
    QGroupBox* m_settingsGroup;
    QComboBox* m_formatCombo;
    QSpinBox* m_compressionSpin;
    QSpinBox* m_threadsSpin;
    QLineEdit* m_passwordEdit;
    QLineEdit* m_outputEdit;
    QPushButton* m_browseButton;
    
    // 操作区域
    QHBoxLayout* m_actionLayout;
    QPushButton* m_packButton;
    QPushButton* m_cancelButton;
    
    // 进度显示
    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;
};





#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>

/**
 * Welcome View
 * 
 * Provides the main entry point for the application, including:
 * - Drag and drop area
 * - Quick action buttons
 * - Recently used files
 * - Application information
 */
class WelcomeView : public QWidget {
    Q_OBJECT

public:
    explicit WelcomeView(QWidget *parent = nullptr);

signals:
    void createArchiveRequested();
    void openArchiveRequested();
    void extractArchiveRequested();
    void recentFileRequested(const QString& filePath);

private slots:
    void onCreateArchive();
    void onOpenArchive();
    void onExtractArchive();
    void onRecentFileClicked();

private:
    void setupUI();
    void setupDropZone();
    void setupQuickActions();
    void setupRecentFiles();
    void setupInfoSection();
    void updateRecentFiles();

private:
    QVBoxLayout* m_mainLayout;
    
    // Drag and drop area
    QFrame* m_dropZone;
    QLabel* m_dropLabel;
    
    // Quick actions
    QFrame* m_quickActionsFrame;
    QPushButton* m_createButton;
    QPushButton* m_openButton;
    QPushButton* m_extractButton;
    
    // Recent files
    QFrame* m_recentFilesFrame;
    QVBoxLayout* m_recentFilesLayout;
    
    // Information area
    QFrame* m_infoFrame;
    QLabel* m_versionLabel;
    QLabel* m_supportedFormatsLabel;
};





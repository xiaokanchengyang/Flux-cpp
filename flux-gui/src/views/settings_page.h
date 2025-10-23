#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QSlider>
#include <QLineEdit>
#include <QSettings>

/**
 * Settings Page
 * 
 * Provides configuration options for the application, including:
 * - Theme selection (light/dark)
 * - Performance settings
 * - Default compression settings
 * - File associations
 * - Language preferences
 */
class SettingsPage : public QWidget {
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = nullptr);
    
    /**
     * Load settings from QSettings
     */
    void loadSettings();
    
    /**
     * Save current settings to QSettings
     */
    void saveSettings();

signals:
    void themeChanged(const QString& theme);
    void settingsChanged();

private slots:
    void onThemeChanged();
    void onCompressionLevelChanged();
    void onThreadCountChanged();
    void onResetDefaults();
    void onApplySettings();

private:
    void setupUI();
    void setupAppearanceGroup();
    void setupPerformanceGroup();
    void setupCompressionGroup();
    void setupAdvancedGroup();
    void setupActionButtons();
    void updateUI();
    void applyTheme(const QString& theme);

private:
    QVBoxLayout* m_mainLayout;
    
    // Appearance settings
    QGroupBox* m_appearanceGroup;
    QComboBox* m_themeCombo;
    QComboBox* m_languageCombo;
    QCheckBox* m_showToolTipsCheck;
    QCheckBox* m_showStatusBarCheck;
    
    // Performance settings
    QGroupBox* m_performanceGroup;
    QSpinBox* m_maxThreadsSpin;
    QSpinBox* m_memoryCacheSpin;
    QCheckBox* m_enablePreviewCheck;
    QSlider* m_previewDelaySlider;
    QLabel* m_previewDelayLabel;
    
    // Compression settings
    QGroupBox* m_compressionGroup;
    QComboBox* m_defaultFormatCombo;
    QSlider* m_defaultCompressionSlider;
    QLabel* m_compressionLevelLabel;
    QCheckBox* m_smartCompressionCheck;
    
    // Advanced settings
    QGroupBox* m_advancedGroup;
    QCheckBox* m_enableLoggingCheck;
    QComboBox* m_logLevelCombo;
    QLineEdit* m_tempDirEdit;
    QPushButton* m_browseTempDirButton;
    QCheckBox* m_associateFilesCheck;
    
    // Action buttons
    QHBoxLayout* m_buttonLayout;
    QPushButton* m_resetButton;
    QPushButton* m_applyButton;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
    
    // Settings storage
    QSettings* m_settings;
    
    // Original values for cancel functionality
    QString m_originalTheme;
    int m_originalCompressionLevel;
    int m_originalThreadCount;
};

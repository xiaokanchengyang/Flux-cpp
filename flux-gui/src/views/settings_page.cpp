#include "settings_page.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <spdlog/spdlog.h>

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_settings(new QSettings("FluxArchiveManager", "FluxGUI", this))
{
    setupUI();
    loadSettings();
}

void SettingsPage::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(15);
    m_mainLayout->setContentsMargins(20, 20, 20, 20);
    
    setupAppearanceGroup();
    setupPerformanceGroup();
    setupCompressionGroup();
    setupAdvancedGroup();
    setupActionButtons();
    
    m_mainLayout->addStretch();
}

void SettingsPage::setupAppearanceGroup() {
    m_appearanceGroup = new QGroupBox("Appearance");
    QGridLayout* layout = new QGridLayout(m_appearanceGroup);
    
    // Theme selection
    layout->addWidget(new QLabel("Theme:"), 0, 0);
    m_themeCombo = new QComboBox();
    m_themeCombo->addItems({"Light", "Dark", "System"});
    layout->addWidget(m_themeCombo, 0, 1);
    
    // Language selection
    layout->addWidget(new QLabel("Language:"), 1, 0);
    m_languageCombo = new QComboBox();
    m_languageCombo->addItems({"English", "Chinese (Simplified)", "French", "German", "Japanese"});
    layout->addWidget(m_languageCombo, 1, 1);
    
    // UI options
    m_showToolTipsCheck = new QCheckBox("Show tooltips");
    layout->addWidget(m_showToolTipsCheck, 2, 0, 1, 2);
    
    m_showStatusBarCheck = new QCheckBox("Show status bar");
    layout->addWidget(m_showStatusBarCheck, 3, 0, 1, 2);
    
    // Connect signals
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsPage::onThemeChanged);
    
    m_mainLayout->addWidget(m_appearanceGroup);
}

void SettingsPage::setupPerformanceGroup() {
    m_performanceGroup = new QGroupBox("Performance");
    QGridLayout* layout = new QGridLayout(m_performanceGroup);
    
    // Max threads
    layout->addWidget(new QLabel("Max Threads:"), 0, 0);
    m_maxThreadsSpin = new QSpinBox();
    m_maxThreadsSpin->setRange(1, 32);
    m_maxThreadsSpin->setValue(QThread::idealThreadCount());
    m_maxThreadsSpin->setSpecialValueText("Auto");
    layout->addWidget(m_maxThreadsSpin, 0, 1);
    
    // Memory cache
    layout->addWidget(new QLabel("Memory Cache (MB):"), 1, 0);
    m_memoryCacheSpin = new QSpinBox();
    m_memoryCacheSpin->setRange(64, 2048);
    m_memoryCacheSpin->setValue(256);
    m_memoryCacheSpin->setSuffix(" MB");
    layout->addWidget(m_memoryCacheSpin, 1, 1);
    
    // Preview settings
    m_enablePreviewCheck = new QCheckBox("Enable file preview");
    layout->addWidget(m_enablePreviewCheck, 2, 0, 1, 2);
    
    layout->addWidget(new QLabel("Preview Delay:"), 3, 0);
    QHBoxLayout* previewLayout = new QHBoxLayout();
    m_previewDelaySlider = new QSlider(Qt::Horizontal);
    m_previewDelaySlider->setRange(100, 2000);
    m_previewDelaySlider->setValue(500);
    m_previewDelayLabel = new QLabel("500ms");
    previewLayout->addWidget(m_previewDelaySlider);
    previewLayout->addWidget(m_previewDelayLabel);
    layout->addLayout(previewLayout, 3, 1);
    
    // Connect signals
    connect(m_previewDelaySlider, &QSlider::valueChanged, [this](int value) {
        m_previewDelayLabel->setText(QString("%1ms").arg(value));
    });
    
    m_mainLayout->addWidget(m_performanceGroup);
}

void SettingsPage::setupCompressionGroup() {
    m_compressionGroup = new QGroupBox("Default Compression Settings");
    QGridLayout* layout = new QGridLayout(m_compressionGroup);
    
    // Default format
    layout->addWidget(new QLabel("Default Format:"), 0, 0);
    m_defaultFormatCombo = new QComboBox();
    m_defaultFormatCombo->addItems({"ZIP", "TAR.ZSTD", "TAR.GZ", "TAR.XZ", "7Z"});
    layout->addWidget(m_defaultFormatCombo, 0, 1);
    
    // Compression level
    layout->addWidget(new QLabel("Compression Level:"), 1, 0);
    QHBoxLayout* compressionLayout = new QHBoxLayout();
    m_defaultCompressionSlider = new QSlider(Qt::Horizontal);
    m_defaultCompressionSlider->setRange(0, 9);
    m_defaultCompressionSlider->setValue(6);
    m_compressionLevelLabel = new QLabel("6 (Balanced)");
    compressionLayout->addWidget(m_defaultCompressionSlider);
    compressionLayout->addWidget(m_compressionLevelLabel);
    layout->addLayout(compressionLayout, 1, 1);
    
    // Smart compression
    m_smartCompressionCheck = new QCheckBox("Smart compression (skip already compressed files)");
    layout->addWidget(m_smartCompressionCheck, 2, 0, 1, 2);
    
    // Connect signals
    connect(m_defaultCompressionSlider, &QSlider::valueChanged,
            this, &SettingsPage::onCompressionLevelChanged);
    
    m_mainLayout->addWidget(m_compressionGroup);
}

void SettingsPage::setupAdvancedGroup() {
    m_advancedGroup = new QGroupBox("Advanced");
    QGridLayout* layout = new QGridLayout(m_advancedGroup);
    
    // Logging settings
    m_enableLoggingCheck = new QCheckBox("Enable logging");
    layout->addWidget(m_enableLoggingCheck, 0, 0, 1, 2);
    
    layout->addWidget(new QLabel("Log Level:"), 1, 0);
    m_logLevelCombo = new QComboBox();
    m_logLevelCombo->addItems({"Error", "Warning", "Info", "Debug", "Trace"});
    m_logLevelCombo->setCurrentText("Info");
    layout->addWidget(m_logLevelCombo, 1, 1);
    
    // Temporary directory
    layout->addWidget(new QLabel("Temp Directory:"), 2, 0);
    QHBoxLayout* tempDirLayout = new QHBoxLayout();
    m_tempDirEdit = new QLineEdit();
    m_tempDirEdit->setText(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    m_browseTempDirButton = new QPushButton("Browse...");
    tempDirLayout->addWidget(m_tempDirEdit);
    tempDirLayout->addWidget(m_browseTempDirButton);
    layout->addLayout(tempDirLayout, 2, 1);
    
    // File associations
    m_associateFilesCheck = new QCheckBox("Associate with archive file types");
    layout->addWidget(m_associateFilesCheck, 3, 0, 1, 2);
    
    // Connect signals
    connect(m_browseTempDirButton, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Temporary Directory",
                                                       m_tempDirEdit->text());
        if (!dir.isEmpty()) {
            m_tempDirEdit->setText(dir);
        }
    });
    
    m_mainLayout->addWidget(m_advancedGroup);
}

void SettingsPage::setupActionButtons() {
    m_buttonLayout = new QHBoxLayout();
    
    m_resetButton = new QPushButton("Reset to Defaults");
    m_applyButton = new QPushButton("Apply");
    m_okButton = new QPushButton("OK");
    m_cancelButton = new QPushButton("Cancel");
    
    m_buttonLayout->addWidget(m_resetButton);
    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_applyButton);
    m_buttonLayout->addWidget(m_okButton);
    m_buttonLayout->addWidget(m_cancelButton);
    
    // Connect signals
    connect(m_resetButton, &QPushButton::clicked, this, &SettingsPage::onResetDefaults);
    connect(m_applyButton, &QPushButton::clicked, this, &SettingsPage::onApplySettings);
    connect(m_okButton, &QPushButton::clicked, [this]() {
        onApplySettings();
        if (parentWidget()) {
            parentWidget()->close();
        }
    });
    connect(m_cancelButton, &QPushButton::clicked, [this]() {
        loadSettings(); // Restore original values
        if (parentWidget()) {
            parentWidget()->close();
        }
    });
    
    m_mainLayout->addLayout(m_buttonLayout);
}

void SettingsPage::loadSettings() {
    // Store original values
    m_originalTheme = m_settings->value("appearance/theme", "System").toString();
    m_originalCompressionLevel = m_settings->value("compression/level", 6).toInt();
    m_originalThreadCount = m_settings->value("performance/threads", QThread::idealThreadCount()).toInt();
    
    // Load appearance settings
    m_themeCombo->setCurrentText(m_originalTheme);
    m_languageCombo->setCurrentText(m_settings->value("appearance/language", "English").toString());
    m_showToolTipsCheck->setChecked(m_settings->value("appearance/tooltips", true).toBool());
    m_showStatusBarCheck->setChecked(m_settings->value("appearance/statusbar", true).toBool());
    
    // Load performance settings
    m_maxThreadsSpin->setValue(m_originalThreadCount);
    m_memoryCacheSpin->setValue(m_settings->value("performance/cache", 256).toInt());
    m_enablePreviewCheck->setChecked(m_settings->value("performance/preview", true).toBool());
    m_previewDelaySlider->setValue(m_settings->value("performance/preview_delay", 500).toInt());
    
    // Load compression settings
    m_defaultFormatCombo->setCurrentText(m_settings->value("compression/format", "ZIP").toString());
    m_defaultCompressionSlider->setValue(m_originalCompressionLevel);
    m_smartCompressionCheck->setChecked(m_settings->value("compression/smart", true).toBool());
    
    // Load advanced settings
    m_enableLoggingCheck->setChecked(m_settings->value("advanced/logging", true).toBool());
    m_logLevelCombo->setCurrentText(m_settings->value("advanced/log_level", "Info").toString());
    m_tempDirEdit->setText(m_settings->value("advanced/temp_dir", 
                                           QStandardPaths::writableLocation(QStandardPaths::TempLocation)).toString());
    m_associateFilesCheck->setChecked(m_settings->value("advanced/file_associations", false).toBool());
    
    updateUI();
}

void SettingsPage::saveSettings() {
    // Save appearance settings
    m_settings->setValue("appearance/theme", m_themeCombo->currentText());
    m_settings->setValue("appearance/language", m_languageCombo->currentText());
    m_settings->setValue("appearance/tooltips", m_showToolTipsCheck->isChecked());
    m_settings->setValue("appearance/statusbar", m_showStatusBarCheck->isChecked());
    
    // Save performance settings
    m_settings->setValue("performance/threads", m_maxThreadsSpin->value());
    m_settings->setValue("performance/cache", m_memoryCacheSpin->value());
    m_settings->setValue("performance/preview", m_enablePreviewCheck->isChecked());
    m_settings->setValue("performance/preview_delay", m_previewDelaySlider->value());
    
    // Save compression settings
    m_settings->setValue("compression/format", m_defaultFormatCombo->currentText());
    m_settings->setValue("compression/level", m_defaultCompressionSlider->value());
    m_settings->setValue("compression/smart", m_smartCompressionCheck->isChecked());
    
    // Save advanced settings
    m_settings->setValue("advanced/logging", m_enableLoggingCheck->isChecked());
    m_settings->setValue("advanced/log_level", m_logLevelCombo->currentText());
    m_settings->setValue("advanced/temp_dir", m_tempDirEdit->text());
    m_settings->setValue("advanced/file_associations", m_associateFilesCheck->isChecked());
    
    m_settings->sync();
    
    spdlog::info("Settings saved successfully");
}

void SettingsPage::onThemeChanged() {
    QString theme = m_themeCombo->currentText();
    applyTheme(theme);
    emit themeChanged(theme);
}

void SettingsPage::onCompressionLevelChanged() {
    int level = m_defaultCompressionSlider->value();
    QString description;
    
    switch (level) {
        case 0: description = "0 (Store only)"; break;
        case 1: description = "1 (Fastest)"; break;
        case 2: description = "2 (Fast)"; break;
        case 3: description = "3 (Fast)"; break;
        case 4: description = "4 (Normal)"; break;
        case 5: description = "5 (Normal)"; break;
        case 6: description = "6 (Balanced)"; break;
        case 7: description = "7 (Good)"; break;
        case 8: description = "8 (Better)"; break;
        case 9: description = "9 (Best)"; break;
        default: description = QString("%1").arg(level); break;
    }
    
    m_compressionLevelLabel->setText(description);
}

void SettingsPage::onThreadCountChanged() {
    // This could be used for real-time updates if needed
}

void SettingsPage::onResetDefaults() {
    int ret = QMessageBox::question(this, "Reset Settings",
                                   "Are you sure you want to reset all settings to their default values?",
                                   QMessageBox::Yes | QMessageBox::No,
                                   QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        // Reset to default values
        m_themeCombo->setCurrentText("System");
        m_languageCombo->setCurrentText("English");
        m_showToolTipsCheck->setChecked(true);
        m_showStatusBarCheck->setChecked(true);
        
        m_maxThreadsSpin->setValue(QThread::idealThreadCount());
        m_memoryCacheSpin->setValue(256);
        m_enablePreviewCheck->setChecked(true);
        m_previewDelaySlider->setValue(500);
        
        m_defaultFormatCombo->setCurrentText("ZIP");
        m_defaultCompressionSlider->setValue(6);
        m_smartCompressionCheck->setChecked(true);
        
        m_enableLoggingCheck->setChecked(true);
        m_logLevelCombo->setCurrentText("Info");
        m_tempDirEdit->setText(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
        m_associateFilesCheck->setChecked(false);
        
        updateUI();
        spdlog::info("Settings reset to defaults");
    }
}

void SettingsPage::onApplySettings() {
    saveSettings();
    emit settingsChanged();
    
    // Apply logging level immediately
    QString logLevel = m_logLevelCombo->currentText();
    if (logLevel == "Error") {
        spdlog::set_level(spdlog::level::err);
    } else if (logLevel == "Warning") {
        spdlog::set_level(spdlog::level::warn);
    } else if (logLevel == "Info") {
        spdlog::set_level(spdlog::level::info);
    } else if (logLevel == "Debug") {
        spdlog::set_level(spdlog::level::debug);
    } else if (logLevel == "Trace") {
        spdlog::set_level(spdlog::level::trace);
    }
    
    QMessageBox::information(this, "Settings Applied", 
                           "Settings have been applied successfully. Some changes may require a restart to take effect.");
}

void SettingsPage::updateUI() {
    // Update dependent controls
    m_logLevelCombo->setEnabled(m_enableLoggingCheck->isChecked());
    m_previewDelaySlider->setEnabled(m_enablePreviewCheck->isChecked());
    m_previewDelayLabel->setEnabled(m_enablePreviewCheck->isChecked());
    
    // Update slider labels
    onCompressionLevelChanged();
    m_previewDelayLabel->setText(QString("%1ms").arg(m_previewDelaySlider->value()));
}

void SettingsPage::applyTheme(const QString& theme) {
    // This would typically apply the theme to the entire application
    // For now, just log the change
    spdlog::info("Theme changed to: {}", theme.toStdString());
    
    // In a real implementation, you would:
    // 1. Load appropriate stylesheets
    // 2. Update color schemes
    // 3. Refresh all widgets
    
    if (theme == "Dark") {
        // Apply dark theme
        qApp->setStyleSheet(R"(
            QWidget {
                background-color: #2b2b2b;
                color: #ffffff;
            }
            QGroupBox {
                font-weight: bold;
                border: 2px solid #555555;
                border-radius: 5px;
                margin-top: 1ex;
                padding-top: 10px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0 5px 0 5px;
            }
        )");
    } else if (theme == "Light") {
        // Apply light theme
        qApp->setStyleSheet("");
    } else {
        // System theme - use default
        qApp->setStyleSheet("");
    }
}

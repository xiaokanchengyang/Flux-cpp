#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QSlider>
#include <QListWidget>
#include <QTreeWidget>
#include <QTextEdit>
#include <QProgressBar>
#include <QFileDialog>
#include <QColorDialog>
#include <QFontDialog>
#include <QSettings>

namespace FluxGUI::UI::Widgets {

/**
 * @brief Comprehensive settings widget with modern interface
 * 
 * The SettingsWidget provides extensive application configuration:
 * - General application preferences
 * - Theme and appearance customization
 * - Compression and extraction defaults
 * - File associations and system integration
 * - Performance and memory settings
 * - Security and privacy options
 * - Advanced developer settings
 */
class SettingsWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Settings categories
     */
    enum class SettingsCategory {
        General,        // General application settings
        Appearance,     // Theme and UI settings
        Compression,    // Default compression settings
        Extraction,     // Default extraction settings
        Integration,    // System integration settings
        Performance,    // Performance and memory settings
        Security,       // Security and privacy settings
        Advanced        // Advanced developer settings
    };

    /**
     * @brief Construct the settings widget
     * @param parent Parent widget
     */
    explicit SettingsWidget(QWidget* parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~SettingsWidget() override = default;

public slots:
    /**
     * @brief Load settings from configuration
     */
    void loadSettings();
    
    /**
     * @brief Save settings to configuration
     */
    void saveSettings();
    
    /**
     * @brief Reset settings to defaults
     */
    void resetToDefaults();
    
    /**
     * @brief Apply current settings
     */
    void applySettings();
    
    /**
     * @brief Show specific settings category
     * @param category Category to show
     */
    void showCategory(SettingsCategory category);

signals:
    /**
     * @brief Emitted when settings are applied
     */
    void settingsApplied();
    
    /**
     * @brief Emitted when settings are reset
     */
    void settingsReset();
    
    /**
     * @brief Emitted when theme settings change
     */
    void themeSettingsChanged();
    
    /**
     * @brief Emitted when system integration settings change
     */
    void integrationSettingsChanged();

private slots:
    /**
     * @brief Handle general settings changes
     */
    void onGeneralSettingsChanged();
    
    /**
     * @brief Handle appearance settings changes
     */
    void onAppearanceSettingsChanged();
    
    /**
     * @brief Handle compression settings changes
     */
    void onCompressionSettingsChanged();
    
    /**
     * @brief Handle extraction settings changes
     */
    void onExtractionSettingsChanged();
    
    /**
     * @brief Handle integration settings changes
     */
    void onIntegrationSettingsChanged();
    
    /**
     * @brief Handle performance settings changes
     */
    void onPerformanceSettingsChanged();
    
    /**
     * @brief Handle security settings changes
     */
    void onSecuritySettingsChanged();
    
    /**
     * @brief Handle advanced settings changes
     */
    void onAdvancedSettingsChanged();
    
    /**
     * @brief Browse for directory
     */
    void browseDirectory();
    
    /**
     * @brief Browse for file
     */
    void browseFile();
    
    /**
     * @brief Choose color
     */
    void chooseColor();
    
    /**
     * @brief Choose font
     */
    void chooseFont();
    
    /**
     * @brief Test system integration
     */
    void testSystemIntegration();
    
    /**
     * @brief Clear cache
     */
    void clearCache();
    
    /**
     * @brief Export settings
     */
    void exportSettings();
    
    /**
     * @brief Import settings
     */
    void importSettings();

private:
    /**
     * @brief Initialize the user interface
     */
    void initializeUI();
    
    /**
     * @brief Create general settings tab
     * @return General settings widget
     */
    QWidget* createGeneralSettingsTab();
    
    /**
     * @brief Create appearance settings tab
     * @return Appearance settings widget
     */
    QWidget* createAppearanceSettingsTab();
    
    /**
     * @brief Create compression settings tab
     * @return Compression settings widget
     */
    QWidget* createCompressionSettingsTab();
    
    /**
     * @brief Create extraction settings tab
     * @return Extraction settings widget
     */
    QWidget* createExtractionSettingsTab();
    
    /**
     * @brief Create integration settings tab
     * @return Integration settings widget
     */
    QWidget* createIntegrationSettingsTab();
    
    /**
     * @brief Create performance settings tab
     * @return Performance settings widget
     */
    QWidget* createPerformanceSettingsTab();
    
    /**
     * @brief Create security settings tab
     * @return Security settings widget
     */
    QWidget* createSecuritySettingsTab();
    
    /**
     * @brief Create advanced settings tab
     * @return Advanced settings widget
     */
    QWidget* createAdvancedSettingsTab();
    
    /**
     * @brief Create button box
     * @return Button box widget
     */
    QWidget* createButtonBox();
    
    /**
     * @brief Connect signals and slots
     */
    void connectSignals();
    
    /**
     * @brief Update UI state
     */
    void updateUIState();
    
    /**
     * @brief Validate settings
     * @return True if settings are valid
     */
    bool validateSettings();
    
    /**
     * @brief Get default value for setting
     * @param key Setting key
     * @return Default value
     */
    QVariant getDefaultValue(const QString& key) const;
    
    /**
     * @brief Create setting group box
     * @param title Group title
     * @param layout Layout to use
     * @return Group box widget
     */
    QGroupBox* createSettingGroup(const QString& title, QLayout* layout);
    
    /**
     * @brief Add setting row
     * @param layout Target layout
     * @param label Label text
     * @param widget Control widget
     * @param description Optional description
     */
    void addSettingRow(QGridLayout* layout, const QString& label, 
                      QWidget* widget, const QString& description = QString());

private:
    // Main layout
    QVBoxLayout* m_mainLayout = nullptr;
    QTabWidget* m_tabWidget = nullptr;
    
    // General settings
    QWidget* m_generalTab = nullptr;
    QComboBox* m_languageCombo = nullptr;
    QCheckBox* m_startMinimizedCheckBox = nullptr;
    QCheckBox* m_minimizeToTrayCheckBox = nullptr;
    QCheckBox* m_closeToTrayCheckBox = nullptr;
    QCheckBox* m_autoStartCheckBox = nullptr;
    QCheckBox* m_checkUpdatesCheckBox = nullptr;
    QLineEdit* m_defaultDirectoryEdit = nullptr;
    QSpinBox* m_recentFilesCountSpinBox = nullptr;
    
    // Appearance settings
    QWidget* m_appearanceTab = nullptr;
    QComboBox* m_themeCombo = nullptr;
    QComboBox* m_accentColorCombo = nullptr;
    QSlider* m_uiScaleSlider = nullptr;
    QCheckBox* m_animationsCheckBox = nullptr;
    QCheckBox* m_transparencyCheckBox = nullptr;
    QComboBox* m_iconThemeCombo = nullptr;
    QPushButton* m_customFontButton = nullptr;
    QLabel* m_fontPreviewLabel = nullptr;
    
    // Compression settings
    QWidget* m_compressionTab = nullptr;
    QComboBox* m_defaultFormatCombo = nullptr;
    QComboBox* m_defaultLevelCombo = nullptr;
    QCheckBox* m_solidArchiveCheckBox = nullptr;
    QCheckBox* m_compressHeadersCheckBox = nullptr;
    QSpinBox* m_dictionarySizeSpinBox = nullptr;
    QSpinBox* m_threadCountSpinBox = nullptr;
    QCheckBox* m_deleteAfterCompressionCheckBox = nullptr;
    QLineEdit* m_defaultPasswordEdit = nullptr;
    
    // Extraction settings
    QWidget* m_extractionTab = nullptr;
    QComboBox* m_overwriteModeCombo = nullptr;
    QCheckBox* m_preservePathsCheckBox = nullptr;
    QCheckBox* m_createSubfolderCheckBox = nullptr;
    QCheckBox* m_openAfterExtractionCheckBox = nullptr;
    QCheckBox* m_deleteAfterExtractionCheckBox = nullptr;
    QLineEdit* m_extractionDirectoryEdit = nullptr;
    QCheckBox* m_verifyExtractedFilesCheckBox = nullptr;
    
    // Integration settings
    QWidget* m_integrationTab = nullptr;
    QCheckBox* m_contextMenuCheckBox = nullptr;
    QCheckBox* m_fileAssociationsCheckBox = nullptr;
    QListWidget* m_contextActionsListWidget = nullptr;
    QListWidget* m_fileTypesListWidget = nullptr;
    QPushButton* m_testIntegrationButton = nullptr;
    QCheckBox* m_systemTrayCheckBox = nullptr;
    
    // Performance settings
    QWidget* m_performanceTab = nullptr;
    QSpinBox* m_memoryCacheSizeSpinBox = nullptr;
    QSpinBox* m_maxConcurrentOperationsSpinBox = nullptr;
    QCheckBox* m_enableMultithreadingCheckBox = nullptr;
    QSlider* m_cpuUsageLimitSlider = nullptr;
    QCheckBox* m_enableLoggingCheckBox = nullptr;
    QComboBox* m_logLevelCombo = nullptr;
    QPushButton* m_clearCacheButton = nullptr;
    
    // Security settings
    QWidget* m_securityTab = nullptr;
    QCheckBox* m_rememberPasswordsCheckBox = nullptr;
    QSpinBox* m_passwordTimeoutSpinBox = nullptr;
    QCheckBox* m_secureDeleteCheckBox = nullptr;
    QCheckBox* m_encryptTempFilesCheckBox = nullptr;
    QCheckBox* m_sendUsageStatsCheckBox = nullptr;
    QCheckBox* m_autoUpdateCheckBox = nullptr;
    
    // Advanced settings
    QWidget* m_advancedTab = nullptr;
    QCheckBox* m_debugModeCheckBox = nullptr;
    QCheckBox* m_verboseLoggingCheckBox = nullptr;
    QLineEdit* m_customTempDirEdit = nullptr;
    QTextEdit* m_customArgumentsEdit = nullptr;
    QPushButton* m_exportSettingsButton = nullptr;
    QPushButton* m_importSettingsButton = nullptr;
    QPushButton* m_resetSettingsButton = nullptr;
    
    // Button box
    QWidget* m_buttonBox = nullptr;
    QPushButton* m_applyButton = nullptr;
    QPushButton* m_resetButton = nullptr;
    QPushButton* m_defaultsButton = nullptr;
    
    // State
    QSettings* m_settings = nullptr;
    bool m_settingsChanged = false;
    QFont m_customFont;
    QColor m_customAccentColor;
};

} // namespace FluxGUI::UI::Widgets

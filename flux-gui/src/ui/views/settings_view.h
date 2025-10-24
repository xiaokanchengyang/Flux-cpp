#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QScrollArea>
#include <QTabWidget>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QToolButton>
#include <QSlider>
#include <QProgressBar>
#include <QTextEdit>
#include <QListWidget>
#include <QTreeWidget>
#include <QTableWidget>
#include <QFileDialog>
#include <QColorDialog>
#include <QFontDialog>
#include <QKeySequenceEdit>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

#include <memory>
#include <unordered_map>

namespace FluxGUI::UI::Views {

/**
 * @brief Comprehensive settings interface for Flux Archive Manager
 * 
 * This view provides a modern, organized settings interface with multiple
 * categories, real-time preview, and intelligent validation.
 * 
 * Features:
 * - Categorized settings with search
 * - Real-time preview and validation
 * - Import/export configuration
 * - Keyboard shortcut customization
 * - Theme and appearance settings
 * - Performance optimization options
 * - Advanced archive handling settings
 * - Accessibility configuration
 * - Plugin management
 * - Backup and restore settings
 */
class SettingsView : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Settings categories
     */
    enum class SettingsCategory {
        General,        ///< General application settings
        Interface,      ///< UI and appearance settings
        Archives,       ///< Archive handling settings
        Performance,    ///< Performance and optimization
        Shortcuts,      ///< Keyboard shortcuts
        Advanced,       ///< Advanced configuration
        Plugins,        ///< Plugin management
        Accessibility,  ///< Accessibility options
        Network,        ///< Network and updates
        Security        ///< Security and privacy
    };

    /**
     * @brief Theme options
     */
    enum class ThemeMode {
        Light,          ///< Light theme
        Dark,           ///< Dark theme
        Auto            ///< System theme
    };

    /**
     * @brief Language options
     */
    enum class Language {
        English,        ///< English
        Chinese,        ///< Chinese (Simplified)
        Japanese,       ///< Japanese
        Korean,         ///< Korean
        German,         ///< German
        French,         ///< French
        Spanish,        ///< Spanish
        Russian,        ///< Russian
        Auto            ///< System language
    };

    /**
     * @brief Compression level options
     */
    enum class CompressionLevel {
        Store,          ///< No compression (store only)
        Fastest,        ///< Fastest compression
        Fast,           ///< Fast compression
        Normal,         ///< Normal compression
        Maximum,        ///< Maximum compression
        Ultra           ///< Ultra compression
    };

    explicit SettingsView(QWidget* parent = nullptr);
    ~SettingsView() override;

    // Settings management
    void loadSettings();
    void saveSettings();
    void resetToDefaults();
    void importSettings(const QString& filePath = QString());
    void exportSettings(const QString& filePath = QString());

    // Category navigation
    void showCategory(SettingsCategory category);
    SettingsCategory currentCategory() const { return m_currentCategory; }

    // Search functionality
    void searchSettings(const QString& query);
    void clearSearch();

    // Validation
    bool validateSettings() const;
    QStringList getValidationErrors() const;

    // Preview
    void enablePreviewMode(bool enabled);
    bool isPreviewModeEnabled() const { return m_previewMode; }

Q_SIGNALS:
    // Settings change notifications
    void settingsChanged();
    void categoryChanged(SettingsCategory category);
    void themeChanged(ThemeMode theme);
    void languageChanged(Language language);
    void shortcutsChanged();
    
    // Validation signals
    void validationStateChanged(bool isValid);
    void settingsApplied();
    void settingsReset();
    
    // Import/export signals
    void settingsImported(const QString& filePath);
    void settingsExported(const QString& filePath);

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private Q_SLOTS:
    // Category navigation
    void onCategorySelected(int index);
    void onSearchTextChanged(const QString& text);
    
    // General settings
    void onLanguageChanged(int index);
    void onThemeChanged(int index);
    void onStartupBehaviorChanged();
    void onAutoUpdateChanged(bool enabled);
    
    // Interface settings
    void onFontSizeChanged(int size);
    void onIconSizeChanged(int size);
    void onAnimationsChanged(bool enabled);
    void onToolbarStyleChanged(int style);
    void onWindowOpacityChanged(int opacity);
    
    // Archive settings
    void onDefaultCompressionChanged(int level);
    void onCompressionThreadsChanged(int threads);
    void onTempDirectoryChanged();
    void onArchiveAssociationsChanged();
    void onEncryptionSettingsChanged();
    
    // Performance settings
    void onMemoryLimitChanged(int limitMB);
    void onCacheSizeChanged(int sizeMB);
    void onPreloadingChanged(bool enabled);
    void onBackgroundProcessingChanged(bool enabled);
    
    // Shortcut settings
    void onShortcutChanged(const QString& action, const QKeySequence& sequence);
    void onResetShortcuts();
    void onImportShortcuts();
    void onExportShortcuts();
    
    // Advanced settings
    void onLoggingLevelChanged(int level);
    void onDebugModeChanged(bool enabled);
    void onExperimentalFeaturesChanged(bool enabled);
    void onConfigDirectoryChanged();
    
    // Plugin settings
    void onPluginStateChanged(const QString& pluginId, bool enabled);
    void onPluginSettingsRequested(const QString& pluginId);
    void onInstallPluginRequested();
    void onUninstallPluginRequested();
    
    // Accessibility settings
    void onHighContrastChanged(bool enabled);
    void onScreenReaderChanged(bool enabled);
    void onKeyboardNavigationChanged(bool enabled);
    void onFocusIndicatorChanged(bool enabled);
    
    // Network settings
    void onProxySettingsChanged();
    void onUpdateChannelChanged(int channel);
    void onDownloadLocationChanged();
    
    // Security settings
    void onPasswordPolicyChanged();
    void onEncryptionAlgorithmChanged(int algorithm);
    void onSecureDeleteChanged(bool enabled);
    
    // UI event handlers
    void onApplyClicked();
    void onResetClicked();
    void onImportClicked();
    void onExportClicked();
    void onDefaultsClicked();
    
    // Validation handlers
    void onSettingValidationRequested();
    void onValidationTimer();

private:
    // UI initialization
    void initializeUI();
    void createCategoryList();
    void createContentArea();
    void createButtonArea();
    void setupLayouts();
    void setupConnections();
    void applyStyles();
    
    // Category pages creation
    void createGeneralPage();
    void createInterfacePage();
    void createArchivesPage();
    void createPerformancePage();
    void createShortcutsPage();
    void createAdvancedPage();
    void createPluginsPage();
    void createAccessibilityPage();
    void createNetworkPage();
    void createSecurityPage();
    
    // Settings management
    void loadGeneralSettings();
    void loadInterfaceSettings();
    void loadArchiveSettings();
    void loadPerformanceSettings();
    void loadShortcutSettings();
    void loadAdvancedSettings();
    void loadPluginSettings();
    void loadAccessibilitySettings();
    void loadNetworkSettings();
    void loadSecuritySettings();
    
    void saveGeneralSettings();
    void saveInterfaceSettings();
    void saveArchiveSettings();
    void savePerformanceSettings();
    void saveShortcutSettings();
    void saveAdvancedSettings();
    void savePluginSettings();
    void saveAccessibilitySettings();
    void saveNetworkSettings();
    void saveSecuritySettings();
    
    // Validation helpers
    bool validateGeneralSettings() const;
    bool validateInterfaceSettings() const;
    bool validateArchiveSettings() const;
    bool validatePerformanceSettings() const;
    bool validateShortcutSettings() const;
    bool validateAdvancedSettings() const;
    bool validatePluginSettings() const;
    bool validateAccessibilitySettings() const;
    bool validateNetworkSettings() const;
    bool validateSecuritySettings() const;
    
    // Utility methods
    void updatePreview();
    void resetCategoryToDefaults(SettingsCategory category);
    void highlightSearchResults(const QString& query);
    void clearSearchHighlights();
    QString getSettingsFilePath() const;
    void createBackup();
    void restoreBackup();
    
    // Animation helpers
    void animateCategoryTransition();
    void fadeInWidget(QWidget* widget);
    void fadeOutWidget(QWidget* widget);

private:
    // Core data
    SettingsCategory m_currentCategory;
    bool m_previewMode;
    bool m_hasUnsavedChanges;
    QString m_searchQuery;
    
    // UI components
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_contentLayout;
    QSplitter* m_mainSplitter;
    
    // Category navigation
    QListWidget* m_categoryList;
    QLineEdit* m_searchEdit;
    
    // Content area
    QScrollArea* m_contentScrollArea;
    QStackedWidget* m_contentStack;
    
    // Category pages
    QWidget* m_generalPage;
    QWidget* m_interfacePage;
    QWidget* m_archivesPage;
    QWidget* m_performancePage;
    QWidget* m_shortcutsPage;
    QWidget* m_advancedPage;
    QWidget* m_pluginsPage;
    QWidget* m_accessibilityPage;
    QWidget* m_networkPage;
    QWidget* m_securityPage;
    
    // Button area
    QWidget* m_buttonArea;
    QPushButton* m_applyButton;
    QPushButton* m_resetButton;
    QPushButton* m_importButton;
    QPushButton* m_exportButton;
    QPushButton* m_defaultsButton;
    
    // General settings controls
    QComboBox* m_languageCombo;
    QComboBox* m_themeCombo;
    QCheckBox* m_startWithSystemCheck;
    QCheckBox* m_minimizeToTrayCheck;
    QCheckBox* m_autoUpdateCheck;
    QCheckBox* m_sendStatisticsCheck;
    
    // Interface settings controls
    QSpinBox* m_fontSizeSpin;
    QSpinBox* m_iconSizeSpin;
    QCheckBox* m_animationsCheck;
    QComboBox* m_toolbarStyleCombo;
    QSlider* m_windowOpacitySlider;
    QCheckBox* m_showStatusBarCheck;
    QCheckBox* m_showToolbarCheck;
    
    // Archive settings controls
    QComboBox* m_defaultCompressionCombo;
    QSpinBox* m_compressionThreadsSpin;
    QLineEdit* m_tempDirectoryEdit;
    QPushButton* m_tempDirectoryButton;
    QCheckBox* m_associateArchivesCheck;
    QCheckBox* m_enableEncryptionCheck;
    QComboBox* m_encryptionAlgorithmCombo;
    
    // Performance settings controls
    QSpinBox* m_memoryLimitSpin;
    QSpinBox* m_cacheSizeSpin;
    QCheckBox* m_preloadingCheck;
    QCheckBox* m_backgroundProcessingCheck;
    QCheckBox* m_multiThreadingCheck;
    QSpinBox* m_maxThreadsSpin;
    
    // Shortcut settings controls
    QTreeWidget* m_shortcutsTree;
    QPushButton* m_resetShortcutsButton;
    QPushButton* m_importShortcutsButton;
    QPushButton* m_exportShortcutsButton;
    
    // Advanced settings controls
    QComboBox* m_loggingLevelCombo;
    QCheckBox* m_debugModeCheck;
    QCheckBox* m_experimentalFeaturesCheck;
    QLineEdit* m_configDirectoryEdit;
    QPushButton* m_configDirectoryButton;
    QCheckBox* m_portableModeCheck;
    
    // Plugin settings controls
    QTableWidget* m_pluginsTable;
    QPushButton* m_installPluginButton;
    QPushButton* m_uninstallPluginButton;
    QPushButton* m_pluginSettingsButton;
    
    // Accessibility settings controls
    QCheckBox* m_highContrastCheck;
    QCheckBox* m_screenReaderCheck;
    QCheckBox* m_keyboardNavigationCheck;
    QCheckBox* m_focusIndicatorCheck;
    QSpinBox* m_textScaleSpin;
    
    // Network settings controls
    QCheckBox* m_useProxyCheck;
    QLineEdit* m_proxyHostEdit;
    QSpinBox* m_proxyPortSpin;
    QComboBox* m_updateChannelCombo;
    QLineEdit* m_downloadLocationEdit;
    QPushButton* m_downloadLocationButton;
    
    // Security settings controls
    QCheckBox* m_rememberPasswordsCheck;
    QSpinBox* m_passwordTimeoutSpin;
    QComboBox* m_encryptionMethodCombo;
    QCheckBox* m_secureDeleteCheck;
    
    // Animation components
    std::unique_ptr<QPropertyAnimation> m_fadeAnimation;
    std::unique_ptr<QGraphicsOpacityEffect> m_opacityEffect;
    
    // Validation
    QTimer* m_validationTimer;
    std::unordered_map<QString, bool> m_validationStates;
    
    // Settings storage
    std::unique_ptr<QSettings> m_settings;
    QString m_settingsFilePath;
    
    // Constants
    static constexpr int ANIMATION_DURATION = 200;
    static constexpr int VALIDATION_DELAY = 500;
    static constexpr int MIN_FONT_SIZE = 8;
    static constexpr int MAX_FONT_SIZE = 24;
    static constexpr int MIN_ICON_SIZE = 16;
    static constexpr int MAX_ICON_SIZE = 64;
    static constexpr int MIN_MEMORY_LIMIT = 64;
    static constexpr int MAX_MEMORY_LIMIT = 8192;
};

} // namespace FluxGUI::UI::Views

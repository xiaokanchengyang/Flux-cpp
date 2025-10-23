#pragma once

#include <QObject>
#include <QVariant>
#include <QSize>
#include <QPoint>
#include <QJsonObject>

class QSettings;

namespace FluxGUI::Core::Config {

/**
 * @brief Centralized settings management system
 * 
 * The SettingsManager provides a comprehensive configuration system with:
 * - Hierarchical JSON-based settings structure
 * - Type-safe setting access with validation
 * - Import/export functionality for settings backup
 * - Live updates with signal notifications
 * - Default settings with user overrides
 */
class SettingsManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Get the singleton instance
     * @return Reference to the settings manager instance
     */
    static SettingsManager& instance();
    
    /**
     * @brief Destructor
     */
    ~SettingsManager() override;

public slots:
    /**
     * @brief Initialize the settings system
     */
    void initialize();
    
    /**
     * @brief Get a setting value
     * @param key Hierarchical key (e.g., "ui/theme/current")
     * @param defaultValue Default value if key doesn't exist
     * @return Setting value
     */
    QVariant value(const QString& key, const QVariant& defaultValue = QVariant()) const;
    
    /**
     * @brief Set a setting value
     * @param key Hierarchical key
     * @param value New value
     */
    void setValue(const QString& key, const QVariant& value);
    
    /**
     * @brief Reset all settings to defaults
     */
    void resetToDefaults();
    
    /**
     * @brief Save settings to disk
     */
    void saveSettings();
    
    /**
     * @brief Load settings from disk
     */
    void loadSettings();
    
    /**
     * @brief Export settings to file
     * @param filePath Target file path
     * @return True if successful
     */
    bool exportSettings(const QString& filePath);
    
    /**
     * @brief Import settings from file
     * @param filePath Source file path
     * @return True if successful
     */
    bool importSettings(const QString& filePath);

public:
    /**
     * @brief Get configuration directory path
     * @return Configuration directory path
     */
    QString configDirectory() const;
    
    /**
     * @brief Get user settings file path
     * @return User settings file path
     */
    QString userSettingsFile() const;
    
    /**
     * @brief Set auto-save mode
     * @param enabled True to enable auto-save
     */
    void setAutoSave(bool enabled);
    
    /**
     * @brief Check if auto-save is enabled
     * @return True if auto-save is enabled
     */
    bool autoSave() const;

    // Convenience methods for common settings
    QString theme() const;
    void setTheme(const QString& theme);
    
    QString language() const;
    void setLanguage(const QString& language);
    
    QSize windowSize() const;
    void setWindowSize(const QSize& size);
    
    QPoint windowPosition() const;
    void setWindowPosition(const QPoint& position);
    
    bool windowMaximized() const;
    void setWindowMaximized(bool maximized);

signals:
    /**
     * @brief Emitted when settings are loaded
     */
    void settingsLoaded();
    
    /**
     * @brief Emitted when a setting changes
     * @param key Setting key
     * @param value New value
     */
    void settingChanged(const QString& key, const QVariant& value);
    
    /**
     * @brief Emitted when settings are reset
     */
    void settingsReset();
    
    /**
     * @brief Emitted when settings are imported
     */
    void settingsImported();

private:
    /**
     * @brief Private constructor for singleton
     * @param parent Parent object
     */
    explicit SettingsManager(QObject* parent = nullptr);
    
    /**
     * @brief Setup configuration directory
     */
    void setupConfigDirectory();
    
    /**
     * @brief Load default settings from resources
     */
    void loadDefaultSettings();
    
    /**
     * @brief Load user settings from file
     */
    void loadUserSettings();
    
    /**
     * @brief Save user settings to file
     */
    void saveUserSettings();
    
    /**
     * @brief Validate settings integrity
     * @return True if settings are valid
     */
    bool validateSettings();
    
    /**
     * @brief Get nested value from JSON object
     * @param obj JSON object
     * @param key Hierarchical key
     * @param defaultValue Default value
     * @return Setting value
     */
    QVariant getNestedValue(const QJsonObject& obj, const QString& key, const QVariant& defaultValue) const;
    
    /**
     * @brief Set nested value in JSON object
     * @param obj JSON object
     * @param key Hierarchical key
     * @param value New value
     */
    void setNestedValue(QJsonObject& obj, const QString& key, const QVariant& value);
    
    /**
     * @brief Check if nested key exists
     * @param obj JSON object
     * @param key Hierarchical key
     * @return True if key exists
     */
    bool hasNestedKey(const QJsonObject& obj, const QString& key) const;
    
    /**
     * @brief Merge JSON objects recursively
     * @param target Target object (modified)
     * @param source Source object
     */
    void mergeJsonObjects(QJsonObject& target, const QJsonObject& source);

private:
    QSettings* m_settings;
    QJsonObject m_settingsData;
    QString m_configDir;
    bool m_settingsLoaded;
    bool m_autoSave = true;
};

} // namespace FluxGUI::Core::Config

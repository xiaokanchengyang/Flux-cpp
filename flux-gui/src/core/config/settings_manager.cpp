#include "settings_manager.h"

#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QFileInfo>

namespace FluxGUI::Core::Config {

SettingsManager& SettingsManager::instance() {
    static SettingsManager instance;
    return instance;
}

SettingsManager::SettingsManager(QObject* parent)
    : QObject(parent)
    , m_settings(nullptr)
    , m_settingsLoaded(false)
{
    // Initialize QSettings
    m_settings = new QSettings(this);
    
    // Set up configuration directory
    setupConfigDirectory();
}

SettingsManager::~SettingsManager() = default;

void SettingsManager::initialize() {
    qDebug() << "Initializing Settings Manager...";
    
    // Load default settings first
    loadDefaultSettings();
    
    // Load user settings (will override defaults)
    loadUserSettings();
    
    // Validate and migrate settings if needed
    validateSettings();
    
    m_settingsLoaded = true;
    
    qDebug() << "Settings Manager initialized";
    emit settingsLoaded();
}

QVariant SettingsManager::value(const QString& key, const QVariant& defaultValue) const {
    if (!m_settingsLoaded) {
        qWarning() << "Settings not loaded yet, using QSettings directly for key:" << key;
        return m_settings->value(key, defaultValue);
    }
    
    return getNestedValue(m_settingsData, key, defaultValue);
}

void SettingsManager::setValue(const QString& key, const QVariant& value) {
    // Update in-memory settings
    setNestedValue(m_settingsData, key, value);
    
    // Update QSettings for immediate persistence
    m_settings->setValue(key, value);
    
    // Emit change signal
    emit settingChanged(key, value);
    
    // Auto-save if enabled
    if (m_autoSave) {
        saveUserSettings();
    }
}

void SettingsManager::resetToDefaults() {
    qDebug() << "Resetting settings to defaults...";
    
    // Clear current settings
    m_settingsData = QJsonObject();
    m_settings->clear();
    
    // Reload defaults
    loadDefaultSettings();
    
    // Save the reset settings
    saveUserSettings();
    
    emit settingsReset();
    qDebug() << "Settings reset to defaults";
}

void SettingsManager::saveSettings() {
    saveUserSettings();
}

void SettingsManager::loadSettings() {
    loadUserSettings();
}

bool SettingsManager::exportSettings(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Cannot open file for writing:" << filePath;
        return false;
    }
    
    QJsonDocument doc(m_settingsData);
    file.write(doc.toJson());
    
    qDebug() << "Settings exported to:" << filePath;
    return true;
}

bool SettingsManager::importSettings(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open file for reading:" << filePath;
        return false;
    }
    
    QByteArray data = file.readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << error.errorString();
        return false;
    }
    
    // Backup current settings
    QJsonObject backup = m_settingsData;
    
    try {
        // Import new settings
        m_settingsData = doc.object();
        
        // Validate imported settings
        if (!validateSettings()) {
            // Restore backup if validation fails
            m_settingsData = backup;
            qWarning() << "Imported settings validation failed, restored backup";
            return false;
        }
        
        // Save imported settings
        saveUserSettings();
        
        emit settingsImported();
        qDebug() << "Settings imported from:" << filePath;
        return true;
        
    } catch (const std::exception& e) {
        // Restore backup on any error
        m_settingsData = backup;
        qWarning() << "Error importing settings:" << e.what();
        return false;
    }
}

QString SettingsManager::configDirectory() const {
    return m_configDir;
}

QString SettingsManager::userSettingsFile() const {
    return QDir(m_configDir).filePath("settings.json");
}

void SettingsManager::setAutoSave(bool enabled) {
    m_autoSave = enabled;
}

bool SettingsManager::autoSave() const {
    return m_autoSave;
}

// Convenience methods for common settings
QString SettingsManager::theme() const {
    return value("ui/theme/current", "dark").toString();
}

void SettingsManager::setTheme(const QString& theme) {
    setValue("ui/theme/current", theme);
}

QString SettingsManager::language() const {
    return value("application/language", "en").toString();
}

void SettingsManager::setLanguage(const QString& language) {
    setValue("application/language", language);
}

QSize SettingsManager::windowSize() const {
    int width = value("ui/mainWindow/width", 1200).toInt();
    int height = value("ui/mainWindow/height", 800).toInt();
    return QSize(width, height);
}

void SettingsManager::setWindowSize(const QSize& size) {
    setValue("ui/mainWindow/width", size.width());
    setValue("ui/mainWindow/height", size.height());
}

QPoint SettingsManager::windowPosition() const {
    int x = value("ui/mainWindow/position/x", -1).toInt();
    int y = value("ui/mainWindow/position/y", -1).toInt();
    return QPoint(x, y);
}

void SettingsManager::setWindowPosition(const QPoint& position) {
    setValue("ui/mainWindow/position/x", position.x());
    setValue("ui/mainWindow/position/y", position.y());
}

bool SettingsManager::windowMaximized() const {
    return value("ui/mainWindow/maximized", false).toBool();
}

void SettingsManager::setWindowMaximized(bool maximized) {
    setValue("ui/mainWindow/maximized", maximized);
}

void SettingsManager::setupConfigDirectory() {
    // Get application data directory
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_configDir = QDir(appDataDir).filePath("config");
    
    // Create directory if it doesn't exist
    QDir dir;
    if (!dir.exists(m_configDir)) {
        if (dir.mkpath(m_configDir)) {
            qDebug() << "Created config directory:" << m_configDir;
        } else {
            qWarning() << "Failed to create config directory:" << m_configDir;
        }
    }
}

void SettingsManager::loadDefaultSettings() {
    // Load default settings from resource
    QFile file(":/config/default-settings.json");
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot load default settings from resources";
        return;
    }
    
    QByteArray data = file.readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Error parsing default settings:" << error.errorString();
        return;
    }
    
    m_settingsData = doc.object();
    qDebug() << "Loaded default settings";
}

void SettingsManager::loadUserSettings() {
    QString settingsFile = userSettingsFile();
    
    if (!QFileInfo::exists(settingsFile)) {
        qDebug() << "User settings file does not exist, using defaults";
        return;
    }
    
    QFile file(settingsFile);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open user settings file:" << settingsFile;
        return;
    }
    
    QByteArray data = file.readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Error parsing user settings:" << error.errorString();
        return;
    }
    
    // Merge user settings with defaults
    mergeJsonObjects(m_settingsData, doc.object());
    
    qDebug() << "Loaded user settings from:" << settingsFile;
}

void SettingsManager::saveUserSettings() {
    QString settingsFile = userSettingsFile();
    
    QFile file(settingsFile);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Cannot open user settings file for writing:" << settingsFile;
        return;
    }
    
    QJsonDocument doc(m_settingsData);
    file.write(doc.toJson());
    
    qDebug() << "Saved user settings to:" << settingsFile;
}

bool SettingsManager::validateSettings() {
    // Basic validation - check required keys exist
    QStringList requiredKeys = {
        "application/version",
        "ui/theme/current",
        "compression/defaultFormat",
        "extraction/overwriteMode"
    };
    
    for (const QString& key : requiredKeys) {
        if (!hasNestedKey(m_settingsData, key)) {
            qWarning() << "Missing required setting:" << key;
            return false;
        }
    }
    
    // Validate theme exists
    QString theme = value("ui/theme/current").toString();
    if (theme != "dark" && theme != "light") {
        qWarning() << "Invalid theme:" << theme;
        setValue("ui/theme/current", "dark");
    }
    
    // Validate compression format
    QString format = value("compression/defaultFormat").toString();
    QStringList validFormats = {"zip", "7z", "tar", "gz", "bz2", "xz"};
    if (!validFormats.contains(format)) {
        qWarning() << "Invalid compression format:" << format;
        setValue("compression/defaultFormat", "zip");
    }
    
    return true;
}

QVariant SettingsManager::getNestedValue(const QJsonObject& obj, const QString& key, const QVariant& defaultValue) const {
    QStringList parts = key.split('/');
    QJsonObject current = obj;
    
    for (int i = 0; i < parts.size() - 1; ++i) {
        if (!current.contains(parts[i]) || !current[parts[i]].isObject()) {
            return defaultValue;
        }
        current = current[parts[i]].toObject();
    }
    
    QString lastKey = parts.last();
    if (!current.contains(lastKey)) {
        return defaultValue;
    }
    
    return current[lastKey].toVariant();
}

void SettingsManager::setNestedValue(QJsonObject& obj, const QString& key, const QVariant& value) {
    QStringList parts = key.split('/');
    QJsonObject* current = &obj;
    
    for (int i = 0; i < parts.size() - 1; ++i) {
        const QString& part = parts[i];
        if (!current->contains(part) || !(*current)[part].isObject()) {
            (*current)[part] = QJsonObject();
        }
        QJsonValueRef ref = (*current)[part];
        QJsonObject subObj = ref.toObject();
        current = &subObj;
        ref = subObj;
    }
    
    (*current)[parts.last()] = QJsonValue::fromVariant(value);
}

bool SettingsManager::hasNestedKey(const QJsonObject& obj, const QString& key) const {
    QStringList parts = key.split('/');
    QJsonObject current = obj;
    
    for (int i = 0; i < parts.size() - 1; ++i) {
        if (!current.contains(parts[i]) || !current[parts[i]].isObject()) {
            return false;
        }
        current = current[parts[i]].toObject();
    }
    
    return current.contains(parts.last());
}

void SettingsManager::mergeJsonObjects(QJsonObject& target, const QJsonObject& source) {
    for (auto it = source.begin(); it != source.end(); ++it) {
        const QString& key = it.key();
        const QJsonValue& value = it.value();
        
        if (value.isObject() && target.contains(key) && target[key].isObject()) {
            // Recursively merge objects
            QJsonObject targetObj = target[key].toObject();
            mergeJsonObjects(targetObj, value.toObject());
            target[key] = targetObj;
        } else {
            // Overwrite with source value
            target[key] = value;
        }
    }
}

} // namespace FluxGUI::Core::Config

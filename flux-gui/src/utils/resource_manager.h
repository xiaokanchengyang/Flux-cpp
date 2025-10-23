#pragma once

#include <QString>
#include <QStringList>
#include <QDir>
#include <QStandardPaths>
#include <QApplication>
#include <QIcon>
#include <QPixmap>
#include <unordered_map>
#include <memory>

/**
 * Resource manager for centralized resource handling
 * 
 * Manages application resources including icons, themes, and paths
 */
class ResourceManager {
public:
    // Singleton pattern
    static ResourceManager& instance();
    
    // Delete copy/move constructors and operators
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    ResourceManager(ResourceManager&&) = delete;
    ResourceManager& operator=(ResourceManager&&) = delete;

    // Resource paths
    struct Paths {
        static constexpr const char* ICONS = ":/icons/";
        static constexpr const char* THEMES = ":/themes/";
        static constexpr const char* TRANSLATIONS = ":/translations/";
        static constexpr const char* CONFIG = "config/";
        static constexpr const char* TEMP = "temp/";
        static constexpr const char* LOGS = "logs/";
    };

    // Icon names
    struct Icons {
        static constexpr const char* APP_ICON = "flux.png";
        static constexpr const char* NEW_ARCHIVE = "new.png";
        static constexpr const char* OPEN_ARCHIVE = "open.png";
        static constexpr const char* EXTRACT = "extract.png";
        static constexpr const char* FOLDER = "folder.png";
        static constexpr const char* FILE = "file.png";
        static constexpr const char* DOCUMENT = "document.png";
        static constexpr const char* IMAGE = "image.png";
        static constexpr const char* AUDIO = "audio.png";
        static constexpr const char* VIDEO = "video.png";
        static constexpr const char* CODE = "code.png";
        static constexpr const char* ARCHIVE = "archive.png";
        static constexpr const char* HOME = "home.png";
    };

    // Theme names
    struct Themes {
        static constexpr const char* LIGHT = "light";
        static constexpr const char* DARK = "dark";
        static constexpr const char* AUTO = "auto";
    };

    // Configuration keys
    struct Config {
        static constexpr const char* THEME = "appearance/theme";
        static constexpr const char* LANGUAGE = "general/language";
        static constexpr const char* RECENT_FILES = "general/recent_files";
        static constexpr const char* DEFAULT_EXTRACT_PATH = "paths/default_extract";
        static constexpr const char* DEFAULT_COMPRESSION_LEVEL = "compression/default_level";
        static constexpr const char* WINDOW_GEOMETRY = "window/geometry";
        static constexpr const char* WINDOW_STATE = "window/state";
    };

    // Resource access methods
    QString getIconPath(const QString& iconName) const;
    QString getThemePath(const QString& themeName) const;
    QString getTranslationPath(const QString& language) const;
    
    // Icon loading with caching
    QIcon getIcon(const QString& iconName);
    QPixmap getPixmap(const QString& iconName, const QSize& size = QSize());
    
    // Directory management
    QString getConfigDir() const;
    QString getTempDir() const;
    QString getLogsDir() const;
    QString getUserDataDir() const;
    
    // Ensure directories exist
    bool ensureDirectoriesExist();
    
    // Resource validation
    bool validateResources() const;
    
    // Cache management
    void clearIconCache();
    void preloadIcons();

private:
    ResourceManager() = default;
    ~ResourceManager() = default;
    
    // Initialize directories (for lazy initialization)
    void initializeDirectories() const;
    
    // Icon cache
    mutable std::unordered_map<QString, QIcon> m_iconCache;
    mutable std::unordered_map<QString, QPixmap> m_pixmapCache;
    
    // Directory paths
    mutable QString m_configDir;
    mutable QString m_tempDir;
    mutable QString m_logsDir;
    mutable QString m_userDataDir;
    
    // Initialization flag
    mutable bool m_initialized{false};
};

// Convenience macros for resource access
#define RESOURCE_MANAGER ResourceManager::instance()
#define GET_ICON(name) RESOURCE_MANAGER.getIcon(name)
#define GET_PIXMAP(name, size) RESOURCE_MANAGER.getPixmap(name, size)
#define GET_CONFIG_DIR() RESOURCE_MANAGER.getConfigDir()
#define GET_TEMP_DIR() RESOURCE_MANAGER.getTempDir()

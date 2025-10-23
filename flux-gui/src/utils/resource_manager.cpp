#include "resource_manager.h"
#include <QDebug>
#include <QFileInfo>
#include <QCoreApplication>

ResourceManager& ResourceManager::instance() {
    static ResourceManager instance;
    return instance;
}

QString ResourceManager::getIconPath(const QString& iconName) const {
    return QString(Paths::ICONS) + iconName;
}

QString ResourceManager::getThemePath(const QString& themeName) const {
    return QString(Paths::THEMES) + themeName + ".qss";
}

QString ResourceManager::getTranslationPath(const QString& language) const {
    return QString(Paths::TRANSLATIONS) + "flux_" + language + ".qm";
}

QIcon ResourceManager::getIcon(const QString& iconName) {
    // Check cache first
    auto it = m_iconCache.find(iconName);
    if (it != m_iconCache.end()) {
        return it->second;
    }
    
    // Load icon and cache it
    QString iconPath = getIconPath(iconName);
    QIcon icon(iconPath);
    
    if (icon.isNull()) {
        qWarning() << "Failed to load icon:" << iconPath;
        // Return a default icon or empty icon
        icon = QIcon(); // Empty icon
    }
    
    m_iconCache[iconName] = icon;
    return icon;
}

QPixmap ResourceManager::getPixmap(const QString& iconName, const QSize& size) {
    QString cacheKey = iconName + QString("_%1x%2").arg(size.width()).arg(size.height());
    
    // Check cache first
    auto it = m_pixmapCache.find(cacheKey);
    if (it != m_pixmapCache.end()) {
        return it->second;
    }
    
    // Load pixmap
    QString iconPath = getIconPath(iconName);
    QPixmap pixmap(iconPath);
    
    if (!pixmap.isNull() && size.isValid()) {
        pixmap = pixmap.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    
    if (pixmap.isNull()) {
        qWarning() << "Failed to load pixmap:" << iconPath;
    }
    
    m_pixmapCache[cacheKey] = pixmap;
    return pixmap;
}

QString ResourceManager::getConfigDir() const {
    if (!m_initialized) {
        initializeDirectories();
    }
    return m_configDir;
}

QString ResourceManager::getTempDir() const {
    if (!m_initialized) {
        initializeDirectories();
    }
    return m_tempDir;
}

QString ResourceManager::getLogsDir() const {
    if (!m_initialized) {
        initializeDirectories();
    }
    return m_logsDir;
}

QString ResourceManager::getUserDataDir() const {
    if (!m_initialized) {
        initializeDirectories();
    }
    return m_userDataDir;
}

bool ResourceManager::ensureDirectoriesExist() {
    if (!m_initialized) {
        initializeDirectories();
    }
    
    QDir dir;
    bool success = true;
    
    // Create directories if they don't exist
    if (!dir.exists(m_configDir)) {
        success &= dir.mkpath(m_configDir);
    }
    
    if (!dir.exists(m_tempDir)) {
        success &= dir.mkpath(m_tempDir);
    }
    
    if (!dir.exists(m_logsDir)) {
        success &= dir.mkpath(m_logsDir);
    }
    
    if (!dir.exists(m_userDataDir)) {
        success &= dir.mkpath(m_userDataDir);
    }
    
    return success;
}

bool ResourceManager::validateResources() const {
    // Check if essential icons exist
    QStringList essentialIcons = {
        Icons::APP_ICON,
        Icons::NEW_ARCHIVE,
        Icons::OPEN_ARCHIVE,
        Icons::EXTRACT,
        Icons::FOLDER,
        Icons::FILE
    };
    
    for (const QString& iconName : essentialIcons) {
        QString iconPath = getIconPath(iconName);
        if (!QFileInfo::exists(iconPath)) {
            qWarning() << "Missing essential icon:" << iconPath;
            return false;
        }
    }
    
    return true;
}

void ResourceManager::clearIconCache() {
    m_iconCache.clear();
    m_pixmapCache.clear();
}

void ResourceManager::preloadIcons() {
    // Preload commonly used icons
    QStringList commonIcons = {
        Icons::APP_ICON,
        Icons::NEW_ARCHIVE,
        Icons::OPEN_ARCHIVE,
        Icons::EXTRACT,
        Icons::FOLDER,
        Icons::FILE,
        Icons::HOME
    };
    
    for (const QString& iconName : commonIcons) {
        getIcon(iconName); // This will cache the icon
    }
}

void ResourceManager::initializeDirectories() const {
    // Get standard paths
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    
    // Set directory paths
    m_configDir = appDataPath + "/" + Paths::CONFIG;
    m_tempDir = tempPath + "/flux/" + Paths::TEMP;
    m_logsDir = appDataPath + "/" + Paths::LOGS;
    m_userDataDir = appDataPath;
    
    m_initialized = true;
}

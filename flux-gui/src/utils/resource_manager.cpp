#include "resource_manager.h"
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>

ResourceManager& ResourceManager::instance() {
    static ResourceManager instance;
    return instance;
}

QString ResourceManager::getIconPath(const QString& iconName) const {
    return QString("%1%2").arg(Paths::ICONS, iconName);
}

QString ResourceManager::getThemePath(const QString& themeName) const {
    return QString("%1%2.qss").arg(Paths::THEMES, themeName);
}

QString ResourceManager::getTranslationPath(const QString& language) const {
    return QString("%1flux_%2.qm").arg(Paths::TRANSLATIONS, language);
}

QIcon ResourceManager::getIcon(const QString& iconName) {
    auto it = m_iconCache.find(iconName);
    if (it != m_iconCache.end()) {
        return it->second;
    }
    
    QString iconPath = getIconPath(iconName);
    QIcon icon(iconPath);
    
    if (!icon.isNull()) {
        m_iconCache[iconName] = icon;
    } else {
        qWarning() << "Failed to load icon:" << iconPath;
        // Return a default icon or empty icon
        icon = QIcon(); // Empty icon as fallback
    }
    
    return icon;
}

QPixmap ResourceManager::getPixmap(const QString& iconName, const QSize& size) {
    QString cacheKey = QString("%1_%2x%3").arg(iconName).arg(size.width()).arg(size.height());
    
    auto it = m_pixmapCache.find(cacheKey);
    if (it != m_pixmapCache.end()) {
        return it->second;
    }
    
    QIcon icon = getIcon(iconName);
    QPixmap pixmap = icon.pixmap(size.isValid() ? size : QSize(32, 32));
    
    if (!pixmap.isNull()) {
        m_pixmapCache[cacheKey] = pixmap;
    }
    
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
    
    // Create config directory
    if (!dir.exists(m_configDir)) {
        success &= dir.mkpath(m_configDir);
    }
    
    // Create temp directory
    if (!dir.exists(m_tempDir)) {
        success &= dir.mkpath(m_tempDir);
    }
    
    // Create logs directory
    if (!dir.exists(m_logsDir)) {
        success &= dir.mkpath(m_logsDir);
    }
    
    // Create user data directory
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
        if (!QFile::exists(iconPath)) {
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
    if (m_initialized) {
        return;
    }
    
    // Get standard paths
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    
    // Set up directory paths
    m_configDir = QDir(appDataPath).filePath(Paths::CONFIG);
    m_tempDir = QDir(tempPath).filePath(QString("flux_%1").arg(QCoreApplication::applicationPid()));
    m_logsDir = QDir(appDataPath).filePath(Paths::LOGS);
    m_userDataDir = appDataPath;
    
    m_initialized = true;
}
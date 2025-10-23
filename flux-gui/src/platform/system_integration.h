#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QSettings>

namespace FluxGUI::Platform {

/**
 * @brief System integration manager for cross-platform functionality
 * 
 * The SystemIntegration class provides platform-specific functionality including:
 * - Context menu registration (right-click integration)
 * - File association management
 * - System tray integration
 * - Desktop notifications
 * - Auto-start configuration
 */
class SystemIntegration : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Context menu actions available
     */
    enum class ContextAction {
        ExtractHere,        // Extract archive to current directory
        ExtractToFolder,    // Extract archive to new folder
        CompressToZip,      // Compress selection to ZIP
        CompressTo7z,       // Compress selection to 7Z
        OpenWithFlux,       // Open archive with Flux
        AddToArchive        // Add files to existing archive
    };

    /**
     * @brief File association types
     */
    enum class AssociationType {
        Archive,            // Archive file types
        Compressed,         // Compressed file types
        All                 // All supported types
    };

    /**
     * @brief Get the singleton instance
     * @return Reference to SystemIntegration instance
     */
    static SystemIntegration& instance();
    
    /**
     * @brief Initialize system integration
     * @param applicationPath Path to the application executable
     * @return True if initialization succeeded
     */
    bool initialize(const QString& applicationPath);
    
    /**
     * @brief Check if running with administrator/root privileges
     * @return True if running with elevated privileges
     */
    bool hasElevatedPrivileges() const;
    
    /**
     * @brief Request administrator/root privileges
     * @param reason Reason for privilege escalation
     * @return True if privileges were granted
     */
    bool requestElevatedPrivileges(const QString& reason = QString());

    // Context Menu Integration
    
    /**
     * @brief Register context menu entries
     * @param actions List of actions to register
     * @return True if registration succeeded
     */
    bool registerContextMenu(const QList<ContextAction>& actions);
    
    /**
     * @brief Unregister context menu entries
     * @return True if unregistration succeeded
     */
    bool unregisterContextMenu();
    
    /**
     * @brief Check if context menu is registered
     * @return True if context menu is registered
     */
    bool isContextMenuRegistered() const;

    // File Associations
    
    /**
     * @brief Register file associations
     * @param types Types of associations to register
     * @return True if registration succeeded
     */
    bool registerFileAssociations(AssociationType types = AssociationType::All);
    
    /**
     * @brief Unregister file associations
     * @return True if unregistration succeeded
     */
    bool unregisterFileAssociations();
    
    /**
     * @brief Check if file associations are registered
     * @return True if associations are registered
     */
    bool areFileAssociationsRegistered() const;
    
    /**
     * @brief Get supported file extensions
     * @return List of supported extensions
     */
    QStringList getSupportedExtensions() const;

    // Auto-start Configuration
    
    /**
     * @brief Enable application auto-start
     * @param minimized Start minimized to system tray
     * @return True if auto-start was enabled
     */
    bool enableAutoStart(bool minimized = true);
    
    /**
     * @brief Disable application auto-start
     * @return True if auto-start was disabled
     */
    bool disableAutoStart();
    
    /**
     * @brief Check if auto-start is enabled
     * @return True if auto-start is enabled
     */
    bool isAutoStartEnabled() const;

    // System Tray Integration
    
    /**
     * @brief Check if system tray is available
     * @return True if system tray is available
     */
    bool isSystemTrayAvailable() const;
    
    /**
     * @brief Show system notification
     * @param title Notification title
     * @param message Notification message
     * @param icon Icon type
     * @param timeout Timeout in milliseconds (0 = system default)
     */
    void showNotification(const QString& title, 
                         const QString& message,
                         const QString& icon = QString(),
                         int timeout = 0);

    // Platform-specific Utilities
    
    /**
     * @brief Get platform name
     * @return Platform identifier string
     */
    QString getPlatformName() const;
    
    /**
     * @brief Get application data directory
     * @return Path to application data directory
     */
    QString getAppDataDirectory() const;
    
    /**
     * @brief Get temporary directory
     * @return Path to temporary directory
     */
    QString getTempDirectory() const;
    
    /**
     * @brief Open file manager at path
     * @param path Path to open
     * @param selectFile If true, select the file instead of opening directory
     */
    void openFileManager(const QString& path, bool selectFile = false);
    
    /**
     * @brief Open URL in default browser
     * @param url URL to open
     */
    void openUrl(const QString& url);

signals:
    /**
     * @brief Emitted when context menu action is triggered
     * @param action The triggered action
     * @param filePaths List of file paths the action was triggered on
     */
    void contextMenuActionTriggered(ContextAction action, const QStringList& filePaths);
    
    /**
     * @brief Emitted when system integration status changes
     * @param integrated True if integration is active
     */
    void integrationStatusChanged(bool integrated);

private:
    explicit SystemIntegration(QObject* parent = nullptr);
    ~SystemIntegration() = default;
    
    // Disable copy and assignment
    SystemIntegration(const SystemIntegration&) = delete;
    SystemIntegration& operator=(const SystemIntegration&) = delete;

    // Platform-specific implementations
    
#ifdef Q_OS_WIN
    /**
     * @brief Windows-specific context menu registration
     */
    bool registerWindowsContextMenu(const QList<ContextAction>& actions);
    bool unregisterWindowsContextMenu();
    bool isWindowsContextMenuRegistered() const;
    
    /**
     * @brief Windows-specific file associations
     */
    bool registerWindowsFileAssociations(AssociationType types);
    bool unregisterWindowsFileAssociations();
    bool areWindowsFileAssociationsRegistered() const;
    
    /**
     * @brief Windows-specific auto-start
     */
    bool enableWindowsAutoStart(bool minimized);
    bool disableWindowsAutoStart();
    bool isWindowsAutoStartEnabled() const;
    
    /**
     * @brief Windows-specific utilities
     */
    void openWindowsFileManager(const QString& path, bool selectFile);
    void showWindowsNotification(const QString& title, const QString& message, int timeout);
#endif

#ifdef Q_OS_MACOS
    /**
     * @brief macOS-specific context menu registration
     */
    bool registerMacOSContextMenu(const QList<ContextAction>& actions);
    bool unregisterMacOSContextMenu();
    bool isMacOSContextMenuRegistered() const;
    
    /**
     * @brief macOS-specific file associations
     */
    bool registerMacOSFileAssociations(AssociationType types);
    bool unregisterMacOSFileAssociations();
    bool areMacOSFileAssociationsRegistered() const;
    
    /**
     * @brief macOS-specific auto-start
     */
    bool enableMacOSAutoStart(bool minimized);
    bool disableMacOSAutoStart();
    bool isMacOSAutoStartEnabled() const;
    
    /**
     * @brief macOS-specific utilities
     */
    void openMacOSFileManager(const QString& path, bool selectFile);
    void showMacOSNotification(const QString& title, const QString& message, int timeout);
#endif

#ifdef Q_OS_LINUX
    /**
     * @brief Linux-specific context menu registration
     */
    bool registerLinuxContextMenu(const QList<ContextAction>& actions);
    bool unregisterLinuxContextMenu();
    bool isLinuxContextMenuRegistered() const;
    
    /**
     * @brief Linux-specific file associations
     */
    bool registerLinuxFileAssociations(AssociationType types);
    bool unregisterLinuxFileAssociations();
    bool areLinuxFileAssociationsRegistered() const;
    
    /**
     * @brief Linux-specific auto-start
     */
    bool enableLinuxAutoStart(bool minimized);
    bool disableLinuxAutoStart();
    bool isLinuxAutoStartEnabled() const;
    
    /**
     * @brief Linux-specific utilities
     */
    void openLinuxFileManager(const QString& path, bool selectFile);
    void showLinuxNotification(const QString& title, const QString& message, int timeout);
#endif

    /**
     * @brief Get context action command
     * @param action Context action
     * @return Command string for the action
     */
    QString getContextActionCommand(ContextAction action) const;
    
    /**
     * @brief Get context action display name
     * @param action Context action
     * @return Display name for the action
     */
    QString getContextActionDisplayName(ContextAction action) const;
    
    /**
     * @brief Get context action icon
     * @param action Context action
     * @return Icon path for the action
     */
    QString getContextActionIcon(ContextAction action) const;
    
    /**
     * @brief Create desktop entry file (Linux)
     * @param filePath Path to create the file
     * @param content File content
     * @return True if file was created successfully
     */
    bool createDesktopEntry(const QString& filePath, const QString& content);
    
    /**
     * @brief Write registry entry (Windows)
     * @param key Registry key
     * @param name Value name
     * @param value Value data
     * @return True if entry was written successfully
     */
    bool writeRegistryEntry(const QString& key, const QString& name, const QString& value);
    
    /**
     * @brief Delete registry entry (Windows)
     * @param key Registry key
     * @param name Value name (empty to delete key)
     * @return True if entry was deleted successfully
     */
    bool deleteRegistryEntry(const QString& key, const QString& name = QString());

private:
    QString m_applicationPath;
    QString m_applicationName = "Flux Archive Manager";
    QString m_applicationId = "com.flux.archivemanager";
    
    QSettings* m_settings = nullptr;
    
    // State tracking
    bool m_contextMenuRegistered = false;
    bool m_fileAssociationsRegistered = false;
    bool m_autoStartEnabled = false;
    bool m_initialized = false;
    
    // Supported file extensions
    QStringList m_archiveExtensions = {
        "zip", "rar", "7z", "tar", "gz", "bz2", "xz", "lzma", "Z"
    };
    
    QStringList m_compressedExtensions = {
        "tar.gz", "tar.bz2", "tar.xz", "tar.lzma", "tgz", "tbz", "tbz2", "txz"
    };
};

} // namespace FluxGUI::Platform

#pragma once

#include <QObject>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QHash>
#include <QPoint>
#include <QWidget>
#include <QMimeType>
#include <QFileInfo>
#include <QTimer>
#include <memory>

QT_BEGIN_NAMESPACE
class QKeySequence;
class QIcon;
QT_END_NAMESPACE

namespace FluxGUI::UI::Managers {

/**
 * Context Menu Manager
 * 
 * Centralized system for managing context menus throughout the application:
 * - Dynamic context-aware menu generation
 * - File type specific actions
 * - Archive operation shortcuts
 * - Plugin integration for custom actions
 * - Keyboard shortcuts and accessibility
 * - Menu customization and user preferences
 */
class ContextMenuManager : public QObject {
    Q_OBJECT

public:
    enum class MenuContext {
        FileList,           // File list context
        ArchiveContent,     // Archive content browser
        RecentFiles,        // Recent files list
        Navigation,         // Navigation panel
        Toolbar,            // Toolbar customization
        StatusBar,          // Status bar context
        General             // General application context
    };

    enum class ActionType {
        // File operations
        Open,
        OpenWith,
        Extract,
        ExtractTo,
        ExtractHere,
        
        // Archive operations
        CreateArchive,
        AddToArchive,
        ViewArchive,
        TestArchive,
        
        // Edit operations
        Cut,
        Copy,
        Paste,
        Delete,
        Rename,
        
        // View operations
        Refresh,
        Properties,
        Preview,
        
        // Navigation
        GoUp,
        GoBack,
        GoForward,
        GoHome,
        
        // Selection
        SelectAll,
        SelectNone,
        InvertSelection,
        
        // Tools
        Compare,
        Benchmark,
        Integrity,
        
        // Custom
        Custom
    };

    struct ActionInfo {
        ActionType type{ActionType::Custom};
        QString text;
        QString tooltip;
        QString statusTip;
        QIcon icon;
        QKeySequence shortcut;
        bool enabled{true};
        bool visible{true};
        bool checkable{false};
        bool checked{false};
        QString group; // For action groups
        QVariant data; // Custom data
        std::function<void()> callback;
    };

    struct MenuConfiguration {
        MenuContext context{MenuContext::General};
        QStringList enabledActions;
        QStringList disabledActions;
        QStringList hiddenActions;
        QHash<QString, QString> customTexts;
        QHash<QString, QIcon> customIcons;
        QHash<QString, QKeySequence> customShortcuts;
        bool showIcons{true};
        bool showShortcuts{true};
        bool showSeparators{true};
        int maxRecentItems{10};
    };

    explicit ContextMenuManager(QObject* parent = nullptr);
    ~ContextMenuManager() override;

    // Menu creation
    QMenu* createContextMenu(MenuContext context, QWidget* parent = nullptr);
    QMenu* createFileContextMenu(const QStringList& filePaths, QWidget* parent = nullptr);
    QMenu* createArchiveContextMenu(const QString& archivePath, QWidget* parent = nullptr);
    QMenu* createSelectionContextMenu(const QStringList& selectedItems, QWidget* parent = nullptr);

    // Action management
    void registerAction(const QString& id, const ActionInfo& info);
    void unregisterAction(const QString& id);
    QAction* getAction(const QString& id) const;
    QList<QAction*> getActions(const QStringList& ids) const;

    // Menu configuration
    void setMenuConfiguration(MenuContext context, const MenuConfiguration& config);
    MenuConfiguration menuConfiguration(MenuContext context) const;
    void resetMenuConfiguration(MenuContext context);

    // Dynamic actions
    void addDynamicAction(MenuContext context, const QString& id, const ActionInfo& info);
    void removeDynamicAction(MenuContext context, const QString& id);
    void clearDynamicActions(MenuContext context);

    // File type associations
    void registerFileTypeActions(const QString& mimeType, const QStringList& actionIds);
    void unregisterFileTypeActions(const QString& mimeType);
    QStringList getFileTypeActions(const QString& mimeType) const;
    QStringList getFileTypeActions(const QFileInfo& fileInfo) const;

    // Recent items management
    void addRecentItem(MenuContext context, const QString& item, const QString& displayText = QString());
    void removeRecentItem(MenuContext context, const QString& item);
    void clearRecentItems(MenuContext context);
    QStringList getRecentItems(MenuContext context) const;

    // Plugin integration
    void registerPluginActions(const QString& pluginId, const QList<ActionInfo>& actions);
    void unregisterPluginActions(const QString& pluginId);
    QStringList getPluginActions(const QString& pluginId) const;

    // Customization
    void setActionEnabled(const QString& id, bool enabled);
    void setActionVisible(const QString& id, bool visible);
    void setActionText(const QString& id, const QString& text);
    void setActionIcon(const QString& id, const QIcon& icon);
    void setActionShortcut(const QString& id, const QKeySequence& shortcut);

    // Menu display
    void showContextMenu(MenuContext context, const QPoint& position, QWidget* parent = nullptr);
    void showFileContextMenu(const QStringList& filePaths, const QPoint& position, QWidget* parent = nullptr);

    // Settings persistence
    void saveSettings();
    void loadSettings();
    void resetToDefaults();

signals:
    // Action signals
    void actionTriggered(const QString& actionId, const QVariant& data);
    void actionToggled(const QString& actionId, bool checked);
    
    // Menu signals
    void menuAboutToShow(MenuContext context, QMenu* menu);
    void menuAboutToHide(MenuContext context, QMenu* menu);
    
    // Configuration signals
    void configurationChanged(MenuContext context);
    void actionRegistered(const QString& actionId);
    void actionUnregistered(const QString& actionId);

private slots:
    void onActionTriggered();
    void onActionToggled(bool checked);
    void onMenuAboutToShow();
    void onMenuAboutToHide();
    void onRecentItemsCleanup();

private:
    // Menu building
    void buildMenu(QMenu* menu, MenuContext context, const QStringList& filePaths = QStringList());
    void addActionsToMenu(QMenu* menu, const QStringList& actionIds);
    void addSeparatorIfNeeded(QMenu* menu);
    void addRecentItemsToMenu(QMenu* menu, MenuContext context);
    void addFileTypeActionsToMenu(QMenu* menu, const QStringList& filePaths);
    void addPluginActionsToMenu(QMenu* menu, MenuContext context);

    // Action creation
    QAction* createAction(const QString& id, const ActionInfo& info, QObject* parent = nullptr);
    void updateAction(QAction* action, const ActionInfo& info);
    void configureAction(QAction* action, MenuContext context);

    // Context analysis
    QStringList analyzeFileContext(const QStringList& filePaths) const;
    QStringList analyzeArchiveContext(const QString& archivePath) const;
    QStringList analyzeSelectionContext(const QStringList& selectedItems) const;
    bool isArchiveFile(const QString& filePath) const;
    bool isImageFile(const QString& filePath) const;
    bool isTextFile(const QString& filePath) const;

    // Default actions
    void registerDefaultActions();
    void createFileOperationActions();
    void createArchiveOperationActions();
    void createEditOperationActions();
    void createViewOperationActions();
    void createNavigationActions();
    void createSelectionActions();
    void createToolActions();

    // Action callbacks
    void onOpenTriggered();
    void onOpenWithTriggered();
    void onExtractTriggered();
    void onExtractToTriggered();
    void onExtractHereTriggered();
    void onCreateArchiveTriggered();
    void onAddToArchiveTriggered();
    void onViewArchiveTriggered();
    void onTestArchiveTriggered();
    void onCutTriggered();
    void onCopyTriggered();
    void onPasteTriggered();
    void onDeleteTriggered();
    void onRenameTriggered();
    void onRefreshTriggered();
    void onPropertiesTriggered();
    void onPreviewTriggered();

    // Utility functions
    QString getActionText(ActionType type) const;
    QIcon getActionIcon(ActionType type) const;
    QKeySequence getActionShortcut(ActionType type) const;
    QString formatRecentItem(const QString& item) const;
    void cleanupOldMenus();

private:
    // Action registry
    QHash<QString, ActionInfo> m_registeredActions;
    QHash<QString, QAction*> m_actionInstances;
    QHash<MenuContext, QStringList> m_dynamicActions;

    // Menu configurations
    QHash<MenuContext, MenuConfiguration> m_menuConfigurations;

    // File type associations
    QHash<QString, QStringList> m_fileTypeActions;

    // Recent items
    QHash<MenuContext, QStringList> m_recentItems;
    QHash<MenuContext, QHash<QString, QString>> m_recentItemTexts;

    // Plugin actions
    QHash<QString, QStringList> m_pluginActions;

    // Active menus tracking
    QHash<QMenu*, MenuContext> m_activeMenus;
    QTimer* m_cleanupTimer{nullptr};

    // Current context data
    QStringList m_currentFilePaths;
    QString m_currentArchivePath;
    QStringList m_currentSelection;
    QWidget* m_currentParent{nullptr};

    // Constants
    static constexpr int MAX_RECENT_ITEMS_DEFAULT = 10;
    static constexpr int CLEANUP_INTERVAL = 60000; // 1 minute
    static constexpr int MAX_MENU_LIFETIME = 300000; // 5 minutes
};

/**
 * Context Menu Builder Helper
 */
class ContextMenuBuilder {
public:
    explicit ContextMenuBuilder(ContextMenuManager* manager);

    ContextMenuBuilder& addAction(const QString& id);
    ContextMenuBuilder& addActions(const QStringList& ids);
    ContextMenuBuilder& addSeparator();
    ContextMenuBuilder& addSubmenu(const QString& title, const QStringList& actionIds);
    ContextMenuBuilder& addRecentItems(ContextMenuManager::MenuContext context);
    ContextMenuBuilder& addFileTypeActions(const QStringList& filePaths);
    ContextMenuBuilder& addPluginActions(ContextMenuManager::MenuContext context);

    QMenu* build(QWidget* parent = nullptr);

private:
    ContextMenuManager* m_manager{nullptr};
    QStringList m_actionIds;
    QStringList m_separatorPositions;
    QHash<QString, QStringList> m_submenus;
    bool m_includeRecentItems{false};
    ContextMenuManager::MenuContext m_recentItemsContext{ContextMenuManager::MenuContext::General};
    bool m_includeFileTypeActions{false};
    QStringList m_fileTypeActionPaths;
    bool m_includePluginActions{false};
    ContextMenuManager::MenuContext m_pluginActionsContext{ContextMenuManager::MenuContext::General};
};

} // namespace FluxGUI::UI::Managers

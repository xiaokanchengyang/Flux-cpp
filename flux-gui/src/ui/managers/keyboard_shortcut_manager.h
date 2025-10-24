#pragma once

#include <QObject>
#include <QShortcut>
#include <QKeySequence>
#include <QAction>
#include <QWidget>
#include <QHash>
#include <QSettings>
#include <memory>

namespace FluxGUI::UI::Managers {

/**
 * Keyboard Shortcut Manager
 * 
 * Provides comprehensive keyboard shortcut management for power users:
 * - Customizable shortcuts with conflict detection
 * - Context-aware shortcuts that change based on current view
 * - Shortcut discovery and help system
 * - Import/export of shortcut configurations
 */
class KeyboardShortcutManager : public QObject {
    Q_OBJECT

public:
    enum class ShortcutContext {
        Global,         // Available everywhere
        Welcome,        // Welcome view specific
        Archive,        // Archive browsing specific
        Creation,       // Archive creation specific
        Extraction,     // Archive extraction specific
        Settings        // Settings view specific
    };

    struct ShortcutInfo {
        QString id;
        QString description;
        QKeySequence defaultSequence;
        QKeySequence currentSequence;
        ShortcutContext context;
        bool customizable{true};
        bool enabled{true};
    };

    explicit KeyboardShortcutManager(QWidget* parent = nullptr);
    ~KeyboardShortcutManager() override;

    // Shortcut registration
    void registerShortcut(const QString& id, const QString& description,
                         const QKeySequence& defaultSequence, ShortcutContext context = ShortcutContext::Global);
    void registerAction(QAction* action, const QString& id, ShortcutContext context = ShortcutContext::Global);
    
    // Shortcut management
    bool setShortcut(const QString& id, const QKeySequence& sequence);
    QKeySequence getShortcut(const QString& id) const;
    void resetShortcut(const QString& id);
    void resetAllShortcuts();
    
    // Context management
    void setCurrentContext(ShortcutContext context);
    ShortcutContext currentContext() const { return m_currentContext; }
    
    // Configuration
    void loadShortcuts();
    void saveShortcuts();
    void importShortcuts(const QString& filePath);
    void exportShortcuts(const QString& filePath);
    
    // Query interface
    QList<ShortcutInfo> getAllShortcuts() const;
    QList<ShortcutInfo> getShortcutsForContext(ShortcutContext context) const;
    QStringList getConflicts(const QKeySequence& sequence, const QString& excludeId = QString()) const;
    
    // Help and discovery
    QString getShortcutHelp(ShortcutContext context = ShortcutContext::Global) const;
    QStringList searchShortcuts(const QString& query) const;

signals:
    // Shortcut signals
    void shortcutActivated(const QString& id);
    void shortcutChanged(const QString& id, const QKeySequence& oldSequence, const QKeySequence& newSequence);
    void contextChanged(ShortcutContext oldContext, ShortcutContext newContext);
    
    // Configuration signals
    void shortcutsLoaded();
    void shortcutsSaved();
    void conflictDetected(const QString& id, const QStringList& conflictingIds);

private slots:
    void onShortcutActivated();
    void onActionTriggered();

private:
    // Setup methods
    void initializeDefaultShortcuts();
    void setupGlobalShortcuts();
    void setupContextualShortcuts();
    void connectShortcuts();
    
    // Shortcut management helpers
    void updateShortcutState(const QString& id);
    void enableShortcutsForContext(ShortcutContext context);
    void disableShortcutsForContext(ShortcutContext context);
    bool isShortcutConflict(const QKeySequence& sequence, const QString& excludeId) const;
    
    // Persistence helpers
    QString contextToString(ShortcutContext context) const;
    ShortcutContext stringToContext(const QString& contextStr) const;

private:
    // Parent widget for shortcut scope
    QWidget* m_parentWidget{nullptr};
    
    // Shortcut storage
    QHash<QString, ShortcutInfo> m_shortcuts;
    QHash<QString, std::unique_ptr<QShortcut>> m_shortcutObjects;
    QHash<QString, QAction*> m_actions;
    
    // Context management
    ShortcutContext m_currentContext{ShortcutContext::Global};
    QHash<ShortcutContext, QStringList> m_contextShortcuts;
    
    // Settings
    std::unique_ptr<QSettings> m_settings;
    
    // Default shortcuts definition
    static const QHash<QString, ShortcutInfo> s_defaultShortcuts;
};

} // namespace FluxGUI::UI::Managers

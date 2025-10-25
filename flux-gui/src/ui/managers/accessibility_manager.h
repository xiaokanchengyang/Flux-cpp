#pragma once

#include <QObject>
#include <QWidget>
#include <QKeySequence>
#include <QShortcut>
#include <QAccessible>
#include <QAccessibleWidget>
#include <QHash>
#include <QStack>
#include <QTimer>
#include <memory>

QT_BEGIN_NAMESPACE
class QFocusFrame;
class QLabel;
class QSoundEffect;
QT_END_NAMESPACE

namespace FluxGUI::UI::Managers {

/**
 * Accessibility Manager
 * 
 * Comprehensive accessibility system providing:
 * - Complete keyboard navigation with focus management
 * - Screen reader compatibility and announcements
 * - High contrast and reduced motion support
 * - Focus indicators and visual accessibility aids
 * - Keyboard shortcuts and alternative input methods
 * - WCAG 2.1 AA compliance features
 */
class AccessibilityManager : public QObject {
    Q_OBJECT

public:
    enum class NavigationMode {
        Standard,       // Normal tab-based navigation
        Spatial,        // Arrow key navigation
        List,           // List-style navigation (up/down)
        Tree,           // Tree navigation with expand/collapse
        Grid            // Grid navigation (arrow keys in 2D)
    };

    enum class AnnouncementPriority {
        Low,            // Background information
        Medium,         // Standard user feedback
        High,           // Important state changes
        Critical        // Urgent information (errors, warnings)
    };

    enum class FocusStyle {
        Default,        // System default focus indicator
        Enhanced,       // High contrast focus ring
        Custom,         // Application-specific focus style
        HighContrast    // Maximum contrast for low vision
    };

    struct AccessibilitySettings {
        bool screenReaderEnabled{false};
        bool highContrastMode{false};
        bool reducedMotion{false};
        bool soundFeedback{false};
        bool enhancedFocus{true};
        bool keyboardNavigation{true};
        FocusStyle focusStyle{FocusStyle::Enhanced};
        int focusTimeout{0}; // 0 = no timeout
        double textScale{1.0};
        bool announceStateChanges{true};
    };

    explicit AccessibilityManager(QObject* parent = nullptr);
    ~AccessibilityManager() override;

    // Settings management
    void setSettings(const AccessibilitySettings& settings);
    AccessibilitySettings settings() const { return m_settings; }
    void loadSettings();
    void saveSettings();

    // Widget registration and management
    void registerWidget(QWidget* widget, const QString& role = QString(), const QString& description = QString());
    void unregisterWidget(QWidget* widget);
    void setWidgetRole(QWidget* widget, const QString& role);
    void setWidgetDescription(QWidget* widget, const QString& description);
    void setWidgetShortcut(QWidget* widget, const QKeySequence& shortcut);

    // Navigation management
    void setNavigationMode(QWidget* container, NavigationMode mode);
    void setFocusOrder(const QList<QWidget*>& widgets);
    void addToFocusChain(QWidget* widget, QWidget* after = nullptr);
    void removeFromFocusChain(QWidget* widget);

    // Focus management
    void setFocus(QWidget* widget, Qt::FocusReason reason = Qt::OtherFocusReason);
    QWidget* currentFocusWidget() const;
    void saveFocusState();
    void restoreFocusState();
    void clearFocusHistory();

    // Screen reader support
    void announce(const QString& text, AnnouncementPriority priority = AnnouncementPriority::Medium);
    void announceStateChange(QWidget* widget, const QString& newState);
    void announceProgress(int percentage, const QString& operation = QString());
    void announceError(const QString& error);
    void announceSuccess(const QString& message);

    // Keyboard shortcuts
    void registerGlobalShortcut(const QKeySequence& sequence, const QString& description, std::function<void()> callback);
    void registerContextShortcut(QWidget* context, const QKeySequence& sequence, const QString& description, std::function<void()> callback);
    void unregisterShortcut(const QKeySequence& sequence);
    QStringList getShortcutHelp() const;

    // Visual accessibility
    void enableHighContrast(bool enable);
    void setTextScale(double scale);
    void enableReducedMotion(bool enable);
    void setFocusStyle(FocusStyle style);

    // Audio feedback
    void enableSoundFeedback(bool enable);
    void playFocusSound();
    void playActivationSound();
    void playErrorSound();
    void playSuccessSound();

    // Utility functions
    bool isScreenReaderActive() const;
    bool isHighContrastActive() const;
    bool isReducedMotionActive() const;
    void updateSystemAccessibilityState();

signals:
    // Settings change signals
    void settingsChanged(const AccessibilitySettings& newSettings);
    void highContrastChanged(bool enabled);
    void reducedMotionChanged(bool enabled);
    void textScaleChanged(double scale);

    // Focus signals
    void focusChanged(QWidget* oldWidget, QWidget* newWidget);
    void focusChainChanged();

    // Navigation signals
    void navigationModeChanged(QWidget* container, NavigationMode mode);
    void shortcutActivated(const QKeySequence& sequence, const QString& description);

    // Announcement signals
    void announcementRequested(const QString& text, AnnouncementPriority priority);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onFocusChanged(QWidget* old, QWidget* now);
    void onShortcutActivated();
    void onAnnouncementTimeout();
    void onSystemAccessibilityChanged();

private:
    // Widget management
    struct WidgetInfo {
        QString role;
        QString description;
        QKeySequence shortcut;
        NavigationMode navigationMode{NavigationMode::Standard};
        bool registered{false};
    };

    // Focus management
    void updateFocusIndicator(QWidget* widget);
    void createFocusFrame();
    void positionFocusFrame(QWidget* widget);
    void hideFocusFrame();

    // Navigation helpers
    QWidget* findNextFocusWidget(QWidget* current, NavigationMode mode, bool forward = true);
    QWidget* findSpatialFocus(QWidget* current, Qt::Key direction);
    QWidget* findListFocus(QWidget* current, bool down);
    QWidget* findTreeFocus(QWidget* current, Qt::Key direction);
    QWidget* findGridFocus(QWidget* current, Qt::Key direction);

    // Screen reader integration
    void initializeScreenReaderSupport();
    void updateAccessibleProperties(QWidget* widget);
    void queueAnnouncement(const QString& text, AnnouncementPriority priority);
    void processAnnouncementQueue();

    // Keyboard shortcut management
    struct ShortcutInfo {
        QKeySequence sequence;
        QString description;
        std::function<void()> callback;
        QWidget* context{nullptr};
        QShortcut* shortcut{nullptr};
    };

    void createShortcut(const ShortcutInfo& info);
    void removeShortcut(const QKeySequence& sequence);

    // Visual accessibility helpers
    void applyHighContrastTheme();
    void applyStandardTheme();
    void updateTextScale();
    void updateFocusStyles();

    // Audio system
    void initializeAudioFeedback();
    void loadSoundEffects();

    // System integration
    void detectSystemAccessibilityFeatures();
    void registerAccessibilityInterface();

private:
    // Settings
    AccessibilitySettings m_settings;

    // Widget tracking
    QHash<QWidget*, WidgetInfo> m_registeredWidgets;
    QHash<QWidget*, NavigationMode> m_navigationModes;
    QList<QWidget*> m_focusOrder;

    // Focus management
    QWidget* m_currentFocus{nullptr};
    QStack<QWidget*> m_focusHistory;
    QFocusFrame* m_focusFrame{nullptr};
    QTimer* m_focusTimer{nullptr};

    // Screen reader support
    QQueue<QPair<QString, AnnouncementPriority>> m_announcementQueue;
    QTimer* m_announcementTimer{nullptr};
    bool m_screenReaderDetected{false};

    // Keyboard shortcuts
    QHash<QKeySequence, ShortcutInfo> m_shortcuts;
    QHash<QWidget*, QList<QKeySequence>> m_contextShortcuts;

    // Audio feedback
    std::unique_ptr<QSoundEffect> m_focusSound;
    std::unique_ptr<QSoundEffect> m_activationSound;
    std::unique_ptr<QSoundEffect> m_errorSound;
    std::unique_ptr<QSoundEffect> m_successSound;

    // Visual accessibility
    QString m_originalStyleSheet;
    double m_currentTextScale{1.0};
    bool m_highContrastActive{false};

    // Constants
    static constexpr int ANNOUNCEMENT_INTERVAL = 100;
    static constexpr int FOCUS_FRAME_WIDTH = 3;
    static constexpr int FOCUS_TIMEOUT_DEFAULT = 30000; // 30 seconds
    static constexpr double MIN_TEXT_SCALE = 0.5;
    static constexpr double MAX_TEXT_SCALE = 3.0;
};

/**
 * Custom Accessible Interface for enhanced screen reader support
 */
class FluxAccessibleInterface : public QAccessibleWidget {
public:
    explicit FluxAccessibleInterface(QWidget* widget, const QString& role = QString());

    // QAccessibleInterface implementation
    QString text(QAccessible::Text type) const override;
    void setText(QAccessible::Text type, const QString& text) override;
    QAccessible::State state() const override;
    QAccessible::Role role() const override;
    QRect rect() const override;

    // Custom methods
    void setCustomRole(const QString& role);
    void setCustomDescription(const QString& description);
    void announceStateChange(const QString& newState);

private:
    QString m_customRole;
    QString m_customDescription;
    QAccessible::Role m_role{QAccessible::NoRole};
};

} // namespace FluxGUI::UI::Managers

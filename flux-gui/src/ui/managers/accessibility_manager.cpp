#include "accessibility_manager.h"

#include <QApplication>
#include <QWidget>
#include <QFocusFrame>
#include <QLabel>
#include <QShortcut>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QTimer>
#include <QAccessible>
#include <QAccessibleWidget>
#include <QSoundEffect>
#include <QUrl>
#include <QStyleOption>
#include <QPainter>
#include <QEvent>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QShowEvent>
#include <QHideEvent>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#ifdef Q_OS_LINUX
#include <QDBusConnection>
#include <QDBusMessage>
#endif

namespace FluxGUI::UI::Managers {

AccessibilityManager::AccessibilityManager(QObject* parent)
    : QObject(parent)
    , m_announcementTimer(new QTimer(this))
    , m_focusTimer(new QTimer(this))
{
    // Setup announcement timer
    m_announcementTimer->setSingleShot(true);
    m_announcementTimer->setInterval(ANNOUNCEMENT_INTERVAL);
    connect(m_announcementTimer, &QTimer::timeout, this, &AccessibilityManager::onAnnouncementTimeout);
    
    // Setup focus timer
    m_focusTimer->setSingleShot(true);
    connect(m_focusTimer, &QTimer::timeout, this, &AccessibilityManager::onAnnouncementTimeout);
    
    // Connect to application focus changes
    connect(qApp, &QApplication::focusChanged, this, &AccessibilityManager::onFocusChanged);
    
    // Load settings
    loadSettings();
    
    // Initialize subsystems
    initializeScreenReaderSupport();
    initializeAudioFeedback();
    createFocusFrame();
    
    // Detect system accessibility features
    detectSystemAccessibilityFeatures();
    updateSystemAccessibilityState();
    
    // Register accessibility interface
    registerAccessibilityInterface();
}

AccessibilityManager::~AccessibilityManager() {
    saveSettings();
    
    // Cleanup shortcuts
    for (auto& shortcutInfo : m_shortcuts) {
        delete shortcutInfo.shortcut;
    }
    
    // Cleanup focus frame
    delete m_focusFrame;
}

void AccessibilityManager::setSettings(const AccessibilitySettings& settings) {
    AccessibilitySettings oldSettings = m_settings;
    m_settings = settings;
    
    // Apply changes
    if (oldSettings.highContrastMode != settings.highContrastMode) {
        enableHighContrast(settings.highContrastMode);
    }
    
    if (oldSettings.textScale != settings.textScale) {
        setTextScale(settings.textScale);
    }
    
    if (oldSettings.reducedMotion != settings.reducedMotion) {
        enableReducedMotion(settings.reducedMotion);
    }
    
    if (oldSettings.focusStyle != settings.focusStyle) {
        setFocusStyle(settings.focusStyle);
    }
    
    if (oldSettings.soundFeedback != settings.soundFeedback) {
        enableSoundFeedback(settings.soundFeedback);
    }
    
    // Update focus timeout
    if (settings.focusTimeout > 0) {
        m_focusTimer->setInterval(settings.focusTimeout);
    }
    
    emit settingsChanged(settings);
}

void AccessibilityManager::loadSettings() {
    QSettings settings("FluxGUI", "Accessibility");
    
    m_settings.screenReaderEnabled = settings.value("screenReaderEnabled", false).toBool();
    m_settings.highContrastMode = settings.value("highContrastMode", false).toBool();
    m_settings.reducedMotion = settings.value("reducedMotion", false).toBool();
    m_settings.soundFeedback = settings.value("soundFeedback", false).toBool();
    m_settings.enhancedFocus = settings.value("enhancedFocus", true).toBool();
    m_settings.keyboardNavigation = settings.value("keyboardNavigation", true).toBool();
    m_settings.focusStyle = static_cast<FocusStyle>(settings.value("focusStyle", static_cast<int>(FocusStyle::Enhanced)).toInt());
    m_settings.focusTimeout = settings.value("focusTimeout", 0).toInt();
    m_settings.textScale = settings.value("textScale", 1.0).toDouble();
    m_settings.announceStateChanges = settings.value("announceStateChanges", true).toBool();
}

void AccessibilityManager::saveSettings() {
    QSettings settings("FluxGUI", "Accessibility");
    
    settings.setValue("screenReaderEnabled", m_settings.screenReaderEnabled);
    settings.setValue("highContrastMode", m_settings.highContrastMode);
    settings.setValue("reducedMotion", m_settings.reducedMotion);
    settings.setValue("soundFeedback", m_settings.soundFeedback);
    settings.setValue("enhancedFocus", m_settings.enhancedFocus);
    settings.setValue("keyboardNavigation", m_settings.keyboardNavigation);
    settings.setValue("focusStyle", static_cast<int>(m_settings.focusStyle));
    settings.setValue("focusTimeout", m_settings.focusTimeout);
    settings.setValue("textScale", m_settings.textScale);
    settings.setValue("announceStateChanges", m_settings.announceStateChanges);
}

void AccessibilityManager::registerWidget(QWidget* widget, const QString& role, const QString& description) {
    if (!widget) {
        return;
    }
    
    WidgetInfo info;
    info.role = role;
    info.description = description;
    info.registered = true;
    
    m_registeredWidgets[widget] = info;
    
    // Install event filter
    widget->installEventFilter(this);
    
    // Update accessible properties
    updateAccessibleProperties(widget);
    
    // Set focus policy if keyboard navigation is enabled
    if (m_settings.keyboardNavigation && widget->focusPolicy() == Qt::NoFocus) {
        widget->setFocusPolicy(Qt::TabFocus);
    }
}

void AccessibilityManager::unregisterWidget(QWidget* widget) {
    if (!widget) {
        return;
    }
    
    m_registeredWidgets.remove(widget);
    m_navigationModes.remove(widget);
    
    // Remove from focus order
    m_focusOrder.removeAll(widget);
    
    // Remove event filter
    widget->removeEventFilter(this);
    
    // Remove context shortcuts
    if (m_contextShortcuts.contains(widget)) {
        for (const auto& sequence : m_contextShortcuts[widget]) {
            unregisterShortcut(sequence);
        }
        m_contextShortcuts.remove(widget);
    }
}

void AccessibilityManager::setWidgetRole(QWidget* widget, const QString& role) {
    if (!widget || !m_registeredWidgets.contains(widget)) {
        return;
    }
    
    m_registeredWidgets[widget].role = role;
    updateAccessibleProperties(widget);
}

void AccessibilityManager::setWidgetDescription(QWidget* widget, const QString& description) {
    if (!widget || !m_registeredWidgets.contains(widget)) {
        return;
    }
    
    m_registeredWidgets[widget].description = description;
    updateAccessibleProperties(widget);
}

void AccessibilityManager::setWidgetShortcut(QWidget* widget, const QKeySequence& shortcut) {
    if (!widget || !m_registeredWidgets.contains(widget)) {
        return;
    }
    
    m_registeredWidgets[widget].shortcut = shortcut;
    
    // Register context shortcut
    registerContextShortcut(widget, shortcut, QString("Activate %1").arg(widget->objectName()), [widget]() {
        if (widget->isEnabled() && widget->isVisible()) {
            widget->setFocus();
            // Simulate activation
            QKeyEvent event(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
            QApplication::sendEvent(widget, &event);
        }
    });
}

void AccessibilityManager::setNavigationMode(QWidget* container, NavigationMode mode) {
    if (!container) {
        return;
    }
    
    m_navigationModes[container] = mode;
    emit navigationModeChanged(container, mode);
}

void AccessibilityManager::setFocusOrder(const QList<QWidget*>& widgets) {
    m_focusOrder = widgets;
    
    // Set Qt focus chain
    for (int i = 0; i < widgets.size() - 1; ++i) {
        QWidget::setTabOrder(widgets[i], widgets[i + 1]);
    }
    
    emit focusChainChanged();
}

void AccessibilityManager::addToFocusChain(QWidget* widget, QWidget* after) {
    if (!widget) {
        return;
    }
    
    if (after && m_focusOrder.contains(after)) {
        int index = m_focusOrder.indexOf(after);
        m_focusOrder.insert(index + 1, widget);
    } else {
        m_focusOrder.append(widget);
    }
    
    // Update Qt focus chain
    if (after) {
        QWidget::setTabOrder(after, widget);
    }
    
    emit focusChainChanged();
}

void AccessibilityManager::removeFromFocusChain(QWidget* widget) {
    m_focusOrder.removeAll(widget);
    emit focusChainChanged();
}

void AccessibilityManager::setFocus(QWidget* widget, Qt::FocusReason reason) {
    if (!widget || !widget->isEnabled() || !widget->isVisible()) {
        return;
    }
    
    // Save current focus for history
    if (m_currentFocus && m_currentFocus != widget) {
        m_focusHistory.push(m_currentFocus);
    }
    
    m_currentFocus = widget;
    widget->setFocus(reason);
    
    // Update focus indicator
    if (m_settings.enhancedFocus) {
        updateFocusIndicator(widget);
    }
    
    // Play focus sound
    if (m_settings.soundFeedback) {
        playFocusSound();
    }
    
    // Announce focus change
    if (m_settings.announceStateChanges && m_registeredWidgets.contains(widget)) {
        const WidgetInfo& info = m_registeredWidgets[widget];
        QString announcement = QString("Focused %1").arg(info.description.isEmpty() ? widget->objectName() : info.description);
        announce(announcement, AnnouncementPriority::Medium);
    }
    
    // Start focus timeout if configured
    if (m_settings.focusTimeout > 0) {
        m_focusTimer->start();
    }
}

QWidget* AccessibilityManager::currentFocusWidget() const {
    return m_currentFocus;
}

void AccessibilityManager::saveFocusState() {
    if (m_currentFocus) {
        m_focusHistory.push(m_currentFocus);
    }
}

void AccessibilityManager::restoreFocusState() {
    if (!m_focusHistory.isEmpty()) {
        QWidget* widget = m_focusHistory.pop();
        if (widget && widget->isEnabled() && widget->isVisible()) {
            setFocus(widget);
        }
    }
}

void AccessibilityManager::clearFocusHistory() {
    m_focusHistory.clear();
}

void AccessibilityManager::announce(const QString& text, AnnouncementPriority priority) {
    if (text.isEmpty() || !m_settings.announceStateChanges) {
        return;
    }
    
    // Queue announcement
    queueAnnouncement(text, priority);
    
    // Process queue if not already processing
    if (!m_announcementTimer->isActive()) {
        processAnnouncementQueue();
    }
    
    emit announcementRequested(text, priority);
}

void AccessibilityManager::announceStateChange(QWidget* widget, const QString& newState) {
    if (!widget || !m_registeredWidgets.contains(widget)) {
        return;
    }
    
    const WidgetInfo& info = m_registeredWidgets[widget];
    QString announcement = QString("%1 %2").arg(info.description.isEmpty() ? widget->objectName() : info.description, newState);
    announce(announcement, AnnouncementPriority::Medium);
}

void AccessibilityManager::announceProgress(int percentage, const QString& operation) {
    QString announcement;
    if (operation.isEmpty()) {
        announcement = QString("Progress %1 percent").arg(percentage);
    } else {
        announcement = QString("%1 progress %2 percent").arg(operation).arg(percentage);
    }
    announce(announcement, AnnouncementPriority::Low);
}

void AccessibilityManager::announceError(const QString& error) {
    announce(QString("Error: %1").arg(error), AnnouncementPriority::Critical);
    
    if (m_settings.soundFeedback) {
        playErrorSound();
    }
}

void AccessibilityManager::announceSuccess(const QString& message) {
    announce(QString("Success: %1").arg(message), AnnouncementPriority::Medium);
    
    if (m_settings.soundFeedback) {
        playSuccessSound();
    }
}

void AccessibilityManager::registerGlobalShortcut(const QKeySequence& sequence, const QString& description, std::function<void()> callback) {
    ShortcutInfo info;
    info.sequence = sequence;
    info.description = description;
    info.callback = callback;
    info.context = nullptr;
    
    createShortcut(info);
    m_shortcuts[sequence] = info;
}

void AccessibilityManager::registerContextShortcut(QWidget* context, const QKeySequence& sequence, const QString& description, std::function<void()> callback) {
    if (!context) {
        return;
    }
    
    ShortcutInfo info;
    info.sequence = sequence;
    info.description = description;
    info.callback = callback;
    info.context = context;
    
    createShortcut(info);
    m_shortcuts[sequence] = info;
    
    // Track context shortcuts
    if (!m_contextShortcuts.contains(context)) {
        m_contextShortcuts[context] = QList<QKeySequence>();
    }
    m_contextShortcuts[context].append(sequence);
}

void AccessibilityManager::unregisterShortcut(const QKeySequence& sequence) {
    if (m_shortcuts.contains(sequence)) {
        ShortcutInfo info = m_shortcuts[sequence];
        delete info.shortcut;
        m_shortcuts.remove(sequence);
        
        // Remove from context shortcuts
        if (info.context && m_contextShortcuts.contains(info.context)) {
            m_contextShortcuts[info.context].removeAll(sequence);
        }
    }
}

QStringList AccessibilityManager::getShortcutHelp() const {
    QStringList help;
    
    for (const auto& info : m_shortcuts) {
        help.append(QString("%1: %2").arg(info.sequence.toString(), info.description));
    }
    
    return help;
}

void AccessibilityManager::enableHighContrast(bool enable) {
    if (m_highContrastActive == enable) {
        return;
    }
    
    m_highContrastActive = enable;
    
    if (enable) {
        applyHighContrastTheme();
    } else {
        applyStandardTheme();
    }
    
    emit highContrastChanged(enable);
}

void AccessibilityManager::setTextScale(double scale) {
    scale = qBound(MIN_TEXT_SCALE, scale, MAX_TEXT_SCALE);
    
    if (qFuzzyCompare(m_currentTextScale, scale)) {
        return;
    }
    
    m_currentTextScale = scale;
    updateTextScale();
    
    emit textScaleChanged(scale);
}

void AccessibilityManager::enableReducedMotion(bool enable) {
    m_settings.reducedMotion = enable;
    emit reducedMotionChanged(enable);
}

void AccessibilityManager::setFocusStyle(FocusStyle style) {
    m_settings.focusStyle = style;
    updateFocusStyles();
}

void AccessibilityManager::enableSoundFeedback(bool enable) {
    m_settings.soundFeedback = enable;
    
    if (enable && !m_focusSound) {
        loadSoundEffects();
    }
}

void AccessibilityManager::playFocusSound() {
    if (m_settings.soundFeedback && m_focusSound) {
        m_focusSound->play();
    }
}

void AccessibilityManager::playActivationSound() {
    if (m_settings.soundFeedback && m_activationSound) {
        m_activationSound->play();
    }
}

void AccessibilityManager::playErrorSound() {
    if (m_settings.soundFeedback && m_errorSound) {
        m_errorSound->play();
    }
}

void AccessibilityManager::playSuccessSound() {
    if (m_settings.soundFeedback && m_successSound) {
        m_successSound->play();
    }
}

bool AccessibilityManager::isScreenReaderActive() const {
    return m_screenReaderDetected || m_settings.screenReaderEnabled;
}

bool AccessibilityManager::isHighContrastActive() const {
    return m_highContrastActive;
}

bool AccessibilityManager::isReducedMotionActive() const {
    return m_settings.reducedMotion;
}

void AccessibilityManager::updateSystemAccessibilityState() {
    detectSystemAccessibilityFeatures();
    
    // Update settings based on system state
    bool systemHighContrast = false;
    bool systemScreenReader = false;
    
#ifdef Q_OS_WIN
    HIGHCONTRAST hc = { sizeof(HIGHCONTRAST) };
    if (SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(HIGHCONTRAST), &hc, 0)) {
        systemHighContrast = (hc.dwFlags & HCF_HIGHCONTRASTON) != 0;
    }
    
    // Check for screen reader (simplified detection)
    systemScreenReader = GetSystemMetrics(SM_SCREENREADER) != 0;
#endif
    
#ifdef Q_OS_LINUX
    // Check for accessibility tools via D-Bus
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (bus.isConnected()) {
        QDBusMessage message = QDBusMessage::createMethodCall(
            "org.a11y.Bus", "/org/a11y/bus", "org.a11y.Bus", "GetAddress");
        QDBusMessage reply = bus.call(message);
        systemScreenReader = reply.type() == QDBusMessage::ReplyMessage;
    }
#endif
    
    if (systemHighContrast != m_settings.highContrastMode) {
        enableHighContrast(systemHighContrast);
    }
    
    if (systemScreenReader != m_screenReaderDetected) {
        m_screenReaderDetected = systemScreenReader;
        m_settings.screenReaderEnabled = systemScreenReader;
    }
}

bool AccessibilityManager::eventFilter(QObject* watched, QEvent* event) {
    QWidget* widget = qobject_cast<QWidget*>(watched);
    if (!widget || !m_registeredWidgets.contains(widget)) {
        return QObject::eventFilter(watched, event);
    }
    
    switch (event->type()) {
    case QEvent::FocusIn:
        if (m_settings.enhancedFocus) {
            updateFocusIndicator(widget);
        }
        break;
        
    case QEvent::FocusOut:
        if (widget == m_currentFocus) {
            hideFocusFrame();
        }
        break;
        
    case QEvent::KeyPress: {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        
        // Handle navigation keys based on navigation mode
        if (m_navigationModes.contains(widget)) {
            NavigationMode mode = m_navigationModes[widget];
            QWidget* nextWidget = nullptr;
            
            switch (keyEvent->key()) {
            case Qt::Key_Up:
                if (mode == NavigationMode::Spatial || mode == NavigationMode::List || mode == NavigationMode::Tree || mode == NavigationMode::Grid) {
                    nextWidget = findNextFocusWidget(widget, mode, false);
                }
                break;
            case Qt::Key_Down:
                if (mode == NavigationMode::Spatial || mode == NavigationMode::List || mode == NavigationMode::Tree || mode == NavigationMode::Grid) {
                    nextWidget = findNextFocusWidget(widget, mode, true);
                }
                break;
            case Qt::Key_Left:
                if (mode == NavigationMode::Spatial || mode == NavigationMode::Tree || mode == NavigationMode::Grid) {
                    nextWidget = findSpatialFocus(widget, Qt::Key_Left);
                }
                break;
            case Qt::Key_Right:
                if (mode == NavigationMode::Spatial || mode == NavigationMode::Tree || mode == NavigationMode::Grid) {
                    nextWidget = findSpatialFocus(widget, Qt::Key_Right);
                }
                break;
            }
            
            if (nextWidget) {
                setFocus(nextWidget, Qt::TabFocusReason);
                return true;
            }
        }
        break;
    }
    
    case QEvent::Show:
        updateAccessibleProperties(widget);
        break;
        
    case QEvent::EnabledChange:
        updateAccessibleProperties(widget);
        break;
        
    default:
        break;
    }
    
    return QObject::eventFilter(watched, event);
}

// Private implementation
void AccessibilityManager::updateFocusIndicator(QWidget* widget) {
    if (!widget || !m_focusFrame) {
        return;
    }
    
    positionFocusFrame(widget);
    m_focusFrame->setWidget(widget);
    m_focusFrame->show();
}

void AccessibilityManager::createFocusFrame() {
    m_focusFrame = new QFocusFrame();
    m_focusFrame->setFrameStyle(QFrame::Box);
    
    updateFocusStyles();
}

void AccessibilityManager::positionFocusFrame(QWidget* widget) {
    if (!m_focusFrame || !widget) {
        return;
    }
    
    // Position focus frame around widget
    QRect rect = widget->rect();
    rect.adjust(-FOCUS_FRAME_WIDTH, -FOCUS_FRAME_WIDTH, FOCUS_FRAME_WIDTH, FOCUS_FRAME_WIDTH);
    
    m_focusFrame->setGeometry(rect);
}

void AccessibilityManager::hideFocusFrame() {
    if (m_focusFrame) {
        m_focusFrame->hide();
    }
}

QWidget* AccessibilityManager::findNextFocusWidget(QWidget* current, NavigationMode mode, bool forward) {
    switch (mode) {
    case NavigationMode::Standard:
        return forward ? current->nextInFocusChain() : current->previousInFocusChain();
    case NavigationMode::Spatial:
        return findSpatialFocus(current, forward ? Qt::Key_Down : Qt::Key_Up);
    case NavigationMode::List:
        return findListFocus(current, forward);
    case NavigationMode::Tree:
        return findTreeFocus(current, forward ? Qt::Key_Down : Qt::Key_Up);
    case NavigationMode::Grid:
        return findGridFocus(current, forward ? Qt::Key_Down : Qt::Key_Up);
    }
    
    return nullptr;
}

QWidget* AccessibilityManager::findSpatialFocus(QWidget* current, Qt::Key direction) {
    if (!current || !current->parentWidget()) {
        return nullptr;
    }
    
    QWidget* parent = current->parentWidget();
    QList<QWidget*> siblings;
    
    // Find all focusable siblings
    for (QWidget* child : parent->findChildren<QWidget*>()) {
        if (child->focusPolicy() != Qt::NoFocus && child->isEnabled() && child->isVisible()) {
            siblings.append(child);
        }
    }
    
    if (siblings.size() <= 1) {
        return nullptr;
    }
    
    QRect currentRect = current->geometry();
    QWidget* bestMatch = nullptr;
    int bestDistance = INT_MAX;
    
    for (QWidget* sibling : siblings) {
        if (sibling == current) {
            continue;
        }
        
        QRect siblingRect = sibling->geometry();
        bool validDirection = false;
        int distance = 0;
        
        switch (direction) {
        case Qt::Key_Up:
            validDirection = siblingRect.bottom() <= currentRect.top();
            distance = currentRect.top() - siblingRect.bottom();
            break;
        case Qt::Key_Down:
            validDirection = siblingRect.top() >= currentRect.bottom();
            distance = siblingRect.top() - currentRect.bottom();
            break;
        case Qt::Key_Left:
            validDirection = siblingRect.right() <= currentRect.left();
            distance = currentRect.left() - siblingRect.right();
            break;
        case Qt::Key_Right:
            validDirection = siblingRect.left() >= currentRect.right();
            distance = siblingRect.left() - currentRect.right();
            break;
        default:
            break;
        }
        
        if (validDirection && distance < bestDistance) {
            bestDistance = distance;
            bestMatch = sibling;
        }
    }
    
    return bestMatch;
}

QWidget* AccessibilityManager::findListFocus(QWidget* current, bool down) {
    // Simple implementation - use focus chain
    return down ? current->nextInFocusChain() : current->previousInFocusChain();
}

QWidget* AccessibilityManager::findTreeFocus(QWidget* current, Qt::Key direction) {
    // For tree navigation, we need more context about the tree structure
    // This is a simplified implementation
    return findSpatialFocus(current, direction);
}

QWidget* AccessibilityManager::findGridFocus(QWidget* current, Qt::Key direction) {
    // For grid navigation, we need to know the grid dimensions
    // This is a simplified implementation
    return findSpatialFocus(current, direction);
}

void AccessibilityManager::initializeScreenReaderSupport() {
    // Enable Qt accessibility
    QAccessible::setActive(true);
    
    // Detect screen readers
    detectSystemAccessibilityFeatures();
}

void AccessibilityManager::updateAccessibleProperties(QWidget* widget) {
    if (!widget || !m_registeredWidgets.contains(widget)) {
        return;
    }
    
    const WidgetInfo& info = m_registeredWidgets[widget];
    
    // Set accessible name
    if (!info.description.isEmpty()) {
        widget->setAccessibleName(info.description);
    }
    
    // Set accessible description
    if (!info.role.isEmpty()) {
        widget->setAccessibleDescription(info.role);
    }
    
    // Update accessible interface
    QAccessibleInterface* interface = QAccessible::queryAccessibleInterface(widget);
    if (interface) {
        // Notify screen readers of changes
        QAccessible::updateAccessibility(QAccessibleEvent(QAccessible::NameChanged, widget, 0));
    }
}

void AccessibilityManager::queueAnnouncement(const QString& text, AnnouncementPriority priority) {
    // Remove lower priority announcements if queue is full
    while (m_announcementQueue.size() >= 10) {
        if (m_announcementQueue.first().second < priority) {
            m_announcementQueue.dequeue();
        } else {
            break;
        }
    }
    
    m_announcementQueue.enqueue({text, priority});
}

void AccessibilityManager::processAnnouncementQueue() {
    if (m_announcementQueue.isEmpty()) {
        return;
    }
    
    auto announcement = m_announcementQueue.dequeue();
    
    // Send to screen reader
    if (isScreenReaderActive()) {
        // Create accessible event for announcement
        QAccessibleEvent event(QAccessible::Alert, qApp->focusWidget(), 0);
        event.setValue(announcement.first);
        QAccessible::updateAccessibility(&event);
    }
    
    // Schedule next announcement
    if (!m_announcementQueue.isEmpty()) {
        m_announcementTimer->start();
    }
}

void AccessibilityManager::createShortcut(const ShortcutInfo& info) {
    QShortcut* shortcut = new QShortcut(info.sequence, info.context ? info.context : qApp->activeWindow());
    shortcut->setContext(info.context ? Qt::WidgetShortcut : Qt::ApplicationShortcut);
    
    connect(shortcut, &QShortcut::activated, this, [this, info]() {
        if (info.callback) {
            info.callback();
        }
        emit shortcutActivated(info.sequence, info.description);
    });
    
    // Store shortcut in info (non-const copy)
    const_cast<ShortcutInfo&>(info).shortcut = shortcut;
}

void AccessibilityManager::removeShortcut(const QKeySequence& sequence) {
    unregisterShortcut(sequence);
}

void AccessibilityManager::applyHighContrastTheme() {
    if (!qApp) {
        return;
    }
    
    // Store original stylesheet
    if (m_originalStyleSheet.isEmpty()) {
        m_originalStyleSheet = qApp->styleSheet();
    }
    
    // Apply high contrast stylesheet
    QString highContrastStyle = R"(
        QWidget {
            background-color: black;
            color: white;
            border: 1px solid white;
        }
        
        QWidget:focus {
            border: 3px solid yellow;
            background-color: #003366;
        }
        
        QPushButton {
            background-color: #333333;
            border: 2px solid white;
            padding: 4px;
        }
        
        QPushButton:hover {
            background-color: #666666;
        }
        
        QPushButton:pressed {
            background-color: #999999;
        }
        
        QLineEdit, QTextEdit, QPlainTextEdit {
            background-color: white;
            color: black;
            border: 2px solid black;
        }
        
        QComboBox {
            background-color: white;
            color: black;
            border: 2px solid black;
        }
        
        QListWidget, QTreeWidget, QTableWidget {
            background-color: white;
            color: black;
            alternate-background-color: #f0f0f0;
        }
        
        QMenuBar {
            background-color: black;
            color: white;
        }
        
        QMenuBar::item:selected {
            background-color: yellow;
            color: black;
        }
        
        QMenu {
            background-color: black;
            color: white;
            border: 2px solid white;
        }
        
        QMenu::item:selected {
            background-color: yellow;
            color: black;
        }
    )";
    
    qApp->setStyleSheet(highContrastStyle);
}

void AccessibilityManager::applyStandardTheme() {
    if (!qApp) {
        return;
    }
    
    // Restore original stylesheet
    qApp->setStyleSheet(m_originalStyleSheet);
}

void AccessibilityManager::updateTextScale() {
    if (!qApp) {
        return;
    }
    
    // Apply text scaling to application font
    QFont font = qApp->font();
    int originalSize = font.pointSize();
    if (originalSize <= 0) {
        originalSize = 10; // Default size
    }
    
    font.setPointSize(static_cast<int>(originalSize * m_currentTextScale));
    qApp->setFont(font);
}

void AccessibilityManager::updateFocusStyles() {
    if (!m_focusFrame) {
        return;
    }
    
    QString styleSheet;
    
    switch (m_settings.focusStyle) {
    case FocusStyle::Default:
        styleSheet = "QFocusFrame { border: 1px solid palette(highlight); }";
        break;
    case FocusStyle::Enhanced:
        styleSheet = "QFocusFrame { border: 2px solid #0078d4; border-radius: 2px; }";
        break;
    case FocusStyle::Custom:
        styleSheet = "QFocusFrame { border: 3px solid #ff6b35; border-radius: 4px; }";
        break;
    case FocusStyle::HighContrast:
        styleSheet = "QFocusFrame { border: 4px solid yellow; background-color: rgba(255, 255, 0, 50); }";
        break;
    }
    
    m_focusFrame->setStyleSheet(styleSheet);
}

void AccessibilityManager::initializeAudioFeedback() {
    if (!m_settings.soundFeedback) {
        return;
    }
    
    loadSoundEffects();
}

void AccessibilityManager::loadSoundEffects() {
    // Load sound effects from resources or system sounds
    QString soundDir = ":/sounds/accessibility/";
    
    m_focusSound = std::make_unique<QSoundEffect>();
    m_focusSound->setSource(QUrl::fromLocalFile(soundDir + "focus.wav"));
    m_focusSound->setVolume(0.3f);
    
    m_activationSound = std::make_unique<QSoundEffect>();
    m_activationSound->setSource(QUrl::fromLocalFile(soundDir + "activate.wav"));
    m_activationSound->setVolume(0.5f);
    
    m_errorSound = std::make_unique<QSoundEffect>();
    m_errorSound->setSource(QUrl::fromLocalFile(soundDir + "error.wav"));
    m_errorSound->setVolume(0.7f);
    
    m_successSound = std::make_unique<QSoundEffect>();
    m_successSound->setSource(QUrl::fromLocalFile(soundDir + "success.wav"));
    m_successSound->setVolume(0.5f);
}

void AccessibilityManager::detectSystemAccessibilityFeatures() {
    // Platform-specific detection
#ifdef Q_OS_WIN
    m_screenReaderDetected = GetSystemMetrics(SM_SCREENREADER) != 0;
#endif
    
#ifdef Q_OS_LINUX
    // Check for common screen readers
    QStringList screenReaders = {"orca", "speakup", "brltty"};
    for (const QString& reader : screenReaders) {
        if (QProcess::execute("which", {reader}) == 0) {
            m_screenReaderDetected = true;
            break;
        }
    }
#endif
    
#ifdef Q_OS_MAC
    // Check for VoiceOver
    QProcess process;
    process.start("defaults", {"read", "com.apple.universalaccess", "voiceOverOnOffKey"});
    process.waitForFinished();
    m_screenReaderDetected = !process.readAllStandardOutput().isEmpty();
#endif
}

void AccessibilityManager::registerAccessibilityInterface() {
    // Register custom accessible interface factory if needed
    // This would be used for custom widgets that need special accessibility support
}

// Slot implementations
void AccessibilityManager::onFocusChanged(QWidget* old, QWidget* now) {
    Q_UNUSED(old)
    
    if (now && m_registeredWidgets.contains(now)) {
        m_currentFocus = now;
        
        if (m_settings.enhancedFocus) {
            updateFocusIndicator(now);
        }
        
        emit focusChanged(old, now);
    } else {
        hideFocusFrame();
    }
}

void AccessibilityManager::onShortcutActivated() {
    QShortcut* shortcut = qobject_cast<QShortcut*>(sender());
    if (!shortcut) {
        return;
    }
    
    // Find shortcut info
    for (const auto& info : m_shortcuts) {
        if (info.shortcut == shortcut) {
            emit shortcutActivated(info.sequence, info.description);
            break;
        }
    }
}

void AccessibilityManager::onAnnouncementTimeout() {
    processAnnouncementQueue();
}

void AccessibilityManager::onSystemAccessibilityChanged() {
    updateSystemAccessibilityState();
}

// FluxAccessibleInterface implementation
FluxAccessibleInterface::FluxAccessibleInterface(QWidget* widget, const QString& role)
    : QAccessibleWidget(widget)
    , m_customRole(role)
{
    // Map custom role to QAccessible::Role
    if (role == "button") {
        m_role = QAccessible::PushButton;
    } else if (role == "edit") {
        m_role = QAccessible::EditableText;
    } else if (role == "list") {
        m_role = QAccessible::List;
    } else if (role == "tree") {
        m_role = QAccessible::Tree;
    } else if (role == "table") {
        m_role = QAccessible::Table;
    } else {
        m_role = QAccessible::Client;
    }
}

QString FluxAccessibleInterface::text(QAccessible::Text type) const {
    switch (type) {
    case QAccessible::Name:
        if (!m_customDescription.isEmpty()) {
            return m_customDescription;
        }
        break;
    case QAccessible::Description:
        if (!m_customRole.isEmpty()) {
            return m_customRole;
        }
        break;
    default:
        break;
    }
    
    return QAccessibleWidget::text(type);
}

void FluxAccessibleInterface::setText(QAccessible::Text type, const QString& text) {
    switch (type) {
    case QAccessible::Name:
        m_customDescription = text;
        break;
    case QAccessible::Description:
        m_customRole = text;
        break;
    default:
        QAccessibleWidget::setText(type, text);
        break;
    }
}

QAccessible::State FluxAccessibleInterface::state() const {
    QAccessible::State state = QAccessibleWidget::state();
    
    // Add custom state information
    if (widget() && !widget()->isEnabled()) {
        state.disabled = true;
    }
    
    return state;
}

QAccessible::Role FluxAccessibleInterface::role() const {
    return m_role;
}

QRect FluxAccessibleInterface::rect() const {
    return QAccessibleWidget::rect();
}

void FluxAccessibleInterface::setCustomRole(const QString& role) {
    m_customRole = role;
}

void FluxAccessibleInterface::setCustomDescription(const QString& description) {
    m_customDescription = description;
}

void FluxAccessibleInterface::announceStateChange(const QString& newState) {
    // Create and send accessibility event
    QAccessibleEvent event(QAccessible::StateChanged, widget(), 0);
    event.setValue(newState);
    QAccessible::updateAccessibility(&event);
}

} // namespace FluxGUI::UI::Managers

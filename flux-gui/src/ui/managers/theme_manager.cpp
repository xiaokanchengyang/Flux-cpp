#include "theme_manager.h"

#include <QApplication>
#include <QStyleFactory>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QScreen>
#include <QWindow>
#include <QOperatingSystemVersion>
#include <QRegularExpression>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QPainterPath>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#endif

#ifdef Q_OS_MACOS
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace FluxGUI::UI::Managers {

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
    , m_currentTheme(ThemeMode::Light)
    , m_previousTheme(ThemeMode::Light)
    , m_systemThemeDetection(true)
    , m_highContrastMode(false)
    , m_styleSheetDirty(true)
{
    // Initialize settings
    QString settingsPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(settingsPath);
    m_settings = std::make_unique<QSettings>(settingsPath + "/theme.ini", QSettings::IniFormat);
    
    // Initialize components
    initializeThemes();
    initializeTypography();
    setupAnimations();
    setupSystemThemeWatcher();
    
    // Load saved settings
    loadThemeSettings();
    
    // Apply initial theme
    applyThemeToApplication();
    
    qDebug() << "ThemeManager initialized with theme:" << static_cast<int>(m_currentTheme);
}

ThemeManager::~ThemeManager() {
    saveThemeSettings();
}

void ThemeManager::setTheme(ThemeMode mode) {
    if (m_currentTheme == mode) return;
    
    ThemeMode oldTheme = m_currentTheme;
    m_previousTheme = m_currentTheme;
    m_currentTheme = mode;
    
    // Update color scheme
    m_currentColorScheme = generateColorScheme(mode);
    
    // Apply theme
    applyColorScheme(m_currentColorScheme);
    applyThemeToApplication();
    
    // Mark stylesheet as dirty
    m_styleSheetDirty = true;
    
    emit themeChanged(mode);
    emit colorSchemeChanged(m_currentColorScheme);
    
    qDebug() << "Theme changed from" << static_cast<int>(oldTheme) 
             << "to" << static_cast<int>(mode);
}

void ThemeManager::setCustomColorScheme(const ColorScheme& scheme) {
    if (!isValidColorScheme(scheme)) {
        qWarning() << "Invalid color scheme provided";
        return;
    }
    
    m_currentColorScheme = scheme;
    m_colorSchemes[ThemeMode::Custom] = scheme;
    
    if (m_currentTheme == ThemeMode::Custom) {
        applyColorScheme(scheme);
        applyThemeToApplication();
        m_styleSheetDirty = true;
    }
    
    emit colorSchemeChanged(scheme);
}

void ThemeManager::enableSystemThemeDetection(bool enabled) {
    if (m_systemThemeDetection == enabled) return;
    
    m_systemThemeDetection = enabled;
    
    if (enabled) {
        m_systemThemeWatcher->start();
        
        // Check current system theme
        if (m_currentTheme == ThemeMode::Auto) {
            ThemeMode detectedTheme = detectSystemTheme();
            if (detectedTheme != m_currentTheme) {
                setTheme(detectedTheme);
            }
        }
    } else {
        m_systemThemeWatcher->stop();
    }
}

ThemeManager::ThemeMode ThemeManager::detectSystemTheme() const {
#ifdef Q_OS_WIN
    // Windows 10/11 dark mode detection
    QSettings registry("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                      QSettings::NativeFormat);
    bool darkMode = registry.value("AppsUseLightTheme", 1).toInt() == 0;
    return darkMode ? ThemeMode::Dark : ThemeMode::Light;
    
#elif defined(Q_OS_MACOS)
    // macOS dark mode detection
    // This would require Objective-C++ code to check NSAppearance
    // For now, return Light as default
    return ThemeMode::Light;
    
#elif defined(Q_OS_LINUX)
    // Linux desktop environment detection
    QString desktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP").toLower();
    
    if (desktop.contains("gnome")) {
        // GNOME dark mode detection
        QProcess process;
        process.start("gsettings", {"get", "org.gnome.desktop.interface", "gtk-theme"});
        process.waitForFinished();
        QString theme = process.readAllStandardOutput().trimmed();
        return theme.contains("dark", Qt::CaseInsensitive) ? ThemeMode::Dark : ThemeMode::Light;
    } else if (desktop.contains("kde")) {
        // KDE Plasma dark mode detection
        QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
        QSettings kdeSettings(configPath + "/kdeglobals", QSettings::IniFormat);
        QString colorScheme = kdeSettings.value("General/ColorScheme", "").toString();
        return colorScheme.contains("Dark", Qt::CaseInsensitive) ? ThemeMode::Dark : ThemeMode::Light;
    }
    
    return ThemeMode::Light;
    
#else
    return ThemeMode::Light;
#endif
}

void ThemeManager::setHighContrastMode(bool enabled) {
    if (m_highContrastMode == enabled) return;
    
    m_highContrastMode = enabled;
    
    if (enabled) {
        setTheme(ThemeMode::HighContrast);
    } else {
        // Restore previous theme
        setTheme(m_previousTheme);
    }
    
    emit highContrastModeChanged(enabled);
}

void ThemeManager::setFontScale(double scale) {
    scale = qBound(MIN_FONT_SCALE, scale, MAX_FONT_SCALE);
    
    if (qFuzzyCompare(m_typography.fontScale, scale)) return;
    
    m_typography.fontScale = scale;
    updateFontSizes();
    applyTypography();
    
    emit fontScaleChanged(scale);
}

void ThemeManager::setAnimationsEnabled(bool enabled) {
    if (m_animationSettings.enabled == enabled) return;
    
    m_animationSettings.enabled = enabled;
    
    emit animationSettingsChanged(enabled, m_animationSettings.duration);
}

void ThemeManager::setAnimationDuration(int duration) {
    duration = qMax(50, duration); // Minimum 50ms
    
    if (m_animationSettings.duration == duration) return;
    
    m_animationSettings.duration = duration;
    m_animationSettings.shortDuration = duration / 2;
    m_animationSettings.longDuration = duration * 2;
    
    emit animationSettingsChanged(m_animationSettings.enabled, duration);
}

void ThemeManager::applyThemeToWidget(QWidget* widget) {
    if (!widget) return;
    
    // Apply custom style
    FluxProxyStyle* proxyStyle = new FluxProxyStyle(this);
    widget->setStyle(proxyStyle);
    
    // Apply palette
    QPalette palette = widget->palette();
    
    // Update palette colors based on current theme
    palette.setColor(QPalette::Window, m_currentColorScheme.background);
    palette.setColor(QPalette::WindowText, m_currentColorScheme.textPrimary);
    palette.setColor(QPalette::Base, m_currentColorScheme.surface);
    palette.setColor(QPalette::AlternateBase, m_currentColorScheme.surfaceAlt);
    palette.setColor(QPalette::Text, m_currentColorScheme.textPrimary);
    palette.setColor(QPalette::Button, m_currentColorScheme.surface);
    palette.setColor(QPalette::ButtonText, m_currentColorScheme.textPrimary);
    palette.setColor(QPalette::Highlight, m_currentColorScheme.primary);
    palette.setColor(QPalette::HighlightedText, m_currentColorScheme.textOnPrimary);
    palette.setColor(QPalette::Link, m_currentColorScheme.primary);
    palette.setColor(QPalette::LinkVisited, m_currentColorScheme.primaryDark);
    
    widget->setPalette(palette);
    
    // Apply stylesheet
    QString styleSheet = generateWidgetStyleSheet(widget->metaObject()->className());
    if (!styleSheet.isEmpty()) {
        widget->setStyleSheet(styleSheet);
    }
}

void ThemeManager::applyThemeToApplication() {
    QApplication* app = qobject_cast<QApplication*>(QApplication::instance());
    if (!app) return;
    
    // Set application palette
    QPalette appPalette;
    
    appPalette.setColor(QPalette::Window, m_currentColorScheme.background);
    appPalette.setColor(QPalette::WindowText, m_currentColorScheme.textPrimary);
    appPalette.setColor(QPalette::Base, m_currentColorScheme.surface);
    appPalette.setColor(QPalette::AlternateBase, m_currentColorScheme.surfaceAlt);
    appPalette.setColor(QPalette::Text, m_currentColorScheme.textPrimary);
    appPalette.setColor(QPalette::Button, m_currentColorScheme.surface);
    appPalette.setColor(QPalette::ButtonText, m_currentColorScheme.textPrimary);
    appPalette.setColor(QPalette::Highlight, m_currentColorScheme.primary);
    appPalette.setColor(QPalette::HighlightedText, m_currentColorScheme.textOnPrimary);
    appPalette.setColor(QPalette::Link, m_currentColorScheme.primary);
    appPalette.setColor(QPalette::LinkVisited, m_currentColorScheme.primaryDark);
    
    // Disabled colors
    appPalette.setColor(QPalette::Disabled, QPalette::WindowText, m_currentColorScheme.textDisabled);
    appPalette.setColor(QPalette::Disabled, QPalette::Text, m_currentColorScheme.textDisabled);
    appPalette.setColor(QPalette::Disabled, QPalette::ButtonText, m_currentColorScheme.textDisabled);
    
    app->setPalette(appPalette);
    
    // Apply global stylesheet
    QString styleSheet = generateStyleSheet();
    app->setStyleSheet(styleSheet);
    
    // Update system tray icon if needed
    updateSystemTrayIcon();
    
    emit styleSheetUpdated(styleSheet);
}

QString ThemeManager::generateStyleSheet() const {
    if (!m_styleSheetDirty && !m_cachedStyleSheet.isEmpty()) {
        return m_cachedStyleSheet;
    }
    
    QString styleSheet;
    
    // Main window styles
    styleSheet += generateMainWindowStyle();
    
    // Widget-specific styles
    styleSheet += generateButtonStyle();
    styleSheet += generateMenuStyle();
    styleSheet += generateToolbarStyle();
    styleSheet += generateListStyle();
    styleSheet += generateScrollBarStyle();
    styleSheet += generateTabStyle();
    styleSheet += generateGroupBoxStyle();
    styleSheet += generateLineEditStyle();
    styleSheet += generateComboBoxStyle();
    styleSheet += generateProgressBarStyle();
    styleSheet += generateSliderStyle();
    styleSheet += generateCheckBoxStyle();
    styleSheet += generateRadioButtonStyle();
    styleSheet += generateSpinBoxStyle();
    styleSheet += generateSplitterStyle();
    styleSheet += generateStatusBarStyle();
    styleSheet += generateTooltipStyle();
    
    // Cache the result
    m_cachedStyleSheet = styleSheet;
    m_styleSheetDirty = false;
    
    return styleSheet;
}

QString ThemeManager::generateWidgetStyleSheet(const QString& widgetType) const {
    if (widgetType == "QPushButton") {
        return generateButtonStyle();
    } else if (widgetType == "QMenuBar" || widgetType == "QMenu") {
        return generateMenuStyle();
    } else if (widgetType == "QToolBar") {
        return generateToolbarStyle();
    } else if (widgetType.contains("List") || widgetType.contains("Tree")) {
        return generateListStyle();
    } else if (widgetType == "QScrollBar") {
        return generateScrollBarStyle();
    } else if (widgetType.contains("Tab")) {
        return generateTabStyle();
    } else if (widgetType == "QGroupBox") {
        return generateGroupBoxStyle();
    } else if (widgetType == "QLineEdit") {
        return generateLineEditStyle();
    } else if (widgetType == "QComboBox") {
        return generateComboBoxStyle();
    } else if (widgetType == "QProgressBar") {
        return generateProgressBarStyle();
    } else if (widgetType == "QSlider") {
        return generateSliderStyle();
    } else if (widgetType == "QCheckBox") {
        return generateCheckBoxStyle();
    } else if (widgetType == "QRadioButton") {
        return generateRadioButtonStyle();
    } else if (widgetType.contains("SpinBox")) {
        return generateSpinBoxStyle();
    } else if (widgetType == "QSplitter") {
        return generateSplitterStyle();
    } else if (widgetType == "QStatusBar") {
        return generateStatusBarStyle();
    }
    
    return QString();
}

QColor ThemeManager::getColor(const QString& colorName) const {
    auto it = m_namedColors.find(colorName);
    if (it != m_namedColors.end()) {
        return it->second;
    }
    
    // Fallback to color scheme properties
    if (colorName == "primary") return m_currentColorScheme.primary;
    if (colorName == "secondary") return m_currentColorScheme.secondary;
    if (colorName == "background") return m_currentColorScheme.background;
    if (colorName == "surface") return m_currentColorScheme.surface;
    if (colorName == "text") return m_currentColorScheme.textPrimary;
    if (colorName == "border") return m_currentColorScheme.border;
    
    return QColor(); // Invalid color
}

QColor ThemeManager::adjustColor(const QColor& color, double factor) const {
    if (isDarkColor(color)) {
        return lighten(color, factor);
    } else {
        return darken(color, factor);
    }
}

QColor ThemeManager::blendColors(const QColor& color1, const QColor& color2, double ratio) const {
    ratio = qBound(0.0, ratio, 1.0);
    
    int r = static_cast<int>(color1.red() * (1.0 - ratio) + color2.red() * ratio);
    int g = static_cast<int>(color1.green() * (1.0 - ratio) + color2.green() * ratio);
    int b = static_cast<int>(color1.blue() * (1.0 - ratio) + color2.blue() * ratio);
    int a = static_cast<int>(color1.alpha() * (1.0 - ratio) + color2.alpha() * ratio);
    
    return QColor(r, g, b, a);
}

bool ThemeManager::isDarkColor(const QColor& color) const {
    // Calculate relative luminance
    double r = color.redF();
    double g = color.greenF();
    double b = color.blueF();
    
    // Apply gamma correction
    r = (r <= 0.03928) ? r / 12.92 : qPow((r + 0.055) / 1.055, 2.4);
    g = (g <= 0.03928) ? g / 12.92 : qPow((g + 0.055) / 1.055, 2.4);
    b = (b <= 0.03928) ? b / 12.92 : qPow((b + 0.055) / 1.055, 2.4);
    
    double luminance = 0.2126 * r + 0.7152 * g + 0.0722 * b;
    
    return luminance < 0.5;
}

void ThemeManager::saveThemeSettings() {
    m_settings->setValue("theme/mode", static_cast<int>(m_currentTheme));
    m_settings->setValue("theme/systemDetection", m_systemThemeDetection);
    m_settings->setValue("theme/highContrast", m_highContrastMode);
    m_settings->setValue("theme/fontScale", m_typography.fontScale);
    m_settings->setValue("theme/animationsEnabled", m_animationSettings.enabled);
    m_settings->setValue("theme/animationDuration", m_animationSettings.duration);
    
    // Save custom color scheme if active
    if (m_currentTheme == ThemeMode::Custom) {
        const ColorScheme& scheme = m_currentColorScheme;
        m_settings->setValue("customTheme/primary", scheme.primary.name());
        m_settings->setValue("customTheme/secondary", scheme.secondary.name());
        m_settings->setValue("customTheme/background", scheme.background.name());
        m_settings->setValue("customTheme/surface", scheme.surface.name());
        m_settings->setValue("customTheme/textPrimary", scheme.textPrimary.name());
        // ... save other colors
    }
    
    m_settings->sync();
}

void ThemeManager::loadThemeSettings() {
    ThemeMode mode = static_cast<ThemeMode>(m_settings->value("theme/mode", static_cast<int>(ThemeMode::Light)).toInt());
    m_systemThemeDetection = m_settings->value("theme/systemDetection", true).toBool();
    m_highContrastMode = m_settings->value("theme/highContrast", false).toBool();
    
    double fontScale = m_settings->value("theme/fontScale", DEFAULT_FONT_SCALE).toDouble();
    setFontScale(fontScale);
    
    bool animationsEnabled = m_settings->value("theme/animationsEnabled", true).toBool();
    setAnimationsEnabled(animationsEnabled);
    
    int animationDuration = m_settings->value("theme/animationDuration", DEFAULT_ANIMATION_DURATION).toInt();
    setAnimationDuration(animationDuration);
    
    // Load custom color scheme if exists
    if (mode == ThemeMode::Custom && m_settings->contains("customTheme/primary")) {
        ColorScheme customScheme;
        customScheme.primary = QColor(m_settings->value("customTheme/primary").toString());
        customScheme.secondary = QColor(m_settings->value("customTheme/secondary").toString());
        customScheme.background = QColor(m_settings->value("customTheme/background").toString());
        customScheme.surface = QColor(m_settings->value("customTheme/surface").toString());
        customScheme.textPrimary = QColor(m_settings->value("customTheme/textPrimary").toString());
        // ... load other colors
        
        setCustomColorScheme(customScheme);
    }
    
    // Apply theme
    if (mode == ThemeMode::Auto && m_systemThemeDetection) {
        mode = detectSystemTheme();
    }
    
    setTheme(mode);
    enableSystemThemeDetection(m_systemThemeDetection);
}

void ThemeManager::resetToDefaults() {
    m_settings->clear();
    
    setTheme(ThemeMode::Light);
    enableSystemThemeDetection(true);
    setHighContrastMode(false);
    setFontScale(DEFAULT_FONT_SCALE);
    setAnimationsEnabled(true);
    setAnimationDuration(DEFAULT_ANIMATION_DURATION);
}

void ThemeManager::animateThemeTransition(QWidget* widget, int duration) {
    if (!widget || !m_animationSettings.enabled) return;
    
    // Create fade animation
    QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect();
    widget->setGraphicsEffect(effect);
    
    QPropertyAnimation* animation = new QPropertyAnimation(effect, "opacity");
    animation->setDuration(duration);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setEasingCurve(m_animationSettings.easingCurve);
    
    connect(animation, &QPropertyAnimation::finished, [effect]() {
        effect->deleteLater();
    });
    
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

// Private implementation methods
void ThemeManager::initializeThemes() {
    createLightTheme();
    createDarkTheme();
    createHighContrastTheme();
    createCustomTheme();
}

void ThemeManager::createLightTheme() {
    ColorScheme lightScheme;
    
    // Primary colors
    lightScheme.primary = QColor(0, 123, 255);           // #007bff
    lightScheme.primaryLight = QColor(102, 178, 255);    // #66b2ff
    lightScheme.primaryDark = QColor(0, 86, 179);        // #0056b3
    
    // Secondary colors
    lightScheme.secondary = QColor(108, 117, 125);       // #6c757d
    lightScheme.secondaryLight = QColor(173, 181, 189);  // #adb5bd
    lightScheme.secondaryDark = QColor(73, 80, 87);      // #495057
    
    // Background colors
    lightScheme.background = QColor(248, 249, 250);      // #f8f9fa
    lightScheme.backgroundAlt = QColor(233, 236, 239);   // #e9ecef
    lightScheme.surface = QColor(255, 255, 255);         // #ffffff
    lightScheme.surfaceAlt = QColor(248, 249, 250);      // #f8f9fa
    
    // Text colors
    lightScheme.textPrimary = QColor(33, 37, 41);        // #212529
    lightScheme.textSecondary = QColor(108, 117, 125);   // #6c757d
    lightScheme.textDisabled = QColor(173, 181, 189);    // #adb5bd
    lightScheme.textOnPrimary = QColor(255, 255, 255);   // #ffffff
    
    // Border colors
    lightScheme.border = QColor(222, 226, 230);          // #dee2e6
    lightScheme.borderLight = QColor(233, 236, 239);     // #e9ecef
    lightScheme.separator = QColor(222, 226, 230);       // #dee2e6
    
    // Status colors
    lightScheme.success = QColor(40, 167, 69);           // #28a745
    lightScheme.warning = QColor(255, 193, 7);           // #ffc107
    lightScheme.error = QColor(220, 53, 69);             // #dc3545
    lightScheme.info = QColor(23, 162, 184);             // #17a2b8
    
    // Interactive colors
    lightScheme.hover = QColor(233, 236, 239);           // #e9ecef
    lightScheme.pressed = QColor(222, 226, 230);         // #dee2e6
    lightScheme.selected = QColor(0, 123, 255, 51);      // #007bff33
    lightScheme.focus = QColor(0, 123, 255, 102);        // #007bff66
    
    // Special colors
    lightScheme.shadow = QColor(0, 0, 0, 25);            // #00000019
    lightScheme.highlight = QColor(255, 235, 59);        // #ffeb3b
    lightScheme.accent = QColor(255, 87, 34);            // #ff5722
    
    m_colorSchemes[ThemeMode::Light] = lightScheme;
}

void ThemeManager::createDarkTheme() {
    ColorScheme darkScheme;
    
    // Primary colors (slightly adjusted for dark theme)
    darkScheme.primary = QColor(13, 110, 253);           // #0d6efd
    darkScheme.primaryLight = QColor(108, 166, 254);     // #6ca6fe
    darkScheme.primaryDark = QColor(10, 88, 202);        // #0a58ca
    
    // Secondary colors
    darkScheme.secondary = QColor(108, 117, 125);        // #6c757d
    darkScheme.secondaryLight = QColor(173, 181, 189);   // #adb5bd
    darkScheme.secondaryDark = QColor(52, 58, 64);       // #343a40
    
    // Background colors
    darkScheme.background = QColor(33, 37, 41);          // #212529
    darkScheme.backgroundAlt = QColor(52, 58, 64);       // #343a40
    darkScheme.surface = QColor(52, 58, 64);             // #343a40
    darkScheme.surfaceAlt = QColor(73, 80, 87);          // #495057
    
    // Text colors
    darkScheme.textPrimary = QColor(248, 249, 250);      // #f8f9fa
    darkScheme.textSecondary = QColor(173, 181, 189);    // #adb5bd
    darkScheme.textDisabled = QColor(108, 117, 125);     // #6c757d
    darkScheme.textOnPrimary = QColor(255, 255, 255);    // #ffffff
    
    // Border colors
    darkScheme.border = QColor(73, 80, 87);              // #495057
    darkScheme.borderLight = QColor(108, 117, 125);      // #6c757d
    darkScheme.separator = QColor(73, 80, 87);           // #495057
    
    // Status colors (adjusted for dark theme)
    darkScheme.success = QColor(25, 135, 84);            // #198754
    darkScheme.warning = QColor(255, 205, 86);           // #ffcd56
    darkScheme.error = QColor(214, 51, 132);             // #d63384
    darkScheme.info = QColor(13, 202, 240);              // #0dcaf0
    
    // Interactive colors
    darkScheme.hover = QColor(73, 80, 87);               // #495057
    darkScheme.pressed = QColor(52, 58, 64);             // #343a40
    darkScheme.selected = QColor(13, 110, 253, 51);      // #0d6efd33
    darkScheme.focus = QColor(13, 110, 253, 102);        // #0d6efd66
    
    // Special colors
    darkScheme.shadow = QColor(0, 0, 0, 76);             // #0000004c
    darkScheme.highlight = QColor(255, 235, 59);         // #ffeb3b
    darkScheme.accent = QColor(255, 87, 34);             // #ff5722
    
    m_colorSchemes[ThemeMode::Dark] = darkScheme;
}

void ThemeManager::createHighContrastTheme() {
    ColorScheme highContrastScheme;
    
    // High contrast colors for accessibility
    highContrastScheme.primary = QColor(0, 0, 255);      // Pure blue
    highContrastScheme.primaryLight = QColor(128, 128, 255);
    highContrastScheme.primaryDark = QColor(0, 0, 128);
    
    highContrastScheme.secondary = QColor(128, 128, 128);
    highContrastScheme.secondaryLight = QColor(192, 192, 192);
    highContrastScheme.secondaryDark = QColor(64, 64, 64);
    
    highContrastScheme.background = QColor(255, 255, 255); // Pure white
    highContrastScheme.backgroundAlt = QColor(240, 240, 240);
    highContrastScheme.surface = QColor(255, 255, 255);
    highContrastScheme.surfaceAlt = QColor(248, 248, 248);
    
    highContrastScheme.textPrimary = QColor(0, 0, 0);     // Pure black
    highContrastScheme.textSecondary = QColor(64, 64, 64);
    highContrastScheme.textDisabled = QColor(128, 128, 128);
    highContrastScheme.textOnPrimary = QColor(255, 255, 255);
    
    highContrastScheme.border = QColor(0, 0, 0);          // Pure black borders
    highContrastScheme.borderLight = QColor(64, 64, 64);
    highContrastScheme.separator = QColor(0, 0, 0);
    
    highContrastScheme.success = QColor(0, 128, 0);       // Pure green
    highContrastScheme.warning = QColor(255, 165, 0);     // Orange
    highContrastScheme.error = QColor(255, 0, 0);         // Pure red
    highContrastScheme.info = QColor(0, 0, 255);          // Pure blue
    
    highContrastScheme.hover = QColor(224, 224, 224);
    highContrastScheme.pressed = QColor(192, 192, 192);
    highContrastScheme.selected = QColor(0, 0, 255, 128);
    highContrastScheme.focus = QColor(255, 0, 0, 128);    // Red focus for visibility
    
    highContrastScheme.shadow = QColor(0, 0, 0, 128);
    highContrastScheme.highlight = QColor(255, 255, 0);   // Pure yellow
    highContrastScheme.accent = QColor(255, 0, 255);      // Magenta
    
    m_colorSchemes[ThemeMode::HighContrast] = highContrastScheme;
}

void ThemeManager::createCustomTheme() {
    // Initialize with light theme as base
    m_colorSchemes[ThemeMode::Custom] = m_colorSchemes[ThemeMode::Light];
}

ThemeManager::ColorScheme ThemeManager::generateColorScheme(ThemeMode mode) const {
    auto it = m_colorSchemes.find(mode);
    if (it != m_colorSchemes.end()) {
        return it->second;
    }
    
    // Fallback to light theme
    return m_colorSchemes.at(ThemeMode::Light);
}

void ThemeManager::applyColorScheme(const ColorScheme& scheme) {
    m_currentColorScheme = scheme;
    
    // Update named colors for easy access
    m_namedColors["primary"] = scheme.primary;
    m_namedColors["secondary"] = scheme.secondary;
    m_namedColors["background"] = scheme.background;
    m_namedColors["surface"] = scheme.surface;
    m_namedColors["text"] = scheme.textPrimary;
    m_namedColors["textSecondary"] = scheme.textSecondary;
    m_namedColors["border"] = scheme.border;
    m_namedColors["success"] = scheme.success;
    m_namedColors["warning"] = scheme.warning;
    m_namedColors["error"] = scheme.error;
    m_namedColors["info"] = scheme.info;
    
    m_styleSheetDirty = true;
}

void ThemeManager::initializeTypography() {
    m_typography.baseFontSize = 10;
    m_typography.fontScale = DEFAULT_FONT_SCALE;
    
    // Load system fonts
    QFontDatabase fontDb;
    
    // Default UI font
    m_typography.defaultFont = QApplication::font();
    m_typography.uiFont = m_typography.defaultFont;
    
    // Heading font (slightly larger and bold)
    m_typography.headingFont = m_typography.defaultFont;
    m_typography.headingFont.setWeight(QFont::Bold);
    
    // Monospace font for code/data
    QStringList monoFonts = {"Consolas", "Monaco", "Courier New", "monospace"};
    for (const QString& fontName : monoFonts) {
        if (fontDb.families().contains(fontName)) {
            m_typography.monoFont = QFont(fontName);
            break;
        }
    }
    
    // Font weights
    m_typography.lightWeight = QFont::Light;
    m_typography.normalWeight = QFont::Normal;
    m_typography.mediumWeight = QFont::Medium;
    m_typography.boldWeight = QFont::Bold;
    
    updateFontSizes();
}

void ThemeManager::updateFontSizes() {
    int scaledSize = static_cast<int>(m_typography.baseFontSize * m_typography.fontScale);
    
    m_typography.defaultFont.setPointSize(scaledSize);
    m_typography.uiFont.setPointSize(scaledSize);
    m_typography.headingFont.setPointSize(scaledSize + 2);
    m_typography.monoFont.setPointSize(scaledSize);
}

void ThemeManager::applyTypography() {
    QApplication::setFont(m_typography.defaultFont);
}

QString ThemeManager::generateMainWindowStyle() const {
    return QString(R"(
        QMainWindow {
            background-color: %1;
            color: %2;
        }
        
        QWidget {
            background-color: %1;
            color: %2;
            font-family: "%3";
            font-size: %4pt;
        }
    )").arg(m_currentColorScheme.background.name())
       .arg(m_currentColorScheme.textPrimary.name())
       .arg(m_typography.defaultFont.family())
       .arg(m_typography.defaultFont.pointSize());
}

QString ThemeManager::generateButtonStyle() const {
    return QString(R"(
        QPushButton {
            background-color: %1;
            color: %2;
            border: 1px solid %3;
            border-radius: 6px;
            padding: 8px 16px;
            font-weight: 500;
            min-width: 80px;
        }
        
        QPushButton:hover {
            background-color: %4;
            border-color: %5;
        }
        
        QPushButton:pressed {
            background-color: %6;
        }
        
        QPushButton:disabled {
            background-color: %7;
            color: %8;
            border-color: %9;
        }
        
        QPushButton:default {
            background-color: %10;
            color: %11;
            border-color: %12;
        }
        
        QPushButton:default:hover {
            background-color: %13;
        }
    )").arg(m_currentColorScheme.surface.name())
       .arg(m_currentColorScheme.textPrimary.name())
       .arg(m_currentColorScheme.border.name())
       .arg(m_currentColorScheme.hover.name())
       .arg(m_currentColorScheme.borderLight.name())
       .arg(m_currentColorScheme.pressed.name())
       .arg(m_currentColorScheme.backgroundAlt.name())
       .arg(m_currentColorScheme.textDisabled.name())
       .arg(m_currentColorScheme.borderLight.name())
       .arg(m_currentColorScheme.primary.name())
       .arg(m_currentColorScheme.textOnPrimary.name())
       .arg(m_currentColorScheme.primary.name())
       .arg(m_currentColorScheme.primaryDark.name());
}

// Additional style generation methods would continue here...
// Due to length constraints, I'll provide key utility methods

QColor ThemeManager::lighten(const QColor& color, double factor) const {
    factor = qBound(0.0, factor, 1.0);
    
    int r = qMin(255, static_cast<int>(color.red() + (255 - color.red()) * factor));
    int g = qMin(255, static_cast<int>(color.green() + (255 - color.green()) * factor));
    int b = qMin(255, static_cast<int>(color.blue() + (255 - color.blue()) * factor));
    
    return QColor(r, g, b, color.alpha());
}

QColor ThemeManager::darken(const QColor& color, double factor) const {
    factor = qBound(0.0, factor, 1.0);
    
    int r = qMax(0, static_cast<int>(color.red() * (1.0 - factor)));
    int g = qMax(0, static_cast<int>(color.green() * (1.0 - factor)));
    int b = qMax(0, static_cast<int>(color.blue() * (1.0 - factor)));
    
    return QColor(r, g, b, color.alpha());
}

void ThemeManager::setupAnimations() {
    m_animationSettings.enabled = true;
    m_animationSettings.duration = DEFAULT_ANIMATION_DURATION;
    m_animationSettings.shortDuration = DEFAULT_ANIMATION_DURATION / 2;
    m_animationSettings.longDuration = DEFAULT_ANIMATION_DURATION * 2;
    m_animationSettings.easingCurve = QEasingCurve::OutCubic;
    
    m_themeTransitionGroup = std::make_unique<QParallelAnimationGroup>();
}

void ThemeManager::setupSystemThemeWatcher() {
    m_systemThemeWatcher = new QTimer(this);
    m_systemThemeWatcher->setInterval(SYSTEM_THEME_CHECK_INTERVAL);
    connect(m_systemThemeWatcher, &QTimer::timeout, this, &ThemeManager::onSystemThemeChanged);
}

void ThemeManager::onSystemThemeChanged() {
    if (!m_systemThemeDetection || m_currentTheme != ThemeMode::Auto) return;
    
    ThemeMode detectedTheme = detectSystemTheme();
    if (detectedTheme != m_currentTheme) {
        setTheme(detectedTheme);
        emit systemThemeChanged(detectedTheme);
    }
}

void ThemeManager::updateSystemTrayIcon() {
    // Update system tray icon based on current theme
    // Implementation would depend on system tray integration
}

bool ThemeManager::isValidColorScheme(const ColorScheme& scheme) const {
    return scheme.primary.isValid() &&
           scheme.secondary.isValid() &&
           scheme.background.isValid() &&
           scheme.surface.isValid() &&
           scheme.textPrimary.isValid();
}

// Placeholder implementations for remaining style methods
QString ThemeManager::generateMenuStyle() const { return QString(); }
QString ThemeManager::generateToolbarStyle() const { return QString(); }
QString ThemeManager::generateListStyle() const { return QString(); }
QString ThemeManager::generateScrollBarStyle() const { return QString(); }
QString ThemeManager::generateTabStyle() const { return QString(); }
QString ThemeManager::generateGroupBoxStyle() const { return QString(); }
QString ThemeManager::generateLineEditStyle() const { return QString(); }
QString ThemeManager::generateComboBoxStyle() const { return QString(); }
QString ThemeManager::generateProgressBarStyle() const { return QString(); }
QString ThemeManager::generateSliderStyle() const { return QString(); }
QString ThemeManager::generateCheckBoxStyle() const { return QString(); }
QString ThemeManager::generateRadioButtonStyle() const { return QString(); }
QString ThemeManager::generateSpinBoxStyle() const { return QString(); }
QString ThemeManager::generateSplitterStyle() const { return QString(); }
QString ThemeManager::generateStatusBarStyle() const { return QString(); }
QString ThemeManager::generateTooltipStyle() const { return QString(); }

// FluxProxyStyle implementation
FluxProxyStyle::FluxProxyStyle(ThemeManager* themeManager, QStyle* baseStyle)
    : QProxyStyle(baseStyle)
    , m_themeManager(themeManager)
{
}

void FluxProxyStyle::drawPrimitive(PrimitiveElement element, const QStyleOption* option,
                                  QPainter* painter, const QWidget* widget) const {
    setupPainter(painter);
    
    switch (element) {
        case PE_PanelButtonCommand:
            drawFluxButton(option, painter, widget);
            break;
        default:
            QProxyStyle::drawPrimitive(element, option, painter, widget);
            break;
    }
}

void FluxProxyStyle::drawControl(ControlElement element, const QStyleOption* option,
                                QPainter* painter, const QWidget* widget) const {
    setupPainter(painter);
    QProxyStyle::drawControl(element, option, painter, widget);
}

void FluxProxyStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex* option,
                                       QPainter* painter, const QWidget* widget) const {
    setupPainter(painter);
    QProxyStyle::drawComplexControl(control, option, painter, widget);
}

QRect FluxProxyStyle::subElementRect(SubElement element, const QStyleOption* option,
                                    const QWidget* widget) const {
    return QProxyStyle::subElementRect(element, option, widget);
}

QSize FluxProxyStyle::sizeFromContents(ContentsType type, const QStyleOption* option,
                                      const QSize& size, const QWidget* widget) const {
    return QProxyStyle::sizeFromContents(type, option, size, widget);
}

int FluxProxyStyle::pixelMetric(PixelMetric metric, const QStyleOption* option,
                               const QWidget* widget) const {
    return QProxyStyle::pixelMetric(metric, option, widget);
}

void FluxProxyStyle::setupPainter(QPainter* painter) const {
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);
}

QColor FluxProxyStyle::getThemeColor(const QString& colorName) const {
    return m_themeManager->getColor(colorName);
}

void FluxProxyStyle::drawFluxButton(const QStyleOption* option, QPainter* painter, const QWidget* widget) const {
    Q_UNUSED(widget)
    
    QRect rect = option->rect;
    QColor backgroundColor = getThemeColor("surface");
    QColor borderColor = getThemeColor("border");
    
    if (option->state & State_MouseOver) {
        backgroundColor = getThemeColor("hover");
    }
    if (option->state & State_Sunken) {
        backgroundColor = getThemeColor("pressed");
    }
    
    drawRoundedRect(painter, rect, 6, backgroundColor);
    
    painter->setPen(borderColor);
    painter->drawRoundedRect(rect, 6, 6);
}

void FluxProxyStyle::drawRoundedRect(QPainter* painter, const QRect& rect, int radius, const QColor& color) const {
    painter->setBrush(color);
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(rect, radius, radius);
}

void FluxProxyStyle::drawGradient(QPainter* painter, const QRect& rect, const QColor& startColor, const QColor& endColor) const {
    QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
    gradient.setColorAt(0, startColor);
    gradient.setColorAt(1, endColor);
    
    painter->setBrush(gradient);
    painter->setPen(Qt::NoPen);
    painter->drawRect(rect);
}

// Placeholder implementations for remaining FluxProxyStyle methods
void FluxProxyStyle::drawFluxScrollBar(const QStyleOption* option, QPainter* painter, const QWidget* widget) const {
    Q_UNUSED(option) Q_UNUSED(painter) Q_UNUSED(widget)
}

void FluxProxyStyle::drawFluxProgressBar(const QStyleOption* option, QPainter* painter, const QWidget* widget) const {
    Q_UNUSED(option) Q_UNUSED(painter) Q_UNUSED(widget)
}

void FluxProxyStyle::drawFluxTab(const QStyleOption* option, QPainter* painter, const QWidget* widget) const {
    Q_UNUSED(option) Q_UNUSED(painter) Q_UNUSED(widget)
}

} // namespace FluxGUI::UI::Managers

#include "theme_manager.moc"

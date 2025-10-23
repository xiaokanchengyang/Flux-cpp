#include "theme_manager.h"

#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QStandardPaths>
#include <QSettings>
#include <QDebug>
#include <QStyleFactory>
#include <QPalette>

namespace FluxGUI::Core::Theme {

ThemeManager& ThemeManager::instance() {
    static ThemeManager instance;
    return instance;
}

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
    , m_currentTheme("dark")
    , m_settings(nullptr)
{
    // Initialize settings
    m_settings = new QSettings(this);
    
    // Load saved theme
    m_currentTheme = m_settings->value("theme/current", "dark").toString();
    
    // Initialize available themes
    initializeThemes();
}

ThemeManager::~ThemeManager() = default;

void ThemeManager::initialize() {
    qDebug() << "Initializing Theme Manager...";
    
    // Load custom fonts
    loadCustomFonts();
    
    // Apply saved theme
    applyTheme(m_currentTheme);
    
    qDebug() << "Theme Manager initialized with theme:" << m_currentTheme;
}

bool ThemeManager::applyTheme(const QString& themeName) {
    if (!m_availableThemes.contains(themeName)) {
        qWarning() << "Theme not found:" << themeName;
        return false;
    }
    
    const ThemeInfo& theme = m_availableThemes[themeName];
    
    // Load and apply stylesheet
    QString styleSheet = loadStyleSheet(theme.styleSheetPath);
    if (styleSheet.isEmpty()) {
        qWarning() << "Failed to load stylesheet for theme:" << themeName;
        return false;
    }
    
    // Apply the stylesheet to the application
    QApplication::instance()->setStyleSheet(styleSheet);
    
    // Set application palette for better integration
    QPalette palette = createPalette(theme);
    QApplication::instance()->setPalette(palette);
    
    // Update current theme
    QString previousTheme = m_currentTheme;
    m_currentTheme = themeName;
    
    // Save to settings
    m_settings->setValue("theme/current", themeName);
    m_settings->sync();
    
    // Emit signals
    emit themeChanged(themeName, previousTheme);
    
    qDebug() << "Applied theme:" << themeName;
    return true;
}

QString ThemeManager::currentTheme() const {
    return m_currentTheme;
}

QStringList ThemeManager::availableThemes() const {
    return m_availableThemes.keys();
}

ThemeInfo ThemeManager::getThemeInfo(const QString& themeName) const {
    return m_availableThemes.value(themeName, ThemeInfo());
}

bool ThemeManager::isDarkTheme() const {
    return isDarkTheme(m_currentTheme);
}

bool ThemeManager::isDarkTheme(const QString& themeName) const {
    const ThemeInfo& theme = m_availableThemes.value(themeName, ThemeInfo());
    return theme.isDark;
}

void ThemeManager::toggleTheme() {
    QString newTheme = isDarkTheme() ? "light" : "dark";
    applyTheme(newTheme);
}

void ThemeManager::setAccentColor(const QColor& color) {
    m_accentColor = color;
    m_settings->setValue("theme/accentColor", color.name());
    
    // Reapply current theme with new accent color
    applyTheme(m_currentTheme);
    
    emit accentColorChanged(color);
}

QColor ThemeManager::accentColor() const {
    if (!m_accentColor.isValid()) {
        // Load from settings or use default
        QString colorName = m_settings->value("theme/accentColor", "#BB86FC").toString();
        return QColor(colorName);
    }
    return m_accentColor;
}

void ThemeManager::setCustomFont(const QFont& font) {
    m_customFont = font;
    m_settings->setValue("theme/customFont", font.toString());
    
    QApplication::instance()->setFont(font);
    
    emit customFontChanged(font);
}

QFont ThemeManager::customFont() const {
    if (m_customFont.family().isEmpty()) {
        // Load from settings or use default
        QString fontString = m_settings->value("theme/customFont", "").toString();
        if (!fontString.isEmpty()) {
            QFont font;
            font.fromString(fontString);
            return font;
        }
        return QApplication::font();
    }
    return m_customFont;
}

void ThemeManager::setUiScale(qreal scale) {
    m_uiScale = scale;
    m_settings->setValue("theme/uiScale", scale);
    
    // Apply UI scaling
    QApplication::instance()->setAttribute(Qt::AA_EnableHighDpiScaling, scale != 1.0);
    
    emit uiScaleChanged(scale);
}

qreal ThemeManager::uiScale() const {
    if (m_uiScale <= 0.0) {
        return m_settings->value("theme/uiScale", 1.0).toReal();
    }
    return m_uiScale;
}

void ThemeManager::initializeThemes() {
    // Dark theme
    ThemeInfo darkTheme;
    darkTheme.name = "Dark";
    darkTheme.description = "Modern dark theme with purple accents";
    darkTheme.isDark = true;
    darkTheme.styleSheetPath = ":/themes/dark.qss";
    darkTheme.primaryColor = QColor("#BB86FC");
    darkTheme.backgroundColor = QColor("#121212");
    darkTheme.textColor = QColor("#FFFFFF");
    darkTheme.accentColor = QColor("#BB86FC");
    m_availableThemes["dark"] = darkTheme;
    
    // Light theme
    ThemeInfo lightTheme;
    lightTheme.name = "Light";
    lightTheme.description = "Clean light theme with material design";
    lightTheme.isDark = false;
    lightTheme.styleSheetPath = ":/themes/light.qss";
    lightTheme.primaryColor = QColor("#6200EE");
    lightTheme.backgroundColor = QColor("#FAFAFA");
    lightTheme.textColor = QColor("#000000");
    lightTheme.accentColor = QColor("#6200EE");
    m_availableThemes["light"] = lightTheme;
}

QString ThemeManager::loadStyleSheet(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open stylesheet file:" << path;
        return QString();
    }
    
    QTextStream stream(&file);
    QString styleSheet = stream.readAll();
    
    // Process stylesheet with custom variables
    styleSheet = processStyleSheetVariables(styleSheet);
    
    return styleSheet;
}

QString ThemeManager::processStyleSheetVariables(const QString& styleSheet) {
    QString processed = styleSheet;
    
    // Replace accent color variables
    QColor accent = accentColor();
    processed.replace("${ACCENT_COLOR}", accent.name());
    processed.replace("${ACCENT_COLOR_HOVER}", accent.lighter(110).name());
    processed.replace("${ACCENT_COLOR_PRESSED}", accent.darker(110).name());
    
    // Replace other theme variables as needed
    const ThemeInfo& theme = m_availableThemes.value(m_currentTheme);
    processed.replace("${PRIMARY_COLOR}", theme.primaryColor.name());
    processed.replace("${BACKGROUND_COLOR}", theme.backgroundColor.name());
    processed.replace("${TEXT_COLOR}", theme.textColor.name());
    
    return processed;
}

QPalette ThemeManager::createPalette(const ThemeInfo& theme) {
    QPalette palette;
    
    if (theme.isDark) {
        // Dark theme palette
        palette.setColor(QPalette::Window, QColor("#121212"));
        palette.setColor(QPalette::WindowText, QColor("#FFFFFF"));
        palette.setColor(QPalette::Base, QColor("#1E1E1E"));
        palette.setColor(QPalette::AlternateBase, QColor("#252525"));
        palette.setColor(QPalette::ToolTipBase, QColor("#2D2D2D"));
        palette.setColor(QPalette::ToolTipText, QColor("#FFFFFF"));
        palette.setColor(QPalette::Text, QColor("#FFFFFF"));
        palette.setColor(QPalette::Button, QColor("#2D2D2D"));
        palette.setColor(QPalette::ButtonText, QColor("#FFFFFF"));
        palette.setColor(QPalette::BrightText, QColor("#BB86FC"));
        palette.setColor(QPalette::Link, QColor("#BB86FC"));
        palette.setColor(QPalette::Highlight, QColor("#BB86FC"));
        palette.setColor(QPalette::HighlightedText, QColor("#000000"));
    } else {
        // Light theme palette
        palette.setColor(QPalette::Window, QColor("#FAFAFA"));
        palette.setColor(QPalette::WindowText, QColor("#000000"));
        palette.setColor(QPalette::Base, QColor("#FFFFFF"));
        palette.setColor(QPalette::AlternateBase, QColor("#F8F8F8"));
        palette.setColor(QPalette::ToolTipBase, QColor("#FFFFFF"));
        palette.setColor(QPalette::ToolTipText, QColor("#000000"));
        palette.setColor(QPalette::Text, QColor("#000000"));
        palette.setColor(QPalette::Button, QColor("#FFFFFF"));
        palette.setColor(QPalette::ButtonText, QColor("#000000"));
        palette.setColor(QPalette::BrightText, QColor("#6200EE"));
        palette.setColor(QPalette::Link, QColor("#6200EE"));
        palette.setColor(QPalette::Highlight, QColor("#6200EE"));
        palette.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
    }
    
    return palette;
}

void ThemeManager::loadCustomFonts() {
    // Custom fonts are loaded in main.cpp
    // This method can be used for additional font loading if needed
    qDebug() << "Custom fonts loaded";
}

} // namespace FluxGUI::Core::Theme
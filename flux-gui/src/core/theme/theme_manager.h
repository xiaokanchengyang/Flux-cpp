#pragma once

#include <QObject>
#include <QApplication>
#include <QSettings>
#include <QString>
#include <QColor>
#include <QFont>
#include <QMap>
#include <memory>

namespace FluxGUI::Core {

/**
 * @brief Theme types supported by the application
 */
enum class ThemeType {
    Dark,
    Light,
    System  // Follow system theme
};

/**
 * @brief Color palette for themes
 */
struct ColorPalette {
    // Primary colors
    QColor primary;
    QColor primaryVariant;
    QColor secondary;
    QColor secondaryVariant;
    
    // Surface colors
    QColor surface;
    QColor surfaceVariant;
    QColor background;
    QColor backgroundVariant;
    
    // Content colors
    QColor onPrimary;
    QColor onSecondary;
    QColor onSurface;
    QColor onBackground;
    
    // State colors
    QColor success;
    QColor warning;
    QColor error;
    QColor info;
    
    // Interactive colors
    QColor hover;
    QColor pressed;
    QColor disabled;
    QColor focus;
    
    // Border and divider colors
    QColor border;
    QColor divider;
    QColor outline;
};

/**
 * @brief Typography settings for themes
 */
struct Typography {
    QFont displayLarge;
    QFont displayMedium;
    QFont displaySmall;
    QFont headlineLarge;
    QFont headlineMedium;
    QFont headlineSmall;
    QFont titleLarge;
    QFont titleMedium;
    QFont titleSmall;
    QFont bodyLarge;
    QFont bodyMedium;
    QFont bodySmall;
    QFont labelLarge;
    QFont labelMedium;
    QFont labelSmall;
};

/**
 * @brief Complete theme definition
 */
struct Theme {
    QString name;
    ThemeType type;
    ColorPalette colors;
    Typography typography;
    QString styleSheet;
};

/**
 * @brief Manages application themes and styling
 * 
 * The ThemeManager provides a centralized way to handle application theming,
 * including dark/light mode switching, custom color palettes, and typography.
 * It follows Material Design 3 principles for modern UI aesthetics.
 */
class ThemeManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Get the singleton instance of ThemeManager
     * @return Reference to the ThemeManager instance
     */
    static ThemeManager& instance();
    
    /**
     * @brief Initialize the theme system
     * @param app Pointer to the QApplication instance
     */
    void initialize(QApplication* app);
    
    /**
     * @brief Get the current theme
     * @return Current theme configuration
     */
    const Theme& currentTheme() const { return *m_currentTheme; }
    
    /**
     * @brief Get the current theme type
     * @return Current theme type
     */
    ThemeType currentThemeType() const { return m_currentThemeType; }
    
    /**
     * @brief Set the theme type
     * @param type Theme type to apply
     */
    void setThemeType(ThemeType type);
    
    /**
     * @brief Toggle between dark and light themes
     */
    void toggleTheme();
    
    /**
     * @brief Check if dark theme is currently active
     * @return True if dark theme is active
     */
    bool isDarkTheme() const { return m_currentThemeType == ThemeType::Dark; }
    
    /**
     * @brief Get available theme names
     * @return List of available theme names
     */
    QStringList availableThemes() const;
    
    /**
     * @brief Apply a custom theme
     * @param themeName Name of the theme to apply
     * @return True if theme was applied successfully
     */
    bool applyTheme(const QString& themeName);
    
    /**
     * @brief Get a color from the current theme
     * @param colorName Name of the color (e.g., "primary", "surface")
     * @return Color value, or invalid color if not found
     */
    QColor getColor(const QString& colorName) const;
    
    /**
     * @brief Get a font from the current theme
     * @param fontName Name of the font (e.g., "bodyLarge", "titleMedium")
     * @return Font configuration
     */
    QFont getFont(const QString& fontName) const;

signals:
    /**
     * @brief Emitted when the theme changes
     * @param newTheme The newly applied theme
     */
    void themeChanged(const Theme& newTheme);
    
    /**
     * @brief Emitted when theme type changes
     * @param type New theme type
     */
    void themeTypeChanged(ThemeType type);

private:
    explicit ThemeManager(QObject* parent = nullptr);
    ~ThemeManager() = default;
    
    // Disable copy and assignment
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;
    
    /**
     * @brief Load built-in themes
     */
    void loadBuiltInThemes();
    
    /**
     * @brief Create the dark theme
     * @return Dark theme configuration
     */
    std::unique_ptr<Theme> createDarkTheme();
    
    /**
     * @brief Create the light theme
     * @return Light theme configuration
     */
    std::unique_ptr<Theme> createLightTheme();
    
    /**
     * @brief Generate stylesheet for a theme
     * @param theme Theme to generate stylesheet for
     * @return Complete stylesheet string
     */
    QString generateStyleSheet(const Theme& theme);
    
    /**
     * @brief Apply theme to application
     * @param theme Theme to apply
     */
    void applyThemeToApplication(const Theme& theme);
    
    /**
     * @brief Load theme settings from configuration
     */
    void loadSettings();
    
    /**
     * @brief Save theme settings to configuration
     */
    void saveSettings();
    
    /**
     * @brief Detect system theme preference
     * @return Detected theme type
     */
    ThemeType detectSystemTheme();

private:
    QApplication* m_app = nullptr;
    QSettings* m_settings = nullptr;
    
    ThemeType m_currentThemeType = ThemeType::Dark;
    std::unique_ptr<Theme> m_currentTheme;
    
    QMap<QString, std::unique_ptr<Theme>> m_themes;
    
    // Theme monitoring
    bool m_systemThemeMonitoring = false;
};

} // namespace FluxGUI::Core

#pragma once

#include <QObject>
#include <QApplication>
#include <QWidget>
#include <QPalette>
#include <QColor>
#include <QFont>
#include <QSettings>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QStyle>
#include <QStyleFactory>
#include <QProxyStyle>
#include <QPainter>
#include <QStyleOption>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QParallelAnimationGroup>

#include <memory>
#include <unordered_map>

namespace FluxGUI::UI::Managers {

/**
 * @brief Advanced theme management system for Flux Archive Manager
 * 
 * This manager provides comprehensive theme support including:
 * - Light, dark, and auto themes
 * - Custom color schemes
 * - Smooth theme transitions
 * - System theme detection
 * - High contrast support
 * - Custom styling for all components
 * - Theme persistence and restoration
 * - Dynamic theme switching
 */
class ThemeManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Available theme modes
     */
    enum class ThemeMode {
        Light,          ///< Light theme
        Dark,           ///< Dark theme
        Auto,           ///< Automatic (follows system)
        HighContrast,   ///< High contrast theme
        Custom          ///< Custom user theme
    };

    /**
     * @brief Color scheme structure
     */
    struct ColorScheme {
        // Primary colors
        QColor primary;
        QColor primaryLight;
        QColor primaryDark;
        
        // Secondary colors
        QColor secondary;
        QColor secondaryLight;
        QColor secondaryDark;
        
        // Background colors
        QColor background;
        QColor backgroundAlt;
        QColor surface;
        QColor surfaceAlt;
        
        // Text colors
        QColor textPrimary;
        QColor textSecondary;
        QColor textDisabled;
        QColor textOnPrimary;
        
        // Border and separator colors
        QColor border;
        QColor borderLight;
        QColor separator;
        
        // Status colors
        QColor success;
        QColor warning;
        QColor error;
        QColor info;
        
        // Interactive colors
        QColor hover;
        QColor pressed;
        QColor selected;
        QColor focus;
        
        // Special colors
        QColor shadow;
        QColor highlight;
        QColor accent;
    };

    /**
     * @brief Typography settings
     */
    struct Typography {
        QFont defaultFont;
        QFont headingFont;
        QFont monoFont;
        QFont uiFont;
        
        int baseFontSize;
        double fontScale;
        
        // Font weights
        int lightWeight;
        int normalWeight;
        int mediumWeight;
        int boldWeight;
    };

    /**
     * @brief Animation settings
     */
    struct AnimationSettings {
        bool enabled;
        int duration;
        int shortDuration;
        int longDuration;
        QEasingCurve::Type easingCurve;
    };

    explicit ThemeManager(QObject* parent = nullptr);
    ~ThemeManager() override;

    // Theme management
    void setTheme(ThemeMode mode);
    ThemeMode currentTheme() const { return m_currentTheme; }
    
    void setCustomColorScheme(const ColorScheme& scheme);
    ColorScheme currentColorScheme() const { return m_currentColorScheme; }
    
    // System integration
    void enableSystemThemeDetection(bool enabled);
    bool isSystemThemeDetectionEnabled() const { return m_systemThemeDetection; }
    ThemeMode detectSystemTheme() const;
    
    // Accessibility
    void setHighContrastMode(bool enabled);
    bool isHighContrastMode() const { return m_highContrastMode; }
    
    void setFontScale(double scale);
    double fontScale() const { return m_typography.fontScale; }
    
    // Animation control
    void setAnimationsEnabled(bool enabled);
    bool areAnimationsEnabled() const { return m_animationSettings.enabled; }
    
    void setAnimationDuration(int duration);
    int animationDuration() const { return m_animationSettings.duration; }
    
    // Style application
    void applyThemeToWidget(QWidget* widget);
    void applyThemeToApplication();
    QString generateStyleSheet() const;
    QString generateWidgetStyleSheet(const QString& widgetType) const;
    
    // Color utilities
    QColor getColor(const QString& colorName) const;
    QColor adjustColor(const QColor& color, double factor) const;
    QColor blendColors(const QColor& color1, const QColor& color2, double ratio) const;
    bool isDarkColor(const QColor& color) const;
    
    // Persistence
    void saveThemeSettings();
    void loadThemeSettings();
    void resetToDefaults();
    
    // Theme transitions
    void animateThemeTransition(QWidget* widget, int duration = 300);
    void animateColorTransition(QWidget* widget, const QColor& fromColor, const QColor& toColor, int duration = 300);

Q_SIGNALS:
    // Theme change notifications
    void themeChanged(ThemeMode newTheme);
    void colorSchemeChanged(const ColorScheme& scheme);
    void systemThemeChanged(ThemeMode detectedTheme);
    
    // Accessibility notifications
    void highContrastModeChanged(bool enabled);
    void fontScaleChanged(double scale);
    
    // Animation notifications
    void animationSettingsChanged(bool enabled, int duration);
    
    // Style notifications
    void styleSheetUpdated(const QString& styleSheet);

private Q_SLOTS:
    void onSystemThemeChanged();
    void onApplicationPaletteChanged();
    void onAnimationFinished();

private:
    // Theme initialization
    void initializeThemes();
    void createLightTheme();
    void createDarkTheme();
    void createHighContrastTheme();
    void createCustomTheme();
    
    // Color scheme management
    void applyColorScheme(const ColorScheme& scheme);
    ColorScheme generateColorScheme(ThemeMode mode) const;
    void interpolateColorScheme(const ColorScheme& from, const ColorScheme& to, double progress);
    
    // Typography management
    void initializeTypography();
    void applyTypography();
    void updateFontSizes();
    
    // Style generation
    QString generateMainWindowStyle() const;
    QString generateButtonStyle() const;
    QString generateMenuStyle() const;
    QString generateToolbarStyle() const;
    QString generateListStyle() const;
    QString generateScrollBarStyle() const;
    QString generateTabStyle() const;
    QString generateGroupBoxStyle() const;
    QString generateLineEditStyle() const;
    QString generateComboBoxStyle() const;
    QString generateProgressBarStyle() const;
    QString generateSliderStyle() const;
    QString generateCheckBoxStyle() const;
    QString generateRadioButtonStyle() const;
    QString generateSpinBoxStyle() const;
    QString generateSplitterStyle() const;
    QString generateStatusBarStyle() const;
    QString generateTooltipStyle() const;
    
    // Animation helpers
    void setupAnimations();
    void animateWidgetProperty(QWidget* widget, const QByteArray& property, 
                             const QVariant& startValue, const QVariant& endValue, int duration);
    void fadeWidget(QWidget* widget, double fromOpacity, double toOpacity, int duration);
    
    // System integration helpers
    void setupSystemThemeWatcher();
    void updateSystemTrayIcon();
    
    // Utility methods
    QColor lighten(const QColor& color, double factor) const;
    QColor darken(const QColor& color, double factor) const;
    QColor desaturate(const QColor& color, double factor) const;
    QColor adjustBrightness(const QColor& color, double factor) const;
    QString colorToString(const QColor& color) const;
    QColor stringToColor(const QString& colorString) const;
    
    // Validation
    bool isValidColorScheme(const ColorScheme& scheme) const;
    bool isValidThemeMode(ThemeMode mode) const;

private:
    // Core theme data
    ThemeMode m_currentTheme;
    ThemeMode m_previousTheme;
    ColorScheme m_currentColorScheme;
    Typography m_typography;
    AnimationSettings m_animationSettings;
    
    // Theme storage
    std::unordered_map<ThemeMode, ColorScheme> m_colorSchemes;
    std::unordered_map<QString, QColor> m_namedColors;
    
    // System integration
    bool m_systemThemeDetection;
    QTimer* m_systemThemeWatcher;
    bool m_highContrastMode;
    
    // Animation components
    std::unique_ptr<QParallelAnimationGroup> m_themeTransitionGroup;
    std::vector<std::unique_ptr<QPropertyAnimation>> m_activeAnimations;
    
    // Settings
    std::unique_ptr<QSettings> m_settings;
    
    // Style caching
    mutable QString m_cachedStyleSheet;
    mutable bool m_styleSheetDirty;
    
    // Constants
    static constexpr double GOLDEN_RATIO = 1.618033988749;
    static constexpr int DEFAULT_ANIMATION_DURATION = 250;
    static constexpr int SYSTEM_THEME_CHECK_INTERVAL = 5000; // 5 seconds
    static constexpr double MIN_FONT_SCALE = 0.5;
    static constexpr double MAX_FONT_SCALE = 3.0;
    static constexpr double DEFAULT_FONT_SCALE = 1.0;
};

/**
 * @brief Custom proxy style for enhanced theming
 */
class FluxProxyStyle : public QProxyStyle {
    Q_OBJECT

public:
    explicit FluxProxyStyle(ThemeManager* themeManager, QStyle* baseStyle = nullptr);
    
    // Style overrides
    void drawPrimitive(PrimitiveElement element, const QStyleOption* option,
                      QPainter* painter, const QWidget* widget = nullptr) const override;
    
    void drawControl(ControlElement element, const QStyleOption* option,
                    QPainter* painter, const QWidget* widget = nullptr) const override;
    
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex* option,
                           QPainter* painter, const QWidget* widget = nullptr) const override;
    
    QRect subElementRect(SubElement element, const QStyleOption* option,
                        const QWidget* widget = nullptr) const override;
    
    QSize sizeFromContents(ContentsType type, const QStyleOption* option,
                          const QSize& size, const QWidget* widget = nullptr) const override;
    
    int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr,
                   const QWidget* widget = nullptr) const override;

private:
    ThemeManager* m_themeManager;
    
    // Custom drawing methods
    void drawFluxButton(const QStyleOption* option, QPainter* painter, const QWidget* widget) const;
    void drawFluxScrollBar(const QStyleOption* option, QPainter* painter, const QWidget* widget) const;
    void drawFluxProgressBar(const QStyleOption* option, QPainter* painter, const QWidget* widget) const;
    void drawFluxTab(const QStyleOption* option, QPainter* painter, const QWidget* widget) const;
    
    // Helper methods
    void setupPainter(QPainter* painter) const;
    QColor getThemeColor(const QString& colorName) const;
    void drawRoundedRect(QPainter* painter, const QRect& rect, int radius, const QColor& color) const;
    void drawGradient(QPainter* painter, const QRect& rect, const QColor& startColor, const QColor& endColor) const;
};

} // namespace FluxGUI::UI::Managers

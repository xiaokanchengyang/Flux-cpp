#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QTranslator>
#include <QLibraryInfo>
#include <QSplashScreen>
#include <QPixmap>
#include <QTimer>
#include <QFontDatabase>
#include <iostream>

#include "ui/main_window.h"
#include "core/theme/theme_manager.h"
#include "platform/system_integration.h"

void setupApplication(QApplication& app) {
    // Set application information
    app.setApplicationName("Flux");
    app.setApplicationDisplayName("Flux Archive Manager");
    app.setApplicationVersion("2.0.0");
    app.setOrganizationName("Flux Team");
    app.setOrganizationDomain("flux-archive.org");
    
    // Set application icon
    app.setWindowIcon(QIcon(":/icons/flux-logo.png"));
    
    // Load custom fonts
    QFontDatabase::addApplicationFont(":/fonts/Inter-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Inter-Medium.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Inter-SemiBold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Inter-Bold.ttf");
    
    // Set default font
    QFont defaultFont("Inter", 10);
    app.setFont(defaultFont);
}

int main(int argc, char *argv[]) {
    // Enable high DPI support
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    
    // Create application instance
    QApplication app(argc, argv);
    
    // Basic setup
    setupApplication(app);
    
    try {
        // Initialize theme manager
        FluxGUI::Core::Theme::ThemeManager::instance().initialize();
        
        // Initialize system integration
        FluxGUI::Platform::SystemIntegration::instance().initialize();
        
        // Create splash screen
        QPixmap splashPixmap(":/images/splash.png");
        QSplashScreen splash(splashPixmap);
        splash.show();
        app.processEvents();
        
        // Show loading message
        splash.showMessage("Loading Flux Archive Manager...", 
                          Qt::AlignBottom | Qt::AlignCenter, Qt::white);
        app.processEvents();
        
        // Simulate loading time
        QTimer::singleShot(1500, [&]() {
            splash.finish(nullptr);
        });
        
        // Create and show main window
        FluxGUI::UI::MainWindow window;
        
        // Apply initial theme
        FluxGUI::Core::Theme::ThemeManager::instance().applyTheme("dark");
        
        // Show window after splash
        QTimer::singleShot(1500, [&]() {
            window.show();
            window.raise();
            window.activateWindow();
        });
        
        // Run application event loop
        return app.exec();
        
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "Flux Archive Manager - Error", 
                             QString("Failed to start application: %1").arg(e.what()));
        return -1;
    } catch (...) {
        QMessageBox::critical(nullptr, "Flux Archive Manager - Error", 
                             "An unknown error occurred while starting the application.");
        return -1;
    }
}

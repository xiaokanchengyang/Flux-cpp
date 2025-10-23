#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QTranslator>
#include <QLibraryInfo>
#include <iostream>

#include "main_window.h"

void setupApplication(QApplication& app) {
    // Set application information
    app.setApplicationName("Flux");
    app.setApplicationDisplayName("Flux Archive Tool");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Flux Team");
    app.setOrganizationDomain("flux-archive.org");
    
    // High DPI support is enabled by default in Qt6, no need to set these attributes
}

int main(int argc, char *argv[]) {
    // Create application instance
    QApplication app(argc, argv);
    
    // Basic setup
    setupApplication(app);
    
    // Create and show main window
    MainWindow window;
    window.show();
    
    // Run application event loop
    return app.exec();
}

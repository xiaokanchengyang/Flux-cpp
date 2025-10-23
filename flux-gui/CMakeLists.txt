# Flux-GUI Graphical User Interface Application
cmake_minimum_required(VERSION 3.22)
project(FluxGUI VERSION 1.0.0 LANGUAGES CXX)

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Widgets 
    Concurrent
    Network
    Charts
)

# Create executable
add_executable(flux-gui
    src/main.cpp
    
    # Main UI components
    src/ui/main_window.cpp
    src/ui/main_window.h
    
    # Core systems
    src/core/theme/theme_manager.cpp
    src/core/theme/theme_manager.h
    src/core/config/settings_manager.cpp
    src/core/config/settings_manager.h
    src/core/archive/archive_manager.cpp
    src/core/archive/archive_manager.h
    
    # Modern UI widgets
    src/ui/widgets/welcome_widget.cpp
    src/ui/widgets/welcome_widget.h
    src/ui/widgets/archive_explorer_widget.cpp
    src/ui/widgets/archive_explorer_widget.h
    src/ui/widgets/compression_widget.cpp
    src/ui/widgets/compression_widget.h
    src/ui/widgets/extraction_widget.cpp
    src/ui/widgets/extraction_widget.h
    src/ui/widgets/settings_widget.cpp
    src/ui/widgets/settings_widget.h
    src/ui/widgets/progress_widget.cpp
    src/ui/widgets/progress_widget.h
    
    # UI components
    src/ui/components/modern_toolbar.cpp
    src/ui/components/modern_toolbar.h
    src/ui/components/status_bar.cpp
    src/ui/components/status_bar.h
    src/ui/components/file_tree_view.cpp
    src/ui/components/file_tree_view.h
    src/ui/components/archive_info_panel.cpp
    src/ui/components/archive_info_panel.h
    
    # Platform integration
    src/platform/system_integration.cpp
    src/platform/system_integration.h
    
    # Utility classes
    src/utils/file_utils.cpp
    src/utils/file_utils.h
    src/utils/resource_manager.cpp
    src/utils/resource_manager.h
    src/utils/ui_utils.cpp
    src/utils/ui_utils.h
    
    # Model classes
    src/models/archive_model.cpp
    src/models/archive_model.h
    src/models/file_system_model.cpp
    src/models/file_system_model.h
    
    # Dialogs
    src/ui/dialogs/about_dialog.cpp
    src/ui/dialogs/about_dialog.h
    src/ui/dialogs/preferences_dialog.cpp
    src/ui/dialogs/preferences_dialog.h
    
    # Resource files
    resources/resources.qrc
)

# Link dependencies
target_link_libraries(flux-gui PRIVATE 
    Qt6::Core
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Network
    Qt6::Charts
    flux-core
)

# Include directories
target_include_directories(flux-gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Platform-specific settings
if(WIN32)
    # Windows-specific settings
    target_compile_definitions(flux-gui PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        UNICODE
        _UNICODE
    )
    # Set as Windows application (not console)
    set_target_properties(flux-gui PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    
    # Add Windows resource file if it exists
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/flux.rc")
        target_sources(flux-gui PRIVATE resources/flux.rc)
    endif()
    
elseif(APPLE)
    # macOS-specific settings
    set_target_properties(flux-gui PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/resources/Info.plist.in"
        MACOSX_BUNDLE_BUNDLE_NAME "Flux"
        MACOSX_BUNDLE_DISPLAY_NAME "Flux Archive Manager"
        MACOSX_BUNDLE_IDENTIFIER "com.flux.archiver"
        MACOSX_BUNDLE_VERSION "1.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
    )
    
elseif(UNIX)
    # Linux-specific settings
    # Enable RPATH for finding shared libraries
    set_target_properties(flux-gui PROPERTIES
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
endif()

# Set target properties
set_target_properties(flux-gui PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    OUTPUT_NAME "FluxGUI"
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

# Platform-specific deployment
if(WIN32)
    # Find Qt6 deployment tool
    find_program(QT_DEPLOY_EXECUTABLE windeployqt HINTS ${Qt6_DIR}/../../../bin)
    
    if(QT_DEPLOY_EXECUTABLE)
        add_custom_command(TARGET flux-gui POST_BUILD
            COMMAND ${QT_DEPLOY_EXECUTABLE} $<TARGET_FILE:flux-gui>
            COMMENT "Deploying Qt libraries for Windows"
        )
    endif()
    
elseif(APPLE)
    # Find Qt6 deployment tool for macOS
    find_program(QT_DEPLOY_EXECUTABLE macdeployqt HINTS ${Qt6_DIR}/../../../bin)
    
    if(QT_DEPLOY_EXECUTABLE)
        add_custom_command(TARGET flux-gui POST_BUILD
            COMMAND ${QT_DEPLOY_EXECUTABLE} $<TARGET_BUNDLE_DIR:flux-gui>
            COMMENT "Deploying Qt libraries for macOS"
        )
    endif()
endif()

# Installation rules
if(WIN32)
    install(TARGETS flux-gui
        RUNTIME DESTINATION .
        COMPONENT gui
    )
elseif(APPLE)
    install(TARGETS flux-gui
        BUNDLE DESTINATION .
        COMPONENT gui
    )
else()
    install(TARGETS flux-gui
        RUNTIME DESTINATION bin
        COMPONENT gui
    )
    
    # Install desktop file (Linux)
    install(FILES resources/flux.desktop
        DESTINATION share/applications
        COMPONENT gui
    )
    
    # Install icon
    install(FILES resources/icons/flux.png
        DESTINATION share/icons/hicolor/256x256/apps
        COMPONENT gui
    )
    
    # Install MIME type information
    install(FILES resources/flux-mimetypes.xml
        DESTINATION share/mime/packages
        COMPONENT gui
        OPTIONAL
    )
endif()

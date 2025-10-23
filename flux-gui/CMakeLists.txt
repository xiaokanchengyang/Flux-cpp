# Flux-GUI Graphical User Interface Application
cmake_minimum_required(VERSION 3.22)

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
    src/main_window.cpp
    src/main_window.h
    
    # View components
    src/views/welcome_view.cpp
    src/views/welcome_view.h
    src/views/pack_view.cpp
    src/views/pack_view.h
    src/views/extract_view.cpp
    src/views/extract_view.h
    src/views/browse_view.cpp
    src/views/browse_view.h
    src/views/browse_page.cpp
    src/views/browse_page.h
    
    # Utility classes
    src/utils/file_utils.cpp
    src/utils/file_utils.h
    src/utils/resource_manager.cpp
    src/utils/resource_manager.h
    
    # Model classes
    src/models/archive_model.cpp
    src/models/archive_model.h
    
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

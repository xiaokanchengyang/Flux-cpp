cmake_minimum_required(VERSION 3.20)

project(FluxGUI 
    VERSION 2.0.0
    DESCRIPTION "Modern GUI for Flux Archive Manager"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required Qt6 components
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Gui
    Concurrent
    Network
    Svg
)

# Enable Qt6 features
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Compiler-specific options
if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Debug/Release configurations
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG_BUILD)
    if(NOT MSVC)
        add_compile_options(-g -O0)
    endif()
else()
    add_compile_definitions(NDEBUG)
    if(NOT MSVC)
        add_compile_options(-O3 -DNDEBUG)
    endif()
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Source files (minimal working set)
set(GUI_SOURCES
    # Application entry
    src/main.cpp
)

# Header files (minimal working set)
set(GUI_HEADERS
    # No headers needed for simple main.cpp
)

# Resource files (commented out for minimal build)
# set(GUI_RESOURCES
#     resources/simple.qrc
# )

# Create the executable
add_executable(FluxGUI
    ${GUI_SOURCES}
    ${GUI_HEADERS}
    # ${GUI_RESOURCES}  # Commented out for minimal build
)

# Link Qt6 libraries
target_link_libraries(FluxGUI
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Concurrent
    Qt6::Network
    Qt6::Svg
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(FluxGUI
        user32
        shell32
        ole32
        oleaut32
        uuid
        advapi32
    )
elseif(UNIX AND NOT APPLE)
    target_link_libraries(FluxGUI
        pthread
        dl
    )
elseif(APPLE)
    target_link_libraries(FluxGUI
        "-framework CoreFoundation"
        "-framework CoreServices"
        "-framework AppKit"
    )
endif()

# Compiler definitions
target_compile_definitions(FluxGUI PRIVATE
    QT_NO_KEYWORDS
    QT_DISABLE_DEPRECATED_BEFORE=0x060000
    FLUX_GUI_VERSION="${PROJECT_VERSION}"
    FLUX_GUI_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    FLUX_GUI_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    FLUX_GUI_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# Set target properties
set_target_properties(FluxGUI PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    OUTPUT_NAME "FluxArchiveManager"
)

# Installation
install(TARGETS FluxGUI
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Windows-specific installation
if(WIN32)
    # Install Qt6 runtime libraries
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${Qt6_DIR}/../../../bin)
    if(WINDEPLOYQT_EXECUTABLE)
        install(CODE "
            execute_process(
                COMMAND ${WINDEPLOYQT_EXECUTABLE} --verbose 2 --no-compiler-runtime \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/bin/FluxArchiveManager.exe
            )
        ")
    endif()
endif()

# macOS-specific installation
if(APPLE)
    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS ${Qt6_DIR}/../../../bin)
    if(MACDEPLOYQT_EXECUTABLE)
        install(CODE "
            execute_process(
                COMMAND ${MACDEPLOYQT_EXECUTABLE} \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/FluxArchiveManager.app
            )
        ")
    endif()
endif()

# Create desktop entry for Linux
if(UNIX AND NOT APPLE)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/flux-archive-manager.desktop.in
        ${CMAKE_CURRENT_BINARY_DIR}/flux-archive-manager.desktop
        @ONLY
    )
    
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/flux-archive-manager.desktop
        DESTINATION share/applications
    )
    
    install(FILES resources/icons/flux-archive-manager.png
        DESTINATION share/pixmaps
    )
endif()

# CPack configuration for packaging
include(CPack)
set(CPACK_PACKAGE_NAME "FluxArchiveManager")
set(CPACK_PACKAGE_VENDOR "Flux Development Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Modern Archive Management Tool")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "FluxArchiveManager")

if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_DISPLAY_NAME "Flux Archive Manager")
    set(CPACK_NSIS_PACKAGE_NAME "FluxArchiveManager")
    set(CPACK_NSIS_CONTACT "support@flux-archive.com")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://flux-archive.com")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop;TGZ")
    set(CPACK_DMG_FORMAT "UDBZ")
    set(CPACK_DMG_VOLUME_NAME "FluxArchiveManager")
else()
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Flux Development Team")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libqt6core6, libqt6widgets6, libqt6gui6")
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    set(CPACK_RPM_PACKAGE_GROUP "Applications/Archiving")
endif()

# Development tools
option(BUILD_TESTS "Build test suite" OFF)
option(BUILD_DOCS "Build documentation" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        add_subdirectory(docs)
    endif()
endif()

if(ENABLE_COVERAGE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_COMPILER_IS_GNUCXX)
        target_compile_options(FluxGUI PRIVATE --coverage)
        target_link_libraries(FluxGUI --coverage)
    endif()
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "Flux GUI Configuration Summary:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Qt6 version: ${Qt6_VERSION}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "  Build docs: ${BUILD_DOCS}")
message(STATUS "  Enable coverage: ${ENABLE_COVERAGE}")
message(STATUS "")
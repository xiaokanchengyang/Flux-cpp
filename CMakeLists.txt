cmake_minimum_required(VERSION 3.25)

# Allow version to be overridden by CI/CD
if(NOT DEFINED FLUX_VERSION)
    set(FLUX_VERSION "1.0.0")
endif()

project(Flux 
    VERSION ${FLUX_VERSION}
    DESCRIPTION "Modern high-performance cross-platform archive management tool"
    HOMEPAGE_URL "https://github.com/xiaokanchengyang/Flux-cpp"
    LANGUAGES CXX
)

# --- Build Options ---
option(FLUX_BUILD_TESTS "Build unit tests" ON)
option(FLUX_BUILD_BENCHMARKS "Build benchmark suite" ON)
option(FLUX_BUILD_GUI "Build GUI application" ON)
option(FLUX_BUILD_CLI "Build CLI application" ON)
option(FLUX_ENABLE_LTO "Enable Link Time Optimization" OFF)
option(FLUX_STATIC_LINKING "Enable static linking where possible" OFF)
option(FLUX_ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)

# --- Global Settings ---
set(CMAKE_CXX_STANDARD 23)  # Use C++23 for latest features and best practices
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Enable modern CMake features
# Note: C++ modules scanning disabled for MinGW compatibility
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# Export compile commands for IDE integration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Cross-platform Configuration ---
# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Enable position independent code for shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# --- Compiler-specific Settings ---
if(MSVC)
    # Enable multi-processor compilation
    add_compile_options(/MP)
    # Set warning level
    add_compile_options(/W4)
    # Disable some unnecessary warnings for Qt compatibility
    add_compile_options(/wd4251 /wd4275)
    # Enable UTF-8 encoding
    add_compile_options(/utf-8)
    # Enable security features
    add_compile_options(/GS /guard:cf)
    # Enable Link Time Code Generation if LTO is enabled
    if(FLUX_ENABLE_LTO)
        add_compile_options(/GL)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /LTCG")
    endif()
else()
    # GCC/Clang settings
    add_compile_options(-Wall -Wextra -Wpedantic)
    # Additional security and quality warnings
    add_compile_options(-Wformat=2 -Wconversion -Wsign-conversion)
    add_compile_options(-Wnull-dereference -Wdouble-promotion)
    
    # Enable colored output for GCC/Clang
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-fdiagnostics-color=always)
        # GCC-specific optimizations
        if(CMAKE_BUILD_TYPE STREQUAL "Release")
            add_compile_options(-ffast-math -funroll-loops)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_options(-fcolor-diagnostics)
    endif()
    
    # Enable Link Time Optimization if requested
    if(FLUX_ENABLE_LTO)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
    
    # Enable sanitizers for debugging builds
    if(FLUX_ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-fsanitize=address -fsanitize=undefined)
        add_link_options(-fsanitize=address -fsanitize=undefined)
    endif()
    
    # Security hardening flags
    add_compile_options(-fstack-protector-strong)
    if(NOT APPLE)
        add_compile_options(-D_FORTIFY_SOURCE=2)
    endif()
endif()

# --- Platform-specific Settings ---
if(WIN32)
    # Windows specific settings
    add_compile_definitions(
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS
        UNICODE
        _UNICODE
    )
elseif(APPLE)
    # macOS specific settings
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")
    set(CMAKE_INSTALL_RPATH "@executable_path/../Frameworks")
elseif(UNIX)
    # Linux specific settings
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
endif()

# --- Find Dependencies ---
find_package(Qt6 REQUIRED COMPONENTS Widgets Core)

# Find compression libraries (optional for now)
# find_package(PkgConfig REQUIRED)
# find_package(libzip CONFIG REQUIRED)
# find_package(ZLIB REQUIRED)
# find_package(LibLZMA REQUIRED)
# find_package(zstd CONFIG REQUIRED)
# find_package(fmt CONFIG REQUIRED)
# find_package(spdlog CONFIG REQUIRED)

# --- Third-party Dependency Management ---
include(FetchContent)

# Set FetchContent properties for better performance
set(FETCHCONTENT_QUIET OFF)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# CLI11 - Command line argument parsing library
FetchContent_Declare(
    CLI11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.4.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(CLI11)

# GoogleTest - Unit testing framework
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(googletest)

# --- Add Our Modules (Subprojects) ---
# Always build the core library
add_subdirectory(flux-core)

# Conditionally build CLI application
if(FLUX_BUILD_CLI)
    add_subdirectory(flux-cli)
endif()

# Conditionally build GUI application
if(FLUX_BUILD_GUI)
    add_subdirectory(flux-gui)
endif()

# --- Enable Testing ---
if(FLUX_BUILD_TESTS)
    enable_testing()
    include(GoogleTest)
    
    # Add test subdirectory if it exists
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        add_subdirectory(tests)
    endif()
endif()

# --- Enable Benchmarks ---
if(FLUX_BUILD_BENCHMARKS)
    # Add benchmark subdirectory if it exists
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/CMakeLists.txt")
        add_subdirectory(benchmarks)
    endif()
endif()

# --- Installation Configuration ---
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Create package config files
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/FluxConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/FluxConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Flux
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/FluxConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# --- CPack Configuration for Cross-Platform Packaging ---
include(InstallRequiredSystemLibraries)

# Basic package information
set(CPACK_PACKAGE_NAME "Flux")
set(CPACK_PACKAGE_VENDOR "xiaokanchengyang")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_CONTACT "xiaokanchengyang@example.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Modern high-performance archive management tool")
set(CPACK_PACKAGE_DESCRIPTION "Flux is a modern, high-performance archive manager with both GUI and CLI interfaces. It supports multiple archive formats and provides excellent user experience.")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/xiaokanchengyang/Flux-cpp")

# Package file naming
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")

# Platform-specific generators
if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
    # NSIS specific settings
    set(CPACK_NSIS_DISPLAY_NAME "Flux Archive Manager")
    set(CPACK_NSIS_PACKAGE_NAME "Flux")
    set(CPACK_NSIS_CONTACT "xiaokanchengyang@example.com")
    set(CPACK_NSIS_HELP_LINK "https://github.com/xiaokanchengyang/Flux-cpp")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/xiaokanchengyang/Flux-cpp")
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
elseif(APPLE)
    set(CPACK_GENERATOR "ZIP;DragNDrop")
    # macOS specific settings
    set(CPACK_DMG_VOLUME_NAME "Flux Archive Manager")
    set(CPACK_DMG_FORMAT "UDZO")
elseif(UNIX)
    set(CPACK_GENERATOR "TGZ;DEB;RPM")
    # Debian package settings
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "xiaokanchengyang <xiaokanchengyang@example.com>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libqt6core6, libqt6gui6, libqt6widgets6")
    
    # RPM package settings
    set(CPACK_RPM_PACKAGE_GROUP "Applications/Archiving")
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtbase >= 6.5")
endif()

# Resource files to include
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# Install targets - executables will be installed by their respective CMakeLists.txt
# This ensures proper installation paths and dependencies

include(CPack)

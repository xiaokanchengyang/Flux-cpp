cmake_minimum_required(VERSION 3.25)
project(Flux 
    VERSION 1.0.0
    DESCRIPTION "Modern high-performance archive management tool"
    HOMEPAGE_URL "https://github.com/flux-archive-manager/flux-cpp"
    LANGUAGES CXX
)

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
    # Disable some unnecessary warnings
    add_compile_options(/wd4251 /wd4275)
    # Enable UTF-8 encoding
    add_compile_options(/utf-8)
else()
    # GCC/Clang settings
    add_compile_options(-Wall -Wextra -Wpedantic)
    # Enable colored output for GCC/Clang
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-fdiagnostics-color=always)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_options(-fcolor-diagnostics)
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
add_subdirectory(flux-core)
add_subdirectory(flux-cli)
add_subdirectory(flux-gui)

# --- Enable Testing ---
enable_testing()
include(GoogleTest)

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

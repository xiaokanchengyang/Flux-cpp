# Flux CLI - Modern Command Line Tool
cmake_minimum_required(VERSION 3.22)

project(flux-cli 
    VERSION 1.0.0
    DESCRIPTION "Flux Archive Manager - Command Line Interface"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Create executable
add_executable(flux-cli)

# Source files with new structure
target_sources(flux-cli PRIVATE
    src/application/main.cpp
    src/application/cli_app.cpp
    src/commands/pack_command.cpp
    src/commands/extract_command.cpp
    src/commands/inspect_command.cpp
    src/commands/auto_command.cpp
    src/commands/batch_command.cpp
    src/commands/smart_command.cpp
    src/utils/progress_bar.cpp
    src/utils/format_utils.cpp
    src/utils/file_utils.cpp
)

# Include directories
target_include_directories(flux-cli PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Use FetchContent to get CLI11
include(FetchContent)

# CLI11 - Modern C++ command line parsing library
FetchContent_Declare(
    CLI11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.3.2
)

# indicators - Modern terminal progress bar library
FetchContent_Declare(
    indicators
    GIT_REPOSITORY https://github.com/p-ranav/indicators.git
    GIT_TAG v2.3
)

# spdlog - High performance logging library
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)

# nlohmann/json - JSON 库
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

# Download and build dependencies
FetchContent_MakeAvailable(CLI11 indicators spdlog nlohmann_json)

# Link libraries
target_link_libraries(flux-cli PRIVATE
    flux-core          # Our core library
    CLI11::CLI11       # Command line parsing
    indicators::indicators  # Progress bars
    spdlog::spdlog     # Logging
    nlohmann_json::nlohmann_json  # JSON library
)

# Compiler specific settings
if(MSVC)
    target_compile_options(flux-cli PRIVATE
        /W4                    # High warning level
        /permissive-          # Strict standard compliance
        /utf-8                # UTF-8 source file encoding
    )
else()
    target_compile_options(flux-cli PRIVATE
        -Wall -Wextra -Wpedantic  # Enable warnings
        -O3                       # Optimization
    )
endif()

# Platform specific settings
if(WIN32)
    target_compile_definitions(flux-cli PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        UNICODE
        _UNICODE
    )
    
    # Windows console UTF-8 support
    target_sources(flux-cli PRIVATE src/platform/windows_console.cpp)
    
elseif(UNIX AND NOT APPLE)
    # Linux specific settings
    target_link_libraries(flux-cli PRIVATE pthread)
    
elseif(APPLE)
    # macOS specific settings
    target_compile_definitions(flux-cli PRIVATE
        _DARWIN_C_SOURCE
    )
endif()

# Set output directory
set_target_properties(flux-cli PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    OUTPUT_NAME "flux"  # Output as 'flux' instead of 'flux-cli'
)

# Install rules
install(TARGETS flux-cli
    RUNTIME DESTINATION bin
    COMPONENT Runtime
)

# Add debug information for debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(flux-cli PRIVATE
        FLUX_DEBUG=1
        SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG
    )
endif()

# Add version information
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
    @ONLY
)

target_include_directories(flux-cli PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)
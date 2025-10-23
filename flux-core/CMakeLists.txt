# Flux-Core Static Library
cmake_minimum_required(VERSION 3.22)
project(flux-core VERSION 1.0.0 LANGUAGES CXX)

# Create static library
add_library(flux-core STATIC)

# Specify source files
target_sources(flux-core PRIVATE
    src/flux.cpp
    src/packer.cpp
    src/extractor.cpp
    src/archive_utils.cpp
    src/format_detector.cpp
    
    # Specific format implementations
    src/formats/zip_packer.cpp
    src/formats/zip_extractor.cpp
    src/formats/tar_packer.cpp
    src/formats/tar_extractor.cpp
    src/formats/sevenzip_packer.cpp
    src/formats/sevenzip_extractor.cpp
)

# Specify public include directories
target_include_directories(flux-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Private include directories (for internal implementation)
target_include_directories(flux-core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(flux-core PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        UNICODE
        _UNICODE
    )
elseif(APPLE)
    # macOS specific settings
    target_compile_definitions(flux-core PRIVATE
        _DARWIN_C_SOURCE
    )
elseif(UNIX)
    # Linux specific settings
    target_compile_definitions(flux-core PRIVATE
        _GNU_SOURCE
        _DEFAULT_SOURCE
    )
endif()

# Link third-party dependencies (commented out for initial build)
# target_link_libraries(flux-core PRIVATE
#     libzip::zip
#     ZLIB::ZLIB
#     LibLZMA::LibLZMA
#     zstd::libzstd_shared
#     fmt::fmt
#     spdlog::spdlog
# )

# Platform-specific libraries
if(WIN32)
    # Windows libraries
    target_link_libraries(flux-core PRIVATE
        # Add Windows-specific libraries here if needed
    )
elseif(APPLE)
    # macOS libraries
    target_link_libraries(flux-core PRIVATE
        # Add macOS-specific frameworks here if needed
    )
elseif(UNIX)
    # Linux libraries
    target_link_libraries(flux-core PRIVATE
        pthread
        # Add Linux-specific libraries here if needed
    )
endif()

# Set target properties
set_target_properties(flux-core PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

# Export compile commands (for IDEs and tools)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add tests (if testing is enabled)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

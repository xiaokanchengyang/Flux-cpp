# Flux CLI - 现代化命令行工具
cmake_minimum_required(VERSION 3.22)

project(flux-cli 
    VERSION 1.0.0
    DESCRIPTION "Flux Archive Manager - Command Line Interface"
    LANGUAGES CXX
)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 创建可执行文件
add_executable(flux-cli)

# 源文件
target_sources(flux-cli PRIVATE
    src/main.cpp
    src/cli_app.cpp
    src/commands/pack_command.cpp
    src/commands/extract_command.cpp
    src/commands/inspect_command.cpp
    src/utils/progress_bar.cpp
    src/utils/format_utils.cpp
    src/utils/file_utils.cpp
)

# 包含目录
target_include_directories(flux-cli PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 使用 FetchContent 获取 CLI11
include(FetchContent)

# CLI11 - 现代 C++ 命令行解析库
FetchContent_Declare(
    CLI11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.3.2
)

# indicators - 现代终端进度条库
FetchContent_Declare(
    indicators
    GIT_REPOSITORY https://github.com/p-ranav/indicators.git
    GIT_TAG v2.3
)

# spdlog - 高性能日志库
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

# 下载并构建依赖
FetchContent_MakeAvailable(CLI11 indicators spdlog nlohmann_json)

# 链接库
target_link_libraries(flux-cli PRIVATE
    flux-core          # 我们的核心库
    CLI11::CLI11       # 命令行解析
    indicators::indicators  # 进度条
    spdlog::spdlog     # 日志
    nlohmann_json::nlohmann_json  # JSON 库
)

# 编译器特定设置
if(MSVC)
    target_compile_options(flux-cli PRIVATE
        /W4                    # 高警告级别
        /permissive-          # 严格标准符合性
        /utf-8                # UTF-8 源文件编码
    )
else()
    target_compile_options(flux-cli PRIVATE
        -Wall -Wextra -Wpedantic  # 启用警告
        -O3                       # 优化
    )
endif()

# 平台特定设置
if(WIN32)
    target_compile_definitions(flux-cli PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        UNICODE
        _UNICODE
    )
    
    # Windows 控制台 UTF-8 支持
    target_sources(flux-cli PRIVATE src/platform/windows_console.cpp)
    
elseif(UNIX AND NOT APPLE)
    # Linux 特定设置
    target_link_libraries(flux-cli PRIVATE pthread)
    
elseif(APPLE)
    # macOS 特定设置
    target_compile_definitions(flux-cli PRIVATE
        _DARWIN_C_SOURCE
    )
endif()

# 设置输出目录
set_target_properties(flux-cli PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    OUTPUT_NAME "flux"  # 输出为 'flux' 而不是 'flux-cli'
)

# 安装规则
install(TARGETS flux-cli
    RUNTIME DESTINATION bin
    COMPONENT Runtime
)

# 为调试构建添加调试信息
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(flux-cli PRIVATE
        FLUX_DEBUG=1
        SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG
    )
endif()

# 添加版本信息
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
    @ONLY
)

target_include_directories(flux-cli PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)
#pragma once

#include <CLI/CLI.hpp>
#include <flux-core/archive.h>
#include <filesystem>
#include <string>

namespace FluxCLI::Commands {
    /**
     * extract 命令的配置和执行
     */
    struct ExtractConfig {
        std::filesystem::path archive;                // 输入归档文件
        std::filesystem::path output_dir;             // 输出目录
        Flux::OverwriteMode overwrite_mode = Flux::OverwriteMode::SKIP;  // 覆盖模式
        bool hoist_single_folder = false;             // 智能目录提升
        int strip_components = 0;                     // 剥离目录层数
        std::string password;                         // 密码
        std::vector<std::string> include_patterns;    // 包含模式
        std::vector<std::string> exclude_patterns;    // 排除模式
        bool preserve_permissions = true;             // 保留权限
        bool preserve_timestamps = true;              // 保留时间戳
        bool verbose = false;                         // 详细模式
        bool quiet = false;                           // 静默模式
        
        // 验证配置
        void validate();
        
        // 转换为 Flux::ExtractOptions
        Flux::ExtractOptions toFluxOptions() const;
    };
    
    /**
     * 设置 extract 子命令
     * @param app CLI 应用对象
     * @param verbose 全局详细标志
     * @param quiet 全局静默标志
     */
    void setupExtractCommand(CLI::App* app, bool& verbose, bool& quiet);
    
    /**
     * 执行 extract 命令
     * @param config 解压配置
     * @return 退出码
     */
    int executeExtractCommand(const ExtractConfig& config);
    
    /**
     * 智能目录提升：检查是否应该提升单一根目录
     * @param archive_path 归档文件路径
     * @return 是否应该提升
     */
    bool shouldHoistDirectory(const std::filesystem::path& archive_path);
    
    /**
     * 预览解压操作（不实际解压）
     * @param config 解压配置
     * @return 将要解压的文件列表
     */
    std::vector<std::string> previewExtraction(const ExtractConfig& config);
    
    /**
     * 验证输出目录
     * @param output_dir 输出目录
     * @param create_if_missing 如果不存在是否创建
     * @return 是否有效
     */
    bool validateOutputDirectory(const std::filesystem::path& output_dir, bool create_if_missing = true);
}


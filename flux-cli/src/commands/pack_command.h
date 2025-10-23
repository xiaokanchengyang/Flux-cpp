#pragma once

#include <CLI/CLI.hpp>
#include <flux-core/archive.h>
#include <vector>
#include <string>
#include <filesystem>

namespace FluxCLI::Commands {
    /**
     * pack 命令的配置和执行
     */
    struct PackConfig {
        std::vector<std::filesystem::path> inputs;    // 输入文件/目录列表
        std::filesystem::path output;                 // 输出归档文件
        std::string format_str;                       // 格式字符串
        Flux::ArchiveFormat format;                   // 解析后的格式
        int compression_level = -1;                   // 压缩级别 (-1 表示使用默认值)
        int num_threads = 0;                          // 线程数 (0 表示自动)
        std::vector<std::string> exclude_patterns;    // 排除模式
        std::string strategy = "auto";                // 压缩策略
        std::string password;                         // 密码保护
        bool preserve_permissions = true;             // 保留权限
        bool preserve_timestamps = true;              // 保留时间戳
        bool verbose = false;                         // 详细模式
        bool quiet = false;                           // 静默模式
        
        // 验证配置
        void validate();
        
        // 转换为 Flux::PackOptions
        Flux::PackOptions toFluxOptions() const;
    };
    
    /**
     * 设置 pack 子命令
     * @param app CLI 应用对象
     * @param verbose 全局详细标志
     * @param quiet 全局静默标志
     */
    void setupPackCommand(CLI::App* app, bool& verbose, bool& quiet);
    
    /**
     * 执行 pack 命令
     * @param config 打包配置
     * @return 退出码
     */
    int executePackCommand(const PackConfig& config);
    
    /**
     * 智能压缩策略：根据文件类型决定是否压缩
     * @param file_path 文件路径
     * @return 是否应该压缩此文件
     */
    bool shouldCompressFile(const std::filesystem::path& file_path);
    
    /**
     * 展开 glob 模式的输入路径
     * @param patterns 输入模式列表
     * @return 展开后的实际文件路径列表
     */
    std::vector<std::filesystem::path> expandInputPatterns(const std::vector<std::string>& patterns);
    
    /**
     * 验证输出路径是否合法
     * @param output_path 输出路径
     * @param inputs 输入路径列表
     * @return 是否合法
     */
    bool validateOutputPath(const std::filesystem::path& output_path, 
                           const std::vector<std::filesystem::path>& inputs);
}


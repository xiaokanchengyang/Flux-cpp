#pragma once

#include <CLI/CLI.hpp>
#include <flux-core/archive.h>
#include <filesystem>
#include <string>
#include <vector>

namespace FluxCLI::Commands {
    /**
     * 输出格式枚举
     */
    enum class OutputFormat {
        LIST,    // 简单列表格式
        TREE,    // 树状结构格式
        JSON,    // JSON 格式 (机器可读)
        DETAILED // 详细信息格式
    };
    
    /**
     * inspect 命令的配置和执行
     */
    struct InspectConfig {
        std::filesystem::path archive;                // 归档文件路径
        OutputFormat output_format = OutputFormat::LIST;  // 输出格式
        bool show_hidden = false;                     // 显示隐藏文件
        bool show_size = true;                        // 显示文件大小
        bool show_date = false;                       // 显示修改日期
        bool show_permissions = false;                // 显示权限
        bool show_checksum = false;                   // 显示校验和
        std::string filter_pattern;                   // 过滤模式
        bool recursive = true;                        // 递归显示
        int max_depth = -1;                          // 最大深度 (-1 表示无限制)
        std::string password;                         // 密码
        bool verbose = false;                         // 详细模式
        bool quiet = false;                           // 静默模式
        
        // 验证配置
        void validate();
    };
    
    /**
     * 归档条目的显示信息
     */
    struct DisplayEntry {
        std::string name;                             // 文件名
        std::string path;                             // 完整路径
        bool is_directory;                            // 是否为目录
        size_t compressed_size;                       // 压缩大小
        size_t uncompressed_size;                     // 原始大小
        std::string modification_time;                // 修改时间
        uint32_t permissions;                         // 权限
        std::string checksum;                         // 校验和
        int depth;                                    // 目录深度
    };
    
    /**
     * 设置 inspect 子命令
     * @param app CLI 应用对象
     * @param verbose 全局详细标志
     * @param quiet 全局静默标志
     */
    void setupInspectCommand(CLI::App* app, bool& verbose, bool& quiet);
    
    /**
     * 执行 inspect 命令
     * @param config 检查配置
     * @return 退出码
     */
    int executeInspectCommand(const InspectConfig& config);
    
    /**
     * 获取归档内容列表
     * @param archive_path 归档文件路径
     * @param password 密码 (可选)
     * @return 条目列表
     */
    std::vector<DisplayEntry> getArchiveContents(const std::filesystem::path& archive_path, 
                                                 const std::string& password = "");
    
    /**
     * 过滤条目列表
     * @param entries 原始条目列表
     * @param config 配置
     * @return 过滤后的条目列表
     */
    std::vector<DisplayEntry> filterEntries(const std::vector<DisplayEntry>& entries, 
                                           const InspectConfig& config);
    
    /**
     * 以列表格式输出
     * @param entries 条目列表
     * @param config 配置
     */
    void outputList(const std::vector<DisplayEntry>& entries, const InspectConfig& config);
    
    /**
     * 以树状格式输出
     * @param entries 条目列表
     * @param config 配置
     */
    void outputTree(const std::vector<DisplayEntry>& entries, const InspectConfig& config);
    
    /**
     * 以 JSON 格式输出
     * @param entries 条目列表
     * @param config 配置
     */
    void outputJSON(const std::vector<DisplayEntry>& entries, const InspectConfig& config);
    
    /**
     * 以详细格式输出
     * @param entries 条目列表
     * @param config 配置
     */
    void outputDetailed(const std::vector<DisplayEntry>& entries, const InspectConfig& config);
    
    /**
     * 计算并显示归档统计信息
     * @param entries 条目列表
     * @param config 配置
     */
    void showStatistics(const std::vector<DisplayEntry>& entries, const InspectConfig& config);
}


#pragma once

#include <flux-core/archive.h>
#include <string>
#include <filesystem>
#include <vector>

namespace FluxCLI::Utils {
    /**
     * 格式检测和转换工具
     */
    class FormatUtils {
    public:
        /**
         * 根据文件扩展名推断归档格式
         * @param filename 文件名或路径
         * @return 推断的格式，如果无法推断则抛出异常
         */
        static Flux::ArchiveFormat detectFormatFromExtension(const std::filesystem::path& filename);
        
        /**
         * 根据文件内容检测归档格式
         * @param filepath 文件路径
         * @return 检测到的格式，如果无法检测则抛出异常
         */
        static Flux::ArchiveFormat detectFormatFromContent(const std::filesystem::path& filepath);
        
        /**
         * 将格式字符串转换为枚举
         * @param format_str 格式字符串 (如 "zip", "tar.gz")
         * @return 对应的格式枚举
         */
        static Flux::ArchiveFormat parseFormatString(const std::string& format_str);
        
        /**
         * 获取格式的默认文件扩展名
         * @param format 归档格式
         * @return 默认扩展名 (如 ".zip", ".tar.gz")
         */
        static std::string getDefaultExtension(Flux::ArchiveFormat format);
        
        /**
         * 获取所有支持的格式字符串列表
         * @return 格式字符串列表
         */
        static std::vector<std::string> getSupportedFormatStrings();
        
        /**
         * 验证压缩级别是否对指定格式有效
         * @param format 归档格式
         * @param level 压缩级别
         * @return 是否有效
         */
        static bool isCompressionLevelValid(Flux::ArchiveFormat format, int level);
        
        /**
         * 获取格式的推荐压缩级别范围
         * @param format 归档格式
         * @return {最小级别, 最大级别, 默认级别}
         */
        static std::tuple<int, int, int> getCompressionLevelRange(Flux::ArchiveFormat format);
        
        /**
         * 格式化文件大小为人类可读格式
         * @param bytes 字节数
         * @param binary 是否使用二进制单位 (1024) 而不是十进制单位 (1000)
         * @return 格式化的字符串
         */
        static std::string formatFileSize(size_t bytes, bool binary = true);
        
        /**
         * 格式化持续时间
         * @param milliseconds 毫秒数
         * @return 格式化的字符串
         */
        static std::string formatDuration(size_t milliseconds);
        
        /**
         * 格式化压缩比
         * @param original_size 原始大小
         * @param compressed_size 压缩后大小
         * @return 格式化的压缩比字符串 (如 "65.2%")
         */
        static std::string formatCompressionRatio(size_t original_size, size_t compressed_size);
    };
    
    /**
     * 文件路径工具
     */
    class PathUtils {
    public:
        /**
         * 规范化路径分隔符（统一使用正斜杠）
         * @param path 输入路径
         * @return 规范化的路径
         */
        static std::string normalizePath(const std::string& path);
        
        /**
         * 检查路径是否为绝对路径
         * @param path 路径
         * @return 是否为绝对路径
         */
        static bool isAbsolutePath(const std::string& path);
        
        /**
         * 获取相对于基础目录的相对路径
         * @param path 目标路径
         * @param base 基础目录
         * @return 相对路径
         */
        static std::filesystem::path getRelativePath(
            const std::filesystem::path& path, 
            const std::filesystem::path& base
        );
        
        /**
         * 安全地连接路径组件（防止路径遍历攻击）
         * @param base 基础路径
         * @param component 要添加的组件
         * @return 安全的连接路径
         */
        static std::filesystem::path safeJoinPath(
            const std::filesystem::path& base,
            const std::string& component
        );
    };
}


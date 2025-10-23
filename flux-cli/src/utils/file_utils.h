#pragma once

#include <filesystem>
#include <vector>
#include <string>

namespace FluxCLI::Utils {
    /**
     * 文件系统工具类
     */
    class FileUtils {
    public:
        /**
         * 递归获取目录中的所有文件
         * @param directory 目录路径
         * @param include_hidden 是否包含隐藏文件
         * @return 文件路径列表
         */
        static std::vector<std::filesystem::path> getFilesRecursively(
            const std::filesystem::path& directory,
            bool include_hidden = false
        );
        
        /**
         * 检查文件是否匹配 glob 模式
         * @param filepath 文件路径
         * @param pattern glob 模式
         * @return 是否匹配
         */
        static bool matchesGlob(const std::filesystem::path& filepath, const std::string& pattern);
        
        /**
         * 获取文件的 MIME 类型
         * @param filepath 文件路径
         * @return MIME 类型字符串
         */
        static std::string getMimeType(const std::filesystem::path& filepath);
        
        /**
         * 检查文件是否为文本文件
         * @param filepath 文件路径
         * @return 是否为文本文件
         */
        static bool isTextFile(const std::filesystem::path& filepath);
        
        /**
         * 安全地创建目录（包括父目录）
         * @param directory 目录路径
         * @return 是否成功
         */
        static bool createDirectorySafely(const std::filesystem::path& directory);
        
        /**
         * 获取文件的权限字符串表示
         * @param permissions 权限值
         * @return 权限字符串 (如 "rwxr-xr-x")
         */
        static std::string formatPermissions(uint32_t permissions);
        
        /**
         * 计算文件的哈希值
         * @param filepath 文件路径
         * @param algorithm 哈希算法 ("md5", "sha1", "sha256")
         * @return 哈希值的十六进制字符串
         */
        static std::string calculateFileHash(const std::filesystem::path& filepath, 
                                           const std::string& algorithm = "sha256");
    };
}
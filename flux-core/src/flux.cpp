#include "flux-core/flux.h"
#include <map>
#include <iostream>

namespace Flux {
    // 全局初始化标志
    static bool g_initialized = false;

    void initialize() {
        if (g_initialized) {
            return;
        }

        // 初始化第三方库
        // TODO: 初始化 zstd, zlib, lzma 等压缩库
        
        std::cout << "Flux library initialized (version " << getVersion() << ")" << std::endl;
        g_initialized = true;
    }

    void cleanup() {
        if (!g_initialized) {
            return;
        }

        // 清理第三方库资源
        // TODO: 清理压缩库资源
        
        std::cout << "Flux library cleaned up" << std::endl;
        g_initialized = false;
    }

    std::string getVersion() {
        return Version::toString();
    }

    std::map<ArchiveFormat, std::string> getSupportedFormatsInfo() {
        return {
            {ArchiveFormat::ZIP, "ZIP - 通用压缩格式，广泛兼容"},
            {ArchiveFormat::TAR_ZSTD, "TAR+ZSTD - 高性能压缩，推荐格式"},
            {ArchiveFormat::TAR_GZ, "TAR+GZIP - 传统Unix压缩格式"},
            {ArchiveFormat::TAR_XZ, "TAR+XZ - 高压缩比格式"},
            {ArchiveFormat::SEVEN_ZIP, "7-Zip - 高压缩比专业格式"}
        };
    }
}


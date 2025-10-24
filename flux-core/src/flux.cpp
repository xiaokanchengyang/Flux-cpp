#include "flux-core/flux.h"
#include <map>
#include <iostream>

namespace Flux {
    // Global initialization flag
    static bool g_initialized = false;

    void initialize() {
        if (g_initialized) {
            return;
        }

        // Initialize third-party libraries
        // TODO: Initialize compression libraries (zstd, zlib, lzma, etc.)
        
        std::cout << "Flux library initialized (version " << getVersion() << ")" << std::endl;
        g_initialized = true;
    }

    void cleanup() {
        if (!g_initialized) {
            return;
        }

        // Cleanup third-party library resources
        // TODO: Cleanup compression library resources
        
        std::cout << "Flux library cleaned up" << std::endl;
        g_initialized = false;
    }

    std::string getVersion() {
        return Version::toString();
    }

    std::map<ArchiveFormat, std::string> getSupportedFormatsInfo() {
        return {
            {ArchiveFormat::ZIP, "ZIP - Universal compression format with wide compatibility"},
            {ArchiveFormat::TAR_ZSTD, "TAR+ZSTD - High-performance compression, recommended format"},
            {ArchiveFormat::TAR_GZ, "TAR+GZIP - Traditional Unix compression format"},
            {ArchiveFormat::TAR_XZ, "TAR+XZ - High compression ratio format"},
            {ArchiveFormat::SEVEN_ZIP, "7-Zip - High compression ratio professional format"}
        };
    }
}


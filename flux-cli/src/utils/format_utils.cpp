#include "format_utils.h"
#include <flux-core/exceptions.h>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <cctype>

namespace FluxCLI::Utils {

Flux::ArchiveFormat FormatUtils::detectFormatFromExtension(const std::filesystem::path& filename) {
    std::string ext = filename.extension().string();
    std::string stem = filename.stem().string();
    
    // Convert to lowercase
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    std::transform(stem.begin(), stem.end(), stem.begin(), ::tolower);
    
    // Check compound extensions
    if (ext == ".gz" && stem.ends_with(".tar")) {
        return Flux::ArchiveFormat::TAR_GZ;
    } else if (ext == ".xz" && stem.ends_with(".tar")) {
        return Flux::ArchiveFormat::TAR_XZ;
    } else if (ext == ".zst" && stem.ends_with(".tar")) {
        return Flux::ArchiveFormat::TAR_ZSTD;
    }
    
    // Check single extensions
    if (ext == ".zip") {
        return Flux::ArchiveFormat::ZIP;
    } else if (ext == ".7z") {
        return Flux::ArchiveFormat::SEVEN_ZIP;
    } else if (ext == ".tgz") {
        return Flux::ArchiveFormat::TAR_GZ;
    } else if (ext == ".txz") {
        return Flux::ArchiveFormat::TAR_XZ;
    }
    
    throw Flux::UnsupportedFormatException("Cannot detect format from extension: " + ext);
}

Flux::ArchiveFormat FormatUtils::detectFormatFromContent(const std::filesystem::path& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        throw Flux::FileNotFoundException(filepath.string());
    }
    
    // Read file header
    char header[16] = {0};
    file.read(header, sizeof(header));
    size_t bytes_read = file.gcount();
    
    if (bytes_read < 4) {
        throw Flux::CorruptedArchiveException("File too small to determine format");
    }
    
    // ZIP format detection
    if (bytes_read >= 4 && 
        static_cast<unsigned char>(header[0]) == 0x50 &&
        static_cast<unsigned char>(header[1]) == 0x4B &&
        (static_cast<unsigned char>(header[2]) == 0x03 || static_cast<unsigned char>(header[2]) == 0x05) &&
        (static_cast<unsigned char>(header[3]) == 0x04 || static_cast<unsigned char>(header[3]) == 0x06)) {
        return Flux::ArchiveFormat::ZIP;
    }
    
    // 7-Zip format detection
    if (bytes_read >= 6 &&
        header[0] == '7' && header[1] == 'z' &&
        static_cast<unsigned char>(header[2]) == 0xBC &&
        static_cast<unsigned char>(header[3]) == 0xAF &&
        static_cast<unsigned char>(header[4]) == 0x27 &&
        static_cast<unsigned char>(header[5]) == 0x1C) {
        return Flux::ArchiveFormat::SEVEN_ZIP;
    }
    
    // TAR format detection (check POSIX tar header)
    if (bytes_read >= 8) {
        // TAR files have "ustar" identifier at offset 257, but we first check if it might be TAR
        file.seekg(257);
        char ustar[6] = {0};
        file.read(ustar, 5);
        if (file.gcount() == 5 && std::string(ustar) == "ustar") {
            // This is a TAR file, but we need to check compression format
            // Since it's already a compressed TAR, we need to determine by extension
            return detectFormatFromExtension(filepath);
        }
    }
    
    // Gzip format detection
    if (bytes_read >= 3 &&
        static_cast<unsigned char>(header[0]) == 0x1F &&
        static_cast<unsigned char>(header[1]) == 0x8B &&
        static_cast<unsigned char>(header[2]) == 0x08) {
        return Flux::ArchiveFormat::TAR_GZ;
    }
    
    // XZ format detection
    if (bytes_read >= 6 &&
        static_cast<unsigned char>(header[0]) == 0xFD &&
        header[1] == '7' && header[2] == 'z' &&
        header[3] == 'X' && header[4] == 'Z' &&
        static_cast<unsigned char>(header[5]) == 0x00) {
        return Flux::ArchiveFormat::TAR_XZ;
    }
    
    // Zstandard format detection
    if (bytes_read >= 4 &&
        static_cast<unsigned char>(header[0]) == 0x28 &&
        static_cast<unsigned char>(header[1]) == 0xB5 &&
        static_cast<unsigned char>(header[2]) == 0x2F &&
        static_cast<unsigned char>(header[3]) == 0xFD) {
        return Flux::ArchiveFormat::TAR_ZSTD;
    }
    
    throw Flux::UnsupportedFormatException("Cannot detect format from file content");
}

Flux::ArchiveFormat FormatUtils::parseFormatString(const std::string& format_str) {
    std::string lower_format = format_str;
    std::transform(lower_format.begin(), lower_format.end(), lower_format.begin(), ::tolower);
    
    if (lower_format == "zip") {
        return Flux::ArchiveFormat::ZIP;
    } else if (lower_format == "tar.gz" || lower_format == "tgz") {
        return Flux::ArchiveFormat::TAR_GZ;
    } else if (lower_format == "tar.xz" || lower_format == "txz") {
        return Flux::ArchiveFormat::TAR_XZ;
    } else if (lower_format == "tar.zst" || lower_format == "tar.zstd") {
        return Flux::ArchiveFormat::TAR_ZSTD;
    } else if (lower_format == "7z") {
        return Flux::ArchiveFormat::SEVEN_ZIP;
    } else {
        throw Flux::UnsupportedFormatException(format_str);
    }
}

std::string FormatUtils::getDefaultExtension(Flux::ArchiveFormat format) {
    switch (format) {
        case Flux::ArchiveFormat::ZIP:
            return ".zip";
        case Flux::ArchiveFormat::TAR_GZ:
            return ".tar.gz";
        case Flux::ArchiveFormat::TAR_XZ:
            return ".tar.xz";
        case Flux::ArchiveFormat::TAR_ZSTD:
            return ".tar.zst";
        case Flux::ArchiveFormat::SEVEN_ZIP:
            return ".7z";
        default:
            return ".unknown";
    }
}

std::vector<std::string> FormatUtils::getSupportedFormatStrings() {
    return {"zip", "tar.gz", "tgz", "tar.xz", "txz", "tar.zst", "tar.zstd", "7z"};
}

bool FormatUtils::isCompressionLevelValid(Flux::ArchiveFormat format, int level) {
    auto [min_level, max_level, default_level] = getCompressionLevelRange(format);
    return level >= min_level && level <= max_level;
}

std::tuple<int, int, int> FormatUtils::getCompressionLevelRange(Flux::ArchiveFormat format) {
    switch (format) {
        case Flux::ArchiveFormat::ZIP:
            return {0, 9, 6};  // ZIP: 0-9, default 6
        case Flux::ArchiveFormat::TAR_GZ:
            return {1, 9, 6};  // Gzip: 1-9, default 6
        case Flux::ArchiveFormat::TAR_XZ:
            return {0, 9, 6};  // XZ: 0-9, default 6
        case Flux::ArchiveFormat::TAR_ZSTD:
            return {1, 22, 3}; // Zstd: 1-22, default 3
        case Flux::ArchiveFormat::SEVEN_ZIP:
            return {0, 9, 5};  // 7-Zip: 0-9, default 5
        default:
            return {0, 9, 3};
    }
}

std::string FormatUtils::formatFileSize(size_t bytes, bool binary) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    const size_t base = binary ? 1024 : 1000;
    
    if (bytes == 0) {
        return "0 B";
    }
    
    int unit = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= base && unit < 5) {
        size /= base;
        unit++;
    }
    
    std::ostringstream oss;
    if (unit == 0) {
        oss << static_cast<size_t>(size) << " " << units[unit];
    } else {
        oss << std::fixed << std::setprecision(1) << size << " " << units[unit];
    }
    
    return oss.str();
}

std::string FormatUtils::formatDuration(size_t milliseconds) {
    if (milliseconds < 1000) {
        return std::to_string(milliseconds) + "ms";
    } else if (milliseconds < 60000) {
        double seconds = milliseconds / 1000.0;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << seconds << "s";
        return oss.str();
    } else if (milliseconds < 3600000) {
        size_t minutes = milliseconds / 60000;
        size_t seconds = (milliseconds % 60000) / 1000;
        return std::to_string(minutes) + "m" + std::to_string(seconds) + "s";
    } else {
        size_t hours = milliseconds / 3600000;
        size_t minutes = (milliseconds % 3600000) / 60000;
        return std::to_string(hours) + "h" + std::to_string(minutes) + "m";
    }
}

std::string FormatUtils::formatCompressionRatio(size_t original_size, size_t compressed_size) {
    if (original_size == 0) {
        return "0.0%";
    }
    
    double ratio = (1.0 - static_cast<double>(compressed_size) / static_cast<double>(original_size)) * 100.0;
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << ratio << "%";
    return oss.str();
}

// PathUtils implementation
std::string PathUtils::normalizePath(const std::string& path) {
    std::string normalized = path;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    return normalized;
}

bool PathUtils::isAbsolutePath(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    
    // Unix style absolute path
    if (path[0] == '/') {
        return true;
    }
    
    // Windows style absolute path
    if (path.length() >= 3 && path[1] == ':' && (path[2] == '/' || path[2] == '\\')) {
        return true;
    }
    
    return false;
}

std::filesystem::path PathUtils::getRelativePath(
    const std::filesystem::path& path, 
    const std::filesystem::path& base) {
    
    try {
        return std::filesystem::relative(path, base);
    } catch (const std::filesystem::filesystem_error&) {
        // If unable to calculate relative path, return original path
        return path;
    }
}

std::filesystem::path PathUtils::safeJoinPath(
    const std::filesystem::path& base,
    const std::string& component) {
    
    // Normalize component path
    std::string safe_component = normalizePath(component);
    
    // Remove leading slashes
    while (!safe_component.empty() && safe_component[0] == '/') {
        safe_component = safe_component.substr(1);
    }
    
    // Check for path traversal attacks
    if (safe_component.find("..") != std::string::npos) {
        throw std::invalid_argument("Path traversal detected: " + component);
    }
    
    return base / safe_component;
}

} // namespace FluxCLI::Utils


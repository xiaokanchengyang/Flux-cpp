#include "file_utils.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <regex>
#include <algorithm>
#include <set>

namespace FluxCLI::Utils {

std::vector<std::filesystem::path> FileUtils::getFilesRecursively(
    const std::filesystem::path& directory,
    bool include_hidden) {
    
    std::vector<std::filesystem::path> files;
    std::error_code ec;
    
    if (!std::filesystem::exists(directory, ec) || !std::filesystem::is_directory(directory, ec)) {
        return files;
    }
    
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory, ec)) {
            if (ec) {
                spdlog::warn("Cannot access: {} ({})", entry.path().string(), ec.message());
                continue;
            }
            
            if (entry.is_regular_file(ec)) {
                std::string filename = entry.path().filename().string();
                
                // Check if it's a hidden file
                if (!include_hidden && filename.starts_with('.')) {
                    continue;
                }
                
                files.push_back(entry.path());
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        spdlog::error("Failed to traverse directory {}: {}", directory.string(), e.what());
    }
    
    return files;
}

bool FileUtils::matchesGlob(const std::filesystem::path& filepath, const std::string& pattern) {
    // Simple glob matching implementation
    // Convert glob pattern to regular expression
    std::string regex_pattern = pattern;
    
    // Escape regex special characters
    std::regex special_chars{R"([-[\]{}()+.,\^$|#\s])"};
    regex_pattern = std::regex_replace(regex_pattern, special_chars, R"(\$&)");
    
    // Convert glob wildcards
    std::regex glob_star{R"(\\\*)"};
    regex_pattern = std::regex_replace(regex_pattern, glob_star, ".*");
    
    std::regex glob_question{R"(\\\?)"};
    regex_pattern = std::regex_replace(regex_pattern, glob_question, ".");
    
    try {
        std::regex pattern_regex(regex_pattern, std::regex_constants::icase);
        return std::regex_match(filepath.filename().string(), pattern_regex);
    } catch (const std::regex_error& e) {
        spdlog::warn("Invalid glob pattern: {}", pattern);
        return false;
    }
}

std::string FileUtils::getMimeType(const std::filesystem::path& filepath) {
    std::string ext = filepath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    // Simple MIME type mapping
    static const std::map<std::string, std::string> mime_types = {
        {".txt", "text/plain"},
        {".md", "text/markdown"},
        {".html", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".json", "application/json"},
        {".xml", "application/xml"},
        {".pdf", "application/pdf"},
        {".zip", "application/zip"},
        {".tar", "application/x-tar"},
        {".gz", "application/gzip"},
        {".7z", "application/x-7z-compressed"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".png", "image/png"},
        {".gif", "image/gif"},
        {".webp", "image/webp"},
        {".mp3", "audio/mpeg"},
        {".wav", "audio/wav"},
        {".mp4", "video/mp4"},
        {".avi", "video/x-msvideo"}
    };
    
    auto it = mime_types.find(ext);
    if (it != mime_types.end()) {
        return it->second;
    }
    
    return "application/octet-stream";
}

bool FileUtils::isTextFile(const std::filesystem::path& filepath) {
    std::string mime_type = getMimeType(filepath);
    
    if (mime_type.starts_with("text/")) {
        return true;
    }
    
    // Check some special text files
    std::string ext = filepath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    static const std::set<std::string> text_extensions = {
        ".c", ".cpp", ".h", ".hpp", ".cc", ".cxx",
        ".py", ".java", ".cs", ".php", ".rb", ".go",
        ".rs", ".swift", ".kt", ".scala", ".clj",
        ".sh", ".bash", ".zsh", ".fish", ".ps1",
        ".yaml", ".yml", ".toml", ".ini", ".cfg",
        ".log", ".conf", ".config", ".gitignore",
        ".dockerfile", ".makefile", ".cmake"
    };
    
    return text_extensions.find(ext) != text_extensions.end();
}

bool FileUtils::createDirectorySafely(const std::filesystem::path& directory) {
    std::error_code ec;
    
    if (std::filesystem::exists(directory, ec)) {
        return std::filesystem::is_directory(directory, ec);
    }
    
    return std::filesystem::create_directories(directory, ec);
}

std::string FileUtils::formatPermissions(uint32_t permissions) {
    std::string result(9, '-');
    
    // Owner permissions
    if (permissions & 0400) result[0] = 'r';
    if (permissions & 0200) result[1] = 'w';
    if (permissions & 0100) result[2] = 'x';
    
    // Group permissions
    if (permissions & 0040) result[3] = 'r';
    if (permissions & 0020) result[4] = 'w';
    if (permissions & 0010) result[5] = 'x';
    
    // Other user permissions
    if (permissions & 0004) result[6] = 'r';
    if (permissions & 0002) result[7] = 'w';
    if (permissions & 0001) result[8] = 'x';
    
    return result;
}

std::string FileUtils::calculateFileHash(const std::filesystem::path& filepath, 
                                        const std::string& algorithm) {
    // Should implement real hash calculation here
    // For simplification, we return a placeholder
    // In actual projects, can use OpenSSL or other crypto libraries
    
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    // Simple checksum calculation (for demonstration only)
    size_t hash = 0;
    char buffer[4096];
    
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        for (int i = 0; i < file.gcount(); ++i) {
            hash = hash * 31 + static_cast<unsigned char>(buffer[i]);
        }
    }
    
    // Convert to hexadecimal string
    std::ostringstream oss;
    oss << std::hex << hash;
    return oss.str();
}

} // namespace FluxCLI::Utils
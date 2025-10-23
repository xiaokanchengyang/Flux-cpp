#include "inspect_command.h"
#include "../utils/format_utils.h"
#include <flux-core/exceptions.h>
#include <flux-core/packer.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <regex>
#include <map>

namespace FluxCLI::Commands {

void InspectConfig::validate() {
    // Validate archive file
    if (archive.empty()) {
        throw std::invalid_argument("Archive file must be specified");
    }
    
    if (!std::filesystem::exists(archive)) {
        throw Flux::FileNotFoundException(archive.string());
    }
    
    if (!std::filesystem::is_regular_file(archive)) {
        throw std::invalid_argument("Specified path is not a file: " + archive.string());
    }
    
    // Validate max depth
    if (max_depth < -1) {
        throw std::invalid_argument("Max depth cannot be less than -1");
    }
}

void setupInspectCommand(CLI::App* app, bool& verbose, bool& quiet) {
    static InspectConfig config;
    
    // Archive file (positional argument)
    std::string archive_string;
    app->add_option("archive", archive_string, "Archive file to inspect")
       ->required()
       ->check(CLI::ExistingFile);
    
    // Output format
    std::string format_string = "list";
    app->add_option("-f,--format", format_string, "Output format")
       ->check(CLI::IsMember({"list", "tree", "json", "detailed"}));
    
    // Tree format shortcut option
    app->add_flag("--tree", [&config](size_t) { config.output_format = OutputFormat::TREE; },
                  "Display as tree structure (equivalent to --format=tree)");
    
    // JSON format shortcut option
    app->add_flag("--json", [&config](size_t) { config.output_format = OutputFormat::JSON; },
                  "Output in JSON format (equivalent to --format=json)");
    
    // Display options
    app->add_flag("-a,--all", config.show_hidden, "Show hidden files");
    app->add_flag("-s,--size", config.show_size, "Show file sizes (enabled by default)");
    app->add_flag("-d,--date", config.show_date, "Show modification dates");
    app->add_flag("-p,--permissions", config.show_permissions, "Show file permissions");
    app->add_flag("-c,--checksum", config.show_checksum, "Show file checksums");
    
    // Filtering and depth
    app->add_option("--filter", config.filter_pattern, "Filter filenames (supports regex)");
    app->add_option("--max-depth", config.max_depth, "Maximum display depth")
       ->check(CLI::Range(-1, 100));
    
    // Password
    app->add_option("--password", config.password, "Archive password (leave empty to prompt)");
    
    // Command callback
    app->callback([&config, &archive_string, &format_string, &verbose, &quiet]() {
        config.archive = archive_string;
        config.verbose = verbose;
        config.quiet = quiet;
        
        // Parse output format
        if (format_string == "list") {
            config.output_format = OutputFormat::LIST;
        } else if (format_string == "tree") {
            config.output_format = OutputFormat::TREE;
        } else if (format_string == "json") {
            config.output_format = OutputFormat::JSON;
        } else if (format_string == "detailed") {
            config.output_format = OutputFormat::DETAILED;
        }
        
        // 验证配置
        config.validate();
        
        // 执行命令
        int exit_code = executeInspectCommand(config);
        if (exit_code != 0) {
            std::exit(exit_code);
        }
    });
}

int executeInspectCommand(const InspectConfig& config) {
    try {
        if (!config.quiet) {
            spdlog::info("Inspecting archive: {}", config.archive.string());
        }
        
        // Detect archive format
        Flux::ArchiveFormat format;
        try {
            format = Utils::FormatUtils::detectFormatFromContent(config.archive);
        } catch (const Flux::UnsupportedFormatException&) {
            format = Utils::FormatUtils::detectFormatFromExtension(config.archive);
        }
        
        if (config.verbose) {
            spdlog::debug("Detected format: {}", Flux::formatToString(format));
        }
        
        // Get archive contents
        auto entries = getArchiveContents(config.archive, config.password);
        
        if (entries.empty()) {
            if (!config.quiet) {
                spdlog::info("Archive is empty or cannot read contents");
            }
            return 0;
        }
        
        // Filter entries
        auto filtered_entries = filterEntries(entries, config);
        
        // Output according to format
        switch (config.output_format) {
            case OutputFormat::LIST:
                outputList(filtered_entries, config);
                break;
            case OutputFormat::TREE:
                outputTree(filtered_entries, config);
                break;
            case OutputFormat::JSON:
                outputJSON(filtered_entries, config);
                break;
            case OutputFormat::DETAILED:
                outputDetailed(filtered_entries, config);
                break;
        }
        
        // Show statistics (except for JSON format)
        if (config.output_format != OutputFormat::JSON && !config.quiet) {
            showStatistics(filtered_entries, config);
        }
        
        return 0;
        
    } catch (const Flux::FileNotFoundException& e) {
        spdlog::error("File not found: {}", e.what());
        return 2;
    } catch (const Flux::PermissionDeniedException& e) {
        spdlog::error("Permission denied: {}", e.what());
        return 3;
    } catch (const Flux::CorruptedArchiveException& e) {
        spdlog::error("Archive file corrupted: {}", e.what());
        return 4;
    } catch (const Flux::UnsupportedFormatException& e) {
        spdlog::error("Unsupported format: {}", e.what());
        return 5;
    } catch (const Flux::InvalidPasswordException& e) {
        spdlog::error("Invalid password: {}", e.what());
        return 6;
    } catch (const std::exception& e) {
        spdlog::error("Error: {}", e.what());
        return 1;
    }
}

std::vector<DisplayEntry> getArchiveContents(const std::filesystem::path& archive_path, 
                                            const std::string& password) {
    std::vector<DisplayEntry> entries;
    
    try {
        // TODO: Implement archive content retrieval logic here
        // Use Flux::listArchiveContents() or similar function
        
        // Pseudo code:
        // auto flux_entries = Flux::listArchiveContents(archive_path, password);
        // for (const auto& flux_entry : flux_entries) {
        //     DisplayEntry entry;
        //     entry.name = flux_entry.name;
        //     entry.path = flux_entry.path.string();
        //     entry.is_directory = flux_entry.is_directory;
        //     entry.compressed_size = flux_entry.compressed_size;
        //     entry.uncompressed_size = flux_entry.uncompressed_size;
        //     entry.modification_time = flux_entry.modification_time;
        //     entry.permissions = flux_entry.permissions;
        //     entry.depth = std::count(entry.path.begin(), entry.path.end(), '/');
        //     entries.push_back(entry);
        // }
        
        // Temporary mock data
        DisplayEntry entry1;
        entry1.name = "example.txt";
        entry1.path = "example.txt";
        entry1.is_directory = false;
        entry1.compressed_size = 1024;
        entry1.uncompressed_size = 2048;
        entry1.modification_time = "2024-01-01 12:00:00";
        entry1.permissions = 0644;
        entry1.depth = 0;
        entries.push_back(entry1);
        
        DisplayEntry entry2;
        entry2.name = "folder";
        entry2.path = "folder/";
        entry2.is_directory = true;
        entry2.compressed_size = 0;
        entry2.uncompressed_size = 0;
        entry2.modification_time = "2024-01-01 12:00:00";
        entry2.permissions = 0755;
        entry2.depth = 0;
        entries.push_back(entry2);
        
    } catch (const std::exception& e) {
        throw Flux::CorruptedArchiveException("Cannot read archive contents: " + std::string(e.what()));
    }
    
    return entries;
}

std::vector<DisplayEntry> filterEntries(const std::vector<DisplayEntry>& entries, 
                                       const InspectConfig& config) {
    std::vector<DisplayEntry> filtered;
    
    std::regex filter_regex;
    bool use_filter = false;
    
    if (!config.filter_pattern.empty()) {
        try {
            filter_regex = std::regex(config.filter_pattern, std::regex_constants::icase);
            use_filter = true;
        } catch (const std::regex_error& e) {
            spdlog::warn("Invalid filter regex pattern: {}", config.filter_pattern);
        }
    }
    
    for (const auto& entry : entries) {
        // Check hidden files
        if (!config.show_hidden && entry.name.starts_with('.')) {
            continue;
        }
        
        // Check depth limit
        if (config.max_depth >= 0 && entry.depth > config.max_depth) {
            continue;
        }
        
        // Check filter pattern
        if (use_filter && !std::regex_search(entry.name, filter_regex)) {
            continue;
        }
        
        filtered.push_back(entry);
    }
    
    return filtered;
}

void outputList(const std::vector<DisplayEntry>& entries, const InspectConfig& config) {
    for (const auto& entry : entries) {
        std::cout << entry.path;
        
        if (config.show_size && !entry.is_directory) {
            std::cout << " (" << Utils::FormatUtils::formatFileSize(entry.uncompressed_size) << ")";
        }
        
        if (config.show_date) {
            std::cout << " " << entry.modification_time;
        }
        
        if (config.show_permissions) {
            std::cout << " " << std::oct << entry.permissions << std::dec;
        }
        
        std::cout << "\n";
    }
}

void outputTree(const std::vector<DisplayEntry>& entries, const InspectConfig& config) {
    // Build tree structure
    std::map<std::string, std::vector<DisplayEntry>> tree;
    
    for (const auto& entry : entries) {
        std::string parent_path;
        size_t last_slash = entry.path.find_last_of('/');
        if (last_slash != std::string::npos) {
            parent_path = entry.path.substr(0, last_slash);
        }
        tree[parent_path].push_back(entry);
    }
    
    // Recursively output tree structure
    std::function<void(const std::string&, int)> print_tree = [&](const std::string& path, int depth) {
        auto it = tree.find(path);
        if (it == tree.end()) return;
        
        for (size_t i = 0; i < it->second.size(); ++i) {
            const auto& entry = it->second[i];
            
            // Print indentation
            for (int j = 0; j < depth; ++j) {
                std::cout << "  ";
            }
            
            // Print tree connector
            if (i == it->second.size() - 1) {
                std::cout << "└── ";
            } else {
                std::cout << "├── ";
            }
            
            // Print filename
            std::cout << entry.name;
            
            if (entry.is_directory) {
                std::cout << "/";
            } else if (config.show_size) {
                std::cout << " (" << Utils::FormatUtils::formatFileSize(entry.uncompressed_size) << ")";
            }
            
            std::cout << "\n";
            
            // Recursively process subdirectories
            if (entry.is_directory) {
                print_tree(entry.path, depth + 1);
            }
        }
    };
    
    print_tree("", 0);
}

void outputJSON(const std::vector<DisplayEntry>& entries, const InspectConfig& config) {
    nlohmann::json json_output;
    json_output["archive"] = config.archive.string();
    json_output["format"] = "unknown"; // TODO: Get from configuration
    json_output["entries"] = nlohmann::json::array();
    
    for (const auto& entry : entries) {
        nlohmann::json json_entry;
        json_entry["name"] = entry.name;
        json_entry["path"] = entry.path;
        json_entry["is_directory"] = entry.is_directory;
        json_entry["compressed_size"] = entry.compressed_size;
        json_entry["uncompressed_size"] = entry.uncompressed_size;
        json_entry["modification_time"] = entry.modification_time;
        json_entry["permissions"] = entry.permissions;
        json_entry["depth"] = entry.depth;
        
        json_output["entries"].push_back(json_entry);
    }
    
    std::cout << json_output.dump(2) << std::endl;
}

void outputDetailed(const std::vector<DisplayEntry>& entries, const InspectConfig& config) {
    // Calculate column widths
    size_t max_name_width = 0;
    size_t max_size_width = 0;
    
    for (const auto& entry : entries) {
        max_name_width = std::max(max_name_width, entry.name.length());
        if (!entry.is_directory) {
            std::string size_str = Utils::FormatUtils::formatFileSize(entry.uncompressed_size);
            max_size_width = std::max(max_size_width, size_str.length());
        }
    }
    
    // Output table header
    std::cout << std::left 
              << std::setw(max_name_width + 2) << "Name"
              << std::setw(max_size_width + 2) << "Size"
              << std::setw(12) << "Compressed"
              << std::setw(20) << "Modified"
              << "Permissions\n";
    
    std::cout << std::string(max_name_width + max_size_width + 36, '-') << "\n";
    
    // Output entries
    for (const auto& entry : entries) {
        std::cout << std::left 
                  << std::setw(max_name_width + 2) << entry.name;
        
        if (entry.is_directory) {
            std::cout << std::setw(max_size_width + 2) << "<DIR>";
        } else {
            std::cout << std::setw(max_size_width + 2) 
                      << Utils::FormatUtils::formatFileSize(entry.uncompressed_size);
        }
        
        std::cout << std::setw(12) 
                  << Utils::FormatUtils::formatFileSize(entry.compressed_size)
                  << std::setw(20) << entry.modification_time
                  << std::oct << entry.permissions << std::dec << "\n";
    }
}

void showStatistics(const std::vector<DisplayEntry>& entries, const InspectConfig& config) {
    size_t total_files = 0;
    size_t total_dirs = 0;
    size_t total_compressed = 0;
    size_t total_uncompressed = 0;
    
    for (const auto& entry : entries) {
        if (entry.is_directory) {
            total_dirs++;
        } else {
            total_files++;
            total_compressed += entry.compressed_size;
            total_uncompressed += entry.uncompressed_size;
        }
    }
    
    std::cout << "\nStatistics:\n";
    std::cout << "  Files: " << total_files << "\n";
    std::cout << "  Directories: " << total_dirs << "\n";
    std::cout << "  Original size: " << Utils::FormatUtils::formatFileSize(total_uncompressed) << "\n";
    std::cout << "  Compressed size: " << Utils::FormatUtils::formatFileSize(total_compressed) << "\n";
    
    if (total_uncompressed > 0) {
        std::cout << "  Compression ratio: " 
                  << Utils::FormatUtils::formatCompressionRatio(total_uncompressed, total_compressed) 
                  << "\n";
    }
}

} // namespace FluxCLI::Commands


#include <iostream>
#include <iomanip>
#include <algorithm>
#include "flux-core/flux.h"

int handleListCommand(const std::string& archive,
                     const std::string& password,
                     bool detailed,
                     bool verbose) {
    
    try {
        if (verbose) {
            std::cout << "=== Flux List Command ===" << std::endl;
            std::cout << "Archive: " << archive << std::endl;
            std::cout << "Detailed: " << (detailed ? "yes" : "no") << std::endl;
            if (!password.empty()) {
                std::cout << "Password: [protected]" << std::endl;
            }
            std::cout << std::endl;
        }

        // Auto-detect format and create extractor
        auto extractor = Flux::createExtractorAuto(archive);
        if (!extractor) {
            std::cerr << "Error: Cannot create extractor for archive: " << archive << std::endl;
            return 1;
        }

        // Get archive contents list
        std::cout << "Listing contents of: " << archive << std::endl;
        auto entries = extractor->listContents(archive, password);

        if (entries.empty()) {
            std::cout << "Archive is empty." << std::endl;
            return 0;
        }

        // Calculate statistics
        size_t total_files = 0;
        size_t total_dirs = 0;
        size_t total_compressed = 0;
        size_t total_uncompressed = 0;

        for (const auto& entry : entries) {
            if (entry.is_directory) {
                total_dirs++;
            } else {
                total_files++;
            }
            total_compressed += entry.compressed_size;
            total_uncompressed += entry.uncompressed_size;
        }

        std::cout << std::endl;

        if (detailed) {
            // Detailed list format
            std::cout << std::left 
                      << std::setw(12) << "Size" 
                      << std::setw(12) << "Compressed"
                      << std::setw(20) << "Modified"
                      << std::setw(10) << "Type"
                      << "Name" << std::endl;
            std::cout << std::string(80, '-') << std::endl;

            for (const auto& entry : entries) {
                std::cout << std::left
                          << std::setw(12) << formatBytes(entry.uncompressed_size)
                          << std::setw(12) << formatBytes(entry.compressed_size)
                          << std::setw(20) << entry.modification_time
                          << std::setw(10) << (entry.is_directory ? "DIR" : "FILE")
                          << entry.path.string() << std::endl;
            }
        } else {
            // Simple list format
            for (const auto& entry : entries) {
                if (entry.is_directory) {
                    std::cout << entry.path.string() << "/" << std::endl;
                } else {
                    std::cout << entry.path.string() << std::endl;
                }
            }
        }

        // Display statistics
        std::cout << std::endl;
        std::cout << "Summary:" << std::endl;
        std::cout << "  Files: " << total_files << std::endl;
        std::cout << "  Directories: " << total_dirs << std::endl;
        std::cout << "  Total size: " << formatBytes(total_uncompressed) << std::endl;
        std::cout << "  Compressed size: " << formatBytes(total_compressed) << std::endl;
        
        if (total_uncompressed > 0) {
            double ratio = (double)total_compressed / total_uncompressed;
            std::cout << "  Compression ratio: " << std::fixed << std::setprecision(1) 
                      << (ratio * 100) << "%" << std::endl;
        }

        return 0;

    } catch (const Flux::InvalidPasswordException& e) {
        std::cerr << "Error: Invalid password or corrupted archive." << std::endl;
        return 1;
    } catch (const Flux::CorruptedArchiveException& e) {
        std::cerr << "Error: Archive is corrupted: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}



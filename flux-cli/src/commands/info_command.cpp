#include <iostream>
#include <iomanip>
#include "flux-core/flux.h"

int handleInfoCommand(const std::string& archive,
                     const std::string& password,
                     bool verbose) {
    
    try {
        if (verbose) {
            std::cout << "=== Flux Info Command ===" << std::endl;
            std::cout << "Archive: " << archive << std::endl;
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

        // Get archive information
        auto info = extractor->getArchiveInfo(archive, password);

        // Display basic information
        std::cout << "Archive Information" << std::endl;
        std::cout << std::string(50, '=') << std::endl;
        
        std::cout << std::left << std::setw(20) << "File:" << info.path.filename().string() << std::endl;
        std::cout << std::left << std::setw(20) << "Full Path:" << info.path.string() << std::endl;
        std::cout << std::left << std::setw(20) << "Format:" << Flux::formatToString(info.format) << std::endl;
        std::cout << std::left << std::setw(20) << "Compressed Size:" << formatBytes(info.compressed_size) << std::endl;
        std::cout << std::left << std::setw(20) << "Uncompressed Size:" << formatBytes(info.uncompressed_size) << std::endl;
        
        if (info.uncompressed_size > 0) {
            double ratio = (double)info.compressed_size / info.uncompressed_size;
            std::cout << std::left << std::setw(20) << "Compression Ratio:" 
                      << std::fixed << std::setprecision(1) << (ratio * 100) << "%" << std::endl;
        }
        
        std::cout << std::left << std::setw(20) << "File Count:" << info.file_count << std::endl;
        std::cout << std::left << std::setw(20) << "Encrypted:" << (info.is_encrypted ? "Yes" : "No") << std::endl;
        
        if (!info.creation_time.empty()) {
            std::cout << std::left << std::setw(20) << "Created:" << info.creation_time << std::endl;
        }

        // Get filesystem information
        std::error_code ec;
        auto file_status = std::filesystem::status(info.path, ec);
        if (!ec) {
            auto file_time = std::filesystem::last_write_time(info.path, ec);
            if (!ec) {
                // Convert file time to string (simplified version)
                std::cout << std::left << std::setw(20) << "Modified:" << "[file system time]" << std::endl;
            }
            
            // Display permission information (Unix-style)
            auto perms = file_status.permissions();
            std::cout << std::left << std::setw(20) << "Permissions:" << "[file permissions]" << std::endl;
        }

        // If in verbose mode, display more information
        if (verbose) {
            std::cout << std::endl;
            std::cout << "Technical Details" << std::endl;
            std::cout << std::string(30, '-') << std::endl;
            
            // Try to get format-specific information
            try {
                auto detected_format = extractor->detectFormat(archive);
                std::cout << "Detected Format: " << Flux::formatToString(detected_format) << std::endl;
            } catch (...) {
                std::cout << "Format Detection: Failed" << std::endl;
            }
            
            // Display supported operations
            std::cout << "Supported Operations:" << std::endl;
            std::cout << "  - Extract: Yes" << std::endl;
            std::cout << "  - List Contents: Yes" << std::endl;
            std::cout << "  - Verify Integrity: Yes" << std::endl;
            std::cout << "  - Partial Extract: " << (info.format != Flux::ArchiveFormat::SEVEN_ZIP ? "Yes" : "Limited") << std::endl;
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



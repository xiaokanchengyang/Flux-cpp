#include <iostream>
#include <chrono>
#include "flux-core/flux.h"

int handleVerifyCommand(const std::string& archive,
                       const std::string& password,
                       bool verbose) {
    
    try {
        if (verbose) {
            std::cout << "=== Flux Verify Command ===" << std::endl;
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

        std::cout << "Verifying archive integrity: " << archive << std::endl;
        
        auto start_time = std::chrono::high_resolution_clock::now();

        // Execute integrity verification
        auto [is_valid, error_message] = extractor->verifyIntegrity(archive, password);

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        if (is_valid) {
            std::cout << "✅ Archive integrity verified successfully!" << std::endl;
            
            if (verbose) {
                // Display additional verification information
                try {
                    auto info = extractor->getArchiveInfo(archive, password);
                    std::cout << std::endl;
                    std::cout << "Verification Details:" << std::endl;
                    std::cout << "  Format: " << Flux::formatToString(info.format) << std::endl;
                    std::cout << "  File count: " << info.file_count << std::endl;
                    std::cout << "  Total size: " << formatBytes(info.uncompressed_size) << std::endl;
                    std::cout << "  Encrypted: " << (info.is_encrypted ? "Yes" : "No") << std::endl;
                } catch (const std::exception& e) {
                    if (verbose) {
                        std::cout << "  Note: Could not retrieve detailed info: " << e.what() << std::endl;
                    }
                }
            }
            
            std::cout << "Verification time: " << duration.count() << "ms" << std::endl;
            return 0;
        } else {
            std::cerr << "❌ Archive integrity verification failed!" << std::endl;
            if (!error_message.empty()) {
                std::cerr << "Error: " << error_message << std::endl;
            }
            std::cerr << "Verification time: " << duration.count() << "ms" << std::endl;
            return 1;
        }

    } catch (const Flux::InvalidPasswordException& e) {
        std::cerr << "Error: Invalid password. Cannot verify encrypted archive without correct password." << std::endl;
        return 1;
    } catch (const Flux::CorruptedArchiveException& e) {
        std::cerr << "Error: Archive is corrupted: " << e.what() << std::endl;
        return 1;
    } catch (const Flux::FileNotFoundException& e) {
        std::cerr << "Error: Archive file not found: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}



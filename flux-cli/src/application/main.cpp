/**
 * Flux Archive Manager - Command Line Interface
 * 
 * Modern high-performance archive tool command line interface
 * Supports multiple formats: ZIP, TAR+ZSTD, TAR+GZ, TAR+XZ, 7-Zip
 * 
 * Features:
 * - Multi-threaded compression/decompression
 * - Smart progress display
 * - Detailed logging output
 * - Cross-platform compatibility
 * - POSIX-compatible command line interface
 */

#include "cli_app.h"
#include <flux-core/flux.h>
#include <iostream>
#include <exception>
#include <span>

int main(int argc, char** argv) noexcept {
    try {
        // Use span for modern C++ approach to command line arguments
        std::span<char*> args(argv, argc);
        
        FluxCLI::CLIApp app;
        return app.run(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error" << std::endl;
        return 1;
    }
}
#pragma once

#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>
#include <memory>
#include <string>
#include <functional>
#include <expected>
#include <string_view>
#include <span>

namespace FluxCLI {
    /**
     * Main CLI application class
     * Responsible for command line parsing, subcommand management and global configuration
     */
    class CLIApp {
    public:
        CLIApp();
        ~CLIApp() = default;
        
        // Delete copy and move constructors/operators for singleton-like behavior
        CLIApp(const CLIApp&) = delete;
        CLIApp& operator=(const CLIApp&) = delete;
        CLIApp(CLIApp&&) = delete;
        CLIApp& operator=(CLIApp&&) = delete;

        /**
         * Run CLI application
         * @param argc Command line argument count
         * @param argv Command line argument array
         * @return Exit code
         */
        [[nodiscard]] int run(int argc, char** argv);

        // Exit code definitions using scoped enum (public for external use)
        enum class ExitCode : int {
            Success = 0,
            GeneralError = 1,
            FileNotFound = 2,
            PermissionDenied = 3,
            CorruptedArchive = 4,
            UnsupportedFormat = 5,
            InvalidPassword = 6,
            OperationCancelled = 7
        };

    private:
        void setupGlobalOptions();
        void setupCommands();
        void setupLogging();
        void printVersion() const;
        void printHelp() const;
        
        // Global option handlers using functional programming
        void handleVerbose() noexcept;
        void handleQuiet() noexcept;

        std::unique_ptr<CLI::App> m_app;
        
        // Global flags with default initialization
        bool m_verbose{false};
        bool m_quiet{false};
        bool m_version{false};
        
        // Log level with default value
        spdlog::level::level_enum m_logLevel{spdlog::level::info};
    };

    /**
     * Convert Flux exception to appropriate exit code
     * @param e Exception object
     * @return Exit code
     */
    [[nodiscard]] CLIApp::ExitCode exceptionToExitCode(const std::exception& e) noexcept;
    
    /**
     * Format file size to human-readable format using functional approach
     * @param bytes Number of bytes
     * @return Formatted string (e.g. "1.5 MB")
     */
    [[nodiscard]] std::string formatFileSize(size_t bytes) noexcept;
    
    /**
     * Format duration to human-readable format using functional approach
     * @param milliseconds Number of milliseconds
     * @return Formatted string (e.g. "2.5s")
     */
    [[nodiscard]] std::string formatDuration(size_t milliseconds) noexcept;
}


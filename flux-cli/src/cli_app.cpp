#include "cli_app.h"
#include "version.h"
#include "commands/pack_command.h"
#include "commands/extract_command.h"
#include "commands/inspect_command.h"
#include "utils/format_utils.h"

#include <flux-core/flux.h>
#include <flux-core/exceptions.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <sstream>

#ifdef _WIN32
#include "platform/windows_console.h"
#endif

// Add missing function declarations
namespace Flux {
    std::map<ArchiveFormat, std::string> getSupportedFormatsInfo();
}

namespace FluxCLI {

CLIApp::CLIApp() {
    // Initialize Flux core library
    Flux::initialize();
    
    // Create main CLI application
    m_app = std::make_unique<CLI::App>("Flux Archive Manager - Modern High-Performance Archive Tool", "flux");
    m_app->set_version_flag("-V,--version", FLUX_CLI_VERSION_STRING);
    m_app->require_subcommand(1); // Require at least one subcommand
    
    setupGlobalOptions();
    setupCommands();
    
#ifdef _WIN32
    // Windows console UTF-8 support
    Platform::enableUTF8Console();
#endif
}

int CLIApp::run(int argc, char** argv) {
    try {
        m_app->parse(argc, argv);
        
        // Handle global options
        if (m_version) {
            printVersion();
            return static_cast<int>(ExitCode::Success);
        }
        
        setupLogging();
        
        // Subcommands are already executed during parsing
        return static_cast<int>(ExitCode::Success);
        
    } catch (const CLI::ParseError& e) {
        return m_app->exit(e);
    } catch (const Flux::FileNotFoundException& e) {
        spdlog::error("File not found: {}", e.what());
        return static_cast<int>(ExitCode::FileNotFound);
    } catch (const Flux::PermissionDeniedException& e) {
        spdlog::error("Permission denied: {}", e.what());
        return static_cast<int>(ExitCode::PermissionDenied);
    } catch (const Flux::CorruptedArchiveException& e) {
        spdlog::error("Archive corrupted: {}", e.what());
        return static_cast<int>(ExitCode::CorruptedArchive);
    } catch (const Flux::UnsupportedFormatException& e) {
        spdlog::error("Unsupported format: {}", e.what());
        return static_cast<int>(ExitCode::UnsupportedFormat);
    } catch (const Flux::InvalidPasswordException& e) {
        spdlog::error("Invalid password: {}", e.what());
        return static_cast<int>(ExitCode::InvalidPassword);
    } catch (const Flux::OperationCancelledException& e) {
        spdlog::info("Operation cancelled");
        return static_cast<int>(ExitCode::OperationCancelled);
    } catch (const Flux::FluxException& e) {
        spdlog::error("Flux error: {}", e.what());
        return static_cast<int>(ExitCode::GeneralError);
    } catch (const std::exception& e) {
        spdlog::error("Unknown error: {}", e.what());
        return static_cast<int>(ExitCode::GeneralError);
    }
}

void CLIApp::setupGlobalOptions() {
    // Verbose mode
    m_app->add_flag("-v,--verbose", m_verbose, "Enable verbose output mode");
    
    // Quiet mode
    m_app->add_flag("-q,--quiet", m_quiet, "Quiet mode, only output error messages");
    
    // Version information
    m_app->add_flag("--version", m_version, "Show version information");
    
    // Mutually exclusive options: verbose and quiet cannot be used together
    m_app->add_flag("-v,--verbose", m_verbose, "Enable verbose output mode")
         ->excludes(m_app->add_flag("-q,--quiet", m_quiet, "Quiet mode, only output error messages"));
}

void CLIApp::setupCommands() {
    // pack command
    auto pack_cmd = m_app->add_subcommand("pack", "Pack files and folders into archive");
    Commands::setupPackCommand(pack_cmd, m_verbose, m_quiet);
    
    // extract command
    auto extract_cmd = m_app->add_subcommand("extract", "Extract files from archive");
    Commands::setupExtractCommand(extract_cmd, m_verbose, m_quiet);
    
    // inspect command (alias ls)
    auto inspect_cmd = m_app->add_subcommand("inspect", "View archive contents");
    m_app->add_subcommand("ls", "View archive contents (alias for inspect)")
         ->callback([inspect_cmd]() { inspect_cmd->parse(""); });
    Commands::setupInspectCommand(inspect_cmd, m_verbose, m_quiet);
}

void CLIApp::setupLogging() {
    // Set log level based on global flags
    if (m_quiet) {
        m_logLevel = spdlog::level::err;
    } else if (m_verbose) {
        m_logLevel = spdlog::level::debug;
    } else {
        m_logLevel = spdlog::level::info;
    }
    
    // Create colored console logger
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(m_logLevel);
    
    // Set log format
    if (m_verbose) {
        console_sink->set_pattern("[%H:%M:%S] [%^%l%$] %v");
    } else {
        console_sink->set_pattern("%v");
    }
    
    auto logger = std::make_shared<spdlog::logger>("flux", console_sink);
    logger->set_level(m_logLevel);
    spdlog::set_default_logger(logger);
}

void CLIApp::printVersion() const {
    std::cout << "Flux Archive Manager CLI v" << FLUX_CLI_VERSION_STRING << "\n";
    std::cout << "Based on Flux-Core v" << Flux::getVersion() << "\n";
    std::cout << "\n";
    std::cout << "Supported formats:\n";
    
    auto formats = Flux::getSupportedFormatsInfo();
    for (const auto& [format, description] : formats) {
        std::cout << "  • " << Flux::formatToString(format) << " - " << description << "\n";
    }
    
    std::cout << "\n";
    std::cout << "Copyright (c) 2024 Flux Archive Manager Project\n";
    std::cout << "MIT License - https://github.com/flux-archive/flux\n";
}

// Utility function implementations
CLIApp::ExitCode exceptionToExitCode(const std::exception& e) noexcept {
    if (dynamic_cast<const Flux::FileNotFoundException*>(&e)) {
        return CLIApp::ExitCode::FileNotFound;
    } else if (dynamic_cast<const Flux::PermissionDeniedException*>(&e)) {
        return CLIApp::ExitCode::PermissionDenied;
    } else if (dynamic_cast<const Flux::CorruptedArchiveException*>(&e)) {
        return CLIApp::ExitCode::CorruptedArchive;
    } else if (dynamic_cast<const Flux::UnsupportedFormatException*>(&e)) {
        return CLIApp::ExitCode::UnsupportedFormat;
    } else if (dynamic_cast<const Flux::InvalidPasswordException*>(&e)) {
        return CLIApp::ExitCode::InvalidPassword;
    } else if (dynamic_cast<const Flux::OperationCancelledException*>(&e)) {
        return CLIApp::ExitCode::OperationCancelled;
    } else {
        return CLIApp::ExitCode::GeneralError;
    }
}

std::string formatFileSize(size_t bytes) noexcept {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
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

std::string formatDuration(size_t milliseconds) noexcept {
    if (milliseconds < 1000) {
        return std::to_string(milliseconds) + "ms";
    } else if (milliseconds < 60000) {
        double seconds = milliseconds / 1000.0;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << seconds << "s";
        return oss.str();
    } else {
        size_t minutes = milliseconds / 60000;
        size_t seconds = (milliseconds % 60000) / 1000;
        return std::to_string(minutes) + "m" + std::to_string(seconds) + "s";
    }
}

} // namespace FluxCLI

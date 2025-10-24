#include "progress_bar.h"
#include "format_utils.h"
#include <indicators/color.hpp>
#include <spdlog/spdlog.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <cmath>

#ifdef _WIN32
#include "../platform/windows_console.h"
#endif

namespace FluxCLI::Utils {

ProgressBarManager::ProgressBarManager(bool quiet_mode)
    : m_quietMode(quiet_mode)
    , m_started(false)
    , m_cancelled(false)
    , m_lastProcessedBytes(0)
    , m_totalSize(0) {
}

ProgressBarManager::~ProgressBarManager() {
    if (m_started && m_progressBar) {
        finish(true, "");
    }
}

Flux::ProgressCallback ProgressBarManager::createProgressCallback() {
    return [this](std::string_view current_file, float percentage,
                  size_t processed_bytes, size_t total_bytes) {
        updateProgress(std::string(current_file), percentage, processed_bytes, total_bytes);
    };
}

void ProgressBarManager::start(const std::string& task_name, size_t total_size) {
    if (m_quietMode) {
        return;
    }
    
    m_taskName = task_name;
    m_totalSize = total_size;
    m_started = true;
    m_cancelled = false;
    m_startTime = std::chrono::steady_clock::now();
    m_lastUpdateTime = m_startTime;
    m_lastProcessedBytes = 0;
    
    setupProgressBar();
    
    spdlog::info("Starting {}", task_name);
}

void ProgressBarManager::setupProgressBar() {
    using namespace indicators;
    
    // Get console width
    int console_width = 80;
#ifdef _WIN32
    console_width = Platform::getConsoleWidth();
#endif
    
    // Calculate progress bar width (leave space for other info)
    int bar_width = std::max(20, console_width - 60);
    
    m_progressBar = std::make_unique<ProgressBar>(
        option::BarWidth{bar_width},
        option::Start{"["},
        option::Fill{"█"},
        option::Lead{"█"},
        option::Remainder{"-"},
        option::End{"]"},
        option::PrefixText{""},
        option::PostfixText{""},
        option::ForegroundColor{Color::cyan},
        option::ShowElapsedTime{true},
        option::ShowRemainingTime{true},
        option::FontStyles{std::vector<FontStyle>{FontStyle::bold}}
    );
}

void ProgressBarManager::updateProgress(const std::string& current_file, 
                                       float percentage, 
                                       size_t processed_bytes, 
                                       size_t total_bytes) {
    if (m_quietMode || !m_started || !m_progressBar) {
        return;
    }
    
    // Update progress bar
    size_t progress = static_cast<size_t>(percentage * 100);
    m_progressBar->set_progress(progress);
    
    // Calculate speed and ETA
    auto now = std::chrono::steady_clock::now();
    auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastUpdateTime);
    
    std::string status_text;
    
    if (time_diff.count() > 100) { // Update statistics every 100ms
        size_t bytes_diff = processed_bytes - m_lastProcessedBytes;
        size_t bytes_per_second = 0;
        
        if (time_diff.count() > 0) {
            bytes_per_second = (bytes_diff * 1000) / time_diff.count();
        }
        
        // Build status text
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << percentage * 100 << "% | ";
        
        if (total_bytes > 0) {
            oss << FormatUtils::formatFileSize(processed_bytes) << "/" << FormatUtils::formatFileSize(total_bytes) << " | ";
        }
        
        if (bytes_per_second > 0) {
            oss << formatSpeed(bytes_per_second);
            
            if (total_bytes > processed_bytes) {
                size_t remaining_bytes = total_bytes - processed_bytes;
                oss << " | ETA: " << formatETA(remaining_bytes, bytes_per_second);
            }
        }
        
        status_text = oss.str();
        
        m_lastProcessedBytes = processed_bytes;
        m_lastUpdateTime = now;
    }
    
    // Display current file (truncate long paths)
    std::string display_file = current_file;
    if (display_file.length() > 40) {
        display_file = "..." + display_file.substr(display_file.length() - 37);
    }
    
    if (!status_text.empty()) {
        m_progressBar->set_option(indicators::option::PostfixText{status_text});
    }
    
    if (!display_file.empty()) {
        m_progressBar->set_option(indicators::option::PrefixText{display_file});
    }
}

void ProgressBarManager::finish(bool success, const std::string& message) {
    if (m_quietMode || !m_started) {
        return;
    }
    
    if (m_progressBar) {
        m_progressBar->set_progress(100);
        m_progressBar->mark_as_completed();
    }
    
    // Calculate total elapsed time
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - m_startTime);
    
    if (success) {
        spdlog::info("✅ {} completed (elapsed: {})", m_taskName, FormatUtils::formatDuration(duration.count()));
        if (!message.empty()) {
            spdlog::info("{}", message);
        }
    } else {
        spdlog::error("❌ {} failed (elapsed: {})", m_taskName, FormatUtils::formatDuration(duration.count()));
    if (!message.empty()) {
            spdlog::error("{}", message);
        }
    }
    
    m_started = false;
}

void ProgressBarManager::setQuietMode(bool quiet) {
    m_quietMode = quiet;
}

bool ProgressBarManager::shouldCancel() const {
    return m_cancelled.load();
}

std::string ProgressBarManager::formatSpeed(size_t bytes_per_second) const {
    if (bytes_per_second < 1024) {
        return std::to_string(bytes_per_second) + " B/s";
    } else if (bytes_per_second < 1024 * 1024) {
        return std::to_string(bytes_per_second / 1024) + " KB/s";
    } else if (bytes_per_second < 1024 * 1024 * 1024) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << (bytes_per_second / (1024.0 * 1024.0)) << " MB/s";
        return oss.str();
    } else {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << (bytes_per_second / (1024.0 * 1024.0 * 1024.0)) << " GB/s";
        return oss.str();
    }
}

std::string ProgressBarManager::formatETA(size_t remaining_bytes, size_t bytes_per_second) const {
    if (bytes_per_second == 0) {
        return "∞";
    }
    
    size_t eta_seconds = remaining_bytes / bytes_per_second;
    
    if (eta_seconds < 60) {
        return std::to_string(eta_seconds) + "s";
    } else if (eta_seconds < 3600) {
        size_t minutes = eta_seconds / 60;
        size_t seconds = eta_seconds % 60;
        return std::to_string(minutes) + "m" + std::to_string(seconds) + "s";
    } else {
        size_t hours = eta_seconds / 3600;
        size_t minutes = (eta_seconds % 3600) / 60;
        return std::to_string(hours) + "h" + std::to_string(minutes) + "m";
    }
}

// Simple progress bar implementation (fallback solution)
SimpleProgressBar::SimpleProgressBar(int width) 
    : m_width(width), m_lastPercentage(-1.0f) {
}

void SimpleProgressBar::update(float percentage, const std::string& message) {
    if (std::abs(percentage - m_lastPercentage) < 0.01f) {
        return; // Avoid frequent updates
    }
    
    int filled = static_cast<int>(percentage * m_width);
    
    std::cout << "\r[";
    for (int i = 0; i < m_width; ++i) {
        if (i < filled) {
            std::cout << "█";
        } else {
            std::cout << "-";
        }
    }
    std::cout << "] " << std::fixed << std::setprecision(1) << percentage * 100 << "%";
    
    if (!message.empty()) {
        std::cout << " " << message;
    }
    
    std::cout << std::flush;
    m_lastPercentage = percentage;
}

void SimpleProgressBar::finish(const std::string& message) {
    update(1.0f, message);
    std::cout << std::endl;
}

// DetailedProgressReporter implementation
DetailedProgressReporter::DetailedProgressReporter(bool quiet_mode)
    : m_quietMode(quiet_mode), m_started(false), m_totalSize(0), m_processedBytes(0),
      m_filesProcessed(0), m_totalFiles(0), m_originalSize(0), m_compressedSize(0),
      m_currentFileSize(0) {
}

DetailedProgressReporter::~DetailedProgressReporter() {
    if (m_started) {
        finish(false, "Operation interrupted");
    }
}

void DetailedProgressReporter::start(const std::string& operation, const std::string& source,
                                   const std::string& destination, size_t total_size) {
    if (m_quietMode) return;
    
    m_operation = operation;
    m_source = source;
    m_destination = destination;
    m_totalSize = total_size;
    m_startTime = std::chrono::steady_clock::now();
    m_started = true;
    
    std::cout << "\n";
    spdlog::info("Starting {} operation", operation);
    spdlog::info("Source: {}", source);
    spdlog::info("Destination: {}", destination);
    
    if (total_size > 0) {
        spdlog::info("Total size: {}", FormatUtils::formatFileSize(total_size));
    }
    
    // Create main progress bar
    m_mainProgress = std::make_unique<indicators::ProgressBar>(
        indicators::option::BarWidth{50},
        indicators::option::Start{"["},
        indicators::option::Fill{"█"},
        indicators::option::Lead{"█"},
        indicators::option::Remainder{" "},
        indicators::option::End{"]"},
        indicators::option::PrefixText{"Overall Progress "},
        indicators::option::ForegroundColor{indicators::Color::green},
        indicators::option::ShowElapsedTime{true},
        indicators::option::ShowRemainingTime{true}
    );
    
    // Create file progress bar
    m_fileProgress = std::make_unique<indicators::ProgressBar>(
        indicators::option::BarWidth{50},
        indicators::option::Start{"["},
        indicators::option::Fill{"█"},
        indicators::option::Lead{"█"},
        indicators::option::Remainder{" "},
        indicators::option::End{"]"},
        indicators::option::PrefixText{"Current File    "},
        indicators::option::ForegroundColor{indicators::Color::blue},
        indicators::option::ShowElapsedTime{false},
        indicators::option::ShowRemainingTime{false}
    );
}

void DetailedProgressReporter::updateFile(const std::string& current_file, size_t file_size,
                                        size_t files_processed, size_t total_files) {
    if (m_quietMode || !m_started) return;
    
    m_currentFile = current_file;
    m_currentFileSize = file_size;
    m_filesProcessed = files_processed;
    m_totalFiles = total_files;
    
    // Update file progress bar
    std::string file_display = current_file;
    if (file_display.length() > 30) {
        file_display = "..." + file_display.substr(file_display.length() - 27);
    }
    
    m_fileProgress->set_option(indicators::option::PostfixText{
        fmt::format(" {} ({}/{})", file_display, files_processed, total_files)
    });
    
    m_fileProgress->set_progress(0);
}

void DetailedProgressReporter::updateOverall(float percentage, size_t processed_bytes, size_t total_bytes) {
    if (m_quietMode || !m_started) return;
    
    m_processedBytes = processed_bytes;
    
    // Update main progress bar
    m_mainProgress->set_progress(static_cast<size_t>(percentage));
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_startTime);
    
    if (processed_bytes > 0 && elapsed.count() > 0) {
        double speed = static_cast<double>(processed_bytes) / elapsed.count();
        m_mainProgress->set_option(indicators::option::PostfixText{
            fmt::format(" {}/{} @ {}/s", 
                       FormatUtils::formatFileSize(processed_bytes),
                       FormatUtils::formatFileSize(total_bytes),
                       FormatUtils::formatFileSize(static_cast<size_t>(speed)))
        });
    }
}

void DetailedProgressReporter::reportCompression(size_t original_size, size_t compressed_size) {
    m_originalSize += original_size;
    m_compressedSize += compressed_size;
}

void DetailedProgressReporter::finish(bool success, const std::string& message) {
    if (!m_started) return;
    
    m_started = false;
    
    if (!m_quietMode) {
        // Complete progress bars
        if (m_mainProgress) {
            m_mainProgress->set_progress(100);
        }
        if (m_fileProgress) {
            m_fileProgress->set_progress(100);
        }
        
        std::cout << "\n";
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - m_startTime);
        
        if (success) {
            spdlog::info("✓ {} completed successfully", m_operation);
        } else {
            spdlog::error("✗ {} failed", m_operation);
            if (!message.empty()) {
                spdlog::error("Error: {}", message);
            }
        }
        
        printDetailedStats();
        printCompressionStats();
    }
}

Flux::ProgressCallback DetailedProgressReporter::createProgressCallback() {
    return [this](float percentage, size_t processed, size_t total, const std::string& current_item) {
        if (!m_quietMode && m_started) {
            // Update file progress if we have current item info
            if (!current_item.empty() && current_item != m_currentFile) {
                updateFile(current_item, 0, m_filesProcessed + 1, m_totalFiles);
            }
            
            // Update file progress bar
            if (m_fileProgress) {
                m_fileProgress->set_progress(static_cast<size_t>(percentage));
            }
            
            // Update overall progress
            updateOverall(percentage, processed, total);
        }
    };
}

void DetailedProgressReporter::printDetailedStats() {
    if (m_quietMode) return;
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - m_startTime);
    
    spdlog::info("Operation Statistics:");
    spdlog::info("  Duration: {}", FormatUtils::formatDuration(duration.count()));
    spdlog::info("  Files processed: {}", m_filesProcessed);
    
    if (m_processedBytes > 0) {
        spdlog::info("  Data processed: {}", FormatUtils::formatFileSize(m_processedBytes));
        
        if (duration.count() > 0) {
            double throughput = static_cast<double>(m_processedBytes) / (duration.count() / 1000.0);
            spdlog::info("  Average throughput: {}/s", FormatUtils::formatFileSize(static_cast<size_t>(throughput)));
        }
    }
}

void DetailedProgressReporter::printCompressionStats() {
    if (m_quietMode || m_originalSize == 0) return;
    
    if (m_compressedSize > 0) {
        double ratio = static_cast<double>(m_compressedSize) / m_originalSize;
        double savings = (1.0 - ratio) * 100.0;
        
        spdlog::info("Compression Statistics:");
        spdlog::info("  Original size: {}", FormatUtils::formatFileSize(m_originalSize));
        spdlog::info("  Compressed size: {}", FormatUtils::formatFileSize(m_compressedSize));
        spdlog::info("  Compression ratio: {:.1f}%", ratio * 100.0);
        spdlog::info("  Space saved: {:.1f}%", savings);
    }
}

} // namespace FluxCLI::Utils
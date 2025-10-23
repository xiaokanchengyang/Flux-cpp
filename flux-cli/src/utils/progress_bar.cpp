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

} // namespace FluxCLI::Utils
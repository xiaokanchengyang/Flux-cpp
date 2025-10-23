#pragma once

#include <cstdint>
#include <indicators/progress_bar.hpp>
#include <indicators/cursor_control.hpp>
#include <flux-core/packer.h>
#include <string>
#include <memory>
#include <atomic>

namespace FluxCLI::Utils {
    /**
     * Modern progress bar manager
     * Provides rich progress information display and user-friendly interface
     */
    class ProgressBarManager {
    public:
        ProgressBarManager(bool quiet_mode = false);
        ~ProgressBarManager();
        
        /**
         * Create Flux core library compatible progress callback function
         * @return Progress callback function
         */
        Flux::ProgressCallback createProgressCallback();
        
        /**
         * Start displaying progress bar
         * @param task_name Task name
         * @param total_size Total size (bytes)
         */
        void start(const std::string& task_name, size_t total_size = 0);
        
        /**
         * Complete progress bar display
         * @param success Whether successful
         * @param message Completion message
         */
        void finish(bool success, const std::string& message = "");
        
        /**
         * Set quiet mode
         * @param quiet Whether to enable quiet mode
         */
        void setQuietMode(bool quiet);
        
        /**
         * Check if operation should be cancelled
         * @return Whether to cancel
         */
        bool shouldCancel() const;
        
    private:
        void updateProgress(const std::string& current_file, float percentage, 
                          size_t processed_bytes, size_t total_bytes);
        
        void setupProgressBar();
        std::string formatSpeed(size_t bytes_per_second) const;
        std::string formatETA(size_t remaining_bytes, size_t bytes_per_second) const;
        
        std::unique_ptr<indicators::ProgressBar> m_progressBar;
        bool m_quietMode;
        bool m_started;
        std::atomic<bool> m_cancelled;
        
        // Performance statistics
        std::chrono::steady_clock::time_point m_startTime;
        size_t m_lastProcessedBytes;
        std::chrono::steady_clock::time_point m_lastUpdateTime;
        
        // Display information
        std::string m_taskName;
        size_t m_totalSize;
    };
    
    /**
     * Simple console progress bar (for cases where indicators is not supported)
     */
    class SimpleProgressBar {
    public:
        SimpleProgressBar(int width = 50);
        
        void update(float percentage, const std::string& message = "");
        void finish(const std::string& message = "");
        
    private:
        int m_width;
        float m_lastPercentage;
    };
}
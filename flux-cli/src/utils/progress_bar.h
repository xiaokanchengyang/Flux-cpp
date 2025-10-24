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
        std::chrono::steady_clock::time_point m_lastUpdateTime;
        size_t m_lastProcessedBytes;
        size_t m_totalSize;
        std::string m_taskName;
    };
    
    /**
     * Enhanced progress reporter with detailed statistics
     */
    class DetailedProgressReporter {
    public:
        DetailedProgressReporter(bool quiet_mode = false);
        ~DetailedProgressReporter();
        
        /**
         * Start progress reporting with detailed information
         */
        void start(const std::string& operation, const std::string& source, 
                  const std::string& destination, size_t total_size = 0);
        
        /**
         * Update progress with file-level details
         */
        void updateFile(const std::string& current_file, size_t file_size, 
                       size_t files_processed, size_t total_files);
        
        /**
         * Update overall progress
         */
        void updateOverall(float percentage, size_t processed_bytes, size_t total_bytes);
        
        /**
         * Report compression statistics
         */
        void reportCompression(size_t original_size, size_t compressed_size);
        
        /**
         * Finish with detailed summary
         */
        void finish(bool success, const std::string& message = "");
        
        /**
         * Create Flux-compatible progress callback
         */
        Flux::ProgressCallback createProgressCallback();
        
    private:
        void printDetailedStats();
        void printCompressionStats();
        
        bool m_quietMode;
        bool m_started;
        std::string m_operation;
        std::string m_source;
        std::string m_destination;
        
        // Statistics
        std::chrono::steady_clock::time_point m_startTime;
        size_t m_totalSize;
        size_t m_processedBytes;
        size_t m_filesProcessed;
        size_t m_totalFiles;
        size_t m_originalSize;
        size_t m_compressedSize;
        
        // Current file info
        std::string m_currentFile;
        size_t m_currentFileSize;
        
        std::unique_ptr<indicators::ProgressBar> m_mainProgress;
        std::unique_ptr<indicators::ProgressBar> m_fileProgress;
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
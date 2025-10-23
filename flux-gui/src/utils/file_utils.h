#pragma once

#include <QString>
#include <QStringList>
#include <QFileInfo>

/**
 * File utility class
 * 
 * Provides utility functions for file operations
 */
class FileUtils {
public:
    /**
     * Format file size
     * @param bytes Number of bytes
     * @return Formatted size string (e.g., "1.5 MB")
     */
    static QString formatFileSize(qint64 bytes);
    
    /**
     * Get file extension
     * @param filePath File path
     * @return Extension (without dot)
     */
    static QString getFileExtension(const QString& filePath);
    
    /**
     * Check if file is a supported archive format
     * @param filePath File path
     * @return Whether it's a supported format
     */
    static bool isSupportedArchive(const QString& filePath);
    
    /**
     * Get list of supported archive formats
     * @return Format list
     */
    static QStringList getSupportedFormats();
    
    /**
     * Get file type icon name
     * @param filePath File path
     * @return Icon name
     */
    static QString getFileIcon(const QString& filePath);
    
    /**
     * Ensure directory exists
     * @param dirPath Directory path
     * @return Whether successful
     */
    static bool ensureDirectoryExists(const QString& dirPath);
    
    /**
     * Get temporary directory path
     * @return Temporary directory path
     */
    static QString getTempDirectory();
    
    /**
     * Clean up temporary files
     * @param tempDir Temporary directory
     */
    static void cleanupTempFiles(const QString& tempDir);
};



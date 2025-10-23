#include "file_utils.h"
#include "resource_manager.h"
#include <QDir>
#include <QStandardPaths>
#include <QFileInfo>
#include <cmath>

QString FileUtils::formatFileSize(qint64 bytes) {
    if (bytes < 0) {
        return "Unknown";
    }
    
    const QStringList units = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unitIndex < units.size() - 1) {
        size /= 1024.0;
        unitIndex++;
    }
    
    if (unitIndex == 0) {
        return QString("%1 %2").arg(static_cast<int>(size)).arg(units[unitIndex]);
    } else {
        return QString("%1 %2").arg(size, 0, 'f', 1).arg(units[unitIndex]);
    }
}

QString FileUtils::getFileExtension(const QString& filePath) {
    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();
    
    // Handle compound extensions
    if (suffix == "gz" || suffix == "xz" || suffix == "bz2" || suffix == "zst") {
        QString completeSuffix = fileInfo.completeSuffix().toLower();
        if (completeSuffix.startsWith("tar.")) {
            return completeSuffix;
        }
    }
    
    return suffix;
}

bool FileUtils::isSupportedArchive(const QString& filePath) {
    QString ext = getFileExtension(filePath);
    QStringList supportedFormats = getSupportedFormats();
    
    return supportedFormats.contains(ext, Qt::CaseInsensitive);
}

QStringList FileUtils::getSupportedFormats() {
    return {
        "zip",
        "7z",
        "tar",
        "tar.gz",
        "tar.xz",
        "tar.bz2",
        "tar.zst",
        "gz",
        "xz",
        "bz2",
        "zst"
    };
}

QString FileUtils::getFileIcon(const QString& filePath) {
    QFileInfo fileInfo(filePath);
    
    if (fileInfo.isDir()) {
        return "folder";
    }
    
    QString ext = getFileExtension(filePath);
    
    // Archive file icon
    if (isSupportedArchive(filePath)) {
        return "archive";
    }
    
    // Image files
    QStringList imageExts = {"jpg", "jpeg", "png", "gif", "bmp", "svg", "webp"};
    if (imageExts.contains(ext, Qt::CaseInsensitive)) {
        return "image";
    }
    
    // Document files
    QStringList docExts = {"txt", "doc", "docx", "pdf", "rtf", "odt"};
    if (docExts.contains(ext, Qt::CaseInsensitive)) {
        return "document";
    }
    
    // Code files
    QStringList codeExts = {"cpp", "h", "c", "py", "js", "html", "css", "java", "cs"};
    if (codeExts.contains(ext, Qt::CaseInsensitive)) {
        return "code";
    }
    
    // Audio files
    QStringList audioExts = {"mp3", "wav", "flac", "ogg", "m4a", "aac"};
    if (audioExts.contains(ext, Qt::CaseInsensitive)) {
        return "audio";
    }
    
    // Video files
    QStringList videoExts = {"mp4", "avi", "mkv", "mov", "wmv", "flv", "webm"};
    if (videoExts.contains(ext, Qt::CaseInsensitive)) {
        return "video";
    }
    
    return "file";
}

bool FileUtils::ensureDirectoryExists(const QString& dirPath) {
    QDir dir;
    return dir.mkpath(dirPath);
}

QString FileUtils::getTempDirectory() {
    // Use resource manager for consistent temp directory handling
    return RESOURCE_MANAGER.getTempDir();
}

void FileUtils::cleanupTempFiles(const QString& tempDir) {
    QDir dir(tempDir);
    if (dir.exists()) {
        dir.removeRecursively();
    }
}



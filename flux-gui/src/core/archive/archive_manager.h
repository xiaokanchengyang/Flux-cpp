#pragma once

#include <QObject>
#include <QStringList>
#include <QVariant>
#include <QHash>

class QMimeDatabase;

namespace FluxGUI::Core::Archive {

/**
 * @brief Archive format information
 */
struct FormatInfo {
    QString name;                    // Format name (e.g., "zip", "7z")
    QString mimeType;               // MIME type
    QStringList extensions;         // File extensions
    bool canCreate = false;         // Can create archives
    bool canExtract = false;        // Can extract archives
    bool extractOnly = false;       // Extract-only format
    int defaultLevel = 6;           // Default compression level
    QList<int> supportedLevels;     // Supported compression levels
};

/**
 * @brief Compression preset information
 */
struct CompressionPreset {
    QString id;                     // Preset identifier
    QString name;                   // Display name
    QString description;            // Description
    QString format;                 // Target format
    int level = 6;                  // Compression level
    QString method;                 // Compression method
    int dictionarySize = 32;        // Dictionary size (MB)
    bool solidArchive = false;      // Solid archive mode
    bool multithread = true;        // Multi-threading support
};

/**
 * @brief Archive creation options
 */
struct ArchiveCreationOptions {
    QString outputPath;             // Output archive path
    QStringList inputFiles;         // Input files/directories
    QString format = "zip";         // Archive format
    int compressionLevel = 6;       // Compression level
    QString method;                 // Compression method
    QString password;               // Archive password
    bool solidArchive = false;      // Solid archive mode
    bool deleteAfterCompression = false; // Delete source files
    QVariantMap extraOptions;       // Format-specific options
};

/**
 * @brief Archive extraction options
 */
struct ArchiveExtractionOptions {
    QString archivePath;            // Source archive path
    QString outputDirectory;        // Output directory
    QStringList selectedFiles;      // Files to extract (empty = all)
    QString password;               // Archive password
    bool preservePaths = true;      // Preserve directory structure
    bool overwriteExisting = false; // Overwrite existing files
    bool createSubfolder = true;    // Create subfolder for extraction
};

/**
 * @brief Archive listing options
 */
struct ArchiveListOptions {
    QString archivePath;            // Archive path
    QString password;               // Archive password
    bool includeHidden = false;     // Include hidden files
};

/**
 * @brief Archive testing options
 */
struct ArchiveTestOptions {
    QString archivePath;            // Archive path
    QString password;               // Archive password
};

/**
 * @brief Archive operation class
 */
class ArchiveOperation : public QObject {
    Q_OBJECT

public:
    enum Type {
        Create,                     // Create archive
        Extract,                    // Extract archive
        List,                       // List archive contents
        Test,                       // Test archive integrity
        Add,                        // Add files to archive
        Delete                      // Delete files from archive
    };

    explicit ArchiveOperation(QObject* parent = nullptr) : QObject(parent) {}
    
    void setOperationType(Type type) { m_type = type; }
    Type operationType() const { return m_type; }
    
    void setOptions(const QVariant& options) { m_options = options; }
    QVariant options() const { return m_options; }
    
    void cancel() { m_cancelled = true; emit cancelled(); }
    bool isCancelled() const { return m_cancelled; }

signals:
    void started();
    void progress(int value, int maximum);
    void finished();
    void error(const QString& message);
    void cancelled();

private:
    Type m_type = Create;
    QVariant m_options;
    bool m_cancelled = false;
};

/**
 * @brief Centralized archive management system
 * 
 * The ArchiveManager provides comprehensive archive handling with:
 * - Multi-format support (ZIP, 7Z, RAR, TAR, etc.)
 * - Asynchronous operations with progress reporting
 * - Compression presets for different use cases
 * - Format detection and validation
 * - Error handling and recovery
 */
class ArchiveManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Get the singleton instance
     * @return Reference to the archive manager instance
     */
    static ArchiveManager& instance();
    
    /**
     * @brief Destructor
     */
    ~ArchiveManager() override;

public slots:
    /**
     * @brief Initialize the archive system
     */
    void initialize();

public:
    /**
     * @brief Get list of supported formats
     * @return List of format names
     */
    QStringList supportedFormats() const;
    
    /**
     * @brief Get list of supported file extensions
     * @return List of extensions (with dots)
     */
    QStringList supportedExtensions() const;
    
    /**
     * @brief Check if format is supported
     * @param format Format name
     * @return True if supported
     */
    bool isFormatSupported(const QString& format) const;
    
    /**
     * @brief Check if file extension is supported
     * @param extension File extension (with or without dot)
     * @return True if supported
     */
    bool isExtensionSupported(const QString& extension) const;
    
    /**
     * @brief Detect archive format from file path
     * @param filePath File path
     * @return Format name or empty string if unknown
     */
    QString detectFormat(const QString& filePath) const;
    
    /**
     * @brief Get format information
     * @param format Format name
     * @return Format information
     */
    FormatInfo getFormatInfo(const QString& format) const;
    
    /**
     * @brief Get list of compression presets
     * @return List of preset names
     */
    QStringList getCompressionPresets() const;
    
    /**
     * @brief Get compression preset information
     * @param presetName Preset name
     * @return Preset information
     */
    CompressionPreset getCompressionPreset(const QString& presetName) const;

    // Archive operations
    /**
     * @brief Create a new archive
     * @param options Creation options
     * @return Operation object (caller takes ownership)
     */
    ArchiveOperation* createArchive(const ArchiveCreationOptions& options);
    
    /**
     * @brief Extract an archive
     * @param options Extraction options
     * @return Operation object (caller takes ownership)
     */
    ArchiveOperation* extractArchive(const ArchiveExtractionOptions& options);
    
    /**
     * @brief List archive contents
     * @param archivePath Archive file path
     * @return Operation object (caller takes ownership)
     */
    ArchiveOperation* listArchive(const QString& archivePath);
    
    /**
     * @brief Test archive integrity
     * @param archivePath Archive file path
     * @return Operation object (caller takes ownership)
     */
    ArchiveOperation* testArchive(const QString& archivePath);
    
    /**
     * @brief Cancel an operation
     * @param operation Operation to cancel
     */
    void cancelOperation(ArchiveOperation* operation);
    
    /**
     * @brief Cancel all active operations
     */
    void cancelAllOperations();
    
    /**
     * @brief Get list of active operations
     * @return List of active operations
     */
    QList<ArchiveOperation*> activeOperations() const;

signals:
    /**
     * @brief Emitted when archive manager is initialized
     */
    void initialized();
    
    /**
     * @brief Emitted when an operation completes successfully
     * @param operation Completed operation
     */
    void operationCompleted(ArchiveOperation* operation);
    
    /**
     * @brief Emitted when an operation fails
     * @param operation Failed operation
     * @param error Error message
     */
    void operationFailed(ArchiveOperation* operation, const QString& error);

private slots:
    /**
     * @brief Handle operation completion
     */
    void operationFinished();
    
    /**
     * @brief Handle operation error
     * @param error Error message
     */
    void operationError(const QString& error);

private:
    /**
     * @brief Private constructor for singleton
     * @param parent Parent object
     */
    explicit ArchiveManager(QObject* parent = nullptr);
    
    /**
     * @brief Load MIME type configuration
     */
    void loadMimeTypeConfig();
    
    /**
     * @brief Load compression presets
     */
    void loadCompressionPresets();
    
    /**
     * @brief Initialize supported formats
     */
    void initializeSupportedFormats();
    
    /**
     * @brief Extract format name from MIME type
     * @param mimeType MIME type string
     * @return Format name
     */
    QString extractFormatName(const QString& mimeType) const;

private:
    QMimeDatabase* m_mimeDatabase;
    QHash<QString, FormatInfo> m_supportedFormats;
    QHash<QString, CompressionPreset> m_compressionPresets;
    QList<ArchiveOperation*> m_activeOperations;
    bool m_initialized;
};

} // namespace FluxGUI::Core::Archive

Q_DECLARE_METATYPE(FluxGUI::Core::Archive::ArchiveCreationOptions)
Q_DECLARE_METATYPE(FluxGUI::Core::Archive::ArchiveExtractionOptions)
Q_DECLARE_METATYPE(FluxGUI::Core::Archive::ArchiveListOptions)
Q_DECLARE_METATYPE(FluxGUI::Core::Archive::ArchiveTestOptions)

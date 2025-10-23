#include "archive_manager.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QThread>
#include <QTimer>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

namespace FluxGUI::Core::Archive {

ArchiveManager& ArchiveManager::instance() {
    static ArchiveManager instance;
    return instance;
}

ArchiveManager::ArchiveManager(QObject* parent)
    : QObject(parent)
    , m_mimeDatabase(new QMimeDatabase(this))
    , m_initialized(false)
{
}

ArchiveManager::~ArchiveManager() = default;

void ArchiveManager::initialize() {
    qDebug() << "Initializing Archive Manager...";
    
    // Load MIME type configurations
    loadMimeTypeConfig();
    
    // Load compression presets
    loadCompressionPresets();
    
    // Initialize supported formats
    initializeSupportedFormats();
    
    m_initialized = true;
    
    qDebug() << "Archive Manager initialized";
    emit initialized();
}

QStringList ArchiveManager::supportedFormats() const {
    return m_supportedFormats.keys();
}

QStringList ArchiveManager::supportedExtensions() const {
    QStringList extensions;
    for (const auto& format : m_supportedFormats) {
        extensions.append(format.extensions);
    }
    extensions.removeDuplicates();
    return extensions;
}

bool ArchiveManager::isFormatSupported(const QString& format) const {
    return m_supportedFormats.contains(format.toLower());
}

bool ArchiveManager::isExtensionSupported(const QString& extension) const {
    QString ext = extension.toLower();
    if (!ext.startsWith('.')) {
        ext = '.' + ext;
    }
    
    for (const auto& format : m_supportedFormats) {
        if (format.extensions.contains(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

QString ArchiveManager::detectFormat(const QString& filePath) const {
    QFileInfo fileInfo(filePath);
    QString extension = '.' + fileInfo.completeSuffix().toLower();
    
    // Check for compound extensions first (e.g., .tar.gz)
    for (const auto& format : m_supportedFormats) {
        for (const QString& ext : format.extensions) {
            if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
                return format.name;
            }
        }
    }
    
    // Fallback to MIME type detection
    QMimeType mimeType = m_mimeDatabase->mimeTypeForFile(filePath);
    QString mimeName = mimeType.name();
    
    for (const auto& format : m_supportedFormats) {
        if (format.mimeType == mimeName) {
            return format.name;
        }
    }
    
    return QString();
}

FormatInfo ArchiveManager::getFormatInfo(const QString& format) const {
    return m_supportedFormats.value(format.toLower(), FormatInfo());
}

QStringList ArchiveManager::getCompressionPresets() const {
    return m_compressionPresets.keys();
}

CompressionPreset ArchiveManager::getCompressionPreset(const QString& presetName) const {
    return m_compressionPresets.value(presetName, CompressionPreset());
}

ArchiveOperation* ArchiveManager::createArchive(const ArchiveCreationOptions& options) {
    if (!m_initialized) {
        qWarning() << "Archive Manager not initialized";
        return nullptr;
    }
    
    // Validate options
    if (options.outputPath.isEmpty() || options.inputFiles.isEmpty()) {
        qWarning() << "Invalid archive creation options";
        return nullptr;
    }
    
    if (!isFormatSupported(options.format)) {
        qWarning() << "Unsupported format:" << options.format;
        return nullptr;
    }
    
    // Create operation
    ArchiveOperation* operation = new ArchiveOperation(this);
    operation->setOperationType(ArchiveOperation::Create);
    operation->setOptions(QVariant::fromValue(options));
    
    // Connect signals
    connect(operation, &ArchiveOperation::finished, this, &ArchiveManager::operationFinished);
    connect(operation, &ArchiveOperation::error, this, &ArchiveManager::operationError);
    
    // Add to active operations
    m_activeOperations.append(operation);
    
    qDebug() << "Created archive operation for:" << options.outputPath;
    return operation;
}

ArchiveOperation* ArchiveManager::extractArchive(const ArchiveExtractionOptions& options) {
    if (!m_initialized) {
        qWarning() << "Archive Manager not initialized";
        return nullptr;
    }
    
    // Validate options
    if (options.archivePath.isEmpty() || options.outputDirectory.isEmpty()) {
        qWarning() << "Invalid archive extraction options";
        return nullptr;
    }
    
    if (!QFileInfo::exists(options.archivePath)) {
        qWarning() << "Archive file does not exist:" << options.archivePath;
        return nullptr;
    }
    
    // Detect format
    QString format = detectFormat(options.archivePath);
    if (format.isEmpty()) {
        qWarning() << "Cannot detect format for:" << options.archivePath;
        return nullptr;
    }
    
    // Create operation
    ArchiveOperation* operation = new ArchiveOperation(this);
    operation->setOperationType(ArchiveOperation::Extract);
    operation->setOptions(QVariant::fromValue(options));
    
    // Connect signals
    connect(operation, &ArchiveOperation::finished, this, &ArchiveManager::operationFinished);
    connect(operation, &ArchiveOperation::error, this, &ArchiveManager::operationError);
    
    // Add to active operations
    m_activeOperations.append(operation);
    
    qDebug() << "Created extraction operation for:" << options.archivePath;
    return operation;
}

ArchiveOperation* ArchiveManager::listArchive(const QString& archivePath) {
    if (!m_initialized) {
        qWarning() << "Archive Manager not initialized";
        return nullptr;
    }
    
    if (!QFileInfo::exists(archivePath)) {
        qWarning() << "Archive file does not exist:" << archivePath;
        return nullptr;
    }
    
    // Create operation
    ArchiveOperation* operation = new ArchiveOperation(this);
    operation->setOperationType(ArchiveOperation::List);
    
    ArchiveListOptions options;
    options.archivePath = archivePath;
    operation->setOptions(QVariant::fromValue(options));
    
    // Connect signals
    connect(operation, &ArchiveOperation::finished, this, &ArchiveManager::operationFinished);
    connect(operation, &ArchiveOperation::error, this, &ArchiveManager::operationError);
    
    // Add to active operations
    m_activeOperations.append(operation);
    
    qDebug() << "Created list operation for:" << archivePath;
    return operation;
}

ArchiveOperation* ArchiveManager::testArchive(const QString& archivePath) {
    if (!m_initialized) {
        qWarning() << "Archive Manager not initialized";
        return nullptr;
    }
    
    if (!QFileInfo::exists(archivePath)) {
        qWarning() << "Archive file does not exist:" << archivePath;
        return nullptr;
    }
    
    // Create operation
    ArchiveOperation* operation = new ArchiveOperation(this);
    operation->setOperationType(ArchiveOperation::Test);
    
    ArchiveTestOptions options;
    options.archivePath = archivePath;
    operation->setOptions(QVariant::fromValue(options));
    
    // Connect signals
    connect(operation, &ArchiveOperation::finished, this, &ArchiveManager::operationFinished);
    connect(operation, &ArchiveOperation::error, this, &ArchiveManager::operationError);
    
    // Add to active operations
    m_activeOperations.append(operation);
    
    qDebug() << "Created test operation for:" << archivePath;
    return operation;
}

void ArchiveManager::cancelOperation(ArchiveOperation* operation) {
    if (!operation || !m_activeOperations.contains(operation)) {
        return;
    }
    
    operation->cancel();
    qDebug() << "Cancelled archive operation";
}

void ArchiveManager::cancelAllOperations() {
    for (ArchiveOperation* operation : m_activeOperations) {
        operation->cancel();
    }
    qDebug() << "Cancelled all archive operations";
}

QList<ArchiveOperation*> ArchiveManager::activeOperations() const {
    return m_activeOperations;
}

void ArchiveManager::loadMimeTypeConfig() {
    QFile file(":/config/mime-types.json");
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot load MIME type configuration";
        return;
    }
    
    QByteArray data = file.readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Error parsing MIME type config:" << error.errorString();
        return;
    }
    
    QJsonObject root = doc.object();
    QJsonObject mimeTypes = root["mimeTypes"].toObject();
    
    for (auto it = mimeTypes.begin(); it != mimeTypes.end(); ++it) {
        QString mimeType = it.key();
        QJsonObject info = it.value().toObject();
        
        FormatInfo format;
        format.mimeType = mimeType;
        format.name = info["description"].toString();
        format.canCreate = info["canCreate"].toBool();
        format.canExtract = info["canExtract"].toBool();
        format.extractOnly = info["extractOnly"].toBool(false);
        format.defaultLevel = info["defaultLevel"].toInt(6);
        
        QJsonArray extensions = info["extensions"].toArray();
        for (const QJsonValue& ext : extensions) {
            format.extensions.append(ext.toString());
        }
        
        QJsonArray levels = info["supportedLevels"].toArray();
        for (const QJsonValue& level : levels) {
            format.supportedLevels.append(level.toInt());
        }
        
        // Extract format name from MIME type or use a mapping
        QString formatName = extractFormatName(mimeType);
        m_supportedFormats[formatName] = format;
    }
    
    qDebug() << "Loaded" << m_supportedFormats.size() << "format configurations";
}

void ArchiveManager::loadCompressionPresets() {
    QFile file(":/config/compression-presets.json");
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot load compression presets";
        return;
    }
    
    QByteArray data = file.readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Error parsing compression presets:" << error.errorString();
        return;
    }
    
    QJsonObject root = doc.object();
    QJsonArray presets = root["presets"].toArray();
    
    for (const QJsonValue& presetValue : presets) {
        QJsonObject presetObj = presetValue.toObject();
        
        CompressionPreset preset;
        preset.id = presetObj["id"].toString();
        preset.name = presetObj["name"].toString();
        preset.description = presetObj["description"].toString();
        preset.format = presetObj["format"].toString();
        preset.level = presetObj["level"].toInt();
        preset.method = presetObj["method"].toString();
        preset.dictionarySize = presetObj["dictionarySize"].toInt();
        preset.solidArchive = presetObj["solidArchive"].toBool();
        preset.multithread = presetObj["multithread"].toBool();
        
        m_compressionPresets[preset.id] = preset;
    }
    
    qDebug() << "Loaded" << m_compressionPresets.size() << "compression presets";
}

void ArchiveManager::initializeSupportedFormats() {
    // This method can be used to add runtime format detection
    // or integrate with external libraries
    
    qDebug() << "Initialized" << m_supportedFormats.size() << "supported formats";
}

QString ArchiveManager::extractFormatName(const QString& mimeType) const {
    // Map MIME types to format names
    static QHash<QString, QString> mimeToFormat = {
        {"application/zip", "zip"},
        {"application/x-7z-compressed", "7z"},
        {"application/vnd.rar", "rar"},
        {"application/x-tar", "tar"},
        {"application/gzip", "gz"},
        {"application/x-bzip2", "bz2"},
        {"application/x-xz", "xz"},
        {"application/x-lzma", "lzma"},
        {"application/vnd.ms-cab-compressed", "cab"},
        {"application/x-iso9660-image", "iso"}
    };
    
    return mimeToFormat.value(mimeType, "unknown");
}

void ArchiveManager::operationFinished() {
    ArchiveOperation* operation = qobject_cast<ArchiveOperation*>(sender());
    if (!operation) {
        return;
    }
    
    m_activeOperations.removeAll(operation);
    
    qDebug() << "Archive operation finished successfully";
    emit operationCompleted(operation);
    
    // Clean up operation after a delay to allow signal processing
    QTimer::singleShot(1000, operation, &QObject::deleteLater);
}

void ArchiveManager::operationError(const QString& error) {
    ArchiveOperation* operation = qobject_cast<ArchiveOperation*>(sender());
    if (!operation) {
        return;
    }
    
    m_activeOperations.removeAll(operation);
    
    qWarning() << "Archive operation failed:" << error;
    emit operationFailed(operation, error);
    
    // Clean up operation after a delay to allow signal processing
    QTimer::singleShot(1000, operation, &QObject::deleteLater);
}

} // namespace FluxGUI::Core::Archive

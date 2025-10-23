#pragma once
#include <stdexcept>
#include <string>

namespace Flux {
    /**
     * Flux base exception class
     */
    class FluxException : public std::runtime_error {
    public:
        explicit FluxException(const std::string& message)
            : std::runtime_error(message) {}
    };

    /**
     * Unsupported format exception
     */
    class UnsupportedFormatException : public FluxException {
    public:
        explicit UnsupportedFormatException(const std::string& format)
            : FluxException("Unsupported archive format: " + format) {}
    };

    /**
     * File not found exception
     */
    class FileNotFoundException : public FluxException {
    public:
        explicit FileNotFoundException(const std::string& path)
            : FluxException("File not found: " + path) {}
    };

    /**
     * Permission denied exception
     */
    class PermissionDeniedException : public FluxException {
    public:
        explicit PermissionDeniedException(const std::string& path)
            : FluxException("Permission denied: " + path) {}
    };

    /**
     * Compression/decompression exception
     */
    class CompressionException : public FluxException {
    public:
        explicit CompressionException(const std::string& message)
            : FluxException("Compression error: " + message) {}
    };

    /**
     * Invalid password exception
     */
    class InvalidPasswordException : public FluxException {
    public:
        InvalidPasswordException()
            : FluxException("Invalid password or archive is corrupted") {}
    };

    /**
     * Corrupted archive exception
     */
    class CorruptedArchiveException : public FluxException {
    public:
        explicit CorruptedArchiveException(const std::string& path)
            : FluxException("Corrupted archive: " + path) {}
    };

    /**
     * Operation cancelled exception
     */
    class OperationCancelledException : public FluxException {
    public:
        OperationCancelledException()
            : FluxException("Operation was cancelled by user") {}
    };
}


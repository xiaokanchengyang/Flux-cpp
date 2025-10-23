#include "flux-core/packer.h"
#include "flux-core/exceptions.h"
#include <spdlog/spdlog.h>
#include <fmt/format.h>

namespace Flux {
    namespace Formats {
        /**
         * TAR format packer implementation (Stub)
         * TODO: Implement with compression libraries (zlib, liblzma, libzstd)
         */
        class TarPackerImpl : public Packer {
        private:
            bool m_cancelled = false;

        public:
            PackResult pack(
                std::span<const std::filesystem::path> inputs,
                const std::filesystem::path& output,
                const PackOptions& options,
                const ProgressCallback& on_progress = nullptr,
                const ErrorCallback& on_error = nullptr) override {
                
                PackResult result;
                result.success = false;
                result.error_message = "TAR packing requires compression libraries (zlib, liblzma, libzstd) - not yet implemented";
                spdlog::error("TAR packing not implemented yet");
                return result;
            }

            void cancel() override {
                m_cancelled = true;
            }

            bool supportsFormat(ArchiveFormat format) const override {
                return format == ArchiveFormat::TAR_GZ || 
                       format == ArchiveFormat::TAR_XZ || 
                       format == ArchiveFormat::TAR_ZSTD;
            }
        };

        // Factory function to create TAR packer
        std::unique_ptr<Packer> createTarPacker() {
            return std::make_unique<TarPackerImpl>();
        }
    }
}

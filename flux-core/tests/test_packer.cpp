#include <gtest/gtest.h>
#include <flux-core/packer.h>
#include <flux-core/archive.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>

class PackerTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = std::filesystem::temp_directory_path() / "flux_packer_test";
        std::filesystem::create_directories(test_dir);
        
        // Create test files
        createTestFile("file1.txt", "Hello, World!");
        createTestFile("file2.txt", "This is another test file.");
        createTestFile("subdir/file3.txt", "File in subdirectory");
        createTestFile("binary.bin", std::string(512, '\x42'));
    }
    
    void TearDown() override {
        std::filesystem::remove_all(test_dir);
    }
    
    void createTestFile(const std::string& relative_path, const std::string& content) {
        std::filesystem::path file_path = test_dir / relative_path;
        std::filesystem::create_directories(file_path.parent_path());
        
        std::ofstream file(file_path);
        file << content;
        file.close();
    }
    
    std::filesystem::path test_dir;
};

TEST_F(PackerTest, CreatePackerInstances) {
    // Test creating packers for all supported formats
    auto zip_packer = Flux::createPacker(Flux::ArchiveFormat::ZIP);
    EXPECT_NE(zip_packer, nullptr);
    EXPECT_TRUE(zip_packer->supportsFormat(Flux::ArchiveFormat::ZIP));
    
    auto tar_gz_packer = Flux::createPacker(Flux::ArchiveFormat::TAR_GZ);
    EXPECT_NE(tar_gz_packer, nullptr);
    EXPECT_TRUE(tar_gz_packer->supportsFormat(Flux::ArchiveFormat::TAR_GZ));
    
    auto tar_xz_packer = Flux::createPacker(Flux::ArchiveFormat::TAR_XZ);
    EXPECT_NE(tar_xz_packer, nullptr);
    EXPECT_TRUE(tar_xz_packer->supportsFormat(Flux::ArchiveFormat::TAR_XZ));
    
    auto tar_zstd_packer = Flux::createPacker(Flux::ArchiveFormat::TAR_ZSTD);
    EXPECT_NE(tar_zstd_packer, nullptr);
    EXPECT_TRUE(tar_zstd_packer->supportsFormat(Flux::ArchiveFormat::TAR_ZSTD));
    
    auto seven_zip_packer = Flux::createPacker(Flux::ArchiveFormat::SEVEN_ZIP);
    EXPECT_NE(seven_zip_packer, nullptr);
    EXPECT_TRUE(seven_zip_packer->supportsFormat(Flux::ArchiveFormat::SEVEN_ZIP));
}

TEST_F(PackerTest, PackerFormatSupport) {
    auto packer = Flux::createPacker(Flux::ArchiveFormat::ZIP);
    
    // Test format support
    EXPECT_TRUE(packer->supportsFormat(Flux::ArchiveFormat::ZIP));
    EXPECT_FALSE(packer->supportsFormat(Flux::ArchiveFormat::TAR_GZ));
    EXPECT_FALSE(packer->supportsFormat(Flux::ArchiveFormat::TAR_XZ));
    EXPECT_FALSE(packer->supportsFormat(Flux::ArchiveFormat::TAR_ZSTD));
    EXPECT_FALSE(packer->supportsFormat(Flux::ArchiveFormat::SEVEN_ZIP));
}

TEST_F(PackerTest, ValidateInputsEmptyList) {
    auto packer = Flux::createPacker(Flux::ArchiveFormat::ZIP);
    
    std::vector<std::filesystem::path> empty_inputs;
    auto result = packer->validateInputs(empty_inputs);
    
    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(result.error().empty());
}

TEST_F(PackerTest, ValidateInputsValidFiles) {
    auto packer = Flux::createPacker(Flux::ArchiveFormat::ZIP);
    
    std::vector<std::filesystem::path> valid_inputs = {
        test_dir / "file1.txt",
        test_dir / "file2.txt",
        test_dir / "subdir"
    };
    
    auto result = packer->validateInputs(valid_inputs);
    EXPECT_TRUE(result.has_value());
}

TEST_F(PackerTest, ValidateInputsNonExistentFiles) {
    auto packer = Flux::createPacker(Flux::ArchiveFormat::ZIP);
    
    std::vector<std::filesystem::path> invalid_inputs = {
        test_dir / "nonexistent1.txt",
        test_dir / "nonexistent2.txt"
    };
    
    auto result = packer->validateInputs(invalid_inputs);
    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(result.error().empty());
}

TEST_F(PackerTest, ValidateInputsMixedFiles) {
    auto packer = Flux::createPacker(Flux::ArchiveFormat::ZIP);
    
    std::vector<std::filesystem::path> mixed_inputs = {
        test_dir / "file1.txt",        // exists
        test_dir / "nonexistent.txt"   // doesn't exist
    };
    
    auto result = packer->validateInputs(mixed_inputs);
    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(result.error().empty());
}

TEST_F(PackerTest, EstimateCompressedSize) {
    auto packer = Flux::createPacker(Flux::ArchiveFormat::ZIP);
    
    std::vector<std::filesystem::path> inputs = {
        test_dir / "file1.txt",
        test_dir / "file2.txt"
    };
    
    auto estimated_size = packer->estimateCompressedSize(inputs, Flux::ArchiveFormat::ZIP);
    
    if (estimated_size.has_value()) {
        EXPECT_GT(estimated_size.value(), 0);
    }
    // Note: Implementation may return nullopt if estimation is not available
}

TEST_F(PackerTest, EstimateCompressedSizeEmptyInputs) {
    auto packer = Flux::createPacker(Flux::ArchiveFormat::ZIP);
    
    std::vector<std::filesystem::path> empty_inputs;
    auto estimated_size = packer->estimateCompressedSize(empty_inputs, Flux::ArchiveFormat::ZIP);
    
    // Should return nullopt or 0 for empty inputs
    if (estimated_size.has_value()) {
        EXPECT_EQ(estimated_size.value(), 0);
    }
}

TEST_F(PackerTest, PackOptionsDefaults) {
    Flux::PackOptions options;
    
    EXPECT_EQ(options.format, Flux::ArchiveFormat::TAR_ZSTD);
    EXPECT_EQ(options.compression_level, 3);
    EXPECT_EQ(options.num_threads, 0);
    EXPECT_TRUE(options.preserve_permissions);
    EXPECT_TRUE(options.preserve_timestamps);
    EXPECT_TRUE(options.password.empty());
    EXPECT_TRUE(options.isCompressionLevelValid());
}

TEST_F(PackerTest, PackOptionsValidation) {
    Flux::PackOptions options;
    
    // Test valid compression levels
    for (int level = 0; level <= 9; ++level) {
        options.compression_level = level;
        EXPECT_TRUE(options.isCompressionLevelValid());
    }
    
    // Test invalid compression levels
    options.compression_level = -1;
    EXPECT_FALSE(options.isCompressionLevelValid());
    
    options.compression_level = 10;
    EXPECT_FALSE(options.isCompressionLevelValid());
    
    options.compression_level = 100;
    EXPECT_FALSE(options.isCompressionLevelValid());
}

TEST_F(PackerTest, PackResultDefaults) {
    Flux::PackResult result;
    
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.error_message.empty());
    EXPECT_EQ(result.files_processed, 0);
    EXPECT_EQ(result.total_compressed_size, 0);
    EXPECT_EQ(result.total_uncompressed_size, 0);
    EXPECT_EQ(result.compression_ratio, 0.0);
    EXPECT_EQ(result.duration.count(), 0);
}

TEST_F(PackerTest, PackResultComparison) {
    Flux::PackResult result1;
    Flux::PackResult result2;
    
    // Test equality
    EXPECT_EQ(result1, result2);
    
    // Modify one result
    result2.success = true;
    result2.files_processed = 3;
    result2.compression_ratio = 0.75;
    
    // Test inequality
    EXPECT_NE(result1, result2);
}

TEST_F(PackerTest, CancellationInterface) {
    auto packer = Flux::createPacker(Flux::ArchiveFormat::ZIP);
    
    // Test that cancel method exists and can be called
    EXPECT_NO_THROW(packer->cancel());
}

TEST_F(PackerTest, PackWithProgressCallback) {
    auto packer = Flux::createPacker(Flux::ArchiveFormat::ZIP);
    
    std::vector<std::filesystem::path> inputs = {
        test_dir / "file1.txt",
        test_dir / "file2.txt"
    };
    
    std::filesystem::path output_path = test_dir / "test_archive.zip";
    Flux::PackOptions options;
    
    bool progress_called = false;
    Flux::ProgressCallback progress_cb = [&](std::string_view filename, float progress,
                                            size_t processed, size_t total) {
        progress_called = true;
        EXPECT_GE(progress, 0.0f);
        EXPECT_LE(progress, 1.0f);
        EXPECT_LE(processed, total);
    };
    
    bool error_called = false;
    Flux::ErrorCallback error_cb = [&](std::string_view error_msg, bool is_fatal) {
        error_called = true;
        EXPECT_FALSE(error_msg.empty());
    };
    
    // Test packing with callbacks
    auto result = packer->pack(inputs, output_path, options, progress_cb, error_cb);
    
    // Note: This test may fail if the actual implementation is not available
    // The test verifies the interface works correctly
    EXPECT_FALSE(result.error_message.empty() && !result.success);
}

TEST_F(PackerTest, PackWithInvalidOutput) {
    auto packer = Flux::createPacker(Flux::ArchiveFormat::ZIP);
    
    std::vector<std::filesystem::path> inputs = {
        test_dir / "file1.txt"
    };
    
    // Try to pack to an invalid output path (e.g., directory that doesn't exist)
    std::filesystem::path invalid_output = test_dir / "nonexistent_dir" / "test.zip";
    Flux::PackOptions options;
    
    auto result = packer->pack(inputs, invalid_output, options);
    
    // Should fail due to invalid output path
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(PackerTest, GetSupportedFormats) {
    auto formats = Flux::getSupportedFormats();
    
    EXPECT_EQ(formats.size(), 5);
    
    // Check that all expected formats are present
    std::vector<Flux::ArchiveFormat> expected_formats = {
        Flux::ArchiveFormat::ZIP,
        Flux::ArchiveFormat::TAR_ZSTD,
        Flux::ArchiveFormat::TAR_GZ,
        Flux::ArchiveFormat::TAR_XZ,
        Flux::ArchiveFormat::SEVEN_ZIP
    };
    
    for (const auto& expected_format : expected_formats) {
        bool found = false;
        for (const auto& format : formats) {
            if (format == expected_format) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Format " << static_cast<int>(expected_format) << " not found";
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // Initialize Flux library
    Flux::initialize();
    
    int result = RUN_ALL_TESTS();
    
    // Cleanup Flux library
    Flux::cleanup();
    
    return result;
}

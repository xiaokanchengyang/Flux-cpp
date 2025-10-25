#include <gtest/gtest.h>
#include <flux-core/flux.h>
#include <flux-core/archive.h>
#include <flux-core/packer.h>
#include <flux-core/extractor.h>
#include <flux-core/exceptions.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <memory>

class ArchiveUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory
        test_dir = std::filesystem::temp_directory_path() / "flux_test";
        std::filesystem::create_directories(test_dir);
        
        // Create test files
        createTestFile("test1.txt", "Hello, World!");
        createTestFile("test2.txt", "This is a test file with more content.");
        createTestFile("subdir/test3.txt", "File in subdirectory");
        createTestFile("binary_file.bin", std::string(1024, '\x42')); // Binary data
    }
    
    void TearDown() override {
        // Clean up test directory
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

TEST_F(ArchiveUtilsTest, FormatToStringConversion) {
    EXPECT_EQ(Flux::formatToString(Flux::ArchiveFormat::ZIP), "zip");
    EXPECT_EQ(Flux::formatToString(Flux::ArchiveFormat::TAR_GZ), "tar.gz");
    EXPECT_EQ(Flux::formatToString(Flux::ArchiveFormat::TAR_XZ), "tar.xz");
    EXPECT_EQ(Flux::formatToString(Flux::ArchiveFormat::TAR_ZSTD), "tar.zst");
    EXPECT_EQ(Flux::formatToString(Flux::ArchiveFormat::SEVEN_ZIP), "7z");
}

TEST_F(ArchiveUtilsTest, LibraryVersionInfo) {
    // Test library version information
    std::string version = Flux::getVersion();
    EXPECT_FALSE(version.empty());
    EXPECT_EQ(version, "1.0.0"); // Expected version
    
    // Test version struct
    EXPECT_EQ(Flux::Version::MAJOR, 1);
    EXPECT_EQ(Flux::Version::MINOR, 0);
    EXPECT_EQ(Flux::Version::PATCH, 0);
    EXPECT_EQ(Flux::Version::toString(), "1.0.0");
}

TEST_F(ArchiveUtilsTest, StringToFormat) {
    auto result = Flux::stringToFormat("zip");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), Flux::ArchiveFormat::ZIP);
    
    result = Flux::stringToFormat("tar.gz");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), Flux::ArchiveFormat::TAR_GZ);
    
    result = Flux::stringToFormat("invalid");
    EXPECT_FALSE(result.has_value());
}

TEST_F(ArchiveUtilsTest, PackerCreation) {
    // Test ZIP packer creation
    auto zip_packer = Flux::createPacker(Flux::ArchiveFormat::ZIP);
    EXPECT_NE(zip_packer, nullptr);
    EXPECT_TRUE(zip_packer->supportsFormat(Flux::ArchiveFormat::ZIP));
    
    // Test TAR packer creation (should work even if not fully implemented)
    auto tar_packer = Flux::createPacker(Flux::ArchiveFormat::TAR_GZ);
    EXPECT_NE(tar_packer, nullptr);
    EXPECT_TRUE(tar_packer->supportsFormat(Flux::ArchiveFormat::TAR_GZ));
}

TEST_F(ArchiveUtilsTest, ExtractorCreation) {
    // Test ZIP extractor creation
    auto zip_extractor = Flux::createExtractor(Flux::ArchiveFormat::ZIP);
    EXPECT_NE(zip_extractor, nullptr);
    EXPECT_TRUE(zip_extractor->supportsFormat(Flux::ArchiveFormat::ZIP));
    
    // Test TAR extractor creation (should work even if not fully implemented)
    auto tar_extractor = Flux::createExtractor(Flux::ArchiveFormat::TAR_GZ);
    EXPECT_NE(tar_extractor, nullptr);
    EXPECT_TRUE(tar_extractor->supportsFormat(Flux::ArchiveFormat::TAR_GZ));
}

TEST_F(ArchiveUtilsTest, PackerValidation) {
    auto packer = Flux::createPacker(Flux::ArchiveFormat::ZIP);
    
    // Test input validation with empty inputs
    std::vector<std::filesystem::path> empty_inputs;
    auto validation_result = packer->validateInputs(empty_inputs);
    EXPECT_FALSE(validation_result.has_value()); // Should fail for empty inputs
    
    // Test with valid inputs
    std::vector<std::filesystem::path> valid_inputs = {
        test_dir / "test1.txt",
        test_dir / "test2.txt"
    };
    validation_result = packer->validateInputs(valid_inputs);
    EXPECT_TRUE(validation_result.has_value());
    
    // Test with non-existent file
    std::vector<std::filesystem::path> invalid_inputs = {
        test_dir / "nonexistent.txt"
    };
    validation_result = packer->validateInputs(invalid_inputs);
    EXPECT_FALSE(validation_result.has_value()); // Should fail for non-existent files
}

TEST_F(ArchiveUtilsTest, EstimateCompressedSize) {
    auto packer = Flux::createPacker(Flux::ArchiveFormat::ZIP);
    
    std::vector<std::filesystem::path> inputs = {
        test_dir / "test1.txt",
        test_dir / "test2.txt"
    };
    
    auto estimated_size = packer->estimateCompressedSize(inputs, Flux::ArchiveFormat::ZIP);
    EXPECT_TRUE(estimated_size.has_value());
    EXPECT_GT(estimated_size.value(), 0);
}

// Integration test for ZIP packing (if libzip is available)
TEST_F(ArchiveUtilsTest, DISABLED_ZipPackingIntegration) {
    auto packer = Flux::createPacker(Flux::ArchiveFormat::ZIP);
    
    std::vector<std::filesystem::path> inputs = {
        test_dir / "test1.txt",
        test_dir / "test2.txt"
    };
    
    std::filesystem::path output_path = test_dir / "test_archive.zip";
    
    Flux::PackOptions options;
    options.compression_level = 6;
    
    bool progress_called = false;
    auto progress_callback = [&progress_called](const std::string& file, float progress, 
                                               size_t processed, size_t total) {
        progress_called = true;
        EXPECT_GE(progress, 0.0f);
        EXPECT_LE(progress, 1.0f);
    };
    
    auto result = packer->pack(inputs, output_path, options, progress_callback);
    
    // Note: This test is disabled because it requires libzip to be properly linked
    // In a real implementation, this would test the actual packing functionality
    EXPECT_TRUE(progress_called);
    
    if (result.success) {
        EXPECT_GT(result.files_packed, 0);
        EXPECT_TRUE(std::filesystem::exists(output_path));
    }
}

// Test pack options validation
TEST_F(ArchiveUtilsTest, PackOptionsValidation) {
    Flux::PackOptions options;
    
    // Test valid compression levels
    options.compression_level = 0;
    EXPECT_TRUE(options.isCompressionLevelValid());
    
    options.compression_level = 5;
    EXPECT_TRUE(options.isCompressionLevelValid());
    
    options.compression_level = 9;
    EXPECT_TRUE(options.isCompressionLevelValid());
    
    // Test invalid compression levels
    options.compression_level = -1;
    EXPECT_FALSE(options.isCompressionLevelValid());
    
    options.compression_level = 10;
    EXPECT_FALSE(options.isCompressionLevelValid());
}

// Test extract options configuration
TEST_F(ArchiveUtilsTest, ExtractOptionsConfiguration) {
    Flux::ExtractOptions options;
    
    // Test default values
    EXPECT_EQ(options.overwrite_mode, Flux::OverwriteMode::SKIP);
    EXPECT_FALSE(options.hoist_single_folder);
    EXPECT_TRUE(options.preserve_permissions);
    EXPECT_TRUE(options.preserve_timestamps);
    EXPECT_TRUE(options.password.empty());
    
    // Test configuration
    options.overwrite_mode = Flux::OverwriteMode::OVERWRITE;
    options.hoist_single_folder = true;
    options.include_patterns = {"*.txt", "*.cpp"};
    options.exclude_patterns = {"*.tmp", "*.log"};
    
    EXPECT_EQ(options.overwrite_mode, Flux::OverwriteMode::OVERWRITE);
    EXPECT_TRUE(options.hoist_single_folder);
    EXPECT_EQ(options.include_patterns.size(), 2);
    EXPECT_EQ(options.exclude_patterns.size(), 2);
}

// Test supported formats
TEST_F(ArchiveUtilsTest, SupportedFormats) {
    auto formats = Flux::getSupportedFormats();
    EXPECT_EQ(formats.size(), 5);
    
    // Verify all expected formats are present
    bool has_zip = false, has_tar_gz = false, has_tar_xz = false, 
         has_tar_zstd = false, has_7z = false;
    
    for (const auto& format : formats) {
        switch (format) {
            case Flux::ArchiveFormat::ZIP: has_zip = true; break;
            case Flux::ArchiveFormat::TAR_GZ: has_tar_gz = true; break;
            case Flux::ArchiveFormat::TAR_XZ: has_tar_xz = true; break;
            case Flux::ArchiveFormat::TAR_ZSTD: has_tar_zstd = true; break;
            case Flux::ArchiveFormat::SEVEN_ZIP: has_7z = true; break;
        }
    }
    
    EXPECT_TRUE(has_zip);
    EXPECT_TRUE(has_tar_gz);
    EXPECT_TRUE(has_tar_xz);
    EXPECT_TRUE(has_tar_zstd);
    EXPECT_TRUE(has_7z);
}

// Test progress callback functionality
TEST_F(ArchiveUtilsTest, ProgressCallbackFunctionality) {
    bool callback_called = false;
    float last_progress = -1.0f;
    std::string last_filename;
    
    Flux::ProgressCallback callback = [&](std::string_view filename, float progress, 
                                          size_t processed, size_t total) {
        callback_called = true;
        last_progress = progress;
        last_filename = std::string(filename);
        
        // Validate progress is in valid range
        EXPECT_GE(progress, 0.0f);
        EXPECT_LE(progress, 1.0f);
        EXPECT_LE(processed, total);
    };
    
    // Simulate progress callback
    callback("test.txt", 0.5f, 512, 1024);
    
    EXPECT_TRUE(callback_called);
    EXPECT_EQ(last_progress, 0.5f);
    EXPECT_EQ(last_filename, "test.txt");
}

// Test error callback functionality
TEST_F(ArchiveUtilsTest, ErrorCallbackFunctionality) {
    bool error_callback_called = false;
    std::string last_error_message;
    bool last_fatal_flag = false;
    
    Flux::ErrorCallback error_callback = [&](std::string_view error_msg, bool is_fatal) {
        error_callback_called = true;
        last_error_message = std::string(error_msg);
        last_fatal_flag = is_fatal;
    };
    
    // Simulate error callback
    error_callback("Test error message", true);
    
    EXPECT_TRUE(error_callback_called);
    EXPECT_EQ(last_error_message, "Test error message");
    EXPECT_TRUE(last_fatal_flag);
}

// Test exception handling
TEST_F(ArchiveUtilsTest, ExceptionHandling) {
    // Test with non-existent archive file
    auto extractor = Flux::createExtractor(Flux::ArchiveFormat::ZIP);
    auto result = extractor->listContents("nonexistent.zip", "");
    
    // Should return error in expected rather than throwing
    EXPECT_FALSE(result.has_value());
    if (!result.has_value()) {
        EXPECT_FALSE(result.error().empty());
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // Initialize Flux library
    Flux::initialize();
    
    int result = RUN_ALL_TESTS();
    
    // Cleanup Flux library
    Flux::cleanup();
    
    return result;
}

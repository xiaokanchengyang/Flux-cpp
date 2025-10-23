#include <gtest/gtest.h>
#include <flux-core/archive.h>
#include <flux-core/packer.h>
#include <flux-core/extractor.h>
#include <flux-core/exceptions.h>
#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>

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
        
        spdlog::set_level(spdlog::level::debug);
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

TEST_F(ArchiveUtilsTest, FormatDetection) {
    // Test format detection from file extensions
    EXPECT_EQ(Flux::detectFormatFromExtension("test.zip"), Flux::ArchiveFormat::ZIP);
    EXPECT_EQ(Flux::detectFormatFromExtension("test.tar.gz"), Flux::ArchiveFormat::TAR_GZ);
    EXPECT_EQ(Flux::detectFormatFromExtension("test.tar.xz"), Flux::ArchiveFormat::TAR_XZ);
    EXPECT_EQ(Flux::detectFormatFromExtension("test.tar.zst"), Flux::ArchiveFormat::TAR_ZSTD);
    EXPECT_EQ(Flux::detectFormatFromExtension("test.7z"), Flux::ArchiveFormat::SEVEN_ZIP);
    
    // Test invalid format
    EXPECT_THROW(Flux::detectFormatFromExtension("test.unknown"), Flux::UnsupportedFormatException);
}

TEST_F(ArchiveUtilsTest, FormatToString) {
    EXPECT_EQ(Flux::formatToString(Flux::ArchiveFormat::ZIP), "zip");
    EXPECT_EQ(Flux::formatToString(Flux::ArchiveFormat::TAR_GZ), "tar.gz");
    EXPECT_EQ(Flux::formatToString(Flux::ArchiveFormat::TAR_XZ), "tar.xz");
    EXPECT_EQ(Flux::formatToString(Flux::ArchiveFormat::TAR_ZSTD), "tar.zstd");
    EXPECT_EQ(Flux::formatToString(Flux::ArchiveFormat::SEVEN_ZIP), "7z");
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
    
    // Test input validation
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

// Test exception handling
TEST_F(ArchiveUtilsTest, ExceptionHandling) {
    // Test file not found exception
    EXPECT_THROW({
        auto extractor = Flux::createExtractor(Flux::ArchiveFormat::ZIP);
        extractor->listContents("nonexistent.zip", "");
    }, Flux::FileNotFoundException);
    
    // Test unsupported format (if we add a format that's not implemented)
    // This would depend on the specific implementation
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

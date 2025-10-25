#include <gtest/gtest.h>
#include <flux-core/extractor.h>
#include <flux-core/archive.h>
#include <filesystem>
#include <fstream>
#include <memory>

class ExtractorTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = std::filesystem::temp_directory_path() / "flux_extractor_test";
        std::filesystem::create_directories(test_dir);
    }
    
    void TearDown() override {
        std::filesystem::remove_all(test_dir);
    }
    
    std::filesystem::path test_dir;
};

TEST_F(ExtractorTest, CreateExtractorInstances) {
    // Test creating extractors for all supported formats
    auto zip_extractor = Flux::createExtractor(Flux::ArchiveFormat::ZIP);
    EXPECT_NE(zip_extractor, nullptr);
    EXPECT_TRUE(zip_extractor->supportsFormat(Flux::ArchiveFormat::ZIP));
    
    auto tar_gz_extractor = Flux::createExtractor(Flux::ArchiveFormat::TAR_GZ);
    EXPECT_NE(tar_gz_extractor, nullptr);
    EXPECT_TRUE(tar_gz_extractor->supportsFormat(Flux::ArchiveFormat::TAR_GZ));
    
    auto tar_xz_extractor = Flux::createExtractor(Flux::ArchiveFormat::TAR_XZ);
    EXPECT_NE(tar_xz_extractor, nullptr);
    EXPECT_TRUE(tar_xz_extractor->supportsFormat(Flux::ArchiveFormat::TAR_XZ));
    
    auto tar_zstd_extractor = Flux::createExtractor(Flux::ArchiveFormat::TAR_ZSTD);
    EXPECT_NE(tar_zstd_extractor, nullptr);
    EXPECT_TRUE(tar_zstd_extractor->supportsFormat(Flux::ArchiveFormat::TAR_ZSTD));
    
    auto seven_zip_extractor = Flux::createExtractor(Flux::ArchiveFormat::SEVEN_ZIP);
    EXPECT_NE(seven_zip_extractor, nullptr);
    EXPECT_TRUE(seven_zip_extractor->supportsFormat(Flux::ArchiveFormat::SEVEN_ZIP));
}

TEST_F(ExtractorTest, ExtractorFormatSupport) {
    auto extractor = Flux::createExtractor(Flux::ArchiveFormat::ZIP);
    
    // Test format support
    EXPECT_TRUE(extractor->supportsFormat(Flux::ArchiveFormat::ZIP));
    EXPECT_FALSE(extractor->supportsFormat(Flux::ArchiveFormat::TAR_GZ));
    EXPECT_FALSE(extractor->supportsFormat(Flux::ArchiveFormat::TAR_XZ));
    EXPECT_FALSE(extractor->supportsFormat(Flux::ArchiveFormat::TAR_ZSTD));
    EXPECT_FALSE(extractor->supportsFormat(Flux::ArchiveFormat::SEVEN_ZIP));
}

TEST_F(ExtractorTest, ListContentsNonExistentFile) {
    auto extractor = Flux::createExtractor(Flux::ArchiveFormat::ZIP);
    
    auto result = extractor->listContents(test_dir / "nonexistent.zip");
    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(result.error().empty());
}

TEST_F(ExtractorTest, GetArchiveInfoNonExistentFile) {
    auto extractor = Flux::createExtractor(Flux::ArchiveFormat::ZIP);
    
    auto result = extractor->getArchiveInfo(test_dir / "nonexistent.zip");
    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(result.error().empty());
}

TEST_F(ExtractorTest, VerifyIntegrityNonExistentFile) {
    auto extractor = Flux::createExtractor(Flux::ArchiveFormat::ZIP);
    
    auto result = extractor->verifyIntegrity(test_dir / "nonexistent.zip");
    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(result.error().empty());
}

TEST_F(ExtractorTest, DetectFormatNonExistentFile) {
    auto extractor = Flux::createExtractor(Flux::ArchiveFormat::ZIP);
    
    auto result = extractor->detectFormat(test_dir / "nonexistent.zip");
    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(result.error().empty());
}

TEST_F(ExtractorTest, ExtractOptionsDefaults) {
    Flux::ExtractOptions options;
    
    EXPECT_EQ(options.overwrite_mode, Flux::OverwriteMode::SKIP);
    EXPECT_FALSE(options.hoist_single_folder);
    EXPECT_TRUE(options.preserve_permissions);
    EXPECT_TRUE(options.preserve_timestamps);
    EXPECT_TRUE(options.password.empty());
    EXPECT_TRUE(options.include_patterns.empty());
    EXPECT_TRUE(options.exclude_patterns.empty());
}

TEST_F(ExtractorTest, ExtractResultDefaults) {
    Flux::ExtractResult result;
    
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.error_message.empty());
    EXPECT_EQ(result.files_extracted, 0);
    EXPECT_EQ(result.total_size, 0);
    EXPECT_EQ(result.duration.count(), 0);
    EXPECT_TRUE(result.skipped_files.empty());
}

TEST_F(ExtractorTest, ExtractResultComparison) {
    Flux::ExtractResult result1;
    Flux::ExtractResult result2;
    
    // Test equality
    EXPECT_EQ(result1, result2);
    
    // Modify one result
    result2.success = true;
    result2.files_extracted = 5;
    
    // Test inequality
    EXPECT_NE(result1, result2);
}

TEST_F(ExtractorTest, CancellationInterface) {
    auto extractor = Flux::createExtractor(Flux::ArchiveFormat::ZIP);
    
    // Test that cancel method exists and can be called
    // (Implementation may be no-op in stub)
    EXPECT_NO_THROW(extractor->cancel());
}

TEST_F(ExtractorTest, AutoDetectExtractor) {
    // Test auto-detection with non-existent file
    auto result = Flux::createExtractorAuto(test_dir / "nonexistent.zip");
    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(result.error().empty());
}

// Test partial extraction interface
TEST_F(ExtractorTest, PartialExtractionInterface) {
    auto extractor = Flux::createExtractor(Flux::ArchiveFormat::ZIP);
    
    std::vector<std::string> patterns = {"*.txt", "*.cpp"};
    Flux::ExtractOptions options;
    
    // Test partial extraction with non-existent archive
    auto result = extractor->extractPartial(
        test_dir / "nonexistent.zip",
        test_dir / "output",
        patterns,
        options
    );
    
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

// Test extraction with progress callback
TEST_F(ExtractorTest, ExtractionWithProgressCallback) {
    auto extractor = Flux::createExtractor(Flux::ArchiveFormat::ZIP);
    
    bool progress_called = false;
    Flux::ProgressCallback progress_cb = [&](std::string_view filename, float progress,
                                            size_t processed, size_t total) {
        progress_called = true;
        EXPECT_GE(progress, 0.0f);
        EXPECT_LE(progress, 1.0f);
    };
    
    bool error_called = false;
    Flux::ErrorCallback error_cb = [&](std::string_view error_msg, bool is_fatal) {
        error_called = true;
        EXPECT_FALSE(error_msg.empty());
    };
    
    Flux::ExtractOptions options;
    
    // Test extraction with callbacks (will fail due to non-existent file)
    auto result = extractor->extract(
        test_dir / "nonexistent.zip",
        test_dir / "output",
        options,
        progress_cb,
        error_cb
    );
    
    EXPECT_FALSE(result.success);
    // Error callback should be called for non-existent file
    EXPECT_TRUE(error_called);
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

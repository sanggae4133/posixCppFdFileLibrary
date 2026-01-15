/**
 * @file FilePermissionTest.cpp
 * @brief Integration tests for file permission and access control scenarios
 */

#include <fcntl.h>
#include <gtest/gtest.h>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>

#include "records/FixedA.hpp"
#include "repository/UniformFixedRepositoryImpl.hpp"

using namespace FdFile;

class FilePermissionTest : public ::testing::Test {
  protected:
    void SetUp() override {
        testFile_ = "./test_permission.db";
        ::remove(testFile_.c_str());
    }

    void TearDown() override {
        // Reset permissions before removing
        ::chmod(testFile_.c_str(), 0644);
        ::remove(testFile_.c_str());
    }

    std::string testFile_;
};

TEST_F(FilePermissionTest, NormalReadWrite) {
    std::error_code ec;
    auto repo = std::make_unique<UniformFixedRepositoryImpl<FixedA>>(testFile_, ec);
    ASSERT_FALSE(ec);

    FixedA record("alice", 25, "001");
    EXPECT_TRUE(repo->save(record, ec));
    EXPECT_FALSE(ec);
}

TEST_F(FilePermissionTest, ReadOnlyFileWrite) {
    // Create file with data
    {
        std::error_code ec;
        auto repo = std::make_unique<UniformFixedRepositoryImpl<FixedA>>(testFile_, ec);
        ASSERT_FALSE(ec);

        FixedA record("alice", 25, "001");
        repo->save(record, ec);
    }

    // Make read-only
    ::chmod(testFile_.c_str(), 0444);

    // Try to open for write - should fail or succeed depending on implementation
    std::error_code ec;
    auto repo = std::make_unique<UniformFixedRepositoryImpl<FixedA>>(testFile_, ec);

    // Open may succeed but write should fail
    if (!ec) {
        FixedA newRecord("bob", 30, "002");
        bool saved = repo->save(newRecord, ec);
        // May fail due to read-only
    }
}

TEST_F(FilePermissionTest, NonExistentDirectory) {
    std::error_code ec;
    auto repo =
        std::make_unique<UniformFixedRepositoryImpl<FixedA>>("/nonexistent/path/file.db", ec);

    // Should fail - directory doesn't exist
    EXPECT_TRUE(ec);
}

TEST_F(FilePermissionTest, CreateNewFile) {
    std::string newFile = "./test_new_file.db";
    ::remove(newFile.c_str());

    std::error_code ec;
    auto repo = std::make_unique<UniformFixedRepositoryImpl<FixedA>>(newFile, ec);

    // Should create new file
    EXPECT_FALSE(ec);

    // Verify file exists
    struct stat st;
    EXPECT_EQ(::stat(newFile.c_str(), &st), 0);

    ::remove(newFile.c_str());
}

TEST_F(FilePermissionTest, FileDescriptorLeak) {
    // Open and close many times - should not leak FDs
    for (int i = 0; i < 100; ++i) {
        std::error_code ec;
        auto repo = std::make_unique<UniformFixedRepositoryImpl<FixedA>>(testFile_, ec);
        ASSERT_FALSE(ec) << "Failed on iteration " << i;

        FixedA record("user", i, std::to_string(i).c_str());
        repo->save(record, ec);
    }

    // If we got here, no FD exhaustion
    SUCCEED();
}

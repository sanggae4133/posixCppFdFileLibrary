/**
 * @file MmapGuardTest.cpp
 * @brief Unit tests for MmapGuard RAII wrapper
 */

#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/mman.h>
#include <unistd.h>

#include "util/MmapGuard.hpp"

using namespace FdFile::detail;

class MmapGuardTest : public ::testing::Test {
  protected:
    void SetUp() override {
        testFile_ = "./test_mmap.tmp";
        ::remove(testFile_.c_str());

        // Create file with some content
        int fd = ::open(testFile_.c_str(), O_CREAT | O_RDWR, 0644);
        ASSERT_GE(fd, 0);
        const char* data = "Hello, MmapGuard Test!";
        ::write(fd, data, strlen(data));
        ::close(fd);
    }

    void TearDown() override { ::remove(testFile_.c_str()); }

    std::string testFile_;
};

TEST_F(MmapGuardTest, DefaultConstructor) {
    MmapGuard mmap;
    EXPECT_EQ(mmap.get(), nullptr);
    EXPECT_EQ(mmap.size(), 0);
    EXPECT_FALSE(mmap.valid());
    EXPECT_FALSE(static_cast<bool>(mmap));
}

TEST_F(MmapGuardTest, ConstructWithValidMapping) {
    int fd = ::open(testFile_.c_str(), O_RDWR);
    ASSERT_GE(fd, 0);

    struct stat st;
    ::fstat(fd, &st);

    void* ptr = ::mmap(nullptr, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ASSERT_NE(ptr, MAP_FAILED);

    MmapGuard mmap(ptr, st.st_size);
    EXPECT_EQ(mmap.get(), ptr);
    EXPECT_EQ(mmap.size(), static_cast<size_t>(st.st_size));
    EXPECT_TRUE(mmap.valid());

    ::close(fd);
}

TEST_F(MmapGuardTest, ConstructWithMapFailed) {
    MmapGuard mmap(MAP_FAILED, 100);
    EXPECT_EQ(mmap.get(), nullptr);
    EXPECT_EQ(mmap.size(), 0);
    EXPECT_FALSE(mmap.valid());
}

TEST_F(MmapGuardTest, MoveConstructor) {
    int fd = ::open(testFile_.c_str(), O_RDWR);
    ASSERT_GE(fd, 0);

    struct stat st;
    ::fstat(fd, &st);

    void* ptr = ::mmap(nullptr, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    MmapGuard mmap1(ptr, st.st_size);
    MmapGuard mmap2(std::move(mmap1));

    EXPECT_EQ(mmap1.get(), nullptr);
    EXPECT_EQ(mmap2.get(), ptr);

    ::close(fd);
}

TEST_F(MmapGuardTest, Reset) {
    int fd = ::open(testFile_.c_str(), O_RDWR);
    ASSERT_GE(fd, 0);

    struct stat st;
    ::fstat(fd, &st);

    void* ptr = ::mmap(nullptr, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    MmapGuard mmap(ptr, st.st_size);
    EXPECT_TRUE(mmap.valid());

    mmap.reset();
    EXPECT_FALSE(mmap.valid());
    EXPECT_EQ(mmap.get(), nullptr);

    ::close(fd);
}

TEST_F(MmapGuardTest, DataAccess) {
    int fd = ::open(testFile_.c_str(), O_RDWR);
    ASSERT_GE(fd, 0);

    struct stat st;
    ::fstat(fd, &st);

    void* ptr = ::mmap(nullptr, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    MmapGuard mmap(ptr, st.st_size);
    EXPECT_STREQ(mmap.data(), "Hello, MmapGuard Test!");

    ::close(fd);
}

TEST_F(MmapGuardTest, Sync) {
    int fd = ::open(testFile_.c_str(), O_RDWR);
    ASSERT_GE(fd, 0);

    struct stat st;
    ::fstat(fd, &st);

    void* ptr = ::mmap(nullptr, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    MmapGuard mmap(ptr, st.st_size);

    // Modify content
    char* data = mmap.data();
    data[0] = 'X';

    // Sync should succeed
    EXPECT_TRUE(mmap.sync());
    EXPECT_TRUE(mmap.sync(true)); // async sync

    ::close(fd);
}

TEST_F(MmapGuardTest, SyncInvalidMapping) {
    MmapGuard mmap;
    EXPECT_FALSE(mmap.sync());
}

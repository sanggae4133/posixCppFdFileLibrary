/**
 * @file FileLockGuardTest.cpp
 * @brief Unit tests for FileLockGuard RAII wrapper
 */

#include <fcntl.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <fdfile/util/FileLockGuard.hpp>

using namespace FdFile::detail;

class FileLockGuardTest : public ::testing::Test {
  protected:
    void SetUp() override {
        testFile_ = "./test_filelock.tmp";
        ::remove(testFile_.c_str());

        fd_ = ::open(testFile_.c_str(), O_CREAT | O_RDWR, 0644);
        ASSERT_GE(fd_, 0);
    }

    void TearDown() override {
        if (fd_ >= 0)
            ::close(fd_);
        ::remove(testFile_.c_str());
    }

    std::string testFile_;
    int fd_ = -1;
};

TEST_F(FileLockGuardTest, DefaultConstructor) {
    FileLockGuard lock;
    EXPECT_FALSE(lock.locked());
}

TEST_F(FileLockGuardTest, SharedLock) {
    std::error_code ec;
    FileLockGuard lock(fd_, FileLockGuard::Mode::Shared, ec);

    EXPECT_FALSE(ec);
    EXPECT_TRUE(lock.locked());
}

TEST_F(FileLockGuardTest, ExclusiveLock) {
    std::error_code ec;
    FileLockGuard lock(fd_, FileLockGuard::Mode::Exclusive, ec);

    EXPECT_FALSE(ec);
    EXPECT_TRUE(lock.locked());
}

TEST_F(FileLockGuardTest, InvalidFdLock) {
    std::error_code ec;
    FileLockGuard lock(-1, FileLockGuard::Mode::Shared, ec);

    EXPECT_TRUE(ec);
    EXPECT_FALSE(lock.locked());
}

TEST_F(FileLockGuardTest, UnlockOnDestruction) {
    {
        std::error_code ec;
        FileLockGuard lock(fd_, FileLockGuard::Mode::Exclusive, ec);
        EXPECT_TRUE(lock.locked());
    } // lock goes out of scope

    // Should be able to lock again
    std::error_code ec;
    FileLockGuard lock2(fd_, FileLockGuard::Mode::Exclusive, ec);
    EXPECT_FALSE(ec);
    EXPECT_TRUE(lock2.locked());
}

TEST_F(FileLockGuardTest, MoveConstructor) {
    std::error_code ec;
    FileLockGuard lock1(fd_, FileLockGuard::Mode::Exclusive, ec);
    ASSERT_TRUE(lock1.locked());

    FileLockGuard lock2(std::move(lock1));

    EXPECT_FALSE(lock1.locked());
    EXPECT_TRUE(lock2.locked());
}

TEST_F(FileLockGuardTest, MoveAssignment) {
    std::error_code ec;
    FileLockGuard lock1(fd_, FileLockGuard::Mode::Exclusive, ec);
    ASSERT_TRUE(lock1.locked());

    FileLockGuard lock2;
    lock2 = std::move(lock1);

    EXPECT_FALSE(lock1.locked());
    EXPECT_TRUE(lock2.locked());
}

TEST_F(FileLockGuardTest, ManualLock) {
    FileLockGuard lock;
    EXPECT_FALSE(lock.locked());

    std::error_code ec;
    bool success = lock.lock(fd_, FileLockGuard::Mode::Shared, ec);

    EXPECT_TRUE(success);
    EXPECT_FALSE(ec);
    EXPECT_TRUE(lock.locked());
}

TEST_F(FileLockGuardTest, UnlockIgnore) {
    std::error_code ec;
    FileLockGuard lock(fd_, FileLockGuard::Mode::Exclusive, ec);
    ASSERT_TRUE(lock.locked());

    lock.unlockIgnore();
    EXPECT_FALSE(lock.locked());
}

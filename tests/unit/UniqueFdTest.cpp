/**
 * @file UniqueFdTest.cpp
 * @brief Unit tests for UniqueFd RAII wrapper
 */

#include <fcntl.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <fdfile/util/UniqueFd.hpp>

using namespace FdFile::detail;

class UniqueFdTest : public ::testing::Test {
  protected:
    void SetUp() override {
        testFile_ = "./test_uniquefd.tmp";
        ::remove(testFile_.c_str());
    }

    void TearDown() override { ::remove(testFile_.c_str()); }

    std::string testFile_;
};

TEST_F(UniqueFdTest, DefaultConstructor) {
    UniqueFd fd;
    EXPECT_EQ(fd.get(), -1);
    EXPECT_FALSE(fd.valid());
    EXPECT_FALSE(static_cast<bool>(fd));
}

TEST_F(UniqueFdTest, ConstructWithValidFd) {
    int rawFd = ::open(testFile_.c_str(), O_CREAT | O_RDWR, 0644);
    ASSERT_GE(rawFd, 0);

    UniqueFd fd(rawFd);
    EXPECT_EQ(fd.get(), rawFd);
    EXPECT_TRUE(fd.valid());
    EXPECT_TRUE(static_cast<bool>(fd));
}

TEST_F(UniqueFdTest, DestructorClosesFd) {
    int rawFd = ::open(testFile_.c_str(), O_CREAT | O_RDWR, 0644);
    ASSERT_GE(rawFd, 0);

    {
        UniqueFd fd(rawFd);
        EXPECT_TRUE(fd.valid());
    } // fd goes out of scope, should close

    // Verify fd is closed - write should fail
    EXPECT_EQ(::write(rawFd, "x", 1), -1);
}

TEST_F(UniqueFdTest, MoveConstructor) {
    int rawFd = ::open(testFile_.c_str(), O_CREAT | O_RDWR, 0644);
    ASSERT_GE(rawFd, 0);

    UniqueFd fd1(rawFd);
    UniqueFd fd2(std::move(fd1));

    EXPECT_EQ(fd1.get(), -1);
    EXPECT_FALSE(fd1.valid());
    EXPECT_EQ(fd2.get(), rawFd);
    EXPECT_TRUE(fd2.valid());
}

TEST_F(UniqueFdTest, MoveAssignment) {
    int rawFd1 = ::open(testFile_.c_str(), O_CREAT | O_RDWR, 0644);
    ASSERT_GE(rawFd1, 0);

    UniqueFd fd1(rawFd1);
    UniqueFd fd2;

    fd2 = std::move(fd1);

    EXPECT_EQ(fd1.get(), -1);
    EXPECT_EQ(fd2.get(), rawFd1);
}

TEST_F(UniqueFdTest, Release) {
    int rawFd = ::open(testFile_.c_str(), O_CREAT | O_RDWR, 0644);
    ASSERT_GE(rawFd, 0);

    UniqueFd fd(rawFd);
    int released = fd.release();

    EXPECT_EQ(released, rawFd);
    EXPECT_EQ(fd.get(), -1);
    EXPECT_FALSE(fd.valid());

    // Must manually close since we released
    ::close(released);
}

TEST_F(UniqueFdTest, Reset) {
    int rawFd1 = ::open(testFile_.c_str(), O_CREAT | O_RDWR, 0644);
    ASSERT_GE(rawFd1, 0);

    UniqueFd fd(rawFd1);

    // Open another file
    std::string testFile2 = "./test_uniquefd2.tmp";
    int rawFd2 = ::open(testFile2.c_str(), O_CREAT | O_RDWR, 0644);
    ASSERT_GE(rawFd2, 0);

    fd.reset(rawFd2);

    EXPECT_EQ(fd.get(), rawFd2);
    // rawFd1 should be closed
    EXPECT_EQ(::write(rawFd1, "x", 1), -1);

    ::remove(testFile2.c_str());
}

TEST_F(UniqueFdTest, ResetToInvalid) {
    int rawFd = ::open(testFile_.c_str(), O_CREAT | O_RDWR, 0644);
    ASSERT_GE(rawFd, 0);

    UniqueFd fd(rawFd);
    fd.reset();

    EXPECT_EQ(fd.get(), -1);
    EXPECT_FALSE(fd.valid());
}

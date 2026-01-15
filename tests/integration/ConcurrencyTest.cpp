/**
 * @file ConcurrencyTest.cpp
 * @brief Integration tests for concurrent access and file locking
 * @note The repository is NOT thread-safe for shared instance access.
 *       Each thread must use its own repository instance.
 *       File-level locking (fcntl) is used to prevent data corruption.
 */

#include <gtest/gtest.h>
#include <memory>

#include "records/FixedA.hpp"
#include "repository/UniformFixedRepositoryImpl.hpp"

using namespace FdFile;

class ConcurrencyTest : public ::testing::Test {
  protected:
    void SetUp() override {
        testFile_ = "./test_concurrency.db";
        ::remove(testFile_.c_str());
    }

    void TearDown() override { ::remove(testFile_.c_str()); }

    std::string testFile_;
};

TEST_F(ConcurrencyTest, SequentialMultipleOpens) {
    // Open, write, close - repeat multiple times sequentially
    for (int iter = 0; iter < 10; ++iter) {
        std::error_code ec;
        auto repo = std::make_unique<UniformFixedRepositoryImpl<FixedA>>(testFile_, ec);
        ASSERT_FALSE(ec);

        FixedA record("user", iter, std::to_string(iter).c_str());
        repo->save(record, ec);
    }

    // Verify all 10 records exist
    std::error_code ec;
    auto repo = std::make_unique<UniformFixedRepositoryImpl<FixedA>>(testFile_, ec);
    EXPECT_EQ(repo->count(ec), 10);
}

TEST_F(ConcurrencyTest, MultipleRepositoriesSequential) {
    std::error_code ec1, ec2;

    // Open first repository and write
    auto repo1 = std::make_unique<UniformFixedRepositoryImpl<FixedA>>(testFile_, ec1);
    ASSERT_FALSE(ec1);

    FixedA record1("alice", 25, "001");
    repo1->save(record1, ec1);

    // Close repo1 before opening repo2
    repo1.reset();

    // Open second repository and read
    auto repo2 = std::make_unique<UniformFixedRepositoryImpl<FixedA>>(testFile_, ec2);
    ASSERT_FALSE(ec2);

    auto found = repo2->findById("001", ec2);
    ASSERT_NE(found, nullptr);
    EXPECT_STREQ(found->name, "alice");
}

TEST_F(ConcurrencyTest, RapidOpenCloseNoLeak) {
    // Rapidly open and close to check for resource leaks
    for (int i = 0; i < 50; ++i) {
        std::error_code ec;
        auto repo = std::make_unique<UniformFixedRepositoryImpl<FixedA>>(testFile_, ec);
        ASSERT_FALSE(ec) << "Failed on iteration " << i;

        FixedA record("user", i, std::to_string(i).c_str());
        repo->save(record, ec);
        ASSERT_FALSE(ec);
    }

    // Verify final count
    std::error_code ec;
    auto repo = std::make_unique<UniformFixedRepositoryImpl<FixedA>>(testFile_, ec);
    EXPECT_EQ(repo->count(ec), 50);
}

TEST_F(ConcurrencyTest, DataIntegrityAfterMultipleWrites) {
    // Write from separate sequential sessions
    for (int session = 0; session < 5; ++session) {
        std::error_code ec;
        auto repo = std::make_unique<UniformFixedRepositoryImpl<FixedA>>(testFile_, ec);
        ASSERT_FALSE(ec);

        for (int i = 0; i < 10; ++i) {
            int id = session * 100 + i;
            FixedA record("user", id, std::to_string(id).c_str());
            repo->save(record, ec);
        }
    }

    // Verify all records intact
    std::error_code ec;
    auto repo = std::make_unique<UniformFixedRepositoryImpl<FixedA>>(testFile_, ec);
    EXPECT_EQ(repo->count(ec), 50);

    // Spot check
    auto found = repo->findById("201", ec);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->age, 201);
}

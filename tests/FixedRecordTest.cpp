/**
 * @file FixedRecordTest.cpp
 * @brief Unit tests for Fixed-length record repositories (FixedA, FixedB)
 */

#include <gtest/gtest.h>
#include <memory>

#include "records/FixedA.hpp"
#include "records/FixedB.hpp"
#include "repository/UniformFixedRepositoryImpl.hpp"

using namespace FdFile;

// =============================================================================
// FixedA Repository Tests
// =============================================================================

class FixedARepositoryTest : public ::testing::Test {
  protected:
    void SetUp() override {
        testFile_ = "./test_fixed_a.db";
        ::remove(testFile_.c_str());
        repo_ = std::make_unique<UniformFixedRepositoryImpl<FixedA>>(testFile_, ec_);
        ASSERT_FALSE(ec_) << "Repository init failed: " << ec_.message();
    }

    void TearDown() override {
        repo_.reset();
        ::remove(testFile_.c_str());
    }

    std::string testFile_;
    std::error_code ec_;
    std::unique_ptr<UniformFixedRepositoryImpl<FixedA>> repo_;
};

TEST_F(FixedARepositoryTest, InsertSingleRecord) {
    FixedA alice("alice", 25, "001");

    ASSERT_TRUE(repo_->save(alice, ec_));
    EXPECT_EQ(repo_->count(ec_), 1);
}

TEST_F(FixedARepositoryTest, InsertMultipleRecords) {
    FixedA alice("alice", 25, "001");
    FixedA bob("bob", 30, "002");
    FixedA charlie("charlie", 35, "003");

    ASSERT_TRUE(repo_->save(alice, ec_));
    ASSERT_TRUE(repo_->save(bob, ec_));
    ASSERT_TRUE(repo_->save(charlie, ec_));

    EXPECT_EQ(repo_->count(ec_), 3);
}

TEST_F(FixedARepositoryTest, UpdateExistingRecord) {
    FixedA alice("alice", 25, "001");
    repo_->save(alice, ec_);

    // Update with same ID
    FixedA updatedAlice("alice_v2", 26, "001");
    ASSERT_TRUE(repo_->save(updatedAlice, ec_));

    // Count should still be 1
    EXPECT_EQ(repo_->count(ec_), 1);

    // Verify update
    auto found = repo_->findById("001", ec_);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->age, 26);
}

TEST_F(FixedARepositoryTest, FindById) {
    FixedA alice("alice", 25, "001");
    FixedA bob("bob", 30, "002");
    repo_->save(alice, ec_);
    repo_->save(bob, ec_);

    auto found = repo_->findById("002", ec_);
    ASSERT_NE(found, nullptr);
    EXPECT_STREQ(found->name, "bob");
    EXPECT_EQ(found->age, 30);
    EXPECT_EQ(found->getId(), "002");
}

TEST_F(FixedARepositoryTest, FindByIdNotFound) {
    FixedA alice("alice", 25, "001");
    repo_->save(alice, ec_);

    auto notFound = repo_->findById("999", ec_);
    EXPECT_EQ(notFound, nullptr);
}

TEST_F(FixedARepositoryTest, FindAll) {
    FixedA alice("alice", 25, "001");
    FixedA bob("bob", 30, "002");
    FixedA charlie("charlie", 35, "003");
    repo_->save(alice, ec_);
    repo_->save(bob, ec_);
    repo_->save(charlie, ec_);

    auto all = repo_->findAll(ec_);
    EXPECT_EQ(all.size(), 3);
}

TEST_F(FixedARepositoryTest, ExistsById) {
    FixedA alice("alice", 25, "001");
    repo_->save(alice, ec_);

    EXPECT_TRUE(repo_->existsById("001", ec_));
    EXPECT_FALSE(repo_->existsById("999", ec_));
}

TEST_F(FixedARepositoryTest, DeleteById) {
    FixedA alice("alice", 25, "001");
    FixedA bob("bob", 30, "002");
    repo_->save(alice, ec_);
    repo_->save(bob, ec_);

    ASSERT_TRUE(repo_->deleteById("001", ec_));
    EXPECT_EQ(repo_->count(ec_), 1);
    EXPECT_FALSE(repo_->existsById("001", ec_));
    EXPECT_TRUE(repo_->existsById("002", ec_));
}

TEST_F(FixedARepositoryTest, DeleteAll) {
    FixedA alice("alice", 25, "001");
    FixedA bob("bob", 30, "002");
    repo_->save(alice, ec_);
    repo_->save(bob, ec_);

    ASSERT_TRUE(repo_->deleteAll(ec_));
    EXPECT_EQ(repo_->count(ec_), 0);
}

TEST_F(FixedARepositoryTest, EdgeCaseEmptyName) {
    FixedA empty("", 0, "E01");

    ASSERT_TRUE(repo_->save(empty, ec_));

    auto found = repo_->findById("E01", ec_);
    ASSERT_NE(found, nullptr);
    EXPECT_STREQ(found->name, "");
}

TEST_F(FixedARepositoryTest, EdgeCaseMaxInt64) {
    FixedA maxAge("max", INT64_MAX, "E02");

    ASSERT_TRUE(repo_->save(maxAge, ec_));

    auto found = repo_->findById("E02", ec_);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->age, INT64_MAX);
}

// =============================================================================
// FixedB Repository Tests
// =============================================================================

class FixedBRepositoryTest : public ::testing::Test {
  protected:
    void SetUp() override {
        testFile_ = "./test_fixed_b.db";
        ::remove(testFile_.c_str());
        repo_ = std::make_unique<UniformFixedRepositoryImpl<FixedB>>(testFile_, ec_);
        ASSERT_FALSE(ec_) << "Repository init failed: " << ec_.message();
    }

    void TearDown() override {
        repo_.reset();
        ::remove(testFile_.c_str());
    }

    std::string testFile_;
    std::error_code ec_;
    std::unique_ptr<UniformFixedRepositoryImpl<FixedB>> repo_;
};

TEST_F(FixedBRepositoryTest, InsertMultipleRecords) {
    FixedB laptop("Laptop", 1500000, "P001");
    FixedB phone("Phone", 800000, "P002");
    FixedB tablet("Tablet", 500000, "P003");

    ASSERT_TRUE(repo_->save(laptop, ec_));
    ASSERT_TRUE(repo_->save(phone, ec_));
    ASSERT_TRUE(repo_->save(tablet, ec_));

    EXPECT_EQ(repo_->count(ec_), 3);
}

TEST_F(FixedBRepositoryTest, FindAll) {
    FixedB laptop("Laptop", 1500000, "P001");
    FixedB phone("Phone", 800000, "P002");
    repo_->save(laptop, ec_);
    repo_->save(phone, ec_);

    auto all = repo_->findAll(ec_);
    ASSERT_EQ(all.size(), 2);

    // Verify content
    bool foundLaptop = false, foundPhone = false;
    for (const auto& rec : all) {
        if (std::string(rec->title) == "Laptop")
            foundLaptop = true;
        if (std::string(rec->title) == "Phone")
            foundPhone = true;
    }
    EXPECT_TRUE(foundLaptop);
    EXPECT_TRUE(foundPhone);
}

TEST_F(FixedBRepositoryTest, UpdateExistingRecord) {
    FixedB phone("Phone", 800000, "P001");
    repo_->save(phone, ec_);

    // Update price (discount!)
    FixedB discountedPhone("Phone", 750000, "P001");
    ASSERT_TRUE(repo_->save(discountedPhone, ec_));

    EXPECT_EQ(repo_->count(ec_), 1);

    auto found = repo_->findById("P001", ec_);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->cost, 750000);
}

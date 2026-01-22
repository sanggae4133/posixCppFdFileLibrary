/**
 * @file VariableRecordScenarioTest.cpp
 * @brief Scenario tests for Variable-length record repository - CRUD workflows and edge cases
 */

#include <gtest/gtest.h>
#include <memory>
#include <vector>

#include "records/A.hpp"
#include "records/B.hpp"
#include <fdfile/repository/VariableFileRepositoryImpl.hpp>

using namespace FdFile;

// =============================================================================
// Basic CRUD Scenario Tests
// =============================================================================

class VariableRecordCrudTest : public ::testing::Test {
  protected:
    void SetUp() override {
        testFile_ = "./test_variable_crud.db";
        ::remove(testFile_.c_str());

        std::vector<std::unique_ptr<VariableRecordBase>> protos;
        protos.push_back(std::make_unique<A>());
        protos.push_back(std::make_unique<B>());

        repo_ = std::make_unique<VariableFileRepositoryImpl>(testFile_, std::move(protos), ec_);
        ASSERT_FALSE(ec_) << "Repository init failed: " << ec_.message();
    }

    void TearDown() override {
        repo_.reset();
        ::remove(testFile_.c_str());
    }

    std::string testFile_;
    std::error_code ec_;
    std::unique_ptr<VariableFileRepositoryImpl> repo_;
};

TEST_F(VariableRecordCrudTest, InsertFindUpdateDelete) {
    // Insert
    A alice("alice", 1);
    ASSERT_TRUE(repo_->save(alice, ec_));
    EXPECT_EQ(repo_->count(ec_), 1);

    // Find
    auto found = repo_->findById("1", ec_);
    ASSERT_NE(found, nullptr);
    auto* aPtr = dynamic_cast<A*>(found.get());
    ASSERT_NE(aPtr, nullptr);
    EXPECT_EQ(aPtr->name, "alice");

    // Update
    A aliceUpdated("alice_updated", 1);
    ASSERT_TRUE(repo_->save(aliceUpdated, ec_));
    EXPECT_EQ(repo_->count(ec_), 1);

    found = repo_->findById("1", ec_);
    aPtr = dynamic_cast<A*>(found.get());
    EXPECT_EQ(aPtr->name, "alice_updated");

    // Delete
    ASSERT_TRUE(repo_->deleteById("1", ec_));
    EXPECT_EQ(repo_->count(ec_), 0);
}

TEST_F(VariableRecordCrudTest, MixedTypesCrud) {
    // Insert both types
    A type_a("name_a", 1);
    B type_b("name_b", 2, "password");

    repo_->save(type_a, ec_);
    repo_->save(type_b, ec_);

    EXPECT_EQ(repo_->count(ec_), 2);

    // Find and verify types
    auto foundA = repo_->findById("1", ec_);
    ASSERT_NE(foundA, nullptr);
    EXPECT_STREQ(foundA->typeName(), "A");

    auto foundB = repo_->findById("2", ec_);
    ASSERT_NE(foundB, nullptr);
    EXPECT_STREQ(foundB->typeName(), "B");

    auto* bPtr = dynamic_cast<B*>(foundB.get());
    ASSERT_NE(bPtr, nullptr);
    EXPECT_EQ(bPtr->pw, "password");
}

TEST_F(VariableRecordCrudTest, LargeNumberOfRecords) {
    const int NUM_RECORDS = 500;

    for (int i = 0; i < NUM_RECORDS; ++i) {
        A record("user" + std::to_string(i), i);
        ASSERT_TRUE(repo_->save(record, ec_)) << "Failed at record " << i;
    }

    EXPECT_EQ(repo_->count(ec_), NUM_RECORDS);

    // Find specific record
    auto found = repo_->findById("250", ec_);
    ASSERT_NE(found, nullptr);
}

TEST_F(VariableRecordCrudTest, RepositoryReopenPersistence) {
    A alice("alice", 1);
    B bob("bob", 2, "secret");
    repo_->save(alice, ec_);
    repo_->save(bob, ec_);

    // Close
    repo_.reset();

    // Reopen with prototypes
    std::vector<std::unique_ptr<VariableRecordBase>> protos;
    protos.push_back(std::make_unique<A>());
    protos.push_back(std::make_unique<B>());
    repo_ = std::make_unique<VariableFileRepositoryImpl>(testFile_, std::move(protos), ec_);
    ASSERT_FALSE(ec_);

    EXPECT_EQ(repo_->count(ec_), 2);

    auto foundAlice = repo_->findById("1", ec_);
    ASSERT_NE(foundAlice, nullptr);

    auto foundBob = repo_->findById("2", ec_);
    ASSERT_NE(foundBob, nullptr);
    auto* bPtr = dynamic_cast<B*>(foundBob.get());
    EXPECT_EQ(bPtr->pw, "secret");
}

TEST_F(VariableRecordCrudTest, EmptyRepositoryOperations) {
    EXPECT_EQ(repo_->count(ec_), 0);
    EXPECT_EQ(repo_->findAll(ec_).size(), 0);
    EXPECT_EQ(repo_->findById("1", ec_), nullptr);
    EXPECT_FALSE(repo_->existsById("1", ec_));
    // deleteById on nonexistent - implementation returns true (no-op)
    repo_->deleteById("1", ec_);
    EXPECT_FALSE(ec_); // No error
    EXPECT_TRUE(repo_->deleteAll(ec_));
}

TEST_F(VariableRecordCrudTest, VeryLongFieldValues) {
    std::string longName(1000, 'x');
    A record(longName, 1);

    ASSERT_TRUE(repo_->save(record, ec_));

    auto found = repo_->findById("1", ec_);
    ASSERT_NE(found, nullptr);
    auto* aPtr = dynamic_cast<A*>(found.get());
    EXPECT_EQ(aPtr->name.size(), 1000);
}

TEST_F(VariableRecordCrudTest, SpecialCharactersInFields) {
    A record("name with \"quotes\" and \\backslash", 1);

    ASSERT_TRUE(repo_->save(record, ec_));

    auto found = repo_->findById("1", ec_);
    ASSERT_NE(found, nullptr);
    auto* aPtr = dynamic_cast<A*>(found.get());
    EXPECT_EQ(aPtr->name, "name with \"quotes\" and \\backslash");
}

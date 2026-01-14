/**
 * @file VariableRecordTest.cpp
 * @brief Unit tests for Variable-length record repositories (A, B types)
 */

#include <gtest/gtest.h>
#include <memory>

#include "records/A.hpp"
#include "records/B.hpp"
#include "repository/VariableFileRepositoryImpl.hpp"

using namespace FdFile;

// =============================================================================
// Variable Repository Tests
// =============================================================================

class VariableRepositoryTest : public ::testing::Test {
  protected:
    void SetUp() override {
        testFile_ = "./test_variable.db";
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

TEST_F(VariableRepositoryTest, InsertSingleRecordTypeA) {
    A alice("alice", 1);

    ASSERT_TRUE(repo_->save(alice, ec_));
    EXPECT_EQ(repo_->count(ec_), 1);
}

TEST_F(VariableRepositoryTest, InsertSingleRecordTypeB) {
    B user("user1", 101, "secret123");

    ASSERT_TRUE(repo_->save(user, ec_));
    EXPECT_EQ(repo_->count(ec_), 1);
}

TEST_F(VariableRepositoryTest, InsertMixedTypes) {
    A alice("alice", 1);
    A bob("bob", 2);
    B user1("user1", 101, "password1");
    B user2("user2", 102, "password2");

    ASSERT_TRUE(repo_->save(alice, ec_));
    ASSERT_TRUE(repo_->save(bob, ec_));
    ASSERT_TRUE(repo_->save(user1, ec_));
    ASSERT_TRUE(repo_->save(user2, ec_));

    EXPECT_EQ(repo_->count(ec_), 4);
}

TEST_F(VariableRepositoryTest, UpdateExistingRecord) {
    A alice("alice", 1);
    repo_->save(alice, ec_);

    // Update with same ID
    A updatedAlice("alice_updated", 1);
    ASSERT_TRUE(repo_->save(updatedAlice, ec_));

    // Count should still be 1
    EXPECT_EQ(repo_->count(ec_), 1);

    // Verify update
    auto found = repo_->findById("1", ec_);
    ASSERT_NE(found, nullptr);

    auto* aPtr = dynamic_cast<A*>(found.get());
    ASSERT_NE(aPtr, nullptr);
    EXPECT_EQ(aPtr->name, "alice_updated");
}

TEST_F(VariableRepositoryTest, FindById) {
    A alice("alice", 1);
    B user("user1", 101, "secret");
    repo_->save(alice, ec_);
    repo_->save(user, ec_);

    // Find type A
    auto foundA = repo_->findById("1", ec_);
    ASSERT_NE(foundA, nullptr);
    EXPECT_STREQ(foundA->typeName(), "A");

    // Find type B
    auto foundB = repo_->findById("101", ec_);
    ASSERT_NE(foundB, nullptr);
    EXPECT_STREQ(foundB->typeName(), "B");

    auto* bPtr = dynamic_cast<B*>(foundB.get());
    ASSERT_NE(bPtr, nullptr);
    EXPECT_EQ(bPtr->pw, "secret");
}

TEST_F(VariableRepositoryTest, FindByIdNotFound) {
    A alice("alice", 1);
    repo_->save(alice, ec_);

    auto notFound = repo_->findById("999", ec_);
    EXPECT_EQ(notFound, nullptr);
}

TEST_F(VariableRepositoryTest, FindAll) {
    A alice("alice", 1);
    A bob("bob", 2);
    B user("user1", 101, "secret");

    repo_->save(alice, ec_);
    repo_->save(bob, ec_);
    repo_->save(user, ec_);

    auto all = repo_->findAll(ec_);
    EXPECT_EQ(all.size(), 3);

    // Count types
    int countA = 0, countB = 0;
    for (const auto& rec : all) {
        if (std::string(rec->typeName()) == "A")
            countA++;
        if (std::string(rec->typeName()) == "B")
            countB++;
    }
    EXPECT_EQ(countA, 2);
    EXPECT_EQ(countB, 1);
}

TEST_F(VariableRepositoryTest, ExistsById) {
    A alice("alice", 1);
    repo_->save(alice, ec_);

    EXPECT_TRUE(repo_->existsById("1", ec_));
    EXPECT_FALSE(repo_->existsById("999", ec_));
}

TEST_F(VariableRepositoryTest, DeleteById) {
    A alice("alice", 1);
    A bob("bob", 2);
    repo_->save(alice, ec_);
    repo_->save(bob, ec_);

    ASSERT_TRUE(repo_->deleteById("1", ec_));
    EXPECT_EQ(repo_->count(ec_), 1);
    EXPECT_FALSE(repo_->existsById("1", ec_));
    EXPECT_TRUE(repo_->existsById("2", ec_));
}

TEST_F(VariableRepositoryTest, DeleteAll) {
    A alice("alice", 1);
    A bob("bob", 2);
    B user("user1", 101, "secret");

    repo_->save(alice, ec_);
    repo_->save(bob, ec_);
    repo_->save(user, ec_);

    ASSERT_TRUE(repo_->deleteAll(ec_));
    EXPECT_EQ(repo_->count(ec_), 0);
}

TEST_F(VariableRepositoryTest, ReinsertAfterDeleteAll) {
    A alice("alice", 1);
    repo_->save(alice, ec_);
    repo_->deleteAll(ec_);

    // Should be able to insert after delete all
    A newUser("new_user", 999);
    ASSERT_TRUE(repo_->save(newUser, ec_));
    EXPECT_EQ(repo_->count(ec_), 1);
}

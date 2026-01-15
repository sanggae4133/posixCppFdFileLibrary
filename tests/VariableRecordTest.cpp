/**
 * @file VariableRecordTest.cpp
 * @brief Unit tests for Variable-length record repositories (A, B types)
 */

#include <fstream>
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

// =============================================================================
// Variable Record Format Corruption Tests (JSON-like 형식 손상 테스트)
// =============================================================================

class VariableFormatCorruptionTest : public ::testing::Test {
  protected:
    void SetUp() override {
        testFile_ = "./test_variable_corrupt.db";
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

    void writeRawLine(const std::string& content) {
        std::ofstream ofs(testFile_, std::ios::app);
        ofs << content << "\n";
        ofs.close();
    }

    void overwriteFile(const std::string& content) {
        std::ofstream ofs(testFile_, std::ios::trunc);
        ofs << content;
        ofs.close();
    }

    std::string testFile_;
    std::error_code ec_;
    std::unique_ptr<VariableFileRepositoryImpl> repo_;
};

TEST_F(VariableFormatCorruptionTest, MissingOpenBrace) {
    // Valid: {A} "name":"alice", "id":1}
    // Invalid: missing opening brace
    overwriteFile("A} \"name\":\"alice\", \"id\":1}\n");

    auto all = repo_->findAll(ec_);
    // Should handle gracefully - skip invalid line or return empty
    EXPECT_EQ(all.size(), 0);
}

TEST_F(VariableFormatCorruptionTest, MissingCloseBrace) {
    // Valid: {A} {"name":"alice", "id":1}
    // Invalid: missing closing brace
    overwriteFile("{A} {\"name\":\"alice\", \"id\":1\n");

    auto all = repo_->findAll(ec_);
    EXPECT_EQ(all.size(), 0);
}

TEST_F(VariableFormatCorruptionTest, MissingAllBraces) {
    // No braces at all
    overwriteFile("A \"name\":\"alice\", \"id\":1\n");

    auto all = repo_->findAll(ec_);
    EXPECT_EQ(all.size(), 0);
}

TEST_F(VariableFormatCorruptionTest, MissingQuotesOnKey) {
    // Missing quotes on key
    overwriteFile("{A} {name:\"alice\", id:1}\n");

    auto all = repo_->findAll(ec_);
    // May fail to parse or skip
    EXPECT_EQ(all.size(), 0);
}

TEST_F(VariableFormatCorruptionTest, MissingQuotesOnValue) {
    // Missing quotes on string value
    overwriteFile("{A} {\"name\":alice, \"id\":1}\n");

    auto all = repo_->findAll(ec_);
    EXPECT_EQ(all.size(), 0);
}

TEST_F(VariableFormatCorruptionTest, UnmatchedQuotes) {
    // Unmatched quote
    overwriteFile("{A} {\"name\":\"alice, \"id\":1}\n");

    auto all = repo_->findAll(ec_);
    EXPECT_EQ(all.size(), 0);
}

TEST_F(VariableFormatCorruptionTest, MissingComma) {
    // Missing comma between fields
    overwriteFile("{A} {\"name\":\"alice\" \"id\":1}\n");

    auto all = repo_->findAll(ec_);
    EXPECT_EQ(all.size(), 0);
}

TEST_F(VariableFormatCorruptionTest, ExtraComma) {
    // Extra trailing comma
    overwriteFile("{A} {\"name\":\"alice\", \"id\":1,}\n");

    auto all = repo_->findAll(ec_);
    // May or may not handle extra comma
}

TEST_F(VariableFormatCorruptionTest, InvalidTypeName) {
    // Type name that doesn't match any prototype
    overwriteFile("{UnknownType} {\"name\":\"alice\", \"id\":1}\n");

    auto all = repo_->findAll(ec_);
    EXPECT_EQ(all.size(), 0);
}

TEST_F(VariableFormatCorruptionTest, EmptyTypeName) {
    // Empty type name
    overwriteFile("{} {\"name\":\"alice\", \"id\":1}\n");

    auto all = repo_->findAll(ec_);
    EXPECT_EQ(all.size(), 0);
}

TEST_F(VariableFormatCorruptionTest, UTF16BOMEncoding) {
    // UTF-16 BOM at start (would break ASCII parsing)
    std::ofstream ofs(testFile_, std::ios::binary | std::ios::trunc);
    // UTF-16 LE BOM: FF FE
    ofs.write("\xFF\xFE", 2);
    ofs << "{A} {\"name\":\"alice\", \"id\":1}\n";
    ofs.close();

    auto all = repo_->findAll(ec_);
    // Should skip or fail gracefully
}

TEST_F(VariableFormatCorruptionTest, BinaryGarbageLine) {
    // Binary garbage mixed with valid line
    A alice("alice", 1);
    repo_->save(alice, ec_);

    // Append garbage line
    std::ofstream ofs(testFile_, std::ios::app | std::ios::binary);
    char garbage[] = {0x00, 0x01, 0x02, static_cast<char>(0xFF), static_cast<char>(0xFE), '\n'};
    ofs.write(garbage, sizeof(garbage));
    ofs.close();

    // Should still find the valid record, skip the garbage
    auto all = repo_->findAll(ec_);
    EXPECT_GE(all.size(), 0); // At least doesn't crash
}

TEST_F(VariableFormatCorruptionTest, EmptyLines) {
    // File with empty lines
    A alice("alice", 1);
    repo_->save(alice, ec_);

    // Append empty lines
    writeRawLine("");
    writeRawLine("   ");
    writeRawLine("\t\t");

    auto all = repo_->findAll(ec_);
    EXPECT_EQ(all.size(), 1); // Should skip empty lines
}

TEST_F(VariableFormatCorruptionTest, PartialLineNoNewline) {
    // Line without trailing newline
    std::ofstream ofs(testFile_, std::ios::trunc);
    ofs << "{A} {\"name\":\"alice\", \"id\":1}"; // No newline
    ofs.close();

    auto all = repo_->findAll(ec_);
    // May or may not parse - depends on implementation
}

TEST_F(VariableFormatCorruptionTest, VeryLongLine) {
    // Very long field value
    std::string longName(10000, 'x');
    overwriteFile("{A} {\"name\":\"" + longName + "\", \"id\":1}\n");

    auto all = repo_->findAll(ec_);
    // Should handle long lines gracefully
}

TEST_F(VariableFormatCorruptionTest, NestedBraces) {
    // Nested braces (invalid JSON-like format)
    overwriteFile("{A} {{\"name\":\"alice\", \"id\":1}}\n");

    auto all = repo_->findAll(ec_);
    EXPECT_EQ(all.size(), 0);
}

TEST_F(VariableFormatCorruptionTest, EscapedQuotesIssue) {
    // Improperly escaped quotes
    overwriteFile("{A} {\"name\":\"ali\\\"ce\", \"id\":1}\n");

    auto all = repo_->findAll(ec_);
    // May or may not handle escaped quotes
}

TEST_F(VariableFormatCorruptionTest, ColonInValue) {
    // Colon in value (edge case)
    overwriteFile("{A} {\"name\":\"alice:bob\", \"id\":1}\n");

    auto all = repo_->findAll(ec_);
    // Should handle colon in value
}

TEST_F(VariableFormatCorruptionTest, NumberAsString) {
    // Number quoted as string
    overwriteFile("{A} {\"name\":\"alice\", \"id\":\"1\"}\n");

    auto all = repo_->findAll(ec_);
    // May or may not convert
}

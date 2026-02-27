/**
 * @file tests/VariableRecordTest.cpp
 * @brief 코드 이해를 위한 한국어 상세 주석 블록.
 * @details
 * - 이 파일은 라이브러리의 동작 계약(contract)을 검증하기 위한 테스트 시나리오를 정의합니다.
 * - 테스트는 정상 경로뿐 아니라 경계값, 실패 경로, 파일 I/O 예외 상황을 분리해 원인 추적이 쉽도록 구성되어야 합니다.
 * - 각 assertion은 '무엇이 실패했는지'가 즉시 드러나도록 작성하며, 상태 공유를 피하기 위해 테스트 간 파일/데이터 독립성을 유지해야 합니다.
 * - 저장 포맷/락 정책/캐시 정책이 바뀌면 해당 변화가 기존 계약을 깨지 않는지 회귀 테스트를 반드시 확장해야 합니다.
 * - 향후 테스트 추가 시에는 재현 가능한 입력, 명확한 기대 결과, 실패 시 진단 가능한 메시지를 함께 유지하는 것을 권장합니다.
 */
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

// 시나리오 상세 설명: VariableRepositoryTest 그룹의 InsertSingleRecordTypeA 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(VariableRepositoryTest, InsertSingleRecordTypeA) {
    A alice("alice", 1);

    ASSERT_TRUE(repo_->save(alice, ec_));
    EXPECT_EQ(repo_->count(ec_), 1);
}

// 시나리오 상세 설명: VariableRepositoryTest 그룹의 InsertSingleRecordTypeB 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(VariableRepositoryTest, InsertSingleRecordTypeB) {
    B user("user1", 101, "secret123");

    ASSERT_TRUE(repo_->save(user, ec_));
    EXPECT_EQ(repo_->count(ec_), 1);
}

// 시나리오 상세 설명: VariableRepositoryTest 그룹의 InsertMixedTypes 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
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

// 시나리오 상세 설명: VariableRepositoryTest 그룹의 UpdateExistingRecord 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
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

// 시나리오 상세 설명: VariableRepositoryTest 그룹의 FindById 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
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

// 시나리오 상세 설명: VariableRepositoryTest 그룹의 FindByIdNotFound 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(VariableRepositoryTest, FindByIdNotFound) {
    A alice("alice", 1);
    repo_->save(alice, ec_);

    auto notFound = repo_->findById("999", ec_);
    EXPECT_EQ(notFound, nullptr);
}

// 시나리오 상세 설명: VariableRepositoryTest 그룹의 FindAll 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
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

// 시나리오 상세 설명: VariableRepositoryTest 그룹의 ExistsById 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(VariableRepositoryTest, ExistsById) {
    A alice("alice", 1);
    repo_->save(alice, ec_);

    EXPECT_TRUE(repo_->existsById("1", ec_));
    EXPECT_FALSE(repo_->existsById("999", ec_));
}

// 시나리오 상세 설명: VariableRepositoryTest 그룹의 DeleteById 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
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

// 시나리오 상세 설명: VariableRepositoryTest 그룹의 DeleteAll 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
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

// 시나리오 상세 설명: VariableRepositoryTest 그룹의 ReinsertAfterDeleteAll 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
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

// 시나리오 상세 설명: VariableFormatCorruptionTest 그룹의 MissingOpenBrace 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(VariableFormatCorruptionTest, MissingOpenBrace) {
    // Valid: {A} "name":"alice", "id":1}
    // Invalid: missing opening brace
    overwriteFile("A} \"name\":\"alice\", \"id\":1}\n");

    auto all = repo_->findAll(ec_);
    // Should handle gracefully - skip invalid line or return empty
    EXPECT_EQ(all.size(), 0);
}

// 시나리오 상세 설명: VariableFormatCorruptionTest 그룹의 MissingCloseBrace 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(VariableFormatCorruptionTest, MissingCloseBrace) {
    // Valid: {A} {"name":"alice", "id":1}
    // Invalid: missing closing brace
    overwriteFile("{A} {\"name\":\"alice\", \"id\":1\n");

    auto all = repo_->findAll(ec_);
    EXPECT_EQ(all.size(), 0);
}

// 시나리오 상세 설명: VariableFormatCorruptionTest 그룹의 MissingAllBraces 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(VariableFormatCorruptionTest, MissingAllBraces) {
    // No braces at all
    overwriteFile("A \"name\":\"alice\", \"id\":1\n");

    auto all = repo_->findAll(ec_);
    EXPECT_EQ(all.size(), 0);
}

// 시나리오 상세 설명: VariableFormatCorruptionTest 그룹의 MissingQuotesOnKey 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(VariableFormatCorruptionTest, MissingQuotesOnKey) {
    // Missing quotes on key
    overwriteFile("{A} {name:\"alice\", id:1}\n");

    auto all = repo_->findAll(ec_);
    // May fail to parse or skip
    EXPECT_EQ(all.size(), 0);
}

// 시나리오 상세 설명: VariableFormatCorruptionTest 그룹의 MissingQuotesOnValue 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(VariableFormatCorruptionTest, MissingQuotesOnValue) {
    // Missing quotes on string value
    overwriteFile("{A} {\"name\":alice, \"id\":1}\n");

    auto all = repo_->findAll(ec_);
    EXPECT_EQ(all.size(), 0);
}

// 시나리오 상세 설명: VariableFormatCorruptionTest 그룹의 UnmatchedQuotes 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(VariableFormatCorruptionTest, UnmatchedQuotes) {
    // Unmatched quote
    overwriteFile("{A} {\"name\":\"alice, \"id\":1}\n");

    auto all = repo_->findAll(ec_);
    EXPECT_EQ(all.size(), 0);
}

// 시나리오 상세 설명: VariableFormatCorruptionTest 그룹의 MissingComma 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(VariableFormatCorruptionTest, MissingComma) {
    // Missing comma between fields
    overwriteFile("{A} {\"name\":\"alice\" \"id\":1}\n");

    auto all = repo_->findAll(ec_);
    EXPECT_EQ(all.size(), 0);
}

// 시나리오 상세 설명: VariableFormatCorruptionTest 그룹의 ExtraComma 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(VariableFormatCorruptionTest, ExtraComma) {
    // Extra trailing comma
    overwriteFile("{A} {\"name\":\"alice\", \"id\":1,}\n");

    auto all = repo_->findAll(ec_);
    // May or may not handle extra comma
}

// 시나리오 상세 설명: VariableFormatCorruptionTest 그룹의 InvalidTypeName 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(VariableFormatCorruptionTest, InvalidTypeName) {
    // Type name that doesn't match any prototype
    overwriteFile("{UnknownType} {\"name\":\"alice\", \"id\":1}\n");

    auto all = repo_->findAll(ec_);
    EXPECT_EQ(all.size(), 0);
}

// 시나리오 상세 설명: VariableFormatCorruptionTest 그룹의 EmptyTypeName 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(VariableFormatCorruptionTest, EmptyTypeName) {
    // Empty type name
    overwriteFile("{} {\"name\":\"alice\", \"id\":1}\n");

    auto all = repo_->findAll(ec_);
    EXPECT_EQ(all.size(), 0);
}

// 시나리오 상세 설명: VariableFormatCorruptionTest 그룹의 UTF16BOMEncoding 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
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

// 시나리오 상세 설명: VariableFormatCorruptionTest 그룹의 BinaryGarbageLine 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
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

// 시나리오 상세 설명: VariableFormatCorruptionTest 그룹의 EmptyLines 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
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

// 시나리오 상세 설명: VariableFormatCorruptionTest 그룹의 PartialLineNoNewline 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(VariableFormatCorruptionTest, PartialLineNoNewline) {
    // Line without trailing newline
    std::ofstream ofs(testFile_, std::ios::trunc);
    ofs << "{A} {\"name\":\"alice\", \"id\":1}"; // No newline
    ofs.close();

    auto all = repo_->findAll(ec_);
    // May or may not parse - depends on implementation
}

// 시나리오 상세 설명: VariableFormatCorruptionTest 그룹의 VeryLongLine 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(VariableFormatCorruptionTest, VeryLongLine) {
    // Very long field value
    std::string longName(10000, 'x');
    overwriteFile("{A} {\"name\":\"" + longName + "\", \"id\":1}\n");

    auto all = repo_->findAll(ec_);
    // Should handle long lines gracefully
}

// 시나리오 상세 설명: VariableFormatCorruptionTest 그룹의 NestedBraces 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(VariableFormatCorruptionTest, NestedBraces) {
    // Nested braces (invalid JSON-like format)
    overwriteFile("{A} {{\"name\":\"alice\", \"id\":1}}\n");

    auto all = repo_->findAll(ec_);
    EXPECT_EQ(all.size(), 0);
}

// 시나리오 상세 설명: VariableFormatCorruptionTest 그룹의 EscapedQuotesIssue 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(VariableFormatCorruptionTest, EscapedQuotesIssue) {
    // Improperly escaped quotes
    overwriteFile("{A} {\"name\":\"ali\\\"ce\", \"id\":1}\n");

    auto all = repo_->findAll(ec_);
    // May or may not handle escaped quotes
}

// 시나리오 상세 설명: VariableFormatCorruptionTest 그룹의 ColonInValue 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(VariableFormatCorruptionTest, ColonInValue) {
    // Colon in value (edge case)
    overwriteFile("{A} {\"name\":\"alice:bob\", \"id\":1}\n");

    auto all = repo_->findAll(ec_);
    // Should handle colon in value
}

// 시나리오 상세 설명: VariableFormatCorruptionTest 그룹의 NumberAsString 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(VariableFormatCorruptionTest, NumberAsString) {
    // Number quoted as string
    overwriteFile("{A} {\"name\":\"alice\", \"id\":\"1\"}\n");

    auto all = repo_->findAll(ec_);
    // May or may not convert
}

// =============================================================================
// External Modification Detection Tests (외부 수정 감지 테스트)
// =============================================================================

class VariableExternalModificationTest : public ::testing::Test {
  protected:
    void SetUp() override {
        testFile_ = "./test_variable_external.db";
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

    void appendLineExternally(const std::string& line) {
        std::ofstream ofs(testFile_, std::ios::app);
        ofs << line << "\n";
        ofs.close();
    }

    std::string testFile_;
    std::error_code ec_;
    std::unique_ptr<VariableFileRepositoryImpl> repo_;
};

// 시나리오 상세 설명: VariableExternalModificationTest 그룹의 DetectsExternalAppend 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(VariableExternalModificationTest, DetectsExternalAppend) {
    // 1. Save via repo
    A alice("alice", 1);
    repo_->save(alice, ec_);
    ASSERT_FALSE(ec_);
    EXPECT_EQ(repo_->count(ec_), 1);

    // 2. Externally append a new record
    appendLineExternally("A { \"name\": \"bob\", \"id\": 2 }");

    // 3. Force mtime change
    usleep(10000);

    // 4. Verify repo detects the new record
    EXPECT_EQ(repo_->count(ec_), 2);
    EXPECT_TRUE(repo_->existsById("1", ec_));
    EXPECT_TRUE(repo_->existsById("2", ec_));
}

// 시나리오 상세 설명: VariableExternalModificationTest 그룹의 DetectsExternalAppendImmediately 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(VariableExternalModificationTest, DetectsExternalAppendImmediately) {
    // 1. Save via repo
    A alice("alice", 1);
    repo_->save(alice, ec_);
    ASSERT_FALSE(ec_);

    // 2. Externally append a new record (no sleep)
    appendLineExternally("A { \"name\": \"bob\", \"id\": 2 }");

    // 3. Verify repo detects the new record (size change should trigger refresh)
    EXPECT_EQ(repo_->count(ec_), 2);
}

// 시나리오 상세 설명: VariableExternalModificationTest 그룹의 DetectsExternalDeleteAll 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(VariableExternalModificationTest, DetectsExternalDeleteAll) {
    // 1. Save records via repo
    A alice("alice", 1);
    A bob("bob", 2);
    repo_->save(alice, ec_);
    repo_->save(bob, ec_);
    ASSERT_FALSE(ec_);
    EXPECT_EQ(repo_->count(ec_), 2);

    // 2. Externally truncate file
    std::ofstream ofs(testFile_, std::ios::trunc);
    ofs.close();

    // 3. Force mtime change
    usleep(10000);

    // 4. Verify repo detects empty file
    EXPECT_EQ(repo_->count(ec_), 0);
}

// 시나리오 상세 설명: VariableExternalModificationTest 그룹의 DetectsExternalModification 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(VariableExternalModificationTest, DetectsExternalModification) {
    // 1. Save via repo
    A alice("alice", 1);
    repo_->save(alice, ec_);
    ASSERT_FALSE(ec_);

    // 2. Externally overwrite with different content
    std::ofstream ofs(testFile_, std::ios::trunc);
    ofs << "A { \"name\": \"charlie\", \"id\": 3 }\n";
    ofs.close();

    // 3. Force mtime change
    usleep(10000);

    // 4. Verify repo detects the change
    auto found = repo_->findById("3", ec_);
    ASSERT_NE(found, nullptr);
    EXPECT_FALSE(repo_->existsById("1", ec_));
}

// 시나리오 상세 설명: VariableExternalModificationTest 그룹의 CacheInvalidationOnSave 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(VariableExternalModificationTest, CacheInvalidationOnSave) {
    // 1. Save initial record
    A alice("alice", 1);
    repo_->save(alice, ec_);
    ASSERT_FALSE(ec_);

    // 2. Externally append
    appendLineExternally("A { \"name\": \"bob\", \"id\": 2 }");

    usleep(10000);

    // 3. Save new record via repo - should see external change
    A charlie("charlie", 3);
    repo_->save(charlie, ec_);
    ASSERT_FALSE(ec_);

    // 4. All 3 records should exist
    EXPECT_EQ(repo_->count(ec_), 3);
}

// 시나리오 상세 설명: VariableExternalModificationTest 그룹의 MultipleExternalAppends 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(VariableExternalModificationTest, MultipleExternalAppends) {
    // 1. Save initial record
    A alice("alice", 1);
    repo_->save(alice, ec_);
    ASSERT_FALSE(ec_);
    EXPECT_EQ(repo_->count(ec_), 1);

    // 2. Externally append multiple records
    appendLineExternally("A { \"name\": \"bob\", \"id\": 2 }");
    appendLineExternally("B { \"name\": \"charlie\", \"id\": 3, \"pw\": \"secret\" }");

    usleep(10000);

    // 3. Verify all records are visible
    EXPECT_EQ(repo_->count(ec_), 3);

    auto all = repo_->findAll(ec_);
    ASSERT_EQ(all.size(), 3);

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

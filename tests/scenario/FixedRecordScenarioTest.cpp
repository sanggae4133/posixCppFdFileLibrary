/**
 * @file tests/scenario/FixedRecordScenarioTest.cpp
 * @brief 코드 이해를 위한 한국어 상세 주석 블록.
 * @details
 * - 이 파일은 라이브러리의 동작 계약(contract)을 검증하기 위한 테스트 시나리오를 정의합니다.
 * - 테스트는 정상 경로뿐 아니라 경계값, 실패 경로, 파일 I/O 예외 상황을 분리해 원인 추적이 쉽도록 구성되어야 합니다.
 * - 각 assertion은 '무엇이 실패했는지'가 즉시 드러나도록 작성하며, 상태 공유를 피하기 위해 테스트 간 파일/데이터 독립성을 유지해야 합니다.
 * - 저장 포맷/락 정책/캐시 정책이 바뀌면 해당 변화가 기존 계약을 깨지 않는지 회귀 테스트를 반드시 확장해야 합니다.
 * - 향후 테스트 추가 시에는 재현 가능한 입력, 명확한 기대 결과, 실패 시 진단 가능한 메시지를 함께 유지하는 것을 권장합니다.
 */
/**
 * @file FixedRecordScenarioTest.cpp
 * @brief Scenario tests for Fixed-length record repository - CRUD workflows and edge cases
 */

#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

#include "records/FixedA.hpp"
#include "records/FixedB.hpp"
#include "repository/UniformFixedRepositoryImpl.hpp"

using namespace FdFile;

// =============================================================================
// Basic CRUD Scenario Tests
// =============================================================================

class FixedRecordCrudTest : public ::testing::Test {
  protected:
    void SetUp() override {
        testFile_ = "./test_fixed_crud.db";
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

// 시나리오 상세 설명: FixedRecordCrudTest 그룹의 InsertFindUpdateDelete 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FixedRecordCrudTest, InsertFindUpdateDelete) {
    // Insert
    FixedA alice("alice", 25, "001");
    ASSERT_TRUE(repo_->save(alice, ec_));
    EXPECT_EQ(repo_->count(ec_), 1);

    // Find
    auto found = repo_->findById("001", ec_);
    ASSERT_NE(found, nullptr);
    EXPECT_STREQ(found->name, "alice");
    EXPECT_EQ(found->age, 25);

    // Update
    FixedA aliceUpdated("alice_v2", 26, "001");
    ASSERT_TRUE(repo_->save(aliceUpdated, ec_));
    EXPECT_EQ(repo_->count(ec_), 1);

    found = repo_->findById("001", ec_);
    EXPECT_STREQ(found->name, "alice_v2");
    EXPECT_EQ(found->age, 26);

    // Delete
    ASSERT_TRUE(repo_->deleteById("001", ec_));
    EXPECT_EQ(repo_->count(ec_), 0);
    EXPECT_EQ(repo_->findById("001", ec_), nullptr);
}

// 시나리오 상세 설명: FixedRecordCrudTest 그룹의 LargeNumberOfRecords 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FixedRecordCrudTest, LargeNumberOfRecords) {
    const int NUM_RECORDS = 1000;

    // Insert 1000 records
    for (int i = 0; i < NUM_RECORDS; ++i) {
        std::string id = std::to_string(i);
        FixedA record("user", i, id.c_str());
        ASSERT_TRUE(repo_->save(record, ec_)) << "Failed at record " << i;
    }

    EXPECT_EQ(repo_->count(ec_), NUM_RECORDS);

    // Random access - cache should help
    auto found = repo_->findById("500", ec_);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->age, 500);

    // Sequential access
    auto all = repo_->findAll(ec_);
    EXPECT_EQ(all.size(), NUM_RECORDS);
}

// 시나리오 상세 설명: FixedRecordCrudTest 그룹의 RepositoryReopenPersistence 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FixedRecordCrudTest, RepositoryReopenPersistence) {
    // Insert records
    FixedA alice("alice", 25, "001");
    FixedA bob("bob", 30, "002");
    repo_->save(alice, ec_);
    repo_->save(bob, ec_);

    // Close repository
    repo_.reset();

    // Reopen
    repo_ = std::make_unique<UniformFixedRepositoryImpl<FixedA>>(testFile_, ec_);
    ASSERT_FALSE(ec_);

    // Verify data persisted
    EXPECT_EQ(repo_->count(ec_), 2);

    auto foundAlice = repo_->findById("001", ec_);
    ASSERT_NE(foundAlice, nullptr);
    EXPECT_STREQ(foundAlice->name, "alice");

    auto foundBob = repo_->findById("002", ec_);
    ASSERT_NE(foundBob, nullptr);
    EXPECT_STREQ(foundBob->name, "bob");
}

// 시나리오 상세 설명: FixedRecordCrudTest 그룹의 EmptyRepositoryOperations 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FixedRecordCrudTest, EmptyRepositoryOperations) {
    // Count on empty
    EXPECT_EQ(repo_->count(ec_), 0);

    // FindAll on empty
    auto all = repo_->findAll(ec_);
    EXPECT_EQ(all.size(), 0);

    // FindById on empty
    auto found = repo_->findById("nonexistent", ec_);
    EXPECT_EQ(found, nullptr);

    // ExistsById on empty
    EXPECT_FALSE(repo_->existsById("nonexistent", ec_));

    // DeleteById on empty - implementation returns true (no-op success)
    repo_->deleteById("nonexistent", ec_);
    EXPECT_FALSE(ec_); // No error

    // DeleteAll on empty
    EXPECT_TRUE(repo_->deleteAll(ec_));
}

// 시나리오 상세 설명: FixedRecordCrudTest 그룹의 DuplicateIdUpsert 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FixedRecordCrudTest, DuplicateIdUpsert) {
    // Save with ID "001"
    FixedA first("first", 10, "001");
    repo_->save(first, ec_);
    EXPECT_EQ(repo_->count(ec_), 1);

    // Save again with same ID - should update
    FixedA second("second", 20, "001");
    repo_->save(second, ec_);
    EXPECT_EQ(repo_->count(ec_), 1);

    // Save third time
    FixedA third("third", 30, "001");
    repo_->save(third, ec_);
    EXPECT_EQ(repo_->count(ec_), 1);

    // Verify last value
    auto found = repo_->findById("001", ec_);
    ASSERT_NE(found, nullptr);
    EXPECT_STREQ(found->name, "third");
    EXPECT_EQ(found->age, 30);
}

// 시나리오 상세 설명: FixedRecordCrudTest 그룹의 SequentialVsRandomAccess 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FixedRecordCrudTest, SequentialVsRandomAccess) {
    // Insert records
    for (int i = 0; i < 100; ++i) {
        std::string id = std::to_string(i);
        FixedA record("user", i, id.c_str());
        repo_->save(record, ec_);
    }

    // Sequential access
    for (int i = 0; i < 100; ++i) {
        std::string id = std::to_string(i);
        auto found = repo_->findById(id, ec_);
        ASSERT_NE(found, nullptr);
        EXPECT_EQ(found->age, i);
    }

    // Random access (cache lookup)
    EXPECT_NE(repo_->findById("50", ec_), nullptr);
    EXPECT_NE(repo_->findById("99", ec_), nullptr);
    EXPECT_NE(repo_->findById("0", ec_), nullptr);
}

// 시나리오 상세 설명: FixedRecordCrudTest 그룹의 MaxFieldLengthBoundary 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FixedRecordCrudTest, MaxFieldLengthBoundary) {
    // Name field is 20 chars - test with exactly 20 chars
    FixedA record("12345678901234567890", 99, "MAX");

    ASSERT_TRUE(repo_->save(record, ec_));

    auto found = repo_->findById("MAX", ec_);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->age, 99);
    // Name should contain the data (may be truncated/null-terminated)
}

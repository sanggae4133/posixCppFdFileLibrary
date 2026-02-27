/**
 * @file tests/scenario/VariableRecordScenarioTest.cpp
 * @brief 코드 이해를 위한 한국어 상세 주석 블록.
 * @details
 * - 이 파일은 라이브러리의 동작 계약(contract)을 검증하기 위한 테스트 시나리오를 정의합니다.
 * - 테스트는 정상 경로뿐 아니라 경계값, 실패 경로, 파일 I/O 예외 상황을 분리해 원인 추적이 쉽도록 구성되어야 합니다.
 * - 각 assertion은 '무엇이 실패했는지'가 즉시 드러나도록 작성하며, 상태 공유를 피하기 위해 테스트 간 파일/데이터 독립성을 유지해야 합니다.
 * - 저장 포맷/락 정책/캐시 정책이 바뀌면 해당 변화가 기존 계약을 깨지 않는지 회귀 테스트를 반드시 확장해야 합니다.
 * - 향후 테스트 추가 시에는 재현 가능한 입력, 명확한 기대 결과, 실패 시 진단 가능한 메시지를 함께 유지하는 것을 권장합니다.
 */
/**
 * @file VariableRecordScenarioTest.cpp
 * @brief Scenario tests for Variable-length record repository - CRUD workflows and edge cases
 */

#include <gtest/gtest.h>
#include <memory>
#include <vector>

#include "records/A.hpp"
#include "records/B.hpp"
#include "repository/VariableFileRepositoryImpl.hpp"

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

// 시나리오 상세 설명: VariableRecordCrudTest 그룹의 InsertFindUpdateDelete 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
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

// 시나리오 상세 설명: VariableRecordCrudTest 그룹의 MixedTypesCrud 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
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

// 시나리오 상세 설명: VariableRecordCrudTest 그룹의 LargeNumberOfRecords 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
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

// 시나리오 상세 설명: VariableRecordCrudTest 그룹의 RepositoryReopenPersistence 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
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

// 시나리오 상세 설명: VariableRecordCrudTest 그룹의 EmptyRepositoryOperations 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
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

// 시나리오 상세 설명: VariableRecordCrudTest 그룹의 VeryLongFieldValues 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(VariableRecordCrudTest, VeryLongFieldValues) {
    std::string longName(1000, 'x');
    A record(longName, 1);

    ASSERT_TRUE(repo_->save(record, ec_));

    auto found = repo_->findById("1", ec_);
    ASSERT_NE(found, nullptr);
    auto* aPtr = dynamic_cast<A*>(found.get());
    EXPECT_EQ(aPtr->name.size(), 1000);
}

// 시나리오 상세 설명: VariableRecordCrudTest 그룹의 SpecialCharactersInFields 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(VariableRecordCrudTest, SpecialCharactersInFields) {
    A record("name with \"quotes\" and \\backslash", 1);

    ASSERT_TRUE(repo_->save(record, ec_));

    auto found = repo_->findById("1", ec_);
    ASSERT_NE(found, nullptr);
    auto* aPtr = dynamic_cast<A*>(found.get());
    EXPECT_EQ(aPtr->name, "name with \"quotes\" and \\backslash");
}

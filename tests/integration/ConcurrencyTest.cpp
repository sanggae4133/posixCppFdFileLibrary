/**
 * @file tests/integration/ConcurrencyTest.cpp
 * @brief 코드 이해를 위한 한국어 상세 주석 블록.
 * @details
 * - 이 파일은 라이브러리의 동작 계약(contract)을 검증하기 위한 테스트 시나리오를 정의합니다.
 * - 테스트는 정상 경로뿐 아니라 경계값, 실패 경로, 파일 I/O 예외 상황을 분리해 원인 추적이 쉽도록 구성되어야 합니다.
 * - 각 assertion은 '무엇이 실패했는지'가 즉시 드러나도록 작성하며, 상태 공유를 피하기 위해 테스트 간 파일/데이터 독립성을 유지해야 합니다.
 * - 저장 포맷/락 정책/캐시 정책이 바뀌면 해당 변화가 기존 계약을 깨지 않는지 회귀 테스트를 반드시 확장해야 합니다.
 * - 향후 테스트 추가 시에는 재현 가능한 입력, 명확한 기대 결과, 실패 시 진단 가능한 메시지를 함께 유지하는 것을 권장합니다.
 */
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
#include <fdfile/repository/UniformFixedRepositoryImpl.hpp>

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

// 시나리오 상세 설명: ConcurrencyTest 그룹의 SequentialMultipleOpens 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
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

// 시나리오 상세 설명: ConcurrencyTest 그룹의 MultipleRepositoriesSequential 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
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

// 시나리오 상세 설명: ConcurrencyTest 그룹의 RapidOpenCloseNoLeak 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
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

// 시나리오 상세 설명: ConcurrencyTest 그룹의 DataIntegrityAfterMultipleWrites 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
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
